#ifndef _QUEUE_H
#define _QUEUE_H

#include <pthread.h>

#include "airplane.h"
#include "airs_protocol.h"
#include "alist.h"

#define DEF_CAPACITY 10



void queue_init(void (*data_free)(void *data));
void queue_free(void *queue_item);
void queue_clear();
int queue_is_empty();
int queue_size();
char* queue_get(int index);
void queue_add(char* val);
void queue_set(int index, char* newval);
void queue_remove(char* plane_id);
void queue_destroy();
int queue_position(char* plane_id);
void queue_print();
int queue_exist(char* plane_id);
void queue_reqtaxi(airplane* plane);
void queue_getahead(airplane* plane);
void queue_inair(airplane* plane);



#endif