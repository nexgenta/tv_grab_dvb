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

#include "platforms.h"

#define PLATFORM_URI_SIZE               64

struct platform_struct
{
	char uri[PLATFORM_URI_SIZE];
};

static size_t nplatform;
static platform_t **platforms;

static platform_t *platform_alloc(void);

platform_t *
platform_add(const char *uri)
{
	platform_t *p;
	
	if(strlen(uri) >= PLATFORM_URI_SIZE)
	{
		errno = EINVAL;
		return NULL;
	}
	if(NULL == (p = platform_locate(uri)))
	{
		if(NULL == (p = platform_alloc()))
		{
			return NULL;
		}
		strcpy(p->uri, uri);
	}
	platform_reset(p);
	return p;
}

platform_t *
platform_add_dvb(int original_network_id)
{
	char uri[32];

	snprintf(uri, sizeof(uri), "dvb://%04x", original_network_id);
	return platform_add(uri);
}

platform_t *
platform_locate(const char *uri)
{
	size_t i;
	
	for(i = 0; i < nplatform; i++)
	{
		if(platforms[i] && !strcmp(platforms[i]->uri, uri))
		{
			return platforms[i];
		}
	}
	return NULL;
}

platform_t *
platform_locate_dvb(int original_network_id)
{
	char uri[32];

	snprintf(uri, sizeof(uri), "dvb://%04x", original_network_id);
	return platform_locate(uri);
}

platform_t *
platform_locate_add(const char *uri)
{
	platform_t *p;
	
	if((p = platform_locate(uri)))
	{
		return p;
	}
	return platform_add(uri);
}

platform_t *
platform_locate_add_dvb(int original_network_id)
{
	char uri[32];

	snprintf(uri, sizeof(uri), "dvb://%04x", original_network_id);
	return platform_locate_add(uri);
}

void
platform_reset(platform_t *platform)
{
	(void) platform;
}

const char *
platform_uri(platform_t *platform)
{
	return platform->uri;
}

static platform_t *
platform_alloc(void)
{
	platform_t *p, **l;
	
	if(NULL == (p = calloc(1, sizeof(platform_t))))
	{
		return NULL;
	}
	if(NULL == (l = realloc(platforms, sizeof(platform_t *) * (nplatform + 1))))
	{
		free(p);
		return NULL;
	}
	platforms = l;
	platforms[nplatform] = p;
	nplatform++;
	return p;
}
