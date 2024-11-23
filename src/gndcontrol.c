// This is the main program for the air traffic ground control server.

// The job of this module is to set the system up and then turn over control
// to the airs_protocol module which will handle the actual communication
// protocol between clients (airplanes) and the server.

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "airplane.h"
#include "airs_protocol.h"
#include "airplanelist.h"
#include "queue.h"

struct global_state {
        int clients_connected;
} global_state;

int create_listener(char *service) {
    int sock_fd;
    if ((sock_fd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // Avoid time delay in reusing port - important for debugging, but
    // probably not used in a production server.

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // First, use getaddrinfo() to fill in address struct for later bind

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    struct addrinfo *result;
    int rval;
    if ((rval=getaddrinfo(NULL, service, &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rval));
        close(sock_fd);
        return -1;
    }

    // Assign a name/addr to the socket - just blindly grabs first result
    // off linked list, but really should be exactly one struct returned.

    int bret = bind(sock_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    result = NULL;  // Not really necessary, but ensures no use-after-free

    if (bret < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    // Finally, set up listener connection queue
    int lret = listen(sock_fd, 128);
    if (lret < 0) {
        perror("listen");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

void* handle_conn(void* arg) {
    airplane* myplane = (airplane*) arg;
    airplanelist_add(myplane);

    pthread_detach(myplane->tid);

    char* lineptr = NULL;
    size_t linesize = 0;


    while (myplane->state != PLANE_DONE) {
        if (getline(&lineptr, &linesize, myplane->fp_recv) < 0) {
            // Failed getline means the client disconnected
            break;
        }
        docommand(myplane, lineptr);
    }
    free(lineptr);
    if (airplane_exist(myplane->id) == 1) {
        airplanelist_remove(myplane);
    }
    if (queue_exist(myplane->id) == 1) {
        queue_remove(myplane->id);
    }
    
    
    global_state.clients_connected--;
    if (global_state.clients_connected == 0) {
        airplanelist_clear();
        queue_clear();
    }
    return NULL;
}

/************************************************************************
 * Part 1 main: Only 1 airplane, doing I/O via stdin and stdout.
 */
int main(int argc, char *argv[]) {

    int sock_fd = create_listener("8080");
    if (sock_fd < 0) {
        fprintf(stderr, "Server setup failed.\n");
        exit(1);
    }


    airplanelist_init(free);
    queue_init(free);

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int comm_fd;


    
    while ((comm_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len)) >= 0) {
        if (comm_fd == -1) continue;
        airplane* new_plane = airplane_create(comm_fd);

        pthread_create(&new_plane->tid, NULL, handle_conn, new_plane);
        

        printf("Got connection from %s (client %ld)\n", 
            inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr), 
            new_plane->tid);
        global_state.clients_connected++;
    }
    airplanelist_destroy();
    queue_destroy();
    return 0;
}
