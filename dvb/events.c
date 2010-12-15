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
#include <sys/time.h>
#include <time.h>

#include "events.h"

#define EVENT_ID_SIZE                   64

struct event_struct
{
	char identifier[EVENT_ID_SIZE];
	char transport_uri[128];
	char pcrid[128];
	char scrid[128];
	char lang[8];
	time_t start;
	time_t duration;
	size_t ntitle;
	event_langstr_t **title;
	size_t nsubtitle;
	event_langstr_t **subtitle;
	event_audio_t audio;
	event_aspect_t aspect;
	service_t *service;
	void *data;
};

static event_langstr_t *event_set_langstr(event_langstr_t ***list, size_t *count, const char *lang, const char *str);
static event_langstr_t *event_locate_langstr(event_langstr_t **list, size_t count, const char *lang);

event_t *
event_alloc(const char *identifier)  
{
	event_t *p;

	if(strlen(identifier) >= EVENT_ID_SIZE)
	{
		errno = EINVAL;
		return NULL;
	}
	if(NULL == (p = (event_t *) calloc(1, sizeof(event_t))))
	{
		return NULL;
	}
	strcpy(p->identifier, identifier);
	p->audio = EA_INVALID;
	p->aspect = EA_INVALID;
	return p;
}

void
event_free(event_t *event)
{
	free(event);
}

const char *
event_identifier(event_t *event)
{
	return event->identifier;
}

void
event_set_lang(event_t *event, const char *lang)
{
	strncpy(event->lang, lang, sizeof(event->lang) - 1);
}

const char *
event_lang(event_t *event)
{
	if(event->lang[0])
	{
		return event->lang;
	}
	return NULL;
}

void
event_set_title(event_t *event, const char *title, const char *lang)
{
	event_set_langstr(&(event->title), &(event->ntitle), lang, title);
}

const char *
event_title(event_t *event, const char *lang)
{
	event_langstr_t *p;
	
	if(NULL == (p = event_locate_langstr(event->title, event->ntitle, lang)))
	{
		return NULL;
	}
	return p->str;
}

const event_langstr_t **
event_titles(event_t *event, size_t *ntitles)
{
	*ntitles = event->ntitle;
	return (const event_langstr_t **) event->title;
}

void
event_set_subtitle(event_t *event, const char *title, const char *lang)
{
	event_set_langstr(&(event->subtitle), &(event->nsubtitle), lang, title);
}

const char *
event_subtitle(event_t *event, const char *lang)
{
	event_langstr_t *p;
	
	if(NULL == (p = event_locate_langstr(event->subtitle, event->ntitle, lang)))
	{
		return NULL;
	}
	return p->str;
}

const event_langstr_t **
event_subtitles(event_t *event, size_t *ntitles)
{
	*ntitles = event->nsubtitle;
	return (const event_langstr_t **) event->subtitle;
}


void
event_set_aspect(event_t *event, event_aspect_t aspect)
{
	event->aspect = aspect;
}

event_aspect_t
event_aspect(event_t *event)
{
	return event->aspect;
}

void
event_set_audio(event_t *event, event_audio_t audio)
{
	event->audio = audio;
}

event_audio_t
event_audio(event_t *event)
{
	return event->audio;
}

void
event_set_pcrid(event_t *event, const char *pcrid)
{
	strncpy(event->pcrid, pcrid, sizeof(event->pcrid) - 1);
}

size_t
event_qual_pcrid(event_t *event, char *buf, size_t buflen)
{	
	const char *authority;

	if(!event->pcrid[0])
	{
		*buf = 0;
		return 0;
	}
	if(event->pcrid[0] != '/')
	{
		snprintf(buf, buflen, "crid://%s", event->pcrid);
		return strlen(event->pcrid) + 7;
	}
	if(!event->service || !(authority = service_authority(event->service)))
	{
		authority = "undefined";
	}
	snprintf(buf, buflen, "crid://%s%s", authority, event->pcrid);
	return strlen(authority) + strlen(event->pcrid) + 7;
}

const char *
event_pcrid(event_t *event)
{
	if(event->pcrid[0])
	{
		return event->pcrid;
	}
	return NULL;
}

void
event_set_scrid(event_t *event, const char *scrid)
{
	strncpy(event->scrid, scrid, sizeof(event->scrid) - 1);
}

const char *
event_scrid(event_t *event)
{
	if(event->scrid[0])
	{
		return event->scrid;
	}
	return NULL;
}

void
event_set_transport_uri(event_t *event, const char *transport_uri)
{
	strncpy(event->transport_uri, transport_uri, sizeof(event->transport_uri) - 1);
}

const char *
event_transport_uri(event_t *event)
{
	if(event->transport_uri[0])
	{
		return event->transport_uri;
	}
	return NULL;
}

void
event_set_service(event_t *event, service_t *service)
{
	event->service = service;
}

service_t *
event_service(event_t *event)
{
	return event->service;
}

void
event_set_start(event_t *event, time_t start)
{
	event->start = start;
}

time_t
event_start(event_t *event)
{
	return event->start;
}

void
event_set_duration(event_t *event, time_t duration)
{
	event->duration = duration;
}

time_t
event_duration(event_t *event)
{
	return event->duration;
}

time_t
event_finish(event_t *event)
{
	return event->start + event->duration;
}

void
event_set_data(event_t *event, void *data)
{
	event->data = data;
}

void *
event_data(event_t *event)
{
	return event->data;
}

void
event_debug(event_t *event)
{
	char buf[256];
	struct tm *tm;
	size_t i;

	fprintf(stderr, " - Event identifier='%s', transport URI='%s'\n", event->identifier, event->transport_uri);
	if(event->start)
	{
		tm = gmtime(&event->start);
		strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tm);
		fprintf(stderr, "   Start: %s, duration: %d seconds\n", buf, (int) event->duration);
	}
	if(event->service)
	{
		fprintf(stderr, "   Service: %s\n", service_uri(event->service));
	}
	if(event->pcrid[0])
	{
		event_qual_pcrid(event, buf, sizeof(buf));
		fprintf(stderr, "   pCrid: %s\n", buf);
	}
	if(event->scrid[0])
	{
		fprintf(stderr, "   sCrid: %s\n", event->scrid);
	}	
	for(i = 0; i < event->ntitle; i++)
	{
		if(event->title[i])
		{
			fprintf(stderr, "   Title[%s]='%s'\n", event->title[i]->lang, event->title[i]->str);
		}
	}
	for(i = 0; i < event->nsubtitle; i++)
	{
		if(event->subtitle[i])
		{
			fprintf(stderr, "   Sub-title[%s]='%s'\n", event->title[i]->lang, event->title[i]->str);
		}
	}
}

static event_langstr_t *
event_set_langstr(event_langstr_t ***list, size_t *count, const char *lang, const char *str)
{
	event_langstr_t *p, **q;
	size_t i;
	ssize_t ff;

	if(str && str[0])
	{
		if(NULL == (p = (event_langstr_t *) calloc(1, sizeof(event_langstr_t) + strlen(str) + 1)))
		{
			return NULL;
		}
		strncpy(p->lang, lang, sizeof(p->lang) - 1);
		strcpy(p->str, str);
	}
	else
	{
		p = NULL;
	}
	ff = -1;
	for(i = 0; i < *count; i++)
	{
		if(NULL == (*list)[i])
		{
			ff = i;
		}
		else if(!strcmp((*list)[i]->lang, lang))
		{
			free((*list)[i]);
			(*list)[i] = p;
			return p;
		}
	}
	if(!p)
	{
		/* Nothing to do */
		return NULL;
	}
	if(ff != -1)
	{
		(*list)[ff] = p;
		return p;
	}
	if(NULL == (q = (event_langstr_t **) realloc(*list, sizeof(event_langstr_t *) * ((*count) + 1))))
	{
		free(p);
		return NULL;
	}
	*list = q;
	q[*count] = p;
	(*count)++;
	return p;
}

static event_langstr_t *
event_locate_langstr(event_langstr_t **list, size_t count, const char *lang)
{
	size_t i;
	
	for(i = 0; i < count; i++)
	{
		if(list[i] && !strcmp(list[i]->lang, lang))
		{
			return list[i];
		}
	}
	return NULL;
}


