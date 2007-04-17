#ifndef __tv_grab_dvd
#define __tv_grab_dvd

#include <stdint.h>
#include <stdlib.h>

/* lookup.c */
struct lookup_table {
	int id;
	char *desc;
};

extern char *lookup(struct lookup_table *l, int id);

/* dvb_info_tables.c */
extern const struct lookup_table description_table[];
extern const struct lookup_table aspect_table[];
extern const struct lookup_table audio_table[];

/* crc32.c */
extern uint32_t _dvb_crc32(const uint8_t *data, size_t len);

#endif
