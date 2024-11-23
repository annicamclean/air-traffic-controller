// The airplane module contains the airplane data type and management functions

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "airplane.h"

/************************************************************************
 * plane_init initializes an airplane structure in the initial PLANE_UNREG
 * state, with given send and receive FILE objects.
 */

airplane* airplane_create(int _comm_fd){
    airplane* new_plane = malloc(sizeof(airplane));
    if (new_plane == NULL) {
        perror("new_airplane");
        exit(1);
    }

    int duplicated_fd = dup(_comm_fd);
    if (duplicated_fd < 0) {
        perror("new_airplane dup");
        free(new_plane);
        return NULL;
    }

    FILE* sender = fdopen(_comm_fd, "w");
    if (sender == NULL) {
        perror("new_airplane fd_open sender");
        close(duplicated_fd);
        close(_comm_fd);
        free(new_plane);
        return NULL;
    }

    FILE* receiver = fdopen(duplicated_fd, "r");
     if (receiver == NULL) {
        perror("new_airplane fd_open receiver");
        fclose(sender);
        close(duplicated_fd);
        free(new_plane);
        return NULL;
    }

    //this makes the sender and receiver line buffered so that it will send something after
    //  every line instead of at the end
    setvbuf(sender, NULL, _IOLBF, 0);
    setvbuf(receiver, NULL, _IOLBF, 0);

    airplane_init(new_plane, sender, receiver);

    return new_plane;
}

void airplane_init(airplane *plane, FILE* fp_send, FILE* fp_recv) {
    plane->state = PLANE_UNREG;
    plane->fp_send = fp_send;
    plane->fp_recv = fp_recv;
    plane->id[0] = '\0';
}

/************************************************************************
 * plane_destroy frees up any resources associated with an airplane, like
 * file handles, so that it can be free'ed.
 */
void airplane_destroy(airplane *plane) {
    plane->state = PLANE_DONE;
    fclose(plane->fp_send);
    fclose(plane->fp_recv);
}
