#ifndef __tv_grab_dvd
#define __tv_grab_dvd

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "dvb/dvb.h"

/* lookup.c */
union lookup_key {
	int i;
	char c[4];
};
struct lookup_table {
	union lookup_key u;
	char *desc;
};

extern char *lookup(const struct lookup_table *l, int id);
extern int load_lookup(struct lookup_table **l, const char *file);

/* dvb_info_tables.c */
extern const struct lookup_table description_table[];
extern const struct lookup_table aspect_table[];
extern const struct lookup_table audio_table[];
extern const struct lookup_table crid_type_table[];

/* langidents.c */
extern const struct lookup_table languageid_table[];

/* crc32.c */
extern uint32_t _dvb_crc32(const uint8_t *data, size_t len);

/* dvb_text.c */
extern char *xmlify(const char *s);
extern char *iso6937_encoding;

/* tv_grab_dvb.c */
typedef struct chninfo {
	struct chninfo *next;
	int sid;
	int eid;
	int ver;
} chninfo_t;

extern int timeout;
extern int programme_count;
extern int update_count;
extern int time_offset;
extern int invalid_date_count;
extern bool ignore_bad_dates;
extern bool ignore_updates;

extern struct lookup_table *channelid_table;
extern struct chninfo *channels;

char *get_channelident(int chanid);

/* dvb-eit.c */
extern int parseEIT(void *data, size_t len, dvb_callbacks_t *callbacks);

/* dvb-si.c */
extern int parse_dvb_si(void *data, size_t len, dvb_callbacks_t *callbacks);

#endif
