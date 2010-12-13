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

#include "debug.h"

/* FIXME: put these as options */
#define CHANNELS_CONF "channels.conf"
#define CHANIDENTS    "chanidents"

static char *ProgName;
static char *demux = "/dev/dvb/adapter0/demux0";

int debug_level = 0;

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
static int service_scan = 600;
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
		int c = getopt_long(arg_count, arg_strings, "udscmpnht:o:f:i:e:S:aA", Long_Options, &Option_Index);
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

static int
read_nit(dvb_callbacks_t *callbacks)
{
	dvb_demux_t *ctx;
	dvb_table_t *table;
	time_t start_time;

	struct dmx_sct_filter_params sct;

	memset(&sct, 0, sizeof(sct));
	sct.pid = 0x0010;
	
	if(!(ctx = dvb_demux_open(0, 0, &sct, NULL, 0)))
	{
		perror("dvb_demux_open");
		exit(1);
	}
	dvb_demux_start(ctx);
	dvb_demux_set_timeout(ctx, timeout);
	start_time = time(NULL);
	do
	{
		if(NULL == (table = dvb_demux_read(ctx, (service_scan ? start_time + service_scan : 0))))
		{
			break;
		}
		dvb_parse_si(table, callbacks);
	}
	while(table->table_id != 0x40);
	
	dvb_demux_close(ctx);
	return 0;
}

static int
check_mux(mux_t *mux, void *data)
{
	int *flag = data;
	
	if(mux_data(mux))
	{
		return 0;
	}
	DBG(5, fprintf(stderr, "[check_mux: Multiplex %s has no SDT yet]\n", mux_uri(mux)));
	*flag = 0;
	return 1;
}

static int
read_sdt(dvb_callbacks_t *callbacks)
{
	dvb_demux_t *ctx;
	dvb_table_t *table;
	time_t start_time;
	mux_t *mux;
	int muxflag, matchflag;

	struct dmx_sct_filter_params sct;

	memset(&sct, 0, sizeof(sct));
	sct.pid = 0x0011;
	
	if(!(ctx = dvb_demux_open(0, 0, &sct, NULL, 0)))
	{
		perror("dvb_demux_open");
		exit(1);
	}
	dvb_demux_start(ctx);
	dvb_demux_set_timeout(ctx, timeout);
	start_time = time(NULL);
	muxflag = 1;
	do
	{
		if(NULL == (table = dvb_demux_read(ctx, (service_scan ? start_time + service_scan : 0))))
		{
			break;
		}
		if(table->table_id != 0x42 && table->table_id != 0x46)
		{
			continue;
		}
		dvb_parse_si(table, callbacks);
		mux = mux_locate_dvb(HILO(table->sections[0]->sdt.original_network_id), HILO(table->sections[0]->sdt.transport_stream_id));
		if(mux)
		{
			mux_set_data(mux, &muxflag);
		}
		matchflag = 1;
		mux_foreach(check_mux, &matchflag);
	}
	while(!matchflag);
	dvb_demux_close(ctx);
	return 0;
}

int main(int argc, char **argv)
{
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

	read_nit(&callbacks);
	read_sdt(&callbacks);

	network_debug_dump();
	return 0;
}

