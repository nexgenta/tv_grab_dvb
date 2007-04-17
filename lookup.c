#include <stdio.h>
#include <string.h>

#include "tv_grab_dvb.h"

char *lookup(const struct lookup_table *l, int id) {
	
//	printf("Looked up %x", id);
	for(;(l->id != id)&&(l->id != -1);l++) {

	}
	if (l->desc==NULL) {
		static char number[255];
		sprintf(number,"Unknown ID: %x",id);
		return (char*)strdup(number);
	}
	return (l->desc);
}
