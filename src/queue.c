#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "alist.h"
#include "airplanelist.h"
#include "airs_protocol.h"
#include "airplane.h"


static alist queue;

//create a lock for airplane list
pthread_rwlock_t queue_lock;
pthread_t qtid[10];
pthread_mutex_t queue_mutex;
pthread_cond_t queue_not_empty;
pthread_cond_t in_air_command;

/***************************************************************************
 * airplanelist_init initializes an array list to empty and with the default
 * capacity and initilizes th thread.
 */
void queue_free(void *item) {
    //free(item);
}

void* process_queue(void*) {
    while (1) {
        
        pthread_mutex_lock(&queue_mutex);

        while (queue.in_use == 0) {
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }


        char* plane_id = alist_get(&queue, 0);
        
        if (strcmp(plane_id, alist_get(&queue, 0))){
             continue;
        }

        airplane* plane = queue_to_airplanelist(plane_id);
        // Send response back to client
        plane->state = PLANE_CLEAR;
        printf("Clearing flight %s\n", plane_id);
        send_takeoff(plane);

        while (plane->state != PLANE_INAIR) {
            pthread_cond_wait(&in_air_command, &queue_mutex);
        }
        
        plane->state = PLANE_DONE;
        alist_remove(&queue, 0);
        pthread_mutex_unlock(&queue_mutex);
        sleep(4); 

    }
}

/***************************************************************************
 * airplanelist_init initializes an array list to empty and with the default
 * capacity and initilizes th thread.
 */
void queue_init(void (*data_free)(void *data)) {
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    alist_init(&queue, queue_free);
    //init the lock
    pthread_rwlock_init(&(queue_lock), NULL);
    pthread_create(&qtid[0], NULL, process_queue, NULL); 
}

/***************************************************************************
 * airplanelist_clear resets the size of the array list to 0 
 * (empties the alist).
 */
void queue_clear() {
    alist_clear(&queue);
}

/***************************************************************************
 * airplanelist_is_empty returns true if and only if the list of airplanes 
 * is empty.
 */
int queue_is_empty() {
    return alist_is_empty(&queue);
}

/***************************************************************************
 * airplanelist_size returns the size of the array list
 */
int queue_size() {
    int size = alist_size(&queue);
    return size;
}

/***************************************************************************
 * airplanelist_get returns the value at array index "index", or NULL if 
 * this is an invalid index.
 */
char* queue_get(int index) {
    pthread_rwlock_wrlock(&(queue_lock));
    char *retval = alist_get(&queue, index);
    pthread_rwlock_unlock(&(queue_lock));
    return retval;
}

/***************************************************************************
 * airplanelist_add appends a new value to the end of the array list.
 */
void queue_add(char* val) {
    pthread_rwlock_wrlock(&(queue_lock));
    alist_add(&queue, val);
    pthread_rwlock_unlock(&(queue_lock));
}

/***************************************************************************
 * airplanelist_set sets the index "index" airplane to value "newval". If the
 * index/position doesn't exist in the list, then nothing happens (the
 * request is ignored).
 */
void queue_set(int index, char* newval) {
    pthread_rwlock_wrlock(&(queue_lock));
    alist_set(&queue, index, newval);
    pthread_rwlock_unlock(&(queue_lock));
}

/***************************************************************************
 * airplanelist_remove take the element at index "index" out of the list
 * (decreasing list size by 1). If the index/position doesn't exist in
 * the list, then nothing happens.
 */
void queue_remove(char* plane_id) {
    int index = -1;
    for (size_t i = 0; i < queue.in_use; i++) {
        char* comapare_airplane_id = queue_get(i);
        if (strcmp(plane_id, comapare_airplane_id) == 0) {
            index = i;
        }
    }
    if (index <= -1) {
        return;
    }
    
    pthread_rwlock_wrlock(&(queue_lock));
    alist_remove(&queue, index);
    pthread_rwlock_unlock(&(queue_lock));
}

/***************************************************************************
 * airplanelist_destroy destroys the current array list, freeing up all memory
 * and resources.
 */
void queue_destroy() {
    alist_destroy(&queue);
    pthread_rwlock_destroy(&queue_lock);
    pthread_cond_destroy(&queue_not_empty);
    pthread_cond_destroy(&in_air_command);
}

/***************************************************************************
 * airplanelist_destroy destroys the current array list, freeing up all memory
 * and resources.
 */
int queue_position(char* plane_id) {
    int position = 0;
    for (size_t i = 0; i < queue.in_use; i++) {
        char* compare_id = queue_get(i);
        int compared_value = strcmp(plane_id, compare_id);
        if (compared_value == 0) {
            position = i;
        }
        
    }
    return position; 
}



/***************************************************************************
 * airplanelist_print prints out list of airplanes in the order they were
 * registered. Used for debugging the program.
 */
void queue_print() {
    printf("Current Queue\n");
    //pthread_rwlock_wrlock(&(airplanelist_lock));
    for (size_t i = 0; i < queue.in_use; i++) {
        char* plane_id = queue_get(i);
        
        printf("%ld. %s\n", (i+1), plane_id);
    }
    //pthread_rwlock_unlock(&(airplanelist_lock));
}

/***************************************************************************
 * airplane_exist will return true the airplane id that is trying to be 
 * registered already exist in the list of airplanes
 */
int queue_exist(char* plane_id) {
    int already_exist = 0;
    for (size_t i = 0; i < queue.in_use; i++) {
        char* compare_id = queue_get(i);
        int compared_value = strcmp(plane_id, compare_id);
        if (compared_value == 0) {
            already_exist = 1;
            return already_exist;
        }
    }
    return already_exist;
    
}

void queue_reqtaxi(airplane* plane) {
    queue_add(plane->id);
    pthread_cond_signal(&queue_not_empty);
}

void queue_getahead(airplane* plane) {
    char* message = "OK ";
    char *list = malloc((PLANE_MAXID + 3) * queue_position(plane->id));
    strcat(list, message);

    int position = queue_position(plane->id);
    for (size_t i = 0; i < position; i++) {
        char* plane_id = queue_get(i);
        strcat(list, plane_id);
        if (i < (position - 1)) {
            strcat(list, ", ");
        }
        
    }
    
    fprintf(plane->fp_send, "%s\n", list);
    free(list);
}

void queue_inair(airplane* plane) {
    send_ok(plane);
    printf("Client %ld disconnected. \n", plane->tid);
    plane->state = PLANE_INAIR;
    fprintf(plane->fp_send, "NOTICE Disconnecting from ground control - please connect to air control\n");
    printf("Flight %s is in the air -- waiting 4 seconds\n", plane->id);
    pthread_cond_signal(&in_air_command);
}