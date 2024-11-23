// Module to implement the ground controller application-layer protocol.

// The protocol is fully defined in the README file. This module
// includes functions to parse and perform commands sent by an
// airplane (the docommand function), and has functions to send
// responses to ensure proper and consistent formatting of these
// messages.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "util.h"
#include "airplane.h"
#include "airs_protocol.h"
#include "airplanelist.h"
#include "queue.h"

/************************************************************************
 * Call this response function if a command was accepted
 */
void send_ok(airplane *plane) {
    fprintf(plane->fp_send, "OK\n");
}

/************************************************************************
 * Call this response function if a first in the queue
 */
void send_takeoff(airplane *plane) {
    fprintf(plane->fp_send, "TAKEOFF\n");
}

/************************************************************************
 * Call this response function if an error can be described by a simple
 * string.
 */
void send_err(airplane *plane, char *desc) {
    fprintf(plane->fp_send, "ERR %s\n", desc);
}

/************************************************************************
 * Call this response function if you want to embed a specific string
 * argument (sarg) into an error reply (which is now a format string).
 */
void send_err_sarg(airplane *plane, char *fmtstring, char *sarg) {
    fprintf(plane->fp_send, "ERR ");
    fprintf(plane->fp_send, fmtstring, sarg);
    fprintf(plane->fp_send, "\n");
}

/************************************************************************
 * Handle the "REG" command.
 */
static void cmd_reg(airplane *plane, char *rest) {
    if (plane->state != PLANE_UNREG) {
        send_err_sarg(plane, "Already registered as %s", plane->id);
        return;
    }

    if (rest == NULL) {
        send_err(plane, "REG missing flightid");
        return;
    }

    int already_exist = airplane_exist(rest);
    if (already_exist == 1) {
        send_err(plane, "Duplicate flight id");
        return;
    }

    char *cp = rest;
    while (*cp != '\0') {
        if (!isalnum(*cp)) {
            send_err(plane, "Invalid flight id -- only alphanumeric characters allowed");
            return;
        }
        cp++;
    }
    
    if (strlen(rest) > PLANE_MAXID) {
        send_err(plane, "Invalid flight id -- too long");
        return;
    }

    plane->state = PLANE_ATTERMINAL;
    strcpy(plane->id, rest);
    send_ok(plane);
}

/************************************************************************
 * Handle the "REQTAXI" command.
 */
static void cmd_reqtaxi(airplane *plane, char *rest) {
    if (plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }

    plane->state = PLANE_TAXIING;
    send_ok(plane);
    queue_reqtaxi(plane);
    
    //send_err(plane, "REQTAXI command not yet implemented");
}

/************************************************************************
 * Handle the "REQTAXI" command.
 */
void takeoff(airplane *plane) {
    plane->state = PLANE_CLEAR;
    send_takeoff(plane);
    printf("Clearing flight %s\n", plane->id);
}

/************************************************************************
 * Handle the "REQPOS" command.
 */
void cmd_reqpos(airplane *plane, char *rest) {
    if (plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }
    
    if (plane->state != PLANE_TAXIING) {
        send_err(plane, "REQPOS can only be used when the plane is taxiing");
        return;
    }
    
    int position = queue_position(plane->id);

    fprintf(plane->fp_send, "OK %d\n", (position + 1));
    //send_err(plane, "REQPOS command not yet implemented");
}

/************************************************************************
 * Handle the "REQAHEAD" command.
 */
static void cmd_reqahead(airplane *plane, char *rest) {
    if (plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }

    if (plane->state != PLANE_TAXIING) {
        send_err(plane, "REQPOS can only be used when the plane is taxiing");
        return;
    }

    queue_getahead(plane);
    //send_err(plane, "REQTAXI command not yet implemented");
}

/************************************************************************
 * Handle the "INAIR" command.
 */
static void cmd_inair(airplane *plane, char *rest) {
    if (plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }

    if (plane->state != PLANE_CLEAR) {
        send_err(plane, "Not registered to fly");
        return;
    }

    queue_inair(plane);

    //send_err(plane, "INAIR command not yet implemented");
}

/************************************************************************
 * Handle the "BYE" command.
 */
static void cmd_bye(airplane *plane, char *rest) {
    plane->state = PLANE_DONE;
}

/************************************************************************
 * Parses and performs the actions in the line of text (command and
 * optionally arguments) passed in as "command".
 */
void docommand(airplane *plane, char *command) {
    char *saveptr;
    char *cmd = strtok_r(command, " \t\r\n", &saveptr);
    if (cmd == NULL) {  // Empty line (no command) -- just ignore line
        return;
    }

    // Get arguments (everything after command, trimmed)
    char *args = strtok_r(NULL, "\r\n", &saveptr);
    if (args != NULL) {
        args = trim(args);
    }

    if (strcmp(cmd, "REG") == 0) {
        cmd_reg(plane, args);
    } else if (strcmp(cmd, "REQTAXI") == 0) {
        cmd_reqtaxi(plane, args);
    } else if (strcmp(cmd, "REQPOS") == 0) {
        cmd_reqpos(plane, args);
    } else if (strcmp(cmd, "REQAHEAD") == 0) {
        cmd_reqahead(plane, args);
    } else if (strcmp(cmd, "INAIR") == 0) {
        cmd_inair(plane, args);
    } else if (strcmp(cmd, "BYE") == 0) {
        cmd_bye(plane, args);
    } else {
        send_err(plane, "Unknown command");
    }
}