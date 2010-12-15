/*
 * Copyright 2010 Mo McRoberts.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

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
