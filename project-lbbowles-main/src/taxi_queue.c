#include "taxi_queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "airplane.h"
#include "alist.h"
#include "planelist.h"
#define TIME_INTERVAL 4

queue request_queue;
pthread_mutex_t queue_mutex;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;
int takeoff_controller = 0;  // if 0 then block if 1 then clear for takeoff
pthread_cond_t wait_time_passed = PTHREAD_COND_INITIALIZER;

/***************************************************************************
 * Initializes the queue and all of the required mutexs and conditions that
 * are needed for the takeoff procedures.
 */
void queue_init() {
    pthread_mutex_init(&queue_mutex, NULL);  // Initialize the mutex and condition variable before creating the thread
    pthread_cond_init(&queue_not_empty, NULL);

    alist_init(&request_queue.list, free);  // Initialize the queue

    pthread_t takeoff_handler;  // Create the actual thread mentioned in the assignment meant to handle the queue
    pthread_create(&takeoff_handler, NULL, queue_handler_thread, NULL);
}

/***************************************************************************
 * Called by the queue_enqueue it uses the alist function add for simplicity
 */
static void queue_enqueue_no_lock(queue *q, void *item) {
    alist_add(&q->list, item);
}

/***************************************************************************
 * Both this and the above code taken from the Examples but is a thread-safe
 * way to add values to the queue
 */
void queue_enqueue(void *item) {
    pthread_mutex_lock(&queue_mutex);
    queue_enqueue_no_lock(&request_queue, item);
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_mutex);
}

/***************************************************************************
 * Destroy the queue when this is called using the alist code already made
 */
void queue_destroy(queue *q) {
    alist_destroy(&q->list);
}

/***************************************************************************
 * taxiqueue_inair is used when the client sends INAIR to their shell this
 * should effectively send the required messages and confirm that all the
 * requirements (queue position, plane state) are applicable, if they are
 * the plane will be allowed to fly out and the safety time will be added
 */

void taxiqueue_inair(airplane *plane, const char *flight_id) {
    pthread_mutex_lock(&queue_mutex);  // lock

    const char *current_flight_id = (const char *)alist_get(&request_queue.list, 0);  // Get current flight at head of queue

    if (strcmp(current_flight_id, flight_id) == 0) {                                                          // Make sure the head of queue id is the same as the plane that is calling
        fprintf(plane->fp_send, "OK\n");                                                                      // Send OK
        fprintf(plane->fp_send, "NOTICE Disconnecting from ground control - please connect to air control");  // Send notice message
        plane->state = PLANE_INAIR;                                                                           // Change the plane state
        printf("Flight %s is in the air -- waiting 4 seconds\n", plane->id);                                  // Send message to server
        sleep(TIME_INTERVAL);                                                                                 // Wait while the plane is in air
        takeoff_controller = 1;                                                                               // After it is cleared, set the controller back to 1 temporarily
        plane->state = PLANE_DONE;                                                                            // Plane is now cleared and is done
    } else {
        fprintf(plane->fp_send, "Plane not next in queue\n");  // If not next in queue send this message
    }

    pthread_mutex_unlock(&queue_mutex);  // Safe to unlock
}

/***************************************************************************
 * If the client requests to see their position, this will be used to show
 * what place their plane that has a taxi is at
 */
void req_pos_request(airplane *plane, const char *flight_id) {
    pthread_mutex_lock(&queue_mutex);  // Lock the queue for safety

    int index = -1;  // Set index to a number that is not something that can be in the queue as we do not yet know what it is

    for (int i = 0; i < alist_size(&request_queue.list); i++) {                           // For the size of the queue iterate through it until it reaches the last flight_id
        const char *current_flight_id = (const char *)alist_get(&request_queue.list, i);  // put the value of the id that is at the current position into "current_flight_id"
        if (strcmp(current_flight_id, flight_id) == 0) {                                  // Compare the current value to the flight_id that was passed
            index = i;                                                                    // If the same value, then the index is determined set the value that was -1 to the value that it matches
            break;                                                                        // Safe to break now
        }
    }

    fprintf(plane->fp_send, "OK ");  // Print ok and space as is shown in the video

    if (index != -1) {                             // If the index is something that was not -1 (original value that indicates nothing was found) then we can print
        fprintf(plane->fp_send, "%d", index + 1);  // We are going to print to the plane using the plane information, we have to add 1 to the index number though since it starts at 0
        fprintf(plane->fp_send, "\n");             // Break line
    } else {
        fprintf(plane->fp_send, "Plane with ID %s not found in the list.\n", flight_id);  // This shouldn't happen but my compiler was upset I did not have this
    }

    pthread_mutex_unlock(&queue_mutex);  // Unlock the queue it is now safe to do so
}

void req_ahead_request(airplane *plane, const char *flight_id) {
    pthread_mutex_lock(&queue_mutex);  // Lock the queue for safety

    int index = -1;  // Set index to a number that is not something that can be in the queue as we do not yet know what it is

    for (int i = 0; i < alist_size(&request_queue.list); i++) {  // First multiple lines of this are the exact same as the REQPOS above
        const char *current_flight_id = (const char *)alist_get(&request_queue.list, i);

        if (strcmp(current_flight_id, flight_id) == 0) {
            index = i;
            break;
        }
    }

    fprintf(plane->fp_send, "OK ");

    if (index != -1) {
        for (int i = 0; i < index; i++) {                                                     // Since we have to print this is where there will be differences, for loop so we can get to the location our desired value was saved in the queue
            const char *current_flight_id = (const char *)alist_get(&request_queue.list, i);  // Get the value of the current location and save to a temp number
            fprintf(plane->fp_send, "%s", current_flight_id);                                 // print to the client shell the current values flight id

            if (i < index - 1) {
                fprintf(plane->fp_send, ", ");  // If we are still not at the last index location commas are needed between the ids
            }
        }

        fprintf(plane->fp_send, "\n");  // At the end, breakline
    } else {
        fprintf(plane->fp_send, "Plane with ID %s not found in the list.\n", flight_id);  // Again this shouldn't necessarily happen but is good practice in case something does go wrong
    }

    pthread_mutex_unlock(&queue_mutex);  // Unlock the queue
}

/***************************************************************************
 * This is the thread that will be controlling all of the information for
 * takeoffs
 */
void *queue_handler_thread(void *arg) {
    while (1) {  // Continue indefinitely
        pthread_mutex_lock(&queue_mutex);       // lock

        while (alist_size(&request_queue.list) == 0) {      // Wait until there is something in the queue 
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

        char *flight_id = (char *)alist_get(&request_queue.list, 0);  // Get the flight id from the current first in the queue
        planelist_clearflight(flight_id);                             // Clear the flight that is at the first for takeoff

        if (takeoff_controller == 1) {             // If takeoff controller is 1, then next flight is able to clear, continue operations as normal 
            alist_remove(&request_queue.list, 0);  // If the last plane has flown away and the controller is set to 1 remove the head of the queue allowing the next head/plane access
            takeoff_controller = 0;                // Reset the controller to 0 and continue
        } else {

           pthread_mutex_unlock(&queue_mutex);
           usleep(100000);  // Sleep for a few moments before the queue repeats or else the program hangs at this point
        }
    }
    //queue_detroy(request_queue); //Did not get to the full clean up
    return NULL;
}