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

/* DVB Network Information Table */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "p_dvb.h"

/* NIT descriptors */
static int parse_nit_network_name_descriptor(network_t *network, nit_t *nit, descr_network_name_t *descr);
static int parse_nit_linkage_descriptor(network_t *network, nit_t *nit, descr_linkage_t *descr);

/* NIT TS descriptors */
static int parse_nit_ts_logical_channel_descriptor(network_t *network, mux_t *mux, nit_t *nit, nit_ts_t *ts, descr_logical_channel_t *descr);

/* Parse a Network Information Table; invoked by parse_dvb_si() */
int
dvb_parse_nit(dvb_table_t *table, dvb_callbacks_t *callbacks)
{
	nit_t *nit;
	nit_mid_t *mid;
	nit_ts_t *ts;
	unsigned char *start, *p, *d, *dd;
	struct descr_gen *descr;
	network_t *network;
	platform_t *platform;
	mux_t *mux;
	size_t i;

	network = NULL;
	for(i = 0; i < table->nsections; i++)
	{
		nit = &(table->sections[i]->nit);
		if(!nit->current_next_indicator)
		{
			return 0;
		}
		DBG(5, fprintf(stderr, "[NIT version=%d, section_number=%d, last_section_number=%d]\n", 
					   nit->version_number, nit->section_number, nit->last_section_number));
		start = (void *) nit;
		/* Locate or create a network object */
		if(!network)
		{
			if((network = network_locate_dvb(HILO(nit->network_id))))
			{
				if(network_version(network) >= nit->version_number)
				{
					return 0;
				}
				network_reset(network);
			}
			else
			{
				network = network_add_dvb(HILO(nit->network_id));
			}
			network_set_version(network, nit->version_number);
		}
		/* Parse descriptors */
		d = start + NIT_LEN;
		p = d + GetNITDescriptorsLoopLength(nit);
		while(d < p)
		{
			descr = (void *) d;
			d += DESCR_GEN_LEN + GetDescriptorLength(d);
			switch(GetDescriptorTag(descr))
			{
			case 0x40:
				/* network_name_descriptor */
				parse_nit_network_name_descriptor(network, nit, (descr_network_name_t *) (void *) descr);
				break;
			case 0x4a:
				/* linkage_descriptor */
				parse_nit_linkage_descriptor(network, nit, (descr_linkage_t *) (void *) descr);
				break;
			default:
				fprintf(stderr, "Warning: parse_dvb_nit: Unknown NIT descriptor 0x%02x (len=%d)\n",
						GetDescriptorTag(descr), (int) GetDescriptorLength(descr));
			}
		}
		mid = (void *) p;
		d = p + SIZE_NIT_MID;
		p = d + HILO(mid->transport_stream_loop_length);
		while(d < p)
		{
			ts = (void *) d;
			platform = platform_locate_add_dvb(HILO(ts->original_network_id));
			mux = mux_locate_add_dvb(HILO(ts->original_network_id), HILO(ts->transport_stream_id));
			mux_set_platform(mux, platform);
			network_add_mux(network, mux);
			DBG(5, fprintf(stderr, "[dvb_parse_nit: Found dvb://%04x.%04x on network %04x]\n",
						HILO(ts->original_network_id), HILO(ts->transport_stream_id), HILO(nit->network_id)));
			dd = d + NIT_TS_LEN;
			d = dd + HILO(ts->transport_descriptors_length);
			while(dd < d)
			{
				descr = (void *) dd;
				dd += DESCR_GEN_LEN + GetDescriptorLength(dd);
				switch(GetDescriptorTag(descr))
				{
				case 0x41:
					/* service_list_descriptor */
					DBG(5, fprintf(stderr, "service_list_descriptor\n"));
					break;
				case 0x42:
					/* stuffing_descriptor */
					break;
				case 0x43:
					DBG(5, fprintf(stderr, "satellite_delivery_system_descriptor\n"));
					break;
				case 0x44:
					DBG(5, fprintf(stderr, "cable_delivery_system_descriptor\n"));
					break;
				case 0x5a:
					/* terrestrial_delivery_system_descriptor */
					DBG(5, fprintf(stderr, "terrestrial_delivery_system_descriptor\n"));
					break;
				case 0x5b:
					/* multilingual_network_name_descriptor */
					break;
				case 0x5f:
					/* private_data_specifier_descriptor */
					DBG(5, fprintf(stderr, "private_data_list_descriptor\n"));
					break;
				case 0x62:
					/* frequency_list_descriptor */
					DBG(5, fprintf(stderr, "frequency_list_descriptor\n"));
					break;
				case 0x6c:
					/* cell_list_descriptor */
					break;
				case 0x6d:
					/* cell_frequency_link_descriptor */
					break;
				case 0x73:
					/* default_authority_descriptor */
					break;
				case 0x77:
					/* time_slice_fec_identifier_descriptor */
					break;
				case 0x79:
					/* S2_satellite_delivery_system_descriptor */
					break;
				case 0x7d:
					/* XAIT location descriptor */
					break;
				case 0x7e:
					/* FTA_content_management_descriptor */
					break;
				case 0x7f:
					/* extension descriptor */
					break;
				case 0x83:
					/* user defined -- logical_channel_descriptor */
					parse_nit_ts_logical_channel_descriptor(network, mux, nit, ts, (descr_logical_channel_t *) (void *) descr);
					break;
				default:
					fprintf(stderr, "Warning: parse_dvb_nit: Unknown TS descriptor 0x%02x (len=%d)\n",
							GetDescriptorTag(descr), (int) GetDescriptorLength(descr));
				}
			}
		}
	}
	if(callbacks && callbacks->network)
	{
		callbacks->network(network, callbacks->network_data);   	
	}
	return 0;
}

static int
parse_nit_network_name_descriptor(network_t *network, nit_t *nit, descr_network_name_t *descr)
{
	char *p = (void *) descr;
	char buf[256];
	
	(void) nit;
	
	p += DESCR_NETWORK_NAME_LEN;
	strncpy(buf, p, descr->descriptor_length);
	buf[descr->descriptor_length] = 0;
	network_set_name(network, buf);
	return 0;
}

static int
parse_nit_linkage_descriptor(network_t *network, nit_t *nit, descr_linkage_t *descr)
{
	(void) network;
	(void) nit;

	DBG(5, fprintf(stderr, "Linkage for dvb://%04x.%04x.%04x on %04x, type = 0x%02x\n",
				   HILO(descr->original_network_id),
				   HILO(descr->transport_stream_id),
				   HILO(descr->service_id),
				   HILO(nit->network_id),
				   descr->linkage_type));
	switch(descr->linkage_type)
	{
	case 0x09:
		DBG(5, fprintf(stderr, " -- System Software Update Service (TS 102 006)\n"));
		break;
	}
	return 0;
}

static int
parse_nit_ts_logical_channel_descriptor(network_t *network, mux_t *mux, nit_t *nit, nit_ts_t *ts, descr_logical_channel_t *descr)
{
	char *p = (void *) descr, *d;
	descr_logical_channel_svc_t *svc;
	service_t *service;

	(void) nit;

	p += DESCR_LC_LEN;
	d = p + GetDescriptorLength(descr);
	while(p < d)
	{
		svc = (void *) p;
		p += DESCR_LCSVC_LEN;
		DBG(5, fprintf(stderr, "Service ID = %04x, LCN = %03d\n", HILO(svc->service_id), HILO(svc->logical_channel_number)));
		service = service_locate_add_dvb(HILO(ts->original_network_id), HILO(ts->transport_stream_id), HILO(svc->service_id));
		service_set_mux(service, mux);
		network_set_service(network, service, svc->visible_service_flag, HILO(svc->logical_channel_number), -1);
	}
	return 0;
}

