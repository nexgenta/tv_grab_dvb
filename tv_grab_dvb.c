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
#include <assert.h>

#include "dvb/dvb.h"
#include "tv_grab_dvb.h"
#include "xmltv.h"
#include "tvanytime.h"

/* FIXME: put these as options */
#define CHANNELS_CONF "channels.conf"
#define CHANIDENTS    "chanidents"

static char *ProgName;
static char *demux = "/dev/dvb/adapter0/demux0";

int timeout  = 10;
static int packet_count = 0;
int programme_count = 0;
int update_count  = 0;
static int crcerr_count  = 0;
int time_offset   = 0;
int invalid_date_count = 0;
static int chan_filter	     = 0;
static int chan_filter_mask = 0;
bool ignore_bad_dates = true;
bool ignore_updates = true;
static bool use_chanidents = false;
static bool silent = false;
static int service_scan;
static int halt_after_service_scan;
static int generate_atom;

struct lookup_table *channelid_table;
struct chninfo *channels;

/* Print usage information. {{{ */
static void usage() {
	fprintf(stderr, "Usage: %s [-d] [-u] [-c] [-n|m|p] [-s] [-t timeout]\n"
			"\t[-e encoding] [-o offset] [-i file] [-f file]\n\n"
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
			"\t-e encoding - Use other than ISO-6937 default encoding\n"
			"\t-S nsecs - Scan for service information for nsecs seconds\n"
			"\t-H - halt after performing service scan\n"
			"\t-a - generate an Atom feed instead of XMLTV\n"
			"\t-A - generate an Atom feed per service in current directory\n"
		"\n", ProgName, demux);
	_exit(1);
} /*}}}*/

/* Print progress indicator. {{{ */
static void status() {
	if (!silent) {
		fprintf(stderr, "\r Status: %d pkts, %d prgms, %d updates, %d invalid, %d CRC err\n",
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
		int c = getopt_long(arg_count, arg_strings, "udscmpnht:o:f:i:e:S:HaA", Long_Options, &Option_Index);
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
				fprintf(stderr, "%s: Invalid time offset\n", ProgName);
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
		case 'S':
			service_scan = atoi(optarg);
			if (0 == service_scan) {
				fprintf(stderr, "%s: Invalid service scan time\n", ProgName);
				usage();
			}
			break;			
		case 'e':
			iso6937_encoding = optarg;
			break;
		case 'H':
			halt_after_service_scan = 1;
			break;
		case 'a':
			generate_atom = 1;
			break;
		case 'A':
			generate_atom = 2;
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
char *get_channelident(int chanid) {
	static char returnstring[256];

	if (use_chanidents && channelid_table) {
		char *c = lookup(channelid_table, chanid);
		if (c)
			return c;
	}
	sprintf(returnstring, "%d.dvb.guide", chanid);
	return returnstring;
} /*}}}*/

/* Exit hook: close xml tags. {{{ */
static void finish_up() {
	if (!silent)
		fprintf(stderr, "\n");
	printf("</tv>\n");
	exit(0);
} /*}}}*/

static void dummy_alarm() {
}

/* Read EIT segments from DVB-demuxer or file. {{{ */
static void readEventTables(int fd, void *handler, time_t until, dvb_callbacks_t *callbacks) {
	int r, n = 0;
	char buf[1<<12], *bhead = buf;
	int (*handlerfn)(void *data, size_t len, dvb_callbacks_t *callbacks);
	
	handlerfn = handler;
	/* The dvb demultiplexer simply outputs individual whole packets (good),
	 * but reading captured data from a file needs re-chunking. (bad). */
	do {
		if (n < sizeof(struct si_tab))
		{
			fprintf(stderr, "Table is too small\n");	   
			goto read_more;
		}
		struct si_tab *tab = (struct si_tab *)bhead;
/*		if (GetTableId(tab) == 0)
		{
			fprintf(stderr, "Table ID = 0x%02x\n", GetTableId(tab));
			goto read_more;
			}*/
		size_t l = sizeof(struct si_tab) + GetSectionLength(tab);
		if (n < l)
			goto read_more;
		packet_count++;
		if (_dvb_crc32((uint8_t *)bhead, l) != 0) {
			/* data or length is wrong. skip bytewise. */
			//l = 1; // FIXME
			crcerr_count++;
		} else {
			fprintf(stderr, "calling handler\n");
			if(handlerfn(bhead, l, callbacks) != 0)
			{
				break;
			}
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
		do
		{
			r = read(fd, buf+n, sizeof(buf)-n);
		}
		while((!until || time(NULL) < until) && r == -1 && errno == EINTR);		
		if(until && time(NULL) >= until)
		{
			fprintf(stderr, "--- Timer reached --\n");
			break;
		}
		if(r == -1)
		{
			perror("read error");
			break;
		}
		fprintf(stderr, "read %d bytes\n", r);
		bhead = buf;
		n += r;
	} while (r > 0);
} /*}}}*/

/* Setup demuxer or open file as STDIN. {{{ */
static int openInput(int pid) {
	int fd_epg, to;
	struct stat stat_buf;
	struct dmx_sct_filter_params sctFilterParams = {
		.pid = pid,
		.timeout = 0,
		.flags =  DMX_IMMEDIATE_START,
		.filter = {
			.filter[0] = 0,
			.mask[0] = 0,
		},
	};
	if(pid == 0x0012)
	{
		/* EIT PID */
		sctFilterParams.filter.filter[0] = chan_filter;
		sctFilterParams.filter.mask[0] = chan_filter_mask;
	}

	if (demux == NULL)
		return 0; // Read from STDIN, which is open al

	if ((fd_epg = dvb_demux_open_path(demux, &sctFilterParams, NULL, NULL)) < 0) {
		perror("fd_epg DEVICE: ");
		return -1;
	}

	if (fstat(fd_epg, &stat_buf) < 0) {
		perror("fd_epg DEVICE: ");
		return -1;
	}
	if (S_ISCHR(stat_buf.st_mode)) {
		bool found = false;

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
			fprintf(stderr, "error polling for data\n");
			close(fd_epg);
			return -1;
		}
		fprintf(stdout, "\n");
		if (!found) {
			fprintf(stderr, "timeout - try tuning to a multiplex?\n");
			close(fd_epg);
			return -1;
		}
		
		if(pid == 0x0012)
		{
			signal(SIGALRM, finish_up);
			alarm(timeout);
		}
	} else {
		// disable alarm timeout for normal files
		timeout = 0;
	}

	return fd_epg;
} /*}}}*/

/* Read [cst]zap channels.conf file and print as XMLTV channel info. {{{ */
static void readZapInfo() {
	FILE *fd_zap;
	char buf[256];
	if ((fd_zap = fopen(CHANNELS_CONF, "r")) == NULL) {
		fprintf(stderr, "No [cst]zap channels.conf to produce channel info\n");
		return;
	}

	/* name:freq:inversion:symbol_rate:fec:quant:vid:aid:chanid:... */
	while (fgets(buf, sizeof(buf), fd_zap)) {
		int i = 0;
		char *c, *id = NULL;
		for (c = buf; *c; c++)
			if (*c == ':') {
				*c = '\0';
				if (++i == 8) /* chanid */
					id = c + 1;
			}
		if (id && *id) {
			int chanid = atoi(id);
            if (chanid) { 
                printf("<channel id=\"%s\">\n", get_channelident(chanid));
                printf("\t<display-name>%s</display-name>\n", xmlify(buf));
                printf("</channel>\n");
            }
		}
	}

	fclose(fd_zap);
} /*}}}*/

/* Main function. {{{ */
int main(int argc, char **argv) {
	int fd;
	time_t start_time;
	tva_options_t tva_opts;
	dvb_callbacks_t callbacks;

	memset(&callbacks, 0, sizeof(callbacks));
	/* Remove path from command */
	ProgName = strrchr(argv[0], '/');
	if (ProgName == NULL)
		ProgName = argv[0];
	else
		ProgName++;
	/* Process command line arguments */
	do_options(argc, argv);
	/* Load lookup tables. */
	if (use_chanidents && load_lookup(&channelid_table, CHANIDENTS))
		fprintf(stderr, "Error loading %s, continuing.\n", CHANIDENTS);
	if (!silent)
		fprintf(stderr, "\n");

	readZapInfo();
	if(service_scan)
	{
		start_time = time(NULL);
		if ((fd = openInput(0x0010)) < 0) {
			fprintf(stderr, "Unable to open demultiplex interface to read SDT\n");
			exit(1);
		}
		signal(SIGALRM, dummy_alarm);		
		alarm(service_scan);
		/* Write TV-Anytime */
		tva_opts.out = fopen("ServiceInformation.xml", "w");
		tva_preamble_service(&tva_opts);
		callbacks.service = tva_write_service;
		callbacks.service_data = &tva_opts;
		readEventTables(fd, parse_dvb_si, start_time + service_scan, &callbacks);
		tva_postamble_service(&tva_opts);
		fclose(tva_opts.out);
		if(fd)
		{
			close(fd);
		}
		if(halt_after_service_scan)
		{
			return 0;
		}	
	}
	if(generate_atom == 1)
	{
		printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		printf("<feed>\n");
		printf("\t<title>Event Information Table</title>\n");		
	}
	else if(!generate_atom)
	{
		printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		printf("<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n");
		printf("<tv generator-info-name=\"dvb-epg-gen\">\n");
	}

	if ((fd = openInput(0x0012)) < 0) {
		fprintf(stderr, "Unable to get event data from multiplex.\n");
		exit(1);
	}
/*	readEventTables(fd, parseEIT, 0, &callbacks); */
	if(fd)
	{
		close(fd);
	}
	finish_up();

	return 0;
} /*}}}*/
// vim: foldmethod=marker ts=4 sw=4

