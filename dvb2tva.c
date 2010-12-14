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
#include "tvanytime.h"
#include "debug.h"

static char *progname;
static int timeout  = 10;
static int service_scan = 90;
static int dvb_adapter = 0;
static int dvb_demux = 0;

int debug_level = 0;

static void 
usage(void)
{
	fprintf(stderr, "Usage: %s [-a NUM] [-d NUM] [-t SECS] [-s SECS] [-D LEVEL]\n"
			" -a NUM            Use DVB adapter NUM (default = 0)\n"
			" -d NUM            Use DVB demux interface NUM (default = 0)\n"
			" -t SECS           Stop after SECS seconds of no new data (default = %d)\n"
			" -s SECS           Stop each pass after SECS seconds if still incomplete (default = %d)\n"
			" -D LEVEL          Set debug level to LEVEL (0 = none, 9 = highest)\n",
			progname, timeout, service_scan);
}

static int
parse_options(int arg_count, char **arg_strings)
{
	static const struct option longopts[] = {
		{"help", 0, 0, 'h'},
		{"debug", 1, 0, 'D'},
		{"adapter", 1, 0, 'a'},
		{"demux", 1, 0, 'd'},
		{"timeout", 1, 0, 't'},
		{"scan", 1, 0, 's'},
		{NULL, 0, 0, 0}
	};
	int idx, c;

	while (1)
	{
		if((c = getopt_long(arg_count, arg_strings, "hD:a:d:t:s:d:", longopts, &idx)) == -1)
		{
			break;
		}
		switch (c)
		{
		case 'h':
		case '?':
			usage();
			exit(EXIT_SUCCESS);
		case 'D':
			debug_level = atoi(optarg);
			break;
		case 'a':
			dvb_adapter = atoi(optarg);
			break;
		case 'd':
			dvb_demux = atoi(optarg);
			break;
		case 't':
			timeout = atoi(optarg);
			if (0 == timeout)
			{
				fprintf(stderr, "%s: Invalid timeout value '%s'\n", progname, optarg);
				exit(EXIT_FAILURE);
			}
			break;		   
		case 's':
			service_scan = atoi(optarg);
			if (0 == service_scan)
			{
				fprintf(stderr, "%s: Invalid scan time '%s'\n", progname, optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 0:
		default:
			fprintf(stderr, "%s: unknown getopt error - returned code %d\n", progname, c);
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}

static int
read_nit(dvb_callbacks_t *callbacks)
{
	dvb_demux_t *ctx;
	dvb_table_t *table;
	time_t start_time;

	struct dmx_sct_filter_params sct;

	memset(&sct, 0, sizeof(sct));
	sct.pid = 0x0010;
	
	if(!(ctx = dvb_demux_open(dvb_adapter, dvb_demux, &sct, NULL, 0)))
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
	
	if(!(ctx = dvb_demux_open(dvb_adapter, dvb_demux, &sct, NULL, 0)))
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

int
main(int argc, char **argv)
{
	tva_options_t opts;

	if((progname = strrchr(argv[0], '/')))
	{
		progname++;
	}
	else
	{
		progname = argv[0];
	}
	parse_options(argc, argv);
	read_nit(NULL);
	read_sdt(NULL);   
	opts.out = stdout;

	/* Write services */
	tva_preamble_service(&opts);
	service_foreach(tva_write_service, &opts);
	tva_postamble_service(&opts);
	return 0;
}

