// gndcontrol.h

#ifndef GNDCONTROL_H
#define GNDCONTROL_H

#include "airplane.h"
#include "alist.h"

// Function declarations
void init_airplane_list();
int find_airplane_id(const char *target_id);
void *thread_handler(void *arg);
int create_listener(char *service);
void docommand(airplane *plane, char *command);

#endif  // GNDCONTROL_H
