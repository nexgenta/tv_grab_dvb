/* Parse DVB Service Information tables */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "si_tables.h"

enum dvb_service_type {
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
};

static int parse_dvb_sdt(void *data, size_t len);
static int parse_service_descriptor(struct sdt *sdt, struct sdt_descr *service, struct descr_gen *descr);
static int parse_default_authority_descriptor(struct sdt *sdt, struct sdt_descr *service, struct descr_gen *descr);

int
parse_dvb_si(void *data, size_t len)
{
	struct si_tab *table = data;

	len -= 4;
	switch(table->table_id)
	{	   
	case 0x42:
	case 0x46:
		return parse_dvb_sdt(data, len);
	}
	fprintf(stderr, "Note: skipping unhandled SI table %02x\n", table->table_id);
	return 0;
}

static int
parse_dvb_sdt(void *data, size_t len)
{
	struct sdt *sdt = data;
	struct sdt_descr *service;
	struct descr_gen *descr;
	unsigned char *start, *end, *p, *d;
	size_t ndescr;

	start = data;
	end = start + len;	
	p = &sdt->data[0];
	fprintf(stderr, "Found SDT for dvb://%04x.%04x\n",
			GetSDTOriginalNetworkId(sdt),
			GetSDTTransportStreamId(sdt));
	while(p < end)
	{
		service = (void *) p;
		ndescr = GetSDTDescriptorsLoopLength(service);
		fprintf(stderr, "- Found description of dvb://%04x.%04x.%04x\n",
				GetSDTOriginalNetworkId(sdt),
				GetSDTTransportStreamId(sdt),
				HILO(service->service_id));
		if(!ndescr)
		{
			p += SDT_DESCR_LEN;
			continue;
		}
		p = &service->data[0] + ndescr;
		fprintf(stderr, "  Service description has %d bytes of descriptors\n", (int) ndescr);
		d = &service->data[0];
		while(d < p)
		{			
			descr = (void *) d;
			d += DESCR_GEN_LEN + GetDescriptorLength(d);
			fprintf(stderr, " => Descriptor type = %02x\n", GetDescriptorTag(descr));
			switch(GetDescriptorTag(descr))
			{
			case 0x48:
				parse_service_descriptor(sdt, service, descr);
				break;
			case 0x73:
				parse_default_authority_descriptor(sdt, service, descr);
				break;
			}
		}		
	}				
	return 0;
}

static int
parse_service_descriptor(struct sdt *sdt, struct sdt_descr *service, struct descr_gen *descr)
{
	struct descr_service *d = (void *) descr;
	char *p = (void *) descr;
	char buf[256];
	unsigned char l;

	fprintf(stderr, " --> Service type = %02x\n", d->service_type);
	p += DESCR_SERVICE_LEN;	
	strncpy(buf, p, d->provider_name_length);
	buf[d->provider_name_length] = 0;
	fprintf(stderr, " --> Provider name = '%s'\n", buf);
	p += d->provider_name_length;
	l = *p;
	p++;
	strncpy(buf, p, l);
	buf[l] = 0;
	fprintf(stderr, " --> Service name = '%s'\n", buf);
	return 0;
}

static int
parse_default_authority_descriptor(struct sdt *sdt, struct sdt_descr *service, struct descr_gen *descr)
{
	descr_default_authority_t *d = (void *) descr;
	char buf[256];
	
	strncpy(buf, (const char *) d->data, d->descriptor_length);
	buf[d->descriptor_length] = 0;
	if(buf[0])
	{
		fprintf(stderr, " --> Default authority = 'crid://%s/'\n", buf);
	}
	else
	{
		fprintf(stderr, " --> Default authority is empty.\n");
	}
	return 0;
}
