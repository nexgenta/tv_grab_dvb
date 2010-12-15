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

#ifndef NETWORKS_H_
# define NETWORKS_H_                     1

# include "services.h"
# include "multiplexes.h"

typedef struct network_struct network_t;
typedef struct network_service_struct network_service_t;

struct network_service_struct
{
	service_t *service;
	int visible;
	int lcn;
	int sublcn;
};

network_t *network_add(const char *identifier);
network_t *network_add_dvb(int network_id);

network_t *network_locate(const char *identifier);
network_t *network_locate_dvb(int network_id);

void network_reset(network_t *network);

void network_set_name(network_t *network, const char *name);
const char *network_name(network_t *network);

void network_set_version(network_t *network, int version);
int network_version(network_t *network);

void network_add_mux(network_t *network, mux_t *mux);
mux_t **network_muxes(network_t *network, size_t *count);

network_service_t *network_set_service(network_t *network, service_t *service, int visible, int lcn, int sublcn);
network_service_t *network_service(network_t *network, int lcn, int sublcn);
network_service_t **network_services(network_t *network, size_t *count);

void network_debug(network_t *network);
void network_debug_dump(void);

#endif /*!NETWORKS_H_*/
