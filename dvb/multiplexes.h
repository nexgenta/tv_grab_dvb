#ifndef MULTIPLEXES_H_
# define MULTIPLEXES_H_                 1

# include "platforms.h"

typedef struct mux_struct mux_t;

mux_t *mux_add(const char *uri);
mux_t *mux_add_dvb(int original_network_id, int transport_stream_id);

mux_t *mux_locate(const char *uri);
mux_t *mux_locate_dvb(int original_network_id, int transport_stream_id);

mux_t *mux_locate_add(const char *uri);
mux_t *mux_locate_add_dvb(int original_network_id, int transport_stream_id);

const char *mux_uri(mux_t *mux);

void mux_reset(mux_t *mux);

void mux_set_platform(mux_t *mux, platform_t *platform);
platform_t *mux_platform(mux_t *mux);

void mux_set_data(mux_t *mux, void *data);
void *mux_data(mux_t *mux);

int mux_foreach(int (*fn)(mux_t *mux, void *data), void *data);

#endif /*!MULTIPLEXES_H_*/
