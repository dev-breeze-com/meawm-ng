/* Png.cc

Copyright © 2003 David Reveman.

This file is part of Meawm_NG.

Meawm_NG is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Meawm_NG is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Meawm_NG; see the file COPYING. If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA. */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef    PNG

extern "C" {
#ifdef    STDC_HEADERS
#  include <stdlib.h>
#endif // STDC_HEADERS

#include "png.h"
}

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

#include "Util.hh"

#define PNG_BYTES_TO_CHECK 4

bool check_if_png(FILE *fp) {
    png_byte buf[PNG_BYTES_TO_CHECK];
    
    if (fread(buf, 1, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK)
        return false;
    
    return (! png_sig_cmp(buf, (png_size_t) 0, PNG_BYTES_TO_CHECK));
}

static void premultiply_data(png_structp, png_row_infop row_info,
                             png_bytep data) {
    for (unsigned int i = 0; i < row_info->rowbytes; i += 4) {
        unsigned char *base = &data[i];
        unsigned char blue = base[0];
        unsigned char green = base[1];
        unsigned char red = base[2];
        unsigned char alpha = base[3];
        WaPixel p;
        
        red = (unsigned) red * (unsigned) alpha / 255;
        green = (unsigned) green * (unsigned) alpha / 255;
        blue = (unsigned) blue * (unsigned) alpha / 255;
        p = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
        memcpy(base, &p, sizeof (WaPixel));
    }
}

unsigned char *read_png_to_rgba(FILE *fp, int *pwidth, int *pheight) {
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type, filter_type;
    int channels;
    unsigned char *data;
    png_bytepp volatile rows = NULL;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL) return NULL;

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
        goto bail;

    png_init_io(png_ptr, fp);

    png_read_info(png_ptr, info_ptr);

    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    if (bit_depth < 1 || bit_depth > 16)
        goto bail;
    
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 &interlace_type, &compression_type, &filter_type);

    if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8) {
        png_set_expand(png_ptr);
    } else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand(png_ptr);
    } else if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_expand(png_ptr);
    } else if (bit_depth < 8) {
        png_set_expand(png_ptr);
    }
    
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    if (interlace_type != PNG_INTERLACE_NONE)
        png_set_interlace_handling(png_ptr);

    png_set_bgr(png_ptr);
    png_set_filler(png_ptr, 255, PNG_FILLER_AFTER);
    
    png_set_read_user_transform_fn(png_ptr, premultiply_data);
    
    png_read_update_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 &interlace_type, &compression_type, &filter_type);
    
    if (width == 0 || height == 0) {
        WARNING << "transformed PNG has zero width or height" << endl;
        goto bail;
    }

    if (bit_depth != 8) {
        WARNING << "bits per channel of transformed PNG is not 8" << endl;
        goto bail;
    }

    if (! (color_type == PNG_COLOR_TYPE_RGB ||
           color_type == PNG_COLOR_TYPE_RGB_ALPHA) ) {
        WARNING << "transformed PNG not RGB or ARGB" << endl;
        goto bail;
    }

    channels = png_get_channels(png_ptr, info_ptr);
    if (! (channels == 3 || channels == 4) ) {
        WARNING << "transformed PNG has unsupported number of channels, "
            "must be 3 or 4." << endl;
        goto bail;
    }

    data = new unsigned char[sizeof(WaPixel) * width * height];

    rows = (png_bytepp) new png_bytep[height];

    for (unsigned int i = 0; i < height; i++)
        rows[i] = data + i * width * sizeof(WaPixel);

    png_read_image(png_ptr, rows);
    png_read_end(png_ptr, info_ptr);

    *pwidth = width;
    *pheight = height;

    delete [] rows;
    
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return data;

 bail:
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return NULL;

}

#endif // PNG
