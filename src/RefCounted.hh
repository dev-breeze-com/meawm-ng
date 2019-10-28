/* RefCounted.hh

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

#ifndef __RefCounted_hh
#define __RefCounted_hh

template <class T>
class RefCounted {
public:
    RefCounted(T *_ref_count_pointer) {
        ref_count_pointer = _ref_count_pointer;
        ref_count = 1;
    }
    
    T *ref(void) {
        ref_count++;
        return ref_count_pointer;
    }
    void unref(void) {
        if (--ref_count == 0) delete ref_count_pointer;
    }
    
private:
    unsigned int ref_count;
    T *ref_count_pointer;
};

#endif // __RefCounted_hh
