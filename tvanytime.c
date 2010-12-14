#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>

#include "tvanytime.h"
#include "dvb/dvb.h"

typedef struct tva_service_struct tva_service_t;

struct tva_service_struct
{
	char serviceid[64];
	FILE *cr; /* Content referencing table */
	FILE *pi; /* Programme information table */
	FILE *pl; /* Programme location table */
};

void
tva_preamble_service(tva_options_t *options)
{
	fprintf(options->out,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<TVAMain xmlns=\"urn:tva:metadata:2005\" xmlns:mpeg7=\"urn:tva:mpeg7:2005\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
			"\t<ProgramDescription>\n"
			"\t\t<ServiceInformationTable>\n");
}

void
tva_postamble_service(tva_options_t *options)
{
	fprintf(options->out,
			"\t\t</ServiceInformationTable>\n"
			"\t</ProgramDescription>\n"
			"</TVAMain>\n");
}

int
tva_write_service(service_t *service, void *data)
{
	service_type_t st;
	tva_service_t *svc;
	const char *s, *p;
	tva_options_t *options = data;
	int c;
	
	if(NULL == (svc = calloc(1, sizeof(tva_service_t))))
	{
		return -1;
	}
	service_set_data(service, svc);
	st = service_type(service);
	switch(service_type(service))
	{
	case ST_TV:
	case ST_MPEG2:
	case ST_ADVANCED_CODEC_SD:
	case ST_ADVANCED_CODEC_HD:
		st = ST_TV;
		break;
	case ST_RADIO:
	case ST_FM:
	case ST_ADVANCED_CODEC_RADIO:
		st = ST_RADIO;
		break;
	default:
		return 0;
	}
	if((s = service_name(service)))
	{
		c = 0;
		for(p = s; c < sizeof(svc->serviceid) - 1 && *p; p++)
		{
			if(isalnum(*p))
			{
				svc->serviceid[c] = *p;
				c++;
			}
		}
		svc->serviceid[c] = 0;
	}
	if(!svc->serviceid[0])
	{
		s = service_uri(service);
		p = strrchr(s, '.');
		if(p)
		{
			p++;
			snprintf(svc->serviceid, sizeof(svc->serviceid), "dvb%s", p);
		}
		s = NULL;
	}	
	fprintf(options->out, "\t\t\t<ServiceInformation serviceId=\"%s\">\n", svc->serviceid);
	if(s)
	{
		fprintf(options->out, "\t\t\t\t<Name>%s</Name>\n", s);
	}
	if((s = service_provider(service)))
	{
		fprintf(options->out, "\t\t\t\t<Owner>%s</Owner>\n", s);
	}
	if((s = service_uri(service)))
	{
		fprintf(options->out, "\t\t\t\t<ServiceURL>%s</ServiceURL>\n", s);
	}
	switch(st)
	{
	case ST_TV:
		fprintf(options->out, "\t\t\t\t<ServiceGenre href=\"urn:tva:metadata:cs:MediaTypeCS:2005:7.1.3\">\n"
				"\t\t\t\t\t<Name>Audio and video</Name>\n"
				"\t\t\t\t</ServiceGenre>\n");
		break;
	case ST_RADIO:
		fprintf(options->out, "\t\t\t\t<ServiceGenre href=\"urn:tva:metadata:cs:MediaTypeCS:2005:7.1.3\">\n"
				"\t\t\t\t\t<Name>Audio only</Name>\n"
				"\t\t\t\t</ServiceGenre>\n");
		break;
	default:
		/*NOTREACHED*/
		break;
	}
	fprintf(options->out, "\t\t\t</ServiceInformation>\n");
	return 0;
}

int
tva_write_event(event_t *event, void *data)
{
	(void) event;
	(void) data;
	
	return 0;
}
