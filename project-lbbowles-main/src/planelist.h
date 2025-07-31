// Function prototypes for the planelist functions

#ifndef _PLANELIST_H
#define _PLANELIST_H

#include "airplane.h"

void planelist_init(void);
void planelist_add(airplane *newplane);
void planelist_changeid(airplane *plane, char *newid);
airplane *planelist_find(char *flightid);
void planelist_remove(airplane *myplane);
void planelist_clearflight(char *flight_id);

#endif  // _PLANELIST_H