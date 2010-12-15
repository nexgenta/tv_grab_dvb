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

#include "services.h"
#include "multiplexes.h"

#define SERVICE_URI_SIZE                128

struct service_struct
{
	char uri[SERVICE_URI_SIZE];
	char name[128];
	char provider[128];
	char authority[128];
	service_type_t type;
	void *data;
	int version;
	mux_t *mux;
};

static size_t nservices, nservalloc;
static service_t **services;

static service_t *service_alloc(void);

service_t *
service_add(const char *uri)
{
	service_t *p;

	if(strlen(uri) >= SERVICE_URI_SIZE)
	{
		errno = EINVAL;
		return NULL;
	}
	if(NULL == (p = service_locate(uri)))
	{
		if(NULL == (p = service_alloc()))
		{
			return NULL;
		}
		strcpy(p->uri, uri);
	}
	service_reset(p);
	return p;
}

service_t *
service_add_dvb(int original_network_id, int transport_stream_id, int service_id)
{
	char uri[64];
	
	sprintf(uri, "dvb://%04x.%04x.%04x", original_network_id, transport_stream_id, service_id);
	return service_add(uri);
}

void
service_reset(service_t *service)
{
	service_t p;

	memset(&p, 0, sizeof(service_t));
	strcpy(p.uri, service->uri);
	p.data = service->data;
	p.version = -1;
	p.type = ST_RESERVED_FF;
	memcpy(service, &p, sizeof(service_t));
}

service_t *
service_locate(const char *uri)
{
	size_t i;
	
	for(i = 0; i < nservices; i++)
	{
		if(services[i] && !strcmp(services[i]->uri, uri))
		{
			return services[i];
		}
	}
	return NULL;
}

service_t *
service_locate_dvb(int original_network_id, int transport_stream_id, int service_id)
{
	char uri[64];
	
	sprintf(uri, "dvb://%04x.%04x.%04x", original_network_id, transport_stream_id, service_id);
	return service_locate(uri);
}

/* Locate a service and return it as-is if it already exist, or else create
 * a new service.
 */
service_t *
service_locate_add(const char *uri)
{
	service_t *s;
	
	if((s = service_locate(uri)))
	{
		return s;
	}
	return service_add(uri);
}

service_t *
service_locate_add_dvb(int original_network_id, int transport_stream_id, int service_id)
{
	char uri[64];
	
	sprintf(uri, "dvb://%04x.%04x.%04x", original_network_id, transport_stream_id, service_id);
	return service_locate_add(uri);
}

const char *
service_uri(service_t *service)
{
	return service->uri;
}

void
service_set_data(service_t *service, void *data)
{
	service->data = data;
}

void *
service_data(service_t *service)
{
	return service->data;
}

void
service_set_type(service_t *service, service_type_t type)
{
	service->type = type;
}

service_type_t
service_type(service_t *service)
{
	return service->type;
}

void
service_set_mux(service_t *service, mux_t *mux)
{
	service->mux = mux;
}

mux_t *
service_mux(service_t *service)
{
	return service->mux;
}

void
service_set_name(service_t *service, const char *name)
{
	strncpy(service->name, name, sizeof(service->name) - 1);
}

const char *
service_name(service_t *service)
{
	if(service->name[0])
	{
		return service->name;
	}
	return NULL;
}

void
service_set_provider(service_t *service, const char *provider)
{
	strncpy(service->provider, provider, sizeof(service->provider) - 1);
}

const char *
service_provider(service_t *service)
{
	if(service->provider[0])
	{
		return service->provider;
	}
	return NULL;
}

void
service_set_authority(service_t *service, const char *authority)
{
	strncpy(service->authority, authority, sizeof(service->authority) - 1);
}

const char *
service_authority(service_t *service)
{
	if(service->authority[0])
	{
		return service->authority;
	}
	return NULL;
}

void
service_debug(service_t *service)
{
	fprintf(stderr, " name='%s', provider='%s', authority='%s', type=0x%02x\n", service->name, service->provider, service->authority, service->type);
	fprintf(stderr, "      URI: %s\n", service->uri);
}

void
service_debug_dump(void)
{
	size_t i;
	
	fprintf(stderr, "----- Services dump (%d allocated, %d defined):\n", (int) nservalloc, (int) nservices);
	for(i = 0; i < nservices; i++)
	{
		fprintf(stderr, " %3d: ", i);
		service_debug(services[i]);
	}
}

static service_t *
service_alloc(void)
{
	service_t **l, *p;
	
	if(NULL == (p = (service_t *) calloc(1, sizeof(service_t))))
	{
		return NULL;
	}  
	if(nservices + 1 > nservalloc)
	{
		if(NULL == (l = (service_t **) realloc(services, sizeof(service_t *) * (nservalloc + 16))))
		{
			free(p);
			return NULL;
		}
		services = l;
		nservalloc += 16;
	}
	services[nservices] = p;
	nservices++;
	return p;
}

int
service_foreach(int (*fn)(service_t *mux, void *data), void *data)
{
	size_t n;
	int r;

	for(n = 0; n < nservices; n++)
	{
		if(services[n])
		{
			if((r = fn(services[n], data)) != 0)
			{
				return r;
			}
		}
	}
	return 0;
}

		
