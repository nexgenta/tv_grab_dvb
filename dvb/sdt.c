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

/* DVB Service Description Table */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "p_dvb.h"

/* SDT descriptors */
static int parse_sdt_service_descriptor(service_t *svc, struct sdt *sdt, struct sdt_descr *service, struct descr_gen *descr);
static int parse_sdt_default_authority_descriptor(service_t *svc, struct sdt *sdt, struct sdt_descr *service, struct descr_gen *descr);

int
dvb_parse_sdt(dvb_table_t *table, dvb_callbacks_t *callbacks)
{
	sdt_t *sdt;
	struct sdt_descr *service;
	struct descr_gen *descr;
	unsigned char *start, *end, *p, *d;
	size_t ndescr, i;
	service_t *svc;
	mux_t *mux;
	
	for(i = 0; i < table->nsections; i++)
	{
		sdt = &(table->sections[i]->sdt);
		DBG(5, fprintf(stderr, "[dvb_parse_sdt:%d: table_id=0x%02x, current_version=%02d]\n", (int) i, GetTableId(sdt), sdt->version_number));
		if(!sdt->current_next_indicator)
		{
			return 0;
		}
		start = (void *) sdt;
		end = start + GetSectionLength(start) + sizeof(si_tab_t) - 4;
		DBG(9, fprintf(stderr, "[dvb_parse_sdt:%d: end - start = %u]\n", i, end - start));
		p = start + SDT_LEN;
		while(p < end)
		{
/*			DBG(9, fprintf(stderr, "[dvb_parse_sdt:%d: p=%08x, end=%08x]\n", i, p, end)); */
			service = (void *) p;
			p += SDT_DESCR_LEN;
			ndescr = GetSDTDescriptorsLoopLength(service);
			DBG(7, fprintf(stderr, "[dvb_parse_sdt:%d: service = dvb://%04x.%04x.%04x]\n",
						   i, GetSDTOriginalNetworkId(sdt), GetSDTTransportStreamId(sdt), HILO(service->service_id)));
			svc = service_add_dvb(GetSDTOriginalNetworkId(sdt), GetSDTTransportStreamId(sdt), HILO(service->service_id));
			mux = mux_locate_add_dvb(GetSDTOriginalNetworkId(sdt), GetSDTTransportStreamId(sdt));
			service_set_mux(svc, mux);
			DBG(9, fprintf(stderr, "[dvb_parse_sdt:%d: there are %u bytes of descriptors]\n", i, ndescr));
			if(!ndescr)
			{
				continue;
			}
			d = p;
			p += ndescr;
			while(d < p)
			{			
				descr = (void *) d;
/*				DBG(9, fprintf(stderr, "[dvb_parse_sdt:%d: d=%08x, p=%08x, end=%08x]\n", i, d, p, end)); */
				DBG(9, fprintf(stderr, "[dvb_parse_sdt:%d: descriptor length is %u]\n", i, DESCR_GEN_LEN + GetDescriptorLength(d)));
				d += DESCR_GEN_LEN + GetDescriptorLength(d);
				switch(GetDescriptorTag(descr))
				{
				case 0x48:
					parse_sdt_service_descriptor(svc, sdt, service, descr);
					break;
				case 0x4b:
					fprintf(stderr, "Warning: parse_dvb:sdt: Skipped NVOD_reference_descriptor\n");
					break;
				case 0x53:
					fprintf(stderr, "Warning: parse_dvb_sdt: Skipped SDT CA_identifier_descriptor (0x%02x; len=%d)\n",
							GetDescriptorTag(descr), (int) GetDescriptorLength(descr));
					break;
				case 0x5f:
					fprintf(stderr, "Warning: parse_dvb_sdt: Skipped private_data_specifier\n");
					break;
				case 0x73:
					parse_sdt_default_authority_descriptor(svc, sdt, service, descr);
					break;
				case 0x7e:
					fprintf(stderr, "Warning: parse_dvb_sdt: Skipped SDT FTA_content_management_descriptor (0x%02x; len=%d)\n",
							GetDescriptorTag(descr), (int) GetDescriptorLength(descr));
					break;
				default:
					if(GetDescriptorTag(descr) >= 0x80 && GetDescriptorTag(descr) <= 0xFE)
					{
						fprintf(stderr, "Warning: parse_dvb_sdt: Skipped user-defined SDT descriptor 0x%02x (len=%d)\n",
								GetDescriptorTag(descr), (int) GetDescriptorLength(descr));
					}
					else
					{
						fprintf(stderr, "Warning: parse_dvb_sdt: Unknown SDT descriptor 0x%02x (len=%d)\n",
								GetDescriptorTag(descr), (int) GetDescriptorLength(descr));
					}
				}
			}
			DBG(5, fprintf(stderr, "[dvb_parse_sdt:%d: service description complete]\n", i));
			if(callbacks && callbacks->service)
			{
				callbacks->service(svc, callbacks->service_data);
			}
		}
	}
	return 0;
}

static int
parse_sdt_service_descriptor(service_t *svc, struct sdt *sdt, struct sdt_descr *service, struct descr_gen *descr)
{
	struct descr_service *d = (void *) descr;
	char *p = (void *) descr;
	char buf[256];
	unsigned char l;

	(void) sdt;
	(void) service;

	service_set_type(svc, d->service_type);
	p += DESCR_SERVICE_LEN;	
	strncpy(buf, p, d->provider_name_length);
	buf[d->provider_name_length] = 0;
	service_set_provider(svc, buf);
	p += d->provider_name_length;
	l = *p;
	p++;
	strncpy(buf, p, l);
	buf[l] = 0;
	service_set_name(svc, buf);
	return 0;
}

static int
parse_sdt_default_authority_descriptor(service_t *svc, struct sdt *sdt, struct sdt_descr *service, struct descr_gen *descr)
{
	descr_default_authority_t *d = (void *) descr;
	char buf[256];

	(void) sdt;
	(void) service;

	strncpy(buf, (const char *) d->data, d->descriptor_length);
	buf[d->descriptor_length] = 0;
	service_set_authority(svc, buf);
	return 0;
}

