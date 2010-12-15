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

#include "multiplexes.h"
#include "platforms.h"

#define MUX_URI_SIZE                    128

struct mux_struct
{
	char uri[MUX_URI_SIZE];
	void *data;
	platform_t *platform;
};

static size_t nmultiplexes, nmuxalloc;
static mux_t **multiplexes;

static mux_t *mux_alloc(void);

mux_t *
mux_add(const char *uri)
{
	mux_t *p;

	if(strlen(uri) >= MUX_URI_SIZE)
	{
		errno = EINVAL;
		return NULL;
	}
	if(NULL == (p = mux_locate(uri)))
	{
		if(NULL == (p = mux_alloc()))
		{
			return NULL;
		}
		strcpy(p->uri, uri);
	}
	mux_reset(p);
	return p;
}

mux_t *
mux_add_dvb(int original_network_id, int transport_stream_id)
{
	char uri[64];
	
	sprintf(uri, "dvb://%04x.%04x", original_network_id, transport_stream_id);
	return mux_add(uri);
}

void
mux_reset(mux_t *multiplex)
{
	mux_t p;

	memset(&p, 0, sizeof(mux_t));
	strcpy(p.uri, multiplex->uri);
	p.data = multiplex->data;
	memcpy(multiplex, &p, sizeof(mux_t));
}

mux_t *
mux_locate(const char *uri)
{
	size_t i;
	
	for(i = 0; i < nmultiplexes; i++)
	{
		if(multiplexes[i] && !strcmp(multiplexes[i]->uri, uri))
		{
			return multiplexes[i];
		}
	}
	return NULL;
}

mux_t *
mux_locate_dvb(int original_network_id, int transport_stream_id)
{
	char uri[64];
	
	sprintf(uri, "dvb://%04x.%04x", original_network_id, transport_stream_id);
	return mux_locate(uri);
}

mux_t *
mux_locate_add(const char *uri)
{
	mux_t *s;
	
	if((s = mux_locate(uri)))
	{
		return s;
	}
	return mux_add(uri);
}

mux_t *
mux_locate_add_dvb(int original_network_id, int transport_stream_id)
{
	char uri[64];
	
	sprintf(uri, "dvb://%04x.%04x", original_network_id, transport_stream_id);
	return mux_locate_add(uri);
}

const char *
mux_uri(mux_t *multiplex)
{
	return multiplex->uri;
}

void
mux_set_data(mux_t *multiplex, void *data)
{
	multiplex->data = data;
}

void *
mux_data(mux_t *multiplex)
{
	return multiplex->data;
}

void
mux_set_platform(mux_t *multiplex, platform_t *platform)
{
	multiplex->platform = platform;
}

platform_t *
mux_platform(mux_t *multiplex)
{
	return multiplex->platform;
}

static mux_t *
mux_alloc(void)
{
	mux_t **l, *p;
	
	if(NULL == (p = (mux_t *) calloc(1, sizeof(mux_t))))
	{
		return NULL;
	}  
	if(nmultiplexes + 1 > nmuxalloc)
	{
		if(NULL == (l = (mux_t **) realloc(multiplexes, sizeof(mux_t *) * (nmuxalloc + 4))))
		{
			free(p);
			return NULL;
		}
		multiplexes = l;
		nmuxalloc += 4;
	}
	multiplexes[nmultiplexes] = p;
	nmultiplexes++;
	return p;
}

int
mux_foreach(int (*fn)(mux_t *mux, void *data), void *data)
{
	size_t n;
	int r;

	for(n = 0; n < nmultiplexes; n++)
	{
		if(multiplexes[n])
		{
			if((r = fn(multiplexes[n], data)) != 0)
			{
				return r;
			}
		}
	}
	return 0;
}

		
