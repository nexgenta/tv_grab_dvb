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

#ifndef SERVICES_H_
# define SERVICES_H_                    1

# include "multiplexes.h"

typedef struct service_struct service_t;

typedef enum service_type {
	ST_RESERVED = 0,
	ST_TV,
	ST_RADIO,
	ST_TELETEXT,
	ST_NVOD_REF,
	ST_NVOD_TIMESHIFT,
	ST_MOSAIC,
	ST_FM,
	ST_DVB_SRM,
	ST_RESERVED_09,
	ST_ADVANCED_CODEC_RADIO,
	ST_ADVANCED_CODEC_MOSAIC,
	ST_DATA,
	ST_RESERVED_CIU,
	ST_RCS_MAP,
	ST_RCS_FLS,
	ST_DVB_MHP, /* 0x10 */
	ST_MPEG2,
	ST_RESERVED_12,
	ST_RESERVED_13,
	ST_RESERVED_14,
	ST_RESERVED_15,
	ST_ADVANCED_CODEC_SD,
	ST_ADVANCED_CODEC_NVOD_TIMESHIFT,
	ST_ADVANCED_CODEC_NVOD_REF,
	ST_ADVANCED_CODEC_HD,
	ST_ADVANCED_CODEC_HD_NVOD_TIMESHIFT,
	ST_ADVANCED_CODEC_HD_NVOD_REF,
	/* 0x1C .. 0x7F */
	ST_USER_DEFINED_MIN = 0x80,
	ST_USER_DEFINED_MAX = 0xFE,
	ST_RESERVED_FF = 0xFF
} service_type_t;

service_t *service_add(const char *uri);
service_t *service_add_dvb(int original_network_id, int transport_stream_id, int service_id);

service_t *service_locate(const char *uri);
service_t *service_locate_dvb(int original_network_id, int transport_stream_id, int service_id);

service_t *service_locate_add(const char *uri);
service_t *service_locate_add_dvb(int original_network_id, int transport_stream_id, int service_id);

void service_reset(service_t *p);

const char *service_uri(service_t *service);

void service_set_data(service_t *service, void *data);
void *service_data(service_t *service);

void service_set_type(service_t *service, service_type_t type);
service_type_t service_type(service_t *service);

void service_set_name(service_t *service, const char *name);
const char *service_name(service_t *service);

void service_set_provider(service_t *service, const char *provider);
const char *service_provider(service_t *service);

void service_set_authority(service_t *service, const char *authority);
const char *service_authority(service_t *service);

void service_set_mux(service_t *service, mux_t *mux);
mux_t *service_mux(service_t *service);

int service_foreach(int (*fn)(service_t *mux, void *data), void *data);

void service_debug(service_t *service);
void service_debug_dump(void);

#endif /*!SERVICES_H_*/
