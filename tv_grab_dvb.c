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

const char *id = "@(#) $Id$";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#include <linux/dvb/dmx.h>
#include "si_tables.h"
#include "tv_grab_dvb.h"

/* FIXME: put these as options */
#define CHANNELS_CONF "channels.conf"
#define CHANIDENTS    "chanidents"
#define DEMUX         "/dev/dvb/adapter0/demux0"

static char *ProgName;

static int timeout  = 10;
static int packet_count = 0;
static int programme_count = 0;
static int update_count  = 0;
static int crcerr_count  = 0;
static int time_offset   = 0;
static int invalid_date_count = 0;
static int chan_filter	     = 0;
static int chan_filter_mask = 0;
static bool ignore_bad_dates = true;
static bool ignore_updates = true;
static bool use_chanidents = false;
static bool silent = false;

typedef struct chninfo {
	struct chninfo *next;
	int sid;
	int eid;
	int ver;
} chninfo_t;

static struct chninfo *channels;

static void errmsg(char *message, ...) {
	va_list ap;

	va_start(ap, message);
	fprintf(stderr, "%s: ", ProgName);
	vfprintf(stderr, message, ap);
	va_end(ap);
}

/* Print usage information. {{{ */
static void usage() {
	errmsg ("tv_grab_dvb - Version 0.9\n\n usage: %s [-d] [-u] [-c] [-n|m|p] [-s] [-t timeout] [-o offset] > dump.xmltv\n\n"
		"\t\t-t timeout - Stop after timeout seconds of no new data\n"
		"\t\t-o offset  - time offset in hours from -12 to 12\n"
		"\t\t-c - Use Channel Identifiers from file 'chanidents'\n"
		"\t\t     (rather than sidnumber.dvb.guide)\n"
		"\t\t-d - output invalid dates\n"
		"\t\t-n - now next info only\n"
		"\t\t-m - current multiplex now_next only\n"
		"\t\t-p - other multiplex now_next only\n"
		"\t\t-s - silent - no status ouput\n"
		"\t\t-u - output updated info - will result in repeated information\n\n", ProgName);
	_exit(1);
} /*}}}*/

/* Print progress indicator. {{{ */
static void status() {
	if (!silent) {
		errmsg ("\r Status: %d pkts, %d prgms, %d updates, %d invalid, %d CRC err",
				packet_count, programme_count, update_count, invalid_date_count, crcerr_count);
	}
} /*}}}*/

/* Parse command line arguments. {{{ */
static int do_options(int arg_count, char **arg_strings) {
	static const struct option Long_Options[] = {
		{"help", 0, 0, 'h'},
		{"timeout", 1, 0, 't'},
		{"chanidents", 1, 0, 'c'},
		{0, 0, 0, 0}
	};
	int c;
	int Option_Index = 0;

	while (1) {
		c = getopt_long(arg_count, arg_strings, "udscmpnht:o:", Long_Options, &Option_Index);
		if (c == EOF)
			break;
		switch (c) {
		case 't':
			timeout = atoi(optarg);
			if (0 == timeout) {
				errmsg("%s: Invalid timeout value\n", ProgName);
				usage();
			}
			break;
		case 'o':
			time_offset = atoi(optarg);
			if ((time_offset < -12) || (time_offset > 12)) {
				errmsg("%s: Invalid time offset", ProgName);
				usage();
			}
			break;
		case 'u':
			ignore_updates = false;
			break;
		case 'd':
			ignore_bad_dates = false;
			break;
		case 'c':
			use_chanidents = true;
			break;
		case 'n':
			chan_filter = 0x4e;
			chan_filter_mask = 0xfe;
			break;
		case 'm':
			chan_filter = 0x4e;
			chan_filter_mask = 0xff;
			break;
		case 'p':
			chan_filter = 0x4f;
			chan_filter_mask = 0xff;
			break;
		case 's':
			silent = true;
			break;
		case 'h':
		case '?':
			usage();
			break;
		case 0:
		default:
			errmsg("%s: unknown getopt error - returned code %02x\n", ProgName, c);
			_exit(1);
		}
	}
	return 0;
} /*}}}*/

/* Lookup channel-id. {{{ */
static char *get_channelident(int chanid) {
	static char returnstring[256];
	FILE *fd_ci;
	char buf2[256];
	int chanmatch = 0;

	if (use_chanidents) {
		fd_ci = fopen(CHANIDENTS, "r");
		while (fgets(buf2, 256, fd_ci) && !chanmatch) {
			sscanf(buf2, "%d %s", &chanmatch, returnstring);
			if (chanmatch != chanid) chanmatch = 0;
		}
		fclose(fd_ci);
	}
	if (!chanmatch)
		sprintf(returnstring, "%d.dvb.guide", chanid);
	return returnstring;
} /*}}}*/

/* Parse 0x4D Short Event Descriptor. {{{ */
static void parseEventDescription(char *evtdesc) {
   int evtlen, dsclen;
   char lang[3];
   char evt[256];
   char dsc[256];

   evtlen = *(evtdesc+5) & 0xff;
   dsclen = *(evtdesc+6+evtlen) & 0xff;
   strncpy(lang, evtdesc + 2, 3);
   strncpy(evt, evtdesc + 6, evtlen);
   strncpy(dsc, evtdesc + 7 + evtlen, dsclen);
   lang[3] = 0;
   evt[evtlen] = 0;
   dsc[dsclen] = 0;

   printf("\t<title lang=\"%s\">%s</title>\n", lang, xmlify(evt));
   if (*dsc)
     printf("\t<desc lang=\"%s\">%s</desc>\n", lang, xmlify(dsc));
}

/* Parse 0x50 Component Descriptor.  {{{
   video is a flag, 1=> output the video information, 0=> output the
   audio information.  seen is a pointer to a counter to ensure we
   only output the first one of each (XMLTV can't cope with more than
   one) */
static void parseComponentDescription(descr_component_t *dc, int video, int *seen) {
	char buf[256];
	char lang[3];

	//assert(dc->descriptor_tag == 0x50);

	strncpy(lang, (char*)(dc)+5, 3);
	lang[3] = 0;

	//Extract and null terminate buffer
	if ((dc->descriptor_length - 6) > 0)
		strncpy(buf, (char*)dc + DESCR_COMPONENT_LEN, dc->descriptor_length - 6);
	buf[dc->descriptor_length - 6] = 0;

	switch (dc->stream_content) {
		case 0x01: // Video Info
                  if (video && !*seen) {
                        //if ((dc->component_type-1)&0x08) //HD TV
                        //if ((dc->component_type-1)&0x04) //30Hz else 25
                        printf("\t<video>\n");
                        printf("\t\t<aspect>%s</aspect>\n", lookup((struct lookup_table*)&aspect_table, (dc->component_type-1) & 0x03));
                        printf("\t</video>\n");
                        (*seen)++;
                        break;
                    }
		case 0x02: // Audio Info
                  if (!video && !*seen) {
			printf("\t<audio>\n");
			printf("\t\t<stereo>%s</stereo>\n", lookup((struct lookup_table*)&audio_table, (dc->component_type)));
			printf("\t</audio>\n");
                        (*seen)++;
			break;
                    }
		case 0x03: // Teletext Info
			// FIXME: is there a suitable XMLTV output for this?
			// if ((dc->component_type)&0x10) //subtitles
			// if ((dc->component_type)&0x20) //subtitles for hard of hearing
			// + other aspect nonsense
			break;
		// case 0x04: // AC3 info
	}
	/*
	printf("\t<StreamComponent>\n");
	printf("\t\t<StreamContent>%d</StreamContent>\n", dc->stream_content);
	printf("\t\t<ComponentType>%x</ComponentType>\n", dc->component_type);
	printf("\t\t<ComponentTag>%x</ComponentTag>\n", dc->component_tag);
	printf("\t\t<Length>%d</Length>\n", dc->component_tag, dc->descriptor_length-6);
	printf("\t\t<Language>%s</Language>\n", lang);
	printf("\t\t<Data>%s</Data>\n", buf);
	printf("\t</StreamComponent>\n");
	*/
} /*}}}*/

/* Parse 0x54 Content Descriptor. {{{ */
static void parseContentDescription(descr_content_t *dc) {
	int c1, c2;
	int i;
	for (i = 0; i < dc->descriptor_length; i += 2) {
		nibble_content_t *nc = CastContentNibble((char*)dc + DESCR_CONTENT_LEN + i);
		c1 = (nc->content_nibble_level_1 << 4) + nc->content_nibble_level_2;
		c2 = (nc->user_nibble_1 << 4) + nc->user_nibble_2;
		if (c1 > 0)
			printf("\t<category>%s</category>\n", lookup((struct lookup_table*)&description_table, c1));
		// This is weird in the uk, they use user but not content, and almost the same values
		if (c2 > 0)
			printf("\t<category>%s</category>\n", lookup((struct lookup_table*)&description_table, c2));
	}
} /*}}}*/

/* Parse Descriptor. {{{
 * Tags should be output in this order:

'title', 'sub-title', 'desc', 'credits', 'date', 'category', 'language',
'orig-language', 'length', 'icon', 'url', 'country', 'episode-num',
'video', 'audio', 'previously-shown', 'premiere', 'last-chance',
'new', 'subtitles', 'rating', 'star-rating'
*/
static void parseDescription(char *desc, int len) {
	int i, round, tag, taglen, seen;
	for (round = 0; round < 4; round++) {
		seen = 0;                        // no video/audio seen in this round
		for (i = 0; i < len; ) {

			tag = *(desc+i) & 0xff;
			taglen = *(desc + i + 1) & 0xff;
			if (taglen > 0) {
				switch (tag) {
					case 0:
						break;
					case 0x4D: //short evt desc, [title] [desc]
						if (round == 0)
							parseEventDescription(desc+i);
						break;
					case 0x4E: //long evt descriptor
						if (round == 0)
							printf("\t<!-- Long Event Info Detected - Bug Author to decode -->\n");
						break;
					case 0x50: //component desc [video] [audio]
						if (round == 2)
							parseComponentDescription(CastComponentDescriptor(desc + i), 1, &seen);
						else if (round == 3)
							parseComponentDescription(CastComponentDescriptor(desc + i), 0, &seen);
						break;
					case 0x54: //content desc [category]
						if (round == 1)
							parseContentDescription(CastContentDescriptor(desc + i));
						break;
					case 0x64: //Data broadcast desc - Text Desc for Data components
						break;
					default:
						if (round == 0)
							printf("\t<!--Unknown_Please_Report ID=\"%x\" Len=\"%d\" -->\n", tag, taglen);
				}
			}

			i += taglen + 2;
		}
	}
} /*}}}*/

/* Parse Event Information Table. {{{ */
static void parseeit(char *eitbuf, int len) {
	char        desc[4096];
	long int    mjd;
	int         year, month, day, i, dll, j;
	eit_t       *e = (eit_t *)eitbuf;
	eit_event_t *evt = (eit_event_t *)(eitbuf + EIT_LEN);
	chninfo_t   *c = channels;
	struct tm   dvb_time;
	time_t      start_time, stop_time, now;
	char        date_strbuf[256];

	if( _dvb_crc32((uint8_t*)eitbuf, len) != 0) {
		crcerr_count++;
		status();
		return;
	}
	len -= 4; //remove CRC

	j = 0;
	// For each event listing
	while (evt != NULL) {
		j++;
		// find existing information?
		while (c != NULL) {
			// found it
			if (c->sid == HILO(e->service_id) && (c->eid == HILO(evt->event_id))) {
				if (c->ver <= e->version_number) // seen it before or its older FIXME: wrap-around to 0
					return;
				else {
					c->ver = e->version_number; // update outputted version
					update_count++;
					if (ignore_updates)
						return;
					break;
				}
			}
			// its not it
			c = c->next;
		}

		// its a new program
		if (c == NULL) {
			chninfo_t *nc = malloc(sizeof(struct chninfo));
			nc->sid = HILO(e->service_id);
			nc->eid = HILO(evt->event_id);
			nc->ver = e->version_number;
			nc->next = channels;
			channels = nc;
		}

		status();
		/* we have more data, refresh alarm */
		alarm(timeout);

		// FIXME: move this into seperate function.

		mjd = HILO(evt->mjd);
		/*
		 *  * Use the routine specified in ETSI EN 300 468 V1.4.1,
		 *  * "Specification for Service Information in Digital Video Broadcasting"
		 *  * to convert from Modified Julian Date to Year, Month, Day.
		 *  */
		year = (int) ((mjd - 15078.2) / 365.25);
		month = (int) ((mjd - 14956.1 - (int) (year * 365.25)) / 30.6001);
		day = mjd - 14956 - (int) (year * 365.25) - (int) (month * 30.6001);
		if (month == 14 || month == 15)
			i = 1;
		else
			i = 0;
		year += i ;
		month = month - 2 - i * 12;

		dvb_time.tm_mday = day;
		dvb_time.tm_mon = month;
		dvb_time.tm_year = year;
		dvb_time.tm_isdst = 0;
		dvb_time.tm_wday = dvb_time.tm_yday = 0;

		dvb_time.tm_sec =  BcdCharToInt(evt->start_time_s);
		dvb_time.tm_min =  BcdCharToInt(evt->start_time_m);
		dvb_time.tm_hour = BcdCharToInt(evt->start_time_h) + time_offset;
		start_time = mktime(&dvb_time);

		dvb_time.tm_sec  += BcdCharToInt(evt->duration_s);
		dvb_time.tm_min  += BcdCharToInt(evt->duration_m);
		dvb_time.tm_hour += BcdCharToInt(evt->duration_h);
		stop_time = mktime(&dvb_time);

		// length of message at end.
		dll = HILO(evt->descriptors_loop_length);
		// No program info! Just skip it
		if (dll == 0)
			return;

		time(&now);
		// basic bad date check. if the program ends before this time yesterday, or two weeks from today, forget it.
		if ((difftime(stop_time, now) < -24*60*60) || (difftime(now, stop_time) > 14*24*60*60) ) {
			invalid_date_count++;
			status();
			if (ignore_bad_dates)
				return;
		}

		programme_count++;

		printf("<programme channel=\"%s\" ", get_channelident(HILO(e->service_id)));
		strftime(date_strbuf, sizeof(date_strbuf), "start=\"%Y%m%d%H%M%S\"", localtime(&start_time) );
		printf("%s ", date_strbuf);
		strftime(date_strbuf, sizeof(date_strbuf), "stop=\"%Y%m%d%H%M%S\"", localtime(&stop_time));
		printf("%s>\n ", date_strbuf);

		//printf("\t<EventID>%i</EventID>\n", HILO(evt->event_id));
		//printf("\t<RunningStatus>%i</RunningStatus>\n", evt->running_status);
		//1 Airing, 2 Starts in a few seconds, 3 Pausing, 4 About to air

		strncpy(desc, (char *)evt+EIT_EVENT_LEN, dll);
		desc[dll + 1] = 0;
		parseDescription(desc, dll);
		printf("</programme>\n");
		//printf("<!--DEBUG Record #%d %d %d %d -->\n", j, dll, len, len-dll-EIT_EVENT_LEN);

		// Skip to next entry in table
		evt = (eit_event_t*)((char*)evt+EIT_EVENT_LEN+dll);
		len -= EIT_EVENT_LEN + dll;
		if (len<EIT_EVENT_LEN)
			return;
	}
} /*}}}*/

/* Exit hook: close xml tags. {{{ */
static void finish_up() {
	fprintf(stderr, "\n");
	printf("</tv>\n");
	exit(0);
} /*}}}*/

/* Read EIT segments from DVB-demuxer or file. {{{ */
static int readEventTables(unsigned int to) {
	int fd_epg, fd_time;
	int n;
	time_t t;
	unsigned char buf[4096];
	struct dmx_sct_filter_params sctFilterParams;
	struct pollfd ufd;
	int found = 0;

	t = 0;

	//FIXME - no hardcoded device
	if ((fd_epg = open(DEMUX, O_RDWR)) < 0) {
		perror("fd_epg DEVICE: ");
		return -1;
	}

	if ((fd_time = open(DEMUX, O_RDWR)) < 0) {
		perror("fd_time DEVICE: ");
		return -1;
	}

	memset(&sctFilterParams, 0, sizeof(sctFilterParams));
	sctFilterParams.pid = 18; //EIT Data
	sctFilterParams.timeout = 0;
	sctFilterParams.flags = DMX_IMMEDIATE_START;
	sctFilterParams.filter.filter[0] = chan_filter; // 4e is now/next this multiplex, 4f others
	sctFilterParams.filter.mask[0] = chan_filter_mask;

	if (ioctl(fd_epg, DMX_SET_FILTER, &sctFilterParams) < 0) {
		perror("DMX_SET_FILTER:");
		close(fd_epg);
		return -1;
	}

	while (to > 0) {
		int res;

		memset(&ufd, 0, sizeof(ufd));
		ufd.fd = fd_epg;
		ufd.events = POLLIN;

		res = poll(&ufd, 1, 1000);
		if (0 == res) {
			fprintf(stderr, ".");
			fflush(stderr);
			to--;
			continue;
		}
		if (1 == res) {
			found = 1;
			break;
		}
		errmsg("error polling for data");
		close(fd_epg);
		return -1;
	}
	fprintf(stdout, "\n");
	if (0 == found) {
		errmsg("timeout - try tuning to a multiplex?\n");
		close(fd_epg);
		return -1;
	}

	/* I only read the first packet here, in my case this is the whole packet
	 * This should be...fixed. esp to make parsing recorded streams for debugging
	 * easier/possible
	 *
	 * (The dvb demultiplexer seems to simply output this in individual whole packets)
	 */
	while ((n = read(fd_epg, buf, 4096))) {
		if (buf[0] == 0)
			continue;
		packet_count++;
		status();
		parseeit(buf, n);
		memset(&buf, 0, sizeof(buf));
	}

	close(fd_epg);
	finish_up();
	return 0;
}

void readZapInfo() {
	FILE *fd_zap;
	char buf[256];
	char *chansep, *id;
	int chanid;
	if ((fd_zap = fopen(CHANNELS_CONF, "r")) == NULL) {
		errmsg("No tzap channels.conf to produce channel info");
		return;
	}

	while (fgets(buf, 256, fd_zap)) {
		id = strrchr(buf, ':')+1;
		chansep = strchr(buf, ':');
		if (id && chansep) {
			*chansep = '\0';
			chanid = atoi(id);
			printf("<channel id=\"%s\">\n", get_channelident(chanid));
			printf("\t<display-name>%s</display-name>\n", xmlify(buf));
			printf("</channel>\n");
		}
	}

	fclose(fd_zap);
}

/* Main function. {{{ */
int main(int argc, char **argv) {
	int ret;
	ProgName = argv[0];
/*
 * Process command line arguments
 */
	do_options(argc, argv);
	fprintf(stderr, "\n");
	printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	printf("<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n");
	printf("<tv generator-info-name=\"dvb-epg-gen\">\n");
	readZapInfo();
	signal(SIGALRM, finish_up);
	alarm(timeout);
	ret = readEventTables(timeout);
	if (ret != 0) {
		errmsg("Unable to get event data from multiplex.\n");
		exit(1);
	}

	return 0;
} /*}}}*/
// vim: foldmethod=marker ts=4 sw=4
