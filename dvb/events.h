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

#ifndef EVENTS_H_
# define EVENTS_H_                      1

# include <time.h>

# include "services.h"

typedef struct event_struct event_t;
typedef struct event_langstr_struct event_langstr_t;

# define EA_INVALID                     0xFF

typedef enum {
	EA_4_3 = 0,
	EA_16_9,
	EA_16_9_NOPAN,
	EA_2_21_1,
} event_aspect_t;

typedef enum {
	EA_MONO = 0x01,
	EA_DUAL_MONO,
	EA_STEREO,
	EA_MULTICHANNEL,
	EA_SURROUND, /* 0x05 */
	EA_VISUALIMPAIRED = 0x40,
	EA_HARDOFHEARING   
} event_audio_t;

struct event_langstr_struct {
	char lang[8];
	char str[1];
};

event_t *event_alloc(const char *identifier);
void event_free(event_t *event);

const char *event_identifier(event_t *event);

void event_set_start(event_t *event, time_t time);
time_t event_start(event_t *event);

void event_set_duration(event_t *event, time_t duration);
time_t event_duration(event_t *event);

time_t event_finish(event_t *event);

void event_set_lang(event_t *event, const char *lang);
const char *event_lang(event_t *event);

void event_set_title(event_t *event, const char *title, const char *lang);
const char *event_title(event_t *event, const char *lang);
const event_langstr_t **event_titles(event_t *event, size_t *ntitles);

void event_set_subtitle(event_t *event, const char *title, const char *lang);
const char *event_subtitle(event_t *event, const char *lang);
const event_langstr_t **event_subtitles(event_t *event, size_t *ntitles);

void event_set_aspect(event_t *event, event_aspect_t aspect);
event_aspect_t event_aspect(event_t *event);

void event_set_audio(event_t *event, event_audio_t audio);
event_audio_t event_audio(event_t *event);

void event_set_pcrid(event_t *event, const char *pcrid);
const char *event_pcrid(event_t *event);
size_t event_qual_pcrid(event_t *event, char *buf, size_t buflen);

void event_set_scrid(event_t *event, const char *scrid);
const char *event_scrid(event_t *event);

void event_set_transport_uri(event_t *event, const char *transport_uri);
const char *event_transport_uri(event_t *event);

void event_set_service(event_t *event, service_t *service);
service_t *event_service(event_t *event);

void event_set_data(event_t *event, void *data);
void *event_data(event_t *event);

void event_debug(event_t *event);

#endif /*!EVENTS_H_*/
