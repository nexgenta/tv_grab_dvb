/*
   
tv_grab_dvb - dump dvb epg info in xmltv
Version 0.2 - 20/04/2004 - First Public Release

Copyright (C) 2004 Mark Bryars <dvb at darkskiez d0t co d0t uk>

DVB code Mercilessly ripped off from dvddate 
dvbdate Copyright (C) Laurence Culhane 2002 <dvbdate@holmes.demon.co.uk>


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
Or, point your browser to http://www.gnu.org/copyleft/gpl.html

*/

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
#include "dvb_info_tables.h"
#define bcdtoint(i) ((((i & 0xf0) >> 4) * 10) + (i & 0x0f))

/* FIXME: put these as options */
#define CHANNELS_CONF "channels.conf"
#define DEMUX         "/dev/dvb/adapter0/demux0"

char *ProgName;

int timeout  = 10;
int packet_count = 0;
int programme_count = 0;
int update_count  = 0;
int crcerr_count  = 0;
int invalid_date_count = 0;

bool ignore_bad_dates = true;
bool ignore_updates = true;

typedef struct chninfo 
{
	struct chninfo *next;
	int sid;
	int eid;
	int ver;
} chninfo_t;

chninfo_t *channels;

void errmsg(char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	fprintf(stderr, "%s: ", ProgName);
	vfprintf(stderr, message, ap);
	va_end(ap);
}

void usage()
{
	errmsg ("tv_grab_dvb - Version 0.5\n\n usage: %s [-d] [-u] [-t timeout] > dump.xmltv\n\n"
		"\t\t-t timeout - Stop after timeout seconds of no new data\n"
		"\t\t-d - output invalid dates\n"
		"\t\t-u - output updated info - will result in repeated information\n\n", ProgName);
	_exit(1);
}

void status() {
	errmsg ("\r Status: %d pkts, %d prgms, %d updates, %d invalid, %d CRC err",
			packet_count,programme_count,update_count,invalid_date_count,crcerr_count);
}

int do_options(int arg_count, char **arg_strings)
{
	static struct option Long_Options[] = {
		{"help", 0, 0, 'h'},
		{"timeout", 1, 0, 't'},
		{0, 0, 0, 0}
	};
	int c;
	int Option_Index = 0;

	while (1) {
		c = getopt_long(arg_count, arg_strings, "duht:", Long_Options, &Option_Index);
		if (c == EOF)
			break;
		switch (c) {
		case 't':
			timeout = atoi(optarg);
			if (0 == timeout) {
				errmsg("%s: invalid timeout value\n", ProgName);
				usage();
			}
			break;
		case 'u':
			ignore_updates = false;
			break;
		case 'd':
			ignore_bad_dates = false;
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
}

/* 
 * This function is horrible horrible horrible
 * FIXME: fix more than & signs, do proper conversion
 * There must be a bit of code somewhere i can lift for this
 */

char *xmlify(char* s) {
	static char *xml=NULL;
	static unsigned bufsz=0;
	int i;
	char *r;

	/*  Patch by Steve Davies <steve one47 co uk> for better memory management */
	if( bufsz < strlen(s)*2 || xml == NULL ) {
		xml=realloc(xml, strlen(s)*2);
		bufsz=strlen(s)*2;
	}
	
	r=xml;
	while ( *s != '\0' ) {
		if (*s == '&') {
			*r++='&';*r++='a';*r++='m';*r++='p';*r++=';';
		} else {
			*r++=*s;
		}
		s++;
	}
	*r='\0';
	return xml;
}

int parseEventDescription(char *evtdesc) 
{
   int evtlen,dsclen;
   char lang[3];
   char evt[256];
   char dsc[256];
   
   evtlen=*(evtdesc+5) &0xff;
   dsclen=*(evtdesc+6+evtlen) &0xff;
   strncpy((char*)&lang,(char*)(evtdesc+2),3);
   strncpy((char*)&evt,(char*)(evtdesc+6),evtlen);
   strncpy((char*)&dsc,(char*)(evtdesc+7+evtlen),dsclen);
   lang[3]=0;
   evt[evtlen]=0;
   dsc[dsclen]=0;

   printf("\t<title lang=\"%s\">%s</title>\n",lang,xmlify(evt));
   printf("\t<desc lang=\"%s\">%s</desc>\n",lang,xmlify(dsc));
   
}

int parseComponentDescription(descr_component_t *dc)
{
	char buf[256];
	char lang[3];
   
	//assert(dc->descriptor_tag==0x50);
   
	strncpy((char*)&lang,(char*)(dc)+5,3);
	lang[3]=0;
   
	//Extract and null terminate buffer
	if ((dc->descriptor_length-6)>0)
		strncpy((char*)&buf,(char*)dc+DESCR_COMPONENT_LEN,dc->descriptor_length-6);
	buf[dc->descriptor_length-6]=0;

     	switch(dc->stream_content) {
		case 0x01: // Video Info
			//if ((dc->component_type-1)&&0x08) //HD TV
			//if ((dc->component_type-1)&&0x04) //30Hz else 25
			printf("\t<video>\n");
			printf("\t\t<aspect>%s</aspect>\n",lookup(&aspect_table,(dc->component_type-1)&&0x03));
			printf("\t</video>\n");
			break;
		case 0x02: // Audio Info
			printf("\t<audio>\n");
			printf("\t\t<stereo>%s</stereo>\n",lookup(&audio_table,(dc->component_type)));
			printf("\t</audio>\n");
			break;;
		case 0x03: // Teletext Info
			// FIXME: is there a suitable XMLTV output for this?
			// if ((dc->component_type)&&0x10) //subtitles
			// if ((dc->component_type)&&0x20) //subtitles for hard of hearing
			// + other aspect nonsense
			break;;
		// case 0x04: // AC3 info
	}
   	/*
	printf("\t<StreamComponent>\n");
	printf("\t\t<StreamContent>%d</StreamContent>\n",dc->stream_content);
	printf("\t\t<ComponentType>%x</ComponentType>\n",dc->component_type);
	printf("\t\t<ComponentTag>%x</ComponentTag>\n",dc->component_tag);
	printf("\t\t<Length>%d</Length>\n",dc->component_tag,dc->descriptor_length-6);
	printf("\t\t<Language>%s</Language>\n",lang);
	printf("\t\t<Data>%s</Data>\n",buf);
	printf("\t</StreamComponent>\n");
	*/
}


int parseContentDescription(descr_content_t *dc) 
{
	int c1,c2;
	int i;   
	for (i=0;i<(dc->descriptor_length);i+=2) 
	{                      
		nibble_content_t *nc=CastContentNibble((char*)dc+DESCR_CONTENT_LEN+i);
		c1=(nc->content_nibble_level_1<<4+nc->content_nibble_level_2);
		c2=(nc->user_nibble_1<<4+nc->user_nibble_2);
		if (c1>0) printf("\t<category>%s</category>\n",lookup(&description_table,c1));
		// This is weird in the uk, they use user but not content, and almost the same values
		if (c2>0) printf("\t<category>%s</category>\n",lookup(&description_table,c2));		
	}
}

  
int parseDescription(char *desc,int len) 
{
   int i,tag,taglen;
   for (i=0;i<len;) 
     {
		
        tag=*(desc+i) &0xff;
	taglen=*(desc+i+1) &0xff;
	if (taglen>0) 
	  {
	     switch (tag) 
	       {
		case 0:
		  break;;
		case 0x4D: //short evt desc
		  parseEventDescription(desc+i);
		  break;;
	        case 0x50: //component desc
		  parseComponentDescription(CastComponentDescriptor(desc+i));
		  break;;
		case 0x54: //content desc
		  parseContentDescription(CastContentDescriptor(desc+i));
		  break;;
		case 0x64: //Data broadcast desc - Text Desc for Data components
		  break;;
		default:
		  printf("\t<!--Unknown_Please_Report ID=\"%x\" Len=\"%d\" -->\n",tag,taglen);
		
	       }
	  } 
	
	i=i+taglen+2;
     }
   
}
  
int parseeit(char *eitbuf, int len) 
{
	char        desc[4096];
	long int    mjd;
	int         year,month,day,i,dll,j;
	eit_t       *e=(eit_t*)eitbuf;
	eit_event_t *evt=(eit_event_t*)((char*)eitbuf+EIT_LEN);
	chninfo_t   *c=channels;
	struct tm   dvb_time;
	time_t      start_time,stop_time,now;
	char        date_strbuf[256];
	
	
	if( _dvb_crc32((uint8_t*)eitbuf, len) != 0) 
	{
		crcerr_count++;
		status();
		return;
	}
	len=len-4; //remove CRC 
	
	j=0;
	// For each event listing
	while (evt!=NULL) {
		j++;	
		// find existing information?
		while (c!=NULL) 
		{
			// found it
			if (c->sid==HILO(e->service_id) && (c->eid==HILO(evt->event_id))) {
				if (c->ver <= e->version_number) // seen it before or its older FIXME: wrap-around to 0
					return;
				else 
				{ 
					c->ver=e->version_number; // update outputted version
					update_count++;			
					if (ignore_updates) return;
					break;
				}
			}
			// its not it
			c=c->next;  
		}
	
		// its a new program
		if (c==NULL) 
		{	
			chninfo_t *nc=(chninfo_t*)malloc(sizeof(chninfo_t));
			nc->sid=HILO(e->service_id);
			nc->eid=HILO(evt->event_id);
			nc->ver=e->version_number;
			nc->next=channels;
			channels=nc;
		} 
		
		status();
		/* we have more data, refresh alarm */
		alarm(timeout); 

		
		// FIXME: move this into seperate function.
		
		mjd=HILO(evt->mjd);
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
	
		dvb_time.tm_sec =  bcdtoint(evt->start_time_s);
		dvb_time.tm_min =  bcdtoint(evt->start_time_m);
		dvb_time.tm_hour = bcdtoint(evt->start_time_h);
		start_time=mktime(&dvb_time);
   
		dvb_time.tm_sec  += bcdtoint(evt->duration_s);
		dvb_time.tm_min  += bcdtoint(evt->duration_m);
		dvb_time.tm_hour += bcdtoint(evt->duration_h);
		stop_time=mktime(&dvb_time);
		
		// length of message at end.
		dll=HILO(evt->descriptors_loop_length);
		// No program info! Just skip it
		if (dll==0) {
			return;
		}
		
		time(&now);
		// basic bad date check. if the program ends before this time yesterday, or two weeks from today, forget it.
		if ((difftime(stop_time,now) < -86400) || (difftime(now,stop_time)>86400*14) ) 
		{
			invalid_date_count++;
			status();
			if (ignore_bad_dates) return;
		}
	
		programme_count++;

		printf("<programme channel=\"%i.dvb.guide\" ",HILO(e->service_id));   
		strftime(date_strbuf, sizeof(date_strbuf), "start=\"%Y%m%d%H%M%S\"", localtime(&start_time) );  
		printf("%s ",date_strbuf);
		strftime(date_strbuf, sizeof(date_strbuf), "stop=\"%Y%m%d%H%M%S\"", localtime(&stop_time));  
		printf("%s >\n ",date_strbuf);
		
		//printf("\t<EventID>%i</EventID>\n",HILO(evt->event_id));
		//printf("\t<RunningStatus>%i</RunningStatus>\n",evt->running_status); 
		//1 Airing, 2 Starts in a few seconds, 3 Pausing, 4 About to air
		
		strncpy((char*)&desc,(char*)evt+EIT_EVENT_LEN,dll);
		desc[dll+1]=0;
		parseDescription((char*)&desc,dll);
		printf("</programme>\n");
		//printf("<!--DEBUG Record #%d %d %d %d -->\n",j,dll,len,len-dll-EIT_EVENT_LEN);

		// Skip to next entry in table
		evt=(eit_event_t*)((char*)evt+EIT_EVENT_LEN+dll);
		len=len-EIT_EVENT_LEN-dll;
		if (len<EIT_EVENT_LEN) return;
	}
}


void finish_up() {
	fprintf(stderr,"\n");
	printf("</tv>\n");
	exit(0);
}

int readEventTables(unsigned int to)
{
	int fd_epg,fd_time;
	int n, seclen;
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
        sctFilterParams.filter.filter[0] = 0x00; // 4e is now/next.
 	sctFilterParams.filter.mask[0] = 0x00;   // ?? 

	if (ioctl(fd_epg, DMX_SET_FILTER, &sctFilterParams) < 0) {
		perror("DMX_SET_FILTER:");
		close(fd_epg);
		return -1;
	}

	while (to > 0) {
		int res;

		memset(&ufd,0,sizeof(ufd));
		ufd.fd=fd_epg;
		ufd.events=POLLIN;

		res = poll(&ufd,1,1000);
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
	while (n = read(fd_epg, buf, 4096)) 
 	{
		if (buf[0]==0)
			continue;
		packet_count++;
		status();
		parseeit(buf,n);
		memset(&buf,0,sizeof(buf));
     	}

	close(fd_epg);
	finish_up();
	return 0;
}


void readZapInfo() {
	FILE *fd_zap;
	char buf[256];
	char *chansep,*id;
	int chanid;
	if ((fd_zap = fopen(CHANNELS_CONF, "r")) == NULL) {
		errmsg("No tzap channels.conf to produce channel info");
		return;		      
	}
	
	while (fgets(buf,256,fd_zap)) {
		id=strrchr(buf,':')+1;
		chansep=strchr(buf,':');
		if (id && chansep) {
			*chansep='\0';
			chanid=atoi(id);
			printf("<channel id=\"%d.dvb.guide\">\n",chanid);
			printf("\t<display-name>%s</display-name>\n",xmlify(buf));
			printf("</channel>\n");
		}
	}
	
	fclose(fd_zap);	 

}

int main(int argc, char **argv)
{
	int ret;
     	ProgName = argv[0];
/*
 * Process command line arguments
 */
	do_options(argc, argv);
	fprintf(stderr,"\n");
       	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n");
	printf("<tv generator-info-name=\"dvb-epg-gen\">\n");
	readZapInfo();
	signal(SIGALRM,finish_up);
	alarm(timeout);
	ret = readEventTables(timeout);
	if (ret != 0) {
		errmsg("Unable to get event data from multiplex.\n");
		exit(1);
	}
   
	return (0);}
