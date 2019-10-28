/* Tst.hh

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

#ifndef __Tst_hh
#define __Tst_hh

#define DEFAULT_TST_NODE_LINE_WIDTH 100

template<class _Tp>
class TstNode {
public:
    unsigned char value;
    
    TstNode<_Tp> *left;
    TstNode<_Tp> *middle;
    TstNode<_Tp> *right;

    _Tp data;
};

template<class _Tp>
class TstNodeLines {
public:
   TstNode<_Tp> *node_line;
   TstNodeLines<_Tp> *next;
};

template<class _Tp>
class Tst {
public:
    Tst(void);
    Tst(int);
    Tst(const char **);
    ~Tst();

    void clean(void);
    void insert(char *, _Tp);
    void remove(char *);

    class iterator {
    public:
        inline iterator(void) {
            current_node = NULL;
        }
        inline iterator(TstNode<_Tp> *node) {
            current_node = node;
        }
        inline iterator(Tst *_tst) {
            tst = _tst;
            line = tst->node_lines;
            pos = 0;
            current_node = tst->nextNode(&line, &pos);
        }
        
        inline _Tp operator*(void) {
            return (_Tp) current_node->data;
        }
        inline _Tp *operator->(void) {
            return (_Tp *) &current_node->data;
        }
        inline iterator & operator++(void) {
            current_node = tst->nextNode(&line, &pos);
            return *this;
        }
        inline iterator operator++(int) {
            iterator tmp (*this); ++(*this); return tmp;
        }
        inline bool operator==(const iterator &x) {
            return (current_node == x.current_node);
        }
        inline bool operator!=(const iterator &x) {
            return (current_node != x.current_node);
        }

        Tst *tst;
        TstNode<_Tp> *current_node;
        TstNodeLines<_Tp> *line;
        int pos;
    };

    iterator begin(void) { return iterator(this); }
    iterator end(void) { return iterator(); }
    iterator find(char *key) {
        TstNode<_Tp> *current_node;
        int key_index;
   
        if (key[0] == 0)
            return iterator();
   
        if (head[(int) key[0]] == NULL)
            return iterator();
   
        current_node = head[(int) key[0]];
        key_index = 1;
   
        while (current_node != NULL) {
            if (key[key_index] == current_node->value) {
                if (current_node->value == 0) {
                    if (current_node->middle) {
                        return iterator(current_node);
                    } else
                        return iterator();
                } else {
                    current_node = current_node->middle;
                    key_index++;
                    continue;
                }
            } else if (((current_node->value == 0) && (key[key_index] < 64)) ||
                       ((current_node->value != 0) && (key[key_index] <
                                                       current_node->value))) {
                current_node = current_node->left;
                continue;
            } else {
                current_node = current_node->right;
                continue;
            }
        }
        return iterator();
    }

    TstNode<_Tp> *nextNode(TstNodeLines<_Tp> **, int *);
    TstNodeLines<_Tp> *node_lines;
    
private:
    void clear(void);
    void init(int);
    void growNodeFreeList(void);
    
    int node_line_width;
    TstNode<_Tp> *free_list;
    TstNode<_Tp> *head[256];
};

template<class _Tp>
Tst<_Tp>::Tst(void) {
    init(DEFAULT_TST_NODE_LINE_WIDTH);
}

template<class _Tp>
Tst<_Tp>::Tst(int width) {
    init(width);
}

template<class _Tp>
Tst<_Tp>::Tst(const char **attr) {
    init(DEFAULT_TST_NODE_LINE_WIDTH);
    if (attr) {
        for (int i = 0; attr[i]; i += 2)
            insert((char *) attr[i], (char *) attr[i + 1]);
    }
}

template<class _Tp>
Tst<_Tp>::~Tst(void) {
    clear();
}

template<class _Tp>
void Tst<_Tp>::clear(void) {
    TstNodeLines<_Tp> *current_line;
    TstNodeLines<_Tp> *next_line;
    
    next_line = node_lines;
    
    do {
        current_line = next_line;
        next_line = current_line->next;
        delete [] current_line->node_line;
        delete current_line;
    } while (next_line != NULL);
}

template<class _Tp>
void Tst<_Tp>::clean(void) {
    clear();
    init(node_line_width);
}

template<class _Tp>
void Tst<_Tp>::init(int width) {
    TstNode<_Tp> *current_node;

    memset(head, 0, sizeof(TstNode<_Tp> *) * 256);

    node_lines = new TstNodeLines<_Tp>;
    memset(node_lines, 0, sizeof(TstNodeLines<_Tp>));
   
    node_line_width = width;
    node_lines->next = NULL;
    node_lines->node_line = new TstNode<_Tp>[node_line_width];
    memset(node_lines->node_line, 0, sizeof(TstNode<_Tp>) * node_line_width);
   
    current_node = node_lines->node_line;
    free_list = current_node;
    
    for (int i = 1; i < node_line_width; i++) {
        current_node->middle = &(node_lines->node_line[i]);
        current_node = current_node->middle;
    }

    current_node->middle = NULL;
}

template<class _Tp>
void Tst<_Tp>::growNodeFreeList(void) {
    TstNode<_Tp> *current_node;
    TstNodeLines<_Tp> *new_line;

    new_line = new TstNodeLines<_Tp>;
    memset(new_line, 0, sizeof(TstNodeLines<_Tp>));

    new_line->node_line = new TstNode<_Tp>[node_line_width];
    memset(new_line->node_line, 0, sizeof(TstNode<_Tp>) * node_line_width);
    
    new_line->next = node_lines;
    node_lines = new_line;
   
    current_node = node_lines->node_line;
    free_list = current_node;
    
    for (int i = 1; i < node_line_width; i++) {
        current_node->middle = &(node_lines->node_line[i]);
        current_node = current_node->middle;
    }

    current_node->middle = NULL;
}

template<class _Tp>
TstNode<_Tp> *Tst<_Tp>::nextNode(TstNodeLines<_Tp> **line, int *pos) {
    do {
        while (*pos < node_line_width) {
            TstNode<_Tp> *node = (*line)->node_line + (*pos)++;
            if (node == free_list) break;
            if (node->value == 0 && node->middle)
                return node;
        }
        *line = (*line)->next;
        *pos = 0;
    } while (*line);

    return NULL;
}

template<class _Tp>
void Tst<_Tp>::insert(char *key, _Tp data) {
    TstNode<_Tp> *current_node;
    TstNode<_Tp> *new_node_tree_begin = NULL;
    int key_index;
    bool perform_loop = true;
   
    if (key == NULL || key[0] == 0) {
        return;
    }

    if (head[(int) key[0]] == NULL) {
        if (free_list == NULL) growNodeFreeList();
        
        head[(int) key[0]] = free_list;
      
        free_list = free_list->middle;
        current_node = head[(int) key[0]];
        current_node->value = key[1];
        if (key[1] == 0) {
            current_node->middle = (TstNode<_Tp> *) 1;
            current_node->data = data;
            return;
        } else
            perform_loop = false;
    }
        
    current_node = head[(int) key[0]];

    key_index = 1;
    if (perform_loop) {
        for (;;) {
            if (key[key_index] == current_node->value) {
                if (key[key_index] == 0) {
                    current_node->middle = (TstNode<_Tp> *) 1;
                    current_node->data = data;
                    return;
                } else {
                    if (current_node->middle == NULL) {
                        if (free_list == NULL) growNodeFreeList();
                        
                        current_node->middle = free_list;
               
                        free_list = free_list->middle;
                        new_node_tree_begin = current_node;
                        current_node = current_node->middle;
                        current_node->value = key[key_index];
                        break;
                    } else {
                        current_node = current_node->middle;
                        key_index++;
                        continue;
                    }
                }
            }
            
            if (((current_node->value == 0) && (key[key_index] < 64)) ||
                ((current_node->value != 0) && (key[key_index] <
                                                current_node->value))) {
                if (current_node->left == NULL) {
                    if (free_list == NULL) growNodeFreeList();
                    
                    current_node->left = free_list;
            
                    free_list = free_list->middle;
                    new_node_tree_begin = current_node;
                    current_node = current_node->left;
                    current_node->value = key[key_index];
                    if (key[key_index] == 0) {
                        current_node->middle = (TstNode<_Tp> *) 1;
                        current_node->data = data;
                        return;
                    } else
                        break;
                } else {
                
                    current_node = current_node->left;
                    continue;
                }
            } else {
                if (current_node->right == NULL) {
                    if (free_list == NULL) growNodeFreeList();
                    
                    current_node->right = free_list;
            
                    free_list = free_list->middle;
                    new_node_tree_begin = current_node;
                    current_node = current_node->right;
                    current_node->value = key[key_index];
                    break;
                } else {
                    current_node = current_node->right;
                    continue;
                }
            }
        }
    }
        
    do {
        key_index++;
   
        if (free_list == NULL) growNodeFreeList();
            
        current_node->middle = free_list;
            
        free_list = free_list->middle;
        current_node = current_node->middle;
        current_node->value = key[key_index];
    } while (key[key_index] != 0);

    current_node->middle = (TstNode<_Tp> *) 1;
    current_node->data = data;
}

template<class _Tp>
void Tst<_Tp>::remove(char *key) {
    TstNode<_Tp> *current_node;
    TstNode<_Tp> *current_node_parent;
    TstNode<_Tp> *last_branch;
    TstNode<_Tp> *last_branch_parent;
    TstNode<_Tp> *next_node;
    TstNode<_Tp> *last_branch_replacement;
    TstNode<_Tp> *last_branch_dangling_child;
    int key_index;

    if (key[0] == 0)
        return;
    
    if (head[(int) key[0]] == NULL)
        return;
    
    last_branch = NULL;
    last_branch_parent = NULL;
    current_node = head[(int) key[0]];
    current_node_parent = NULL;
    key_index = 1;
    while (current_node != NULL) {
        if (key[key_index] == current_node->value) {
            if ((current_node->left != NULL) ||
                (current_node->right != NULL)) {
                last_branch = current_node;
                last_branch_parent = current_node_parent;
            }
            if (key[key_index] == 0)
                break;
            else {
                current_node_parent = current_node;
                current_node = current_node->middle;
                key_index++;
                continue;
            }
        } else if (((current_node->value == 0) && (key[key_index] < 64)) ||
                   ((current_node->value != 0) && (key[key_index] <
                                                   current_node->value))) {
            last_branch_parent = current_node;
            current_node_parent = current_node;
            current_node = current_node->left;
            last_branch = current_node;
            continue;
        } else {
            last_branch_parent = current_node;
            current_node_parent = current_node;
            current_node = current_node->right;
            last_branch = current_node;
            continue;
        }
    }

    if (current_node == NULL)
        return;
   
    if (last_branch == NULL) {
        next_node = head[(int) key[0]];
        head[(int) key[0]] = NULL;
    } else if ((last_branch->left == NULL) && (last_branch->right == NULL)) {
        if (last_branch_parent->left == last_branch)
            last_branch_parent->left = NULL;
        else
            last_branch_parent->right = NULL;
       
        next_node = last_branch;
    } else {
        if ((last_branch->left != NULL) && (last_branch->right != NULL)) {
            last_branch_replacement = last_branch->right;
            last_branch_dangling_child = last_branch->left;
        } else if(last_branch->right != NULL) {
            last_branch_replacement = last_branch->right;
            last_branch_dangling_child = NULL;
        } else {
            last_branch_replacement = last_branch->left;
            last_branch_dangling_child = NULL;
        }
       
        if (last_branch_parent == NULL)
            head[(int) key[0]] = last_branch_replacement;
        else {
            if (last_branch_parent->left == last_branch)
                last_branch_parent->left = last_branch_replacement;
            else if (last_branch_parent->right == last_branch)
                last_branch_parent->right = last_branch_replacement;
            else
                last_branch_parent->middle = last_branch_replacement;
        }
       
        if (last_branch_dangling_child != NULL) {
            current_node = last_branch_replacement;
           
            while (current_node->left != NULL)
                current_node = current_node->left;
           
            current_node->left = last_branch_dangling_child;
        }
        next_node = last_branch;
    }
   
    do {
        current_node = next_node;
        next_node = current_node->middle;
       
        current_node->left = NULL;
        current_node->right = NULL;
        current_node->middle = free_list;
        free_list = current_node;
    } while (current_node->value != 0);
}

#endif // __Tst_hh
