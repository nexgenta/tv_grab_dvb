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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "networks.h"

#define NETWORK_IDENT_SIZE              32

struct network_struct
{
	char ident[NETWORK_IDENT_SIZE];
	char name[128];
	int version;
	void *data;
	size_t nmux;
	size_t muxalloc;
	mux_t **mux;
	size_t nservice;
	size_t servalloc;
	network_service_t **service;
};

static size_t nnetworks, nnetalloc;
static network_t **networks;

static network_t *network_alloc(void);

network_t *
network_add(const char *ident)
{
	network_t *p;

	if(strlen(ident) >= NETWORK_IDENT_SIZE)
	{
		errno = EINVAL;
		return NULL;
	}
	if(NULL == (p = network_locate(ident)))
	{
		if(NULL == (p = network_alloc()))
		{
			return NULL;
		}
		strcpy(p->ident, ident);
	}
	network_reset(p);
	return p;
}

network_t *
network_add_dvb(int network_id)
{
	char ident[32];

	snprintf(ident, sizeof(ident), "dvb:nid:%04x", network_id);
	return network_add(ident);
}

void
network_reset(network_t *network)
{
	network_t p;

	memset(&p, 0, sizeof(network_t));
	strcpy(p.ident, network->ident);
	p.data = network->data;
	p.version = -1;
	free(network->mux);
	memcpy(network, &p, sizeof(network_t));
}

network_t *
network_locate(const char *ident)
{
	size_t i;
	
	for(i = 0; i < nnetworks; i++)
	{
		if(networks[i] && !strcmp(networks[i]->ident, ident))
		{
			return networks[i];
		}
	}
	return NULL;
}

network_t *
network_locate_dvb(int network_id)
{
	char ident[32];
	
	snprintf(ident, sizeof(ident), "dvb:nit:%04x", network_id);
	return network_locate(ident);
}

/* Locate a network and return it as-is if it already exists, or else create
 * a new network.
 */
network_t *
network_locate_add(const char *ident)
{
	network_t *s;
	
	if((s = network_locate(ident)))
	{
		return s;
	}
	return network_add(ident);
}

network_t *
network_locate_add_dvb(int network_id)
{
	char ident[32];

	snprintf(ident, sizeof(ident), "dvb:nit:%04x", network_id);
	return network_locate_add(ident);
}

const char *
network_identifier(network_t *network)
{
	return network->ident;
}

void
network_set_data(network_t *network, void *data)
{
	network->data = data;
}

void *
network_data(network_t *network)
{
	return network->data;
}

void
network_set_name(network_t *network, const char *name)
{
	strncpy(network->name, name, sizeof(network->name) - 1);
}

const char *
network_name(network_t *network)
{
	if(network->name[0])
	{
		return network->name;
	}
	return NULL;
}

void
network_set_version(network_t *network, int version)
{
	network->version = version;
}

int
network_version(network_t *network)
{
	return network->version;
}

void
network_add_mux(network_t *network, mux_t *mux)
{
	size_t i;
	mux_t **l;
	
	for(i = 0; i < network->nmux; i++)
	{
		if(network->mux[i] == mux)
		{
			return;
		}
	}
	if(network->nmux + 1 > network->muxalloc)
	{
		if(NULL == (l = realloc(network->mux, sizeof(mux_t *) * (network->muxalloc + 4))))
		{
			return;
		}
		network->mux = l;
		network->muxalloc += 4;
	}
	network->mux[network->nmux] = mux;
	network->nmux++;
}

mux_t **
network_muxes(network_t *network, size_t *count)
{
	*count = network->nmux;
	return network->mux;
}

network_service_t *
network_set_service(network_t *network, service_t *service, int visible, int lcn, int sublcn)
{
	network_service_t *srv, **l;
	size_t i;

	for(i = 0; i < network->nservice; i++)
	{
		if(!network->service[i])
		{
			continue;
		}
		if(network->service[i]->lcn == lcn && network->service[i]->sublcn == sublcn)
		{
			if(service)
			{
				network->service[i]->service = service;
				network->service[i]->visible = visible;
				return network->service[i];
			}
			free(network->service[i]);
			network->service[i] = NULL;
			return NULL;
		}
	}
	if(NULL == (srv = calloc(1, sizeof(network_service_t))))
	{
		return NULL;
	}
	srv->service = service;
	srv->visible = visible;
	srv->lcn = lcn;
	srv->sublcn = sublcn;
	if(network->nservice + 1 > network->servalloc)
	{
		if(NULL == (l = realloc(network->service, sizeof(network_service_t *) * (network->servalloc + 16))))
		{
			free(srv);
			return NULL;
		}
		network->service = l;
		network->servalloc += 16;
	}
	network->service[network->nservice] = srv;
	network->nservice++;
	return srv;
}

network_service_t *
network_service(network_t *network, int lcn, int sublcn)
{
	size_t i;

	for(i = 0; i < network->nservice; i++)
	{
		if(network->service[i] && network->service[i]->lcn == lcn && network->service[i]->sublcn == sublcn)
		{
			return network->service[i];
		}
	}
	return NULL;
}

network_service_t **
network_services(network_t *network, size_t *count)
{
	*count = network->nservice;
	return network->service;
}

void
network_debug(network_t *network)
{
	size_t i;

	fprintf(stderr, "Network identifier '%s', version %d, %d associated multiplexes, %d services\n",
			network->ident, network->version, network->nmux, network->nservice);
	fprintf(stderr, "- Name: '%s'\n", network->name);
	for(i = 0; i < network->nmux; i++)
	{
		fprintf(stderr, "- Multiplex %d: '%s'\n", i, mux_uri(network->mux[i]));
	}
	for(i = 0; i < network->nservice; i++)
	{
		if(!network->service[i])
		{
			continue;
		}
		if(network->service[i]->sublcn == -1)
		{
			fprintf(stderr, "# %3d    %s - '%s' %c\n", 
					network->service[i]->lcn,
					service_uri(network->service[i]->service),
					service_name(network->service[i]->service),
					(network->service[i]->visible ? ' ' : 'H'));
		}
		else
		{
			fprintf(stderr, "# %3d.%-3d %s - '%s' %c\n", 
					network->service[i]->lcn,
					network->service[i]->sublcn,
					service_uri(network->service[i]->service),
					service_name(network->service[i]->service),
					(network->service[i]->visible ? ' ' : 'H'));
		}
	}
}

void
network_debug_dump(void)
{
	size_t i;
	
	fprintf(stderr, "----- Network dump (%d allocated, %d defined):\n", (int) nnetalloc, (int) nnetworks);
	for(i = 0; i < nnetworks; i++)
	{
		network_debug(networks[i]);
	}
}

static network_t *
network_alloc(void)
{
	network_t **l, *p;
	
	if(NULL == (p = (network_t *) calloc(1, sizeof(network_t))))
	{
		return NULL;
	}  
	if(nnetworks + 1 > nnetalloc)
	{
		if(NULL == (l = (network_t **) realloc(networks, sizeof(network_t *) * (nnetalloc + 4))))
		{
			free(p);
			return NULL;
		}
		networks = l;
		nnetalloc += 4;
	}
	networks[nnetworks] = p;
	nnetworks++;
	return p;
}

		
