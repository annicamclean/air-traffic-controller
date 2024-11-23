#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "alist.h"
#include "airplanelist.h"
#include "airplane.h"


static alist airplanelist;

//create a lock for airplane list
pthread_rwlock_t airplanelist_lock;

/*void airplane_free(void *plane) {
    airplane *new_plane = (airplane *)plane;
    airplane_destroy(new_plane);
    free(new_plane);
}*/

/***************************************************************************
 * airplanelist_init initializes an array list to empty and with the default
 * capacity and initilizes th thread.
 */
void airplanelist_init(void (*data_free)(void *data)) {
    alist_init(&airplanelist, free);
    //init the lock
    pthread_rwlock_init(&(airplanelist_lock), NULL);
}

/***************************************************************************
 * airplanelist_clear resets the size of the array list to 0 
 * (empties the alist).
 */
void airplanelist_clear() {
    alist_clear(&airplanelist);
}

/***************************************************************************
 * airplanelist_is_empty returns true if and only if the list of airplanes 
 * is empty.
 */
int airplanelist_is_empty() {
    return alist_is_empty(&airplanelist);
}

/***************************************************************************
 * airplanelist_size returns the size of the array list
 */
int airplanelist_size() {
    int size = alist_size(&airplanelist);
    return size;
}

/***************************************************************************
 * airplanelist_get returns the value at array index "index", or NULL if 
 * this is an invalid index.
 */
airplane *airplanelist_get(int index) {
    pthread_rwlock_wrlock(&(airplanelist_lock));
    airplane *retval = alist_get(&airplanelist, index);
    pthread_rwlock_unlock(&(airplanelist_lock));
    return retval;
}

/***************************************************************************
 * airplanelist_add appends a new value to the end of the array list.
 */
void airplanelist_add(airplane *val) {
    pthread_rwlock_wrlock(&(airplanelist_lock));
    alist_add(&airplanelist, val);
    pthread_rwlock_unlock(&(airplanelist_lock));
}

/***************************************************************************
 * airplanelist_set sets the index "index" airplane to value "newval". If the
 * index/position doesn't exist in the list, then nothing happens (the
 * request is ignored).
 */
void airplanelist_set(int index, airplane* newval) {
    pthread_rwlock_wrlock(&(airplanelist_lock));
    alist_set(&airplanelist, index, newval);
    pthread_rwlock_unlock(&(airplanelist_lock));
}

/***************************************************************************
 * airplanelist_remove take the element at index "index" out of the list
 * (decreasing list size by 1). If the index/position doesn't exist in
 * the list, then nothing happens.
 */
void airplanelist_remove(airplane* myairplane) {
    
    int index = -1;
    for (size_t i = 0; i < airplanelist.in_use - 1; i++) {
        airplane* comapare_airplane = airplanelist_get(i);
        char* airplane_id = comapare_airplane->id;
        if (strcmp(airplane_id, myairplane->id) == 0) {
            index = i;
        }
    }
    pthread_rwlock_wrlock(&(airplanelist_lock));
    alist_remove(&airplanelist, index);
    pthread_rwlock_unlock(&(airplanelist_lock));
    airplane_destroy(myairplane);
}

/***************************************************************************
 * airplanelist_destroy destroys the current array list, freeing up all memory
 * and resources.
 */
void airplanelist_destroy() {
    alist_destroy(&airplanelist);
    pthread_rwlock_destroy(&airplanelist_lock);
}

/***************************************************************************
 * airplanelist_print prints out list of airplanes in the order they were
 * registered. Used for debugging the program.
 */
void airplanelist_print() {
    printf("Current Airplane List\n");
    //pthread_rwlock_wrlock(&(airplanelist_lock));
    for (size_t i = 0; i < airplanelist.in_use; i++) {
        airplane* new_airplane = airplanelist_get(i);
        
        printf("%ld. %s\n", (i+1), new_airplane->id);
    }
    //pthread_rwlock_unlock(&(airplanelist_lock));
}

/***************************************************************************
 * airplane_exist will return true the airplane id that is trying to be 
 * registered already exist in the list of airplanes
 */
int airplane_exist(char* plane_id) {
    int already_exist = 0;
    //pthread_rwlock_rdlock(&(airplanelist_lock));
    for (size_t i = 0; i < airplanelist.in_use; i++) {
        airplane* compare_plane = airplanelist_get(i);
        char* compare_id = compare_plane->id;
        int compared_value = strcmp(plane_id, compare_id);
        if (compared_value == 0 && (compare_plane->state != PLANE_UNREG)) {
            already_exist = 1;
            //pthread_rwlock_unlock(&(airplanelist_lock));
            return already_exist;
        }
        
    }
    //pthread_rwlock_unlock(&(airplanelist_lock));
    return already_exist;
    
}

airplane* queue_to_airplanelist(char* next_plane_id) {
    airplane* compare_plane;
    for (size_t i = 0; i < airplanelist_size(); i++) {
        compare_plane = airplanelist_get(i);
        char* compare_id = compare_plane->id;
        int compared_value = strcmp(next_plane_id, compare_id);

        if (compared_value == 0) {
            return compare_plane; 
        }
        
    }

    perror("Plane Could Not Be Found");
    return compare_plane;
}