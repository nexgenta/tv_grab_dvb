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

static char *ProgName;
static char *demux = "/dev/dvb/adapter0/demux0";

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

static struct lookup_table *channelid_table;
static struct chninfo *channels;

/* Print usage information. {{{ */
static void usage() {
	fprintf(stderr, "Usage: %s [-d] [-u] [-c] [-n|m|p] [-s] [-t timeout] [-o offset] [-i file] -f file | > dump.xmltv\n\n"
		"\t-i file - Read from file/device instead of %s\n"
		"\t-f file - Write output to file instead of stdout\n"
		"\t-t timeout - Stop after timeout seconds of no new data\n"
		"\t-o offset  - time offset in hours from -12 to 12\n"
		"\t-c - Use Channel Identifiers from file 'chanidents'\n"
		"\t     (rather than sidnumber.dvb.guide)\n"
		"\t-d - output invalid dates\n"
		"\t-n - now next info only\n"
		"\t-m - current multiplex now_next only\n"
		"\t-p - other multiplex now_next only\n"
		"\t-s - silent - no status ouput\n"
		"\t-u - output updated info - will result in repeated information\n"
		"\n", ProgName, demux);
	_exit(1);
} /*}}}*/

/* Print progress indicator. {{{ */
static void status() {
	if (!silent) {
		fprintf(stderr, "\r Status: %d pkts, %d prgms, %d updates, %d invalid, %d CRC err",
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
	int Option_Index = 0;
	int fd;

	while (1) {
		int c = getopt_long(arg_count, arg_strings, "udscmpnht:o:f:i:", Long_Options, &Option_Index);
		if (c == EOF)
			break;
		switch (c) {
		case 'i':
			demux = strcmp(optarg, "-") ? optarg : NULL;
			break;
		case 'f':
			if ((fd = open(optarg, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0) {
				fprintf(stderr, "%s: Can't write file %s\n", ProgName, optarg);
				usage();
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
			break;
		case 't':
			timeout = atoi(optarg);
			if (0 == timeout) {
				fprintf(stderr, "%s: Invalid timeout value\n", ProgName);
				usage();
			}
			break;
		case 'o':
			time_offset = atoi(optarg);
			if ((time_offset < -12) || (time_offset > 12)) {
				fprintf(stderr, "%s: Invalid time offset", ProgName);
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
			fprintf(stderr, "%s: unknown getopt error - returned code %02x\n", ProgName, c);
			_exit(1);
		}
	}
	return 0;
} /*}}}*/

/* Lookup channel-id. {{{ */
static char *get_channelident(int chanid) {
	static char returnstring[256];

	if (use_chanidents && channelid_table) {
		char *c = lookup(channelid_table, chanid);
		if (c)
			return c;
	}
	sprintf(returnstring, "%d.dvb.guide", chanid);
	return returnstring;
} /*}}}*/

/* Parse language-id translation file. {{{ */
static char *xmllang(u_char *l) {
	union lookup_key lang = {
		.c[0] = (char)l[0],
		.c[1] = (char)l[1],
		.c[2] = (char)l[2],
		.c[3] = '\0',
	};

	char *c = lookup(languageid_table, lang.i);
	return c ? c : lang.c;
} /*}}}*/

/* Parse 0x4D Short Event Descriptor. {{{ */
static void parseEventDescription(char *evtdesc) {
   int evtlen, dsclen;
   char evt[256];
   char dsc[256];

   evtlen = *(evtdesc+5) & 0xff;
   dsclen = *(evtdesc+6+evtlen) & 0xff;
   strncpy(evt, evtdesc + 6, evtlen);
   strncpy(dsc, evtdesc + 7 + evtlen, dsclen);
   evt[evtlen] = 0;
   dsc[dsclen] = 0;

   printf("\t<title lang=\"%s\">%s</title>\n", xmllang(evtdesc + 2), xmlify(evt));
   if (*dsc)
     printf("\t<desc lang=\"%s\">%s</desc>\n", xmllang(evtdesc + 2), xmlify(dsc));
}

/* Parse 0x50 Component Descriptor.  {{{
   video is a flag, 1=> output the video information, 0=> output the
   audio information.  seen is a pointer to a counter to ensure we
   only output the first one of each (XMLTV can't cope with more than
   one) */
static void parseComponentDescription(descr_component_t *dc, int video, int *seen) {
	char buf[256];

	//assert(dc->descriptor_tag == 0x50);

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
				printf("\t\t<aspect>%s</aspect>\n", lookup(aspect_table, (dc->component_type-1) & 0x03));
				printf("\t</video>\n");
				(*seen)++;
			}
			break;
		case 0x02: // Audio Info
			if (!video && !*seen) {
				printf("\t<audio>\n");
				printf("\t\t<stereo>%s</stereo>\n", lookup(audio_table, (dc->component_type)));
				printf("\t</audio>\n");
				(*seen)++;
			}
			break;
		case 0x03: // Teletext Info
			// FIXME: is there a suitable XMLTV output for this?
			// if ((dc->component_type)&0x10) //subtitles
			// if ((dc->component_type)&0x20) //subtitles for hard of hearing
			// + other aspect nonsense
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
static void parseContentDescription(descr_content_t *dc) {
	int c1, c2;
	int i;
	for (i = 0; i < dc->descriptor_length; i += 2) {
		nibble_content_t *nc = CastContentNibble((char*)dc + DESCR_CONTENT_LEN + i);
		c1 = (nc->content_nibble_level_1 << 4) + nc->content_nibble_level_2;
		c2 = (nc->user_nibble_1 << 4) + nc->user_nibble_2;
		if (c1 > 0) {
			char *c = lookup(description_table, c1);
			if (c)
				printf("\t<category>%s</category>\n", c);
		}
		// This is weird in the uk, they use user but not content, and almost the same values
		if (c2 > 0) {
			char *c = lookup(description_table, c2);
			if (c)
				printf("\t<category>%s</category>\n", c);
		}
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

/* Parse Event Information Table. {{{ */
static void parseEIT(char *eitbuf, int len) {
	char        desc[4096];
	int         dll, j;
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
		if (timeout) alarm(timeout);

		// No program info at end! Just skip it
		if (GetEITDescriptorsLoopLength(evt) == 0)
			return;

		parseMJD(HILO(evt->mjd), &dvb_time);

		dvb_time.tm_sec =  BcdCharToInt(evt->start_time_s);
		dvb_time.tm_min =  BcdCharToInt(evt->start_time_m);
		dvb_time.tm_hour = BcdCharToInt(evt->start_time_h) + time_offset;
		start_time = timegm(&dvb_time);

		dvb_time.tm_sec  += BcdCharToInt(evt->duration_s);
		dvb_time.tm_min  += BcdCharToInt(evt->duration_m);
		dvb_time.tm_hour += BcdCharToInt(evt->duration_h);
		stop_time = timegm(&dvb_time);

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
		strftime(date_strbuf, sizeof(date_strbuf), "start=\"%Y%m%d%H%M%S %z\"", localtime(&start_time) );
		printf("%s ", date_strbuf);
		strftime(date_strbuf, sizeof(date_strbuf), "stop=\"%Y%m%d%H%M%S %z\"", localtime(&stop_time));
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
static void readEventTables(void) {
	int r, n = 0;
	char buf[1<<12], *bhead = buf;

	/* The dvb demultiplexer simply outputs individual whole packets (good),
	 * but reading captured data from a file needs re-chunking. (bad). */
	do {
		if (n < sizeof(struct si_tab))
			goto read_more;
		struct si_tab *tab = (struct si_tab *)bhead;
		if (GetTableId(tab) == 0)
			goto read_more;
		size_t l = sizeof(struct si_tab) + GetSectionLength(tab);
		if (n < l)
			goto read_more;
		packet_count++;
		if (_dvb_crc32((uint8_t *)bhead, l) != 0) {
			/* data or length is wrong. skip bytewise. */
			//l = 1; // FIXME
			crcerr_count++;
		} else {
			parseEIT(bhead, l);
		}
		status();
		/* remove packet */
		n -= l;
		bhead += l;
		continue;
read_more:
		/* move remaining data to front of buffer */
		if (n > 0)
			memmove(buf, bhead, n);
		/* fill with fresh data */
		r = read(STDIN_FILENO, buf+n, sizeof(buf)-n);
		bhead = buf;
		n += r;
	} while (r > 0);
} /*}}}*/

/* Setup demuxer or open file as STDIN. {{{ */
static int openInput(void) {
	int fd_epg, to;
	struct stat stat_buf;

	if (demux == NULL)
		return 0; // Read from STDIN, which is open al

	if ((fd_epg = open(demux, O_RDWR)) < 0) {
		perror("fd_epg DEVICE: ");
		return -1;
	}

	if (fstat(fd_epg, &stat_buf) < 0) {
		perror("fd_epg DEVICE: ");
		return -1;
	}
	if (S_ISCHR(stat_buf.st_mode)) {
		bool found = false;
		struct dmx_sct_filter_params sctFilterParams = {
			.pid = 18, // EIT data
			.timeout = 0,
			.flags =  DMX_IMMEDIATE_START,
			.filter = {
				.filter[0] = chan_filter, // 4e is now/next this multiplex, 4f others
				.mask[0] = chan_filter_mask,
			},
		};

		if (ioctl(fd_epg, DMX_SET_FILTER, &sctFilterParams) < 0) {
			perror("DMX_SET_FILTER:");
			close(fd_epg);
			return -1;
		}

		for (to = timeout; to > 0; to--) {
			int res;
			struct pollfd ufd = {
				.fd = fd_epg,
				.events = POLLIN,
			};

			res = poll(&ufd, 1, 1000);
			if (0 == res) {
				fprintf(stderr, ".");
				fflush(stderr);
				continue;
			}
			if (1 == res) {
				found = true;
				break;
			}
			fprintf(stderr, "error polling for data");
			close(fd_epg);
			return -1;
		}
		fprintf(stdout, "\n");
		if (!found) {
			fprintf(stderr, "timeout - try tuning to a multiplex?\n");
			close(fd_epg);
			return -1;
		}

		signal(SIGALRM, finish_up);
		alarm(timeout);
	} else {
		// disable alarm timeout for normal files
		timeout = 0;
	}

	dup2(fd_epg, STDIN_FILENO);
	close(fd_epg);

	return 0;
} /*}}}*/

/* Read [cst]zap channels.conf file and print as XMLTV channel info. {{{ */
static void readZapInfo() {
	FILE *fd_zap;
	char buf[256];
	char *chansep, *id;
	int chanid;
	if ((fd_zap = fopen(CHANNELS_CONF, "r")) == NULL) {
		fprintf(stderr, "No [cst]zap channels.conf to produce channel info");
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
} /*}}}*/

/* Main function. {{{ */
int main(int argc, char **argv) {
	ProgName = argv[0];
	/* Process command line arguments */
	do_options(argc, argv);
	/* Load lookup tables. */
	if (load_lookup(&channelid_table, CHANIDENTS))
		fprintf(stderr, "Error loading %s, continuing.\n", CHANIDENTS);
	fprintf(stderr, "\n");

	printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	printf("<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n");
	printf("<tv generator-info-name=\"dvb-epg-gen\">\n");
	if (openInput() != 0) {
		fprintf(stderr, "Unable to get event data from multiplex.\n");
		exit(1);
	}

	readZapInfo();
	readEventTables();
	finish_up();

	return 0;
} /*}}}*/
// vim: foldmethod=marker ts=4 sw=4
