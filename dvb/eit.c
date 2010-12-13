/* DVB Event Information Table */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "dvb.h"

/* Parse an Event Information Table; invoked by parse_dvb_si() */
int
parse_dvb_eit(void *data, size_t len, dvb_callbacks_t *callbacks)
{
	eit_t *e = data;
	void *p;
	struct tm dvb_time;
	char date_strbuf[64], idbuf[256];
	service_t *service;
	event_t *ev;	

	/* Find the service this event is being broadcast on */
	service = service_locate_dvb(HILO(e->original_network_id), HILO(e->transport_stream_id), HILO(e->service_id));
	for (p = &e->data; p < data + len; p += EIT_EVENT_LEN + GetEITDescriptorsLoopLength(p))
	{
		eit_event_t *evt = (void *) p;
		parse_eit_event(service, evt, callbacks);
	}
		struct chninfo *c;
		// find existing information?
		for (c = channels; c != NULL; c = c->next) {
			// found it
			if (c->sid == HILO(e->service_id) && (c->eid == HILO(evt->event_id))) {
				if (c->ver <= e->version_number) // seen it before or its older FIXME: wrap-around to 0
					return 0;
				else {
					c->ver = e->version_number; // update outputted version
					update_count++;
					if (ignore_updates)
						return 0;
					break;
				}
			}
		}

		// its a new program
		
		sprintf(date_strbuf, "%s/%04x", get_channelident(HILO(e->service_id)), HILO(evt->event_id));
		if(NULL == (ev = event_alloc(date_strbuf)))
		{
			perror("error allocating new event");
			return -1;
		}
		event_set_service(ev, service);

		if (c == NULL) {
			chninfo_t *nc = malloc(sizeof(struct chninfo));
			nc->sid = HILO(e->service_id);
			nc->eid = HILO(evt->event_id);
			nc->ver = e->version_number;
			nc->next = channels;
			channels = nc;
		}

		/* we have more data, refresh alarm */
		if (timeout) alarm(timeout);

		// No program info at end! Just skip it
		if (GetEITDescriptorsLoopLength(evt) == 0)
			return 0;

		parseMJD(HILO(evt->mjd), &dvb_time);

		dvb_time.tm_sec =  BcdCharToInt(evt->start_time_s);
		dvb_time.tm_min =  BcdCharToInt(evt->start_time_m);
		dvb_time.tm_hour = BcdCharToInt(evt->start_time_h) + time_offset;
		time_t start_time = timegm(&dvb_time);

		event_set_start(ev, start_time);
		dvb_time.tm_sec  += BcdCharToInt(evt->duration_s);
		dvb_time.tm_min  += BcdCharToInt(evt->duration_m);
		dvb_time.tm_hour += BcdCharToInt(evt->duration_h);
		time_t stop_time = timegm(&dvb_time);
		event_set_duration(ev, stop_time - start_time);
		time_t now;
		time(&now);
		// basic bad date check. if the program ends before this time yesterday, or two weeks from today, forget it.
		if ((difftime(stop_time, now) < -24*60*60) || (difftime(now, stop_time) > 14*24*60*60) ) {
			invalid_date_count++;
			if (ignore_bad_dates)
			{
				event_free(ev);
				return 0;
			}
		}

		// a program must have a title that isn't empty
		if (!validateDescription(&evt->data, GetEITDescriptorsLoopLength(evt))) {
			event_free(ev);
			return 0;
		}

		programme_count++;
		
		strftime(date_strbuf, sizeof(date_strbuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&start_time));
		
		sprintf(idbuf, "dvb://%04x.%04x.%04x;%04x@%s--PT%02dH%02dM%02dS",
				HILO(e->original_network_id),
				HILO(e->transport_stream_id),
				HILO(e->service_id),
				HILO(evt->event_id),
				date_strbuf,
				BcdCharToInt(evt->duration_h),
				BcdCharToInt(evt->duration_m),
				BcdCharToInt(evt->duration_s));
		event_set_transport_uri(ev, idbuf);

		printf("<programme channel=\"%s\" ", get_channelident(HILO(e->service_id)));
		strftime(date_strbuf, sizeof(date_strbuf), "start=\"%Y%m%d%H%M%S %z\"", localtime(&start_time) );
		printf("%s ", date_strbuf);
		strftime(date_strbuf, sizeof(date_strbuf), "stop=\"%Y%m%d%H%M%S %z\"", localtime(&stop_time));
		printf("%s>\n ", date_strbuf);

		//printf("\t<EventID>%i</EventID>\n", HILO(evt->event_id));
		//printf("\t<RunningStatus>%i</RunningStatus>\n", evt->running_status);
		//1 Airing, 2 Starts in a few seconds, 3 Pausing, 4 About to air

		parseDescription(ev, &evt->data, GetEITDescriptorsLoopLength(evt));
		printf("</programme>\n");
		event_debug(ev);		
		if(!event_pcrid(ev))
		{
			fprintf(stderr, "Event with no crid\n");
		}
		if(callbacks->event)
		{
			callbacks->event(ev, callbacks->event_data);
		}
		event_free(ev);
	}
	return 0;
}

static int
parse_eit_event(service_t *svc, eit_t *eit, eit_event_t *evt, size_t len, dvb_callbacks_t *callbacks)
{
	char identifier[64];
	event_t *event;
	char *p, *d;

	if(!GetEITDescriptorsLoopLength(evt))
	{
		/* No descriptors, ignore this entry */
		return 0;
	}
	if(validate_eit_event(&evt->data, GetEITDescriptorsLoopLength(evt)) == -1)
	{
		/* We can't (or have decided not to) deal with this entry */
		return 0;
	}
	/* Determine the start and end times of the event */
	parseMJD(HILO(evt->mjd), &dvb_time);
	
	dvb_time.tm_sec =  BcdCharToInt(evt->start_time_s);
	dvb_time.tm_min =  BcdCharToInt(evt->start_time_m);
	dvb_time.tm_hour = BcdCharToInt(evt->start_time_h) + time_offset;
	time_t start_time = timegm(&dvb_time);
	
	event_set_start(ev, start_time);
	dvb_time.tm_sec  += BcdCharToInt(evt->duration_s);
	dvb_time.tm_min  += BcdCharToInt(evt->duration_m);
	dvb_time.tm_hour += BcdCharToInt(evt->duration_h);
	time_t stop_time = timegm(&dvb_time);
	event_set_duration(ev, stop_time - start_time);
	time_t now;
	time(&now);
	if((difftime(stop_time, now) < -24*60*60) || (difftime(now, stop_time) > 14*24*60*60))
	{
		/* Event ends before this time yesterday, or more than two weeks in the future */
		return 0;
	}
	snprintf(identifier, sizeof(identifier), "%04x.%04x.%04x;%04x",
			 HILO(eit->original_network_id),
			 HILO(eit->transport_stream_id),
			 HILO(eit->service_id),
			 HILO(eit->event_id));
	if((event = event_locate(identifier)))
	{
		if(event_version(event) >= evt->version_number)
		{
			return 0;
		}
		event_reset(event);
	}
	else
	{
		event = event_add(identifier);
	}
	/* Set the associated service */
	event_set_service(event, service);
	/* Set the event version */
	event_set_version(event, evt->version_number);
	/* Set the dvb: URI for the event */
	strftime(date_strbuf, sizeof(date_strbuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&start_time));	
	snprintf(identifier, sizeof(identifier), "dvb://%04x.%04x.%04x;%04x@%s--PT%02dH%02dM%02dS",
			HILO(eit->original_network_id),
			HILO(eit->transport_stream_id),
			HILO(eit->service_id),
			HILO(evt->event_id),
			date_strbuf,
			BcdCharToInt(evt->duration_h),
			BcdCharToInt(evt->duration_m),
			BcdCharToInt(evt->duration_s));
	event_set_transport_uri(ev, idbuf);
	/* Parse the event descriptors */
	p = (void *) evt;
	for(d = p; d < p + len; d += DESCR_GEN_LEN + GetDescriptorLength(d))
	{
		descr = (void *) d;
		switch(GetDescriptorTag(descr))
		{
		default:
			fprintf(stderr, "Warning: parse_eit_event: Unknown EIT event descriptor 0x%02x (len=%d)\n",
					GetDescriptorTag(descr), (int) GetDescriptorLength(descr));
		}
	}
}
