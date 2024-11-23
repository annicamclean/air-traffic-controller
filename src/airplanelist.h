#ifndef _AIRPLANELIST_H
#define _AIRPLANELIST_H

#include <pthread.h>

#include "airplane.h"
#include "alist.h"

#define DEF_CAPACITY 10



void airplanelist_init(void (*data_free)(void *data));
void airplanelist_clear();
int airplanelist_is_empty();
int airplanelist_size();
airplane *airplanelist_get(int index);
void airplanelist_add(airplane* val);
void airplanelist_set(int index, airplane* newval);
void airplanelist_remove(airplane* airplane);
void airplanelist_destroy();
void destroy_airplane(void *a);
void airplanelist_print();
int airplane_exist(char* plane_id);
airplane* queue_to_airplanelist(char* next_plane_id);

#endif