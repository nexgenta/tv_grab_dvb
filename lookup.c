#include <stdio.h>
#include <string.h>

#include "lookup.h"

extern char* lookup(struct lookup_table *l, int id) {
	
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
