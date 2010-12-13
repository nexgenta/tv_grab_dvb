/*
 * tv_grab_dvb - dump dvb epg info in xmltv
 * Version 0.2 - 20/04/2004 - First Public Release
 *
 * Copyright (C) 2004 Mark Bryars <dvb at darkskiez d0t co d0t uk>
 *
 * DVB code Mercilessly ripped off from dvddate
 * dvbdate Copyright (C) Laurence Culhane 2002 <dvbdate@holmes.demon.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "si_tables.h"
#include "tv_grab_dvb.h"
#include "events.h"
#include "xmltv.h"
#include "callbacks.h"

enum ER { TITLE, SUB_TITLE };
enum CR { LANGUAGE, VIDEO, AUDIO, SUBTITLES };

static inline void set_bit(int *bf, int b);
static inline bool get_bit(int *bf, int b);
static char *xmllang(u_char *l);
static void parseMJD(long int mjd, struct tm *t);
static void parseDescription(event_t *ev, void *data, size_t len);
static bool validateDescription(void *data, size_t len);
static void parseEventDescription(event_t *ev, void *data, enum ER round);
static void parseLongEventDescription(event_t *ev, void *data);
static void parseComponentDescription(event_t *ev, void *data, enum CR round, int *seen);
static void parseContentDescription(event_t *ev, void *data);
static void parseRatingDescription(event_t *ev, void *data);
static int parsePrivateDataSpecifier(event_t *ev, void *data);
static void parseContentIdentifierDescription(event_t *ev, void *data);

/* Parse Event Information Table. {{{ */
int parseEIT(void *data, size_t len, dvb_callbacks_t *callbacks) {
	struct eit *e = data;
	void *p;
	struct tm  dvb_time;
	char       date_strbuf[64], idbuf[256];
	service_t *service;
	event_t *ev;	

	len -= 4; //remove CRC

	service = service_locate_dvb(HILO(e->original_network_id), HILO(e->transport_stream_id), HILO(e->service_id));
	// For each event listing
	for (p = &e->data; p < data + len; p += EIT_EVENT_LEN + GetEITDescriptorsLoopLength(p)) {
		struct eit_event *evt = p;
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
} /*}}}*/

static inline void set_bit(int *bf, int b) {
	int i = b / 8 / sizeof(int);
	int s = b % (8 * sizeof(int));
	bf[i] |= (1 << s);
}

static inline bool get_bit(int *bf, int b) {
	int i = b / 8 / sizeof(int);
	int s = b % (8 * sizeof(int));
	return bf[i] & (1 << s);
}

/* Parse language-id translation file. {{{ */
static char *xmllang(u_char *l) {
	static union lookup_key lang;
	lang.c[0] = (char)l[0];
	lang.c[1] = (char)l[1];
	lang.c[2] = (char)l[2];
	lang.c[3] = '\0';

	char *c = lookup(languageid_table, lang.i);
	return c ? c : lang.c;
} /*}}}*/

/* Use the routine specified in ETSI EN 300 468 V1.4.1, {{{
 * "Specification for Service Information in Digital Video Broadcasting"
 * to convert from Modified Julian Date to Year, Month, Day. */
static void parseMJD(long int mjd, struct tm *t) {
	int year = (int) ((mjd - 15078.2) / 365.25);
	int month = (int) ((mjd - 14956.1 - (int) (year * 365.25)) / 30.6001);
	int day = mjd - 14956 - (int) (year * 365.25) - (int) (month * 30.6001);
	int i = (month == 14 || month == 15) ? 1 : 0;
	year += i ;
	month = month - 2 - i * 12;

	t->tm_mday = day;
	t->tm_mon = month;
	t->tm_year = year;
	t->tm_isdst = -1;
	t->tm_wday = t->tm_yday = 0;
} /*}}}*/

/* Parse Descriptor. {{{
 * Tags should be output in this order:

'title', 'sub-title', 'desc', 'credits', 'date', 'category', 'language',
'orig-language', 'length', 'icon', 'url', 'country', 'episode-num',
'video', 'audio', 'previously-shown', 'premiere', 'last-chance',
'new', 'subtitles', 'rating', 'star-rating'
*/
static void parseDescription(event_t *ev, void *data, size_t len) {
	int round, pds = 0;

	for (round = 0; round < 8; round++) {
		int seen = 0; // no title/language/video/audio/subtitles seen in this round
		void *p;
		for (p = data; p < data + len; p += DESCR_GEN_LEN + GetDescriptorLength(p)) {
			struct descr_gen *desc = p;
			switch (GetDescriptorTag(desc)) {
				case 0:
					break;
				case 0x4D: //short evt desc, [title] [sub-title]
					// there can be multiple language versions of these
					if (round == 0) {
						parseEventDescription(ev, desc, TITLE);
					}
					else if (round == 1)
						parseEventDescription(ev, desc, SUB_TITLE);
					break;
				case 0x4E: //long evt descriptor [desc]
					if (round == 2)
						parseLongEventDescription(ev, desc);
					break;
				case 0x50: //component desc [language] [video] [audio] [subtitles]
					if (round == 4)
						parseComponentDescription(ev, desc, LANGUAGE, &seen);
					else if (round == 5)
						parseComponentDescription(ev, desc, VIDEO, &seen);
					else if (round == 6)
						parseComponentDescription(ev, desc, AUDIO, &seen);
					else if (round == 7)
						parseComponentDescription(ev, desc, SUBTITLES, &seen);
					break;
				case 0x53: // CA Identifier Descriptor
					break;
				case 0x54: // content desc [category]
					if (round == 3)
						parseContentDescription(ev, desc);
					break;
				case 0x55: // Parental Rating Descriptor [rating]
					if (round == 7)
						parseRatingDescription(ev, desc);
					break;
				case 0x5f: // Private Data Specifier
					pds = parsePrivateDataSpecifier(ev, desc);
					break;
				case 0x64: // Data broadcast desc - Text Desc for Data components
					break;
				case 0x69: // Programm Identification Label
					break;
				case 0x81: // TODO ???
					if (pds == 5) // ARD_ZDF_ORF
						break;
				case 0x82: // VPS (ARD, ZDF, ORF)
					if (pds == 5) // ARD_ZDF_ORF
						// TODO: <programme @vps-start="???">
						break;
				case 0x4F: // Time Shifted Event
				case 0x52: // Stream Identifier Descriptor
				case 0x5E: // Multi Lingual Component Descriptor
				case 0x83: // Logical Channel Descriptor (some kind of news-ticker on ARD-MHP-Data?)
				case 0x84: // Preferred Name List Descriptor
				case 0x85: // Preferred Name Identifier Descriptor
				case 0x86: // Eacem Stream Identifier Descriptor
					break;
				case 0x76: // Content identifier descriptor
					if (round == 5)
						parseContentIdentifierDescription(ev, desc);
					break;
				default:
					if (round == 0)
						printf("\t<!--Unknown_Please_Report ID=\"%x\" Len=\"%d\" -->\n", GetDescriptorTag(desc), GetDescriptorLength(desc));
			}
		}
	}
} /*}}}*/

/* Check that program has at least a title as is required by xmltv.dtd. {{{ */
static bool validateDescription(void *data, size_t len) {
	void *p;
	for (p = data; p < data + len; p += DESCR_GEN_LEN + GetDescriptorLength(p)) {
		struct descr_gen *desc = p;
		if (GetDescriptorTag(desc) == 0x4D) {
			struct descr_short_event *evtdesc = p;
			// make sure that title isn't empty
			if (evtdesc->event_name_length) return true;
		}
	}
	return false;
} /*}}}*/

/* Parse 0x4D Short Event Descriptor. {{{ */
static void parseEventDescription(event_t *ev, void *data, enum ER round) {
	assert(GetDescriptorTag(data) == 0x4D);
	struct descr_short_event *evtdesc = data;
	char evt[256];
	char dsc[256];

	int evtlen = evtdesc->event_name_length;
	if (round == TITLE) {
		if (!evtlen)
			return;
		assert(evtlen < sizeof(evt));
		memcpy(evt, (char *)&evtdesc->data, evtlen);
		evt[evtlen] = '\0';
		/* XXX FIXME: UTF-8 */
		event_set_title(ev, evt, xmllang(&evtdesc->lang_code1));
		printf("\t<title lang=\"%s\">%s</title>\n", xmllang(&evtdesc->lang_code1), xmlify(evt));
		return;
	}

	if (round == SUB_TITLE) {
		int dsclen = evtdesc->data[evtlen];
		assert(dsclen < sizeof(dsc));
		memcpy(dsc, (char *)&evtdesc->data[evtlen+1], dsclen);
		dsc[dsclen] = '\0';
		if (*dsc) {
			/* XXX FIXME: UTF-8 */
			event_set_subtitle(ev, dsc, xmllang(&evtdesc->lang_code1));
			char *d = xmlify(dsc);
			if (d && *d)
				printf("\t<sub-title lang=\"%s\">%s</sub-title>\n", xmllang(&evtdesc->lang_code1), d);
		}
	}
} /*}}}*/

/* Parse 0x4E Extended Event Descriptor. {{{ */
static void parseLongEventDescription(event_t *ev, void *data) {
	assert(GetDescriptorTag(data) == 0x4E);
	struct descr_extended_event *levt = data;
	char dsc[256];
	bool non_empty = (levt->descriptor_number || levt->last_descriptor_number || levt->length_of_items || levt->data[0]);

	if (non_empty && levt->descriptor_number == 0)
		printf("\t<desc lang=\"%s\">", xmllang(&levt->lang_code1));

	void *p = &levt->data;
	void *data_end = data + DESCR_GEN_LEN + GetDescriptorLength(data);
	while (p < (void *)levt->data + levt->length_of_items) {
		struct item_extended_event *name = p;
		int name_len = name->item_description_length;
		assert(p + ITEM_EXTENDED_EVENT_LEN + name_len < data_end);
		assert(name_len < sizeof(dsc));
		memcpy(dsc, (char *)&name->data, name_len);
		dsc[name_len] = '\0';
		printf("%s: ", xmlify(dsc));

		p += ITEM_EXTENDED_EVENT_LEN + name_len;

		struct item_extended_event *value = p;
		int value_len = value->item_description_length;
		assert(p + ITEM_EXTENDED_EVENT_LEN + value_len < data_end);
		assert(value_len < sizeof(dsc));
		memcpy(dsc, (char *)&value->data, value_len);
		dsc[value_len] = '\0';
		printf("%s; ", xmlify(dsc));

		p += ITEM_EXTENDED_EVENT_LEN + value_len;
	}
	struct item_extended_event *text = p;
	int len = text->item_description_length;
	if (non_empty && len) {
		assert(len < sizeof(dsc));
		memcpy(dsc, (char *)&text->data, len);
		dsc[len] = '\0';
		printf("%s", xmlify(dsc));
	}

	//printf("/%d/%d/%s", levt->descriptor_number, levt->last_descriptor_number, xmlify(dsc));
	if (non_empty && levt->descriptor_number == levt->last_descriptor_number)
		printf("</desc>\n");
} /*}}}*/

/* Parse 0x50 Component Descriptor.  {{{
   video is a flag, 1=> output the video information, 0=> output the
   audio information.  seen is a pointer to a counter to ensure we
   only output the first one of each (XMLTV can't cope with more than
   one) */
static void parseComponentDescription(event_t *ev, void *data, enum CR round, int *seen) {
	assert(GetDescriptorTag(data) == 0x50);
	struct descr_component *dc = data;
	char buf[256];

	int len = dc->descriptor_length;
	assert(len < sizeof(buf));
	memcpy(buf, (char *)&dc->data, len);
	buf[len] = '\0';

	switch (dc->stream_content) {
		case 0x01: // Video Info
			if (round == VIDEO && !*seen) {
				event_set_aspect(ev, (dc->component_type - 1) & 0x03);
				//if ((dc->component_type-1)&0x08) //HD TV
				//if ((dc->component_type-1)&0x04) //30Hz else 25
				printf("\t<video>\n");
				printf("\t\t<aspect>%s</aspect>\n", lookup(aspect_table, (dc->component_type-1) & 0x03));
				printf("\t</video>\n");
				(*seen)++;
			}
			break;
		case 0x02: // Audio Info
			if (round == AUDIO && !*seen) {
				event_set_audio(ev, dc->component_type);
				printf("\t<audio>\n");
				printf("\t\t<stereo>%s</stereo>\n", lookup(audio_table, (dc->component_type)));
				printf("\t</audio>\n");
				(*seen)++;
			}
			if (round == LANGUAGE) {
				event_set_lang(ev, xmllang(&dc->lang_code1));
				if (!*seen)
					printf("\t<language>%s</language>\n", xmllang(&dc->lang_code1));
				else
					printf("\t<!--language>%s</language-->\n", xmllang(&dc->lang_code1));
				(*seen)++;
			}
			break;
		case 0x03: // Teletext Info
			if (round == SUBTITLES) {
			// FIXME: is there a suitable XMLTV output for this?
			// if ((dc->component_type)&0x10) //subtitles
			// if ((dc->component_type)&0x20) //subtitles for hard of hearing
				printf("\t<subtitles type=\"teletext\">\n");
				printf("\t\t<language>%s</language>\n", xmllang(&dc->lang_code1));
				printf("\t</subtitles>\n");
			}
			break;
			// case 0x04: // AC3 info
	}
#if 0
	printf("\t<StreamComponent>\n");
	printf("\t\t<StreamContent>%d</StreamContent>\n", dc->stream_content);
	printf("\t\t<ComponentType>%x</ComponentType>\n", dc->component_type);
	printf("\t\t<ComponentTag>%x</ComponentTag>\n", dc->component_tag);
	printf("\t\t<Length>%d</Length>\n", dc->component_tag, dc->descriptor_length-6);
	printf("\t\t<Language>%s</Language>\n", lang);
	printf("\t\t<Data>%s</Data>\n", buf);
	printf("\t</StreamComponent>\n");
#endif
} /*}}}*/

/* Parse 0x54 Content Descriptor. {{{ */
static void parseContentDescription(event_t *ev, void *data) {
	assert(GetDescriptorTag(data) == 0x54);
	struct descr_content *dc = data;
	int once[256/8/sizeof(int)] = {0,};
	void *p;
	for (p = &dc->data; p < data + dc->descriptor_length; p += NIBBLE_CONTENT_LEN) {
		struct nibble_content *nc = p;
		int c1 = (nc->content_nibble_level_1 << 4) + nc->content_nibble_level_2;
#ifdef CATEGORY_UNKNOWN
		int c2 = (nc->user_nibble_1 << 4) + nc->user_nibble_2;
#endif
		if (c1 > 0 && !get_bit(once, c1)) {
			set_bit(once, c1);
			char *c = lookup(description_table, c1);
			if (c)
				if (c[0])
					printf("\t<category>%s</category>\n", c);
#ifdef CATEGORY_UNKNOWN
				else
					printf("\t<!--category>%s %02X %02X</category-->\n", c+1, c1, c2);
			else
				printf("\t<!--category>%02X %02X</category-->\n", c1, c2);
#endif
		}
		// This is weird in the uk, they use user but not content, and almost the same values
	}
} /*}}}*/

/* Parse 0x55 Rating Descriptor. {{{ */
static void parseRatingDescription(event_t *ev, void *data) {
	assert(GetDescriptorTag(data) == 0x55);
	struct descr_parental_rating *pr = data;
	void *p;
	for (p = &pr->data; p < data + pr->descriptor_length; p += PARENTAL_RATING_ITEM_LEN) {
		struct parental_rating_item *pr = p;
		switch (pr->rating) {
			case 0x00: /*undefined*/
				break;
			case 0x01 ... 0x0F:
				printf("\t<rating system=\"dvb\">\n");
				printf("\t\t<value>%d</value>\n", pr->rating + 3);
				printf("\t</rating>\n");
				break;
			case 0x10 ... 0xFF: /*broadcaster defined*/
				break;
		}
	}
} /*}}}*/

/* Parse 0x5F Private Data Specifier. {{{ */
static int parsePrivateDataSpecifier(event_t *ev, void *data) {
	assert(GetDescriptorTag(data) == 0x5F);
	return GetPrivateDataSpecifier(data);
} /*}}}*/

/* Parse 0x76 Content Identifier Descriptor. {{{ */
/* See ETSI TS 102 323, section 12 */
static void parseContentIdentifierDescription(event_t *ev, void *data) {
	assert(GetDescriptorTag(data) == 0x76);
	struct descr_content_identifier *ci = data;
	void *p;
	for (p = &ci->data; p < data + ci->descriptor_length; /* at end */) {
		struct descr_content_identifier_crid *crid = p;
		struct descr_content_identifier_crid_local *crid_data;

		int crid_length = 3;

		char type_buf[32];
		char *type;
		char buf[256];

		type = lookup(crid_type_table, crid->crid_type);
		if (type == NULL)
		{
			type = type_buf;
			sprintf(type_buf, "0x%2x", crid->crid_type);
		}

		switch (crid->crid_location)
		{
		case 0x00: /* Carried explicitly within descriptor */
			crid_data = (descr_content_identifier_crid_local_t *)&crid->crid_ref_data;
			int cridlen = crid_data->crid_length;
			assert(cridlen < sizeof(buf));
			memcpy(buf, (char *)&crid_data->crid_byte, cridlen);
			buf[cridlen] = '\0';
			if(crid->crid_type == 0x01 || crid->crid_type == 0x31)
			{
				event_set_pcrid(ev, buf);
			}
			else if(crid->crid_type == 0x02 || crid->crid_type == 0x32)
			{
				event_set_scrid(ev, buf);
			}
			printf("\t<crid type='%s'>%s</crid>\n", type, xmlify(buf));
			crid_length = 2 + crid_data->crid_length;
			break;
		case 0x01: /* Carried in Content Identifier Table (CIT) */
			fprintf(stderr, "--- Reference to crid in CIT\n");
			exit(1);
			break;
		default:
			break;
		}

		p += crid_length;
	}
} /*}}}*/



