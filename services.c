#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "services.h"

#define SERVICE_URI_SIZE                128

struct service_struct
{
	char uri[SERVICE_URI_SIZE];
	char name[128];
	char provider[128];
	char authority[128];
	service_type_t type;
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
	}
	memset(p, 0, sizeof(service_t));
	p->type = ST_RESERVED_FF;
	strcpy(p->uri, uri);	
	return p;
}

service_t *
service_add_dvb(int original_network_id, int transport_stream_id, int service_id)
{
	char uri[64];
	
	sprintf(uri, "dvb://%04x.%04x.%04x", original_network_id, transport_stream_id, service_id);
	return service_add(uri);
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
service_debug_dump(void)
{
	size_t i;
	
	fprintf(stderr, "----- Services dump (%d allocated, %d defined):\n", (int) nservalloc, (int) nservices);
	for(i = 0; i < nservices; i++)
	{
		fprintf(stderr, " %3d: name='%s', provider='%s', authority='%s', type=0x%02x\n", i, services[i]->name, services[i]->provider, services[i]->authority, services[i]->type);
		fprintf(stderr, "      URI: %s\n", services[i]->uri);
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

		
