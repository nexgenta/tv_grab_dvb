#ifndef __lookup
#define __lookup

struct lookup_table {
	int id;
	char *desc;
};

char* lookup(struct lookup_table *l, int id);
#endif

