#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "xmltv.h"

#include "tv_grab_dvb.h"

void
xmltv_write_event(event_t *event, void *data)
{
	char chanbuf[64], startbuf[64], stopbuf[64];
	time_t t;
	const char *s;
	event_aspect_t aspect;
	event_audio_t audio;
	size_t count, i;
	const event_langstr_t **ll;

	chanbuf[0] = 0;

	t = event_start(event);
	strftime(startbuf, sizeof(startbuf), "%Y%m%d%H%M%S %z", localtime(&t));
	
	t += event_duration(event);
	strftime(stopbuf, sizeof(stopbuf), "%Y%m%d%H%M%S %z", localtime(&t));
	
	printf("\t<programme channel=\"%s\" start=\"%s\" stop=\"%s\">\n",
		   chanbuf, startbuf, stopbuf);
    /* Tags should be output in this order:
	 *
	 * 'title', 'sub-title', 'desc', 'credits', 'date', 'category', 'language',
	 * 'orig-language', 'length', 'icon', 'url', 'country', 'episode-num',
	 * 'video', 'audio', 'previously-shown', 'premiere', 'last-chance',
	 * 'new', 'subtitles', 'rating', 'star-rating'
	 */
	if((ll = event_titles(event, &count)))
	{
		for(i = 0; i < count; i++)
		{
			printf("\t\t<title lang=\"%s\">%s</title>\n", ll[i]->lang, xmlify(ll[i]->str));
		}
	}
	if((ll = event_subtitles(event, &count)))
	{
		for(i = 0; i < count; i++)
		{
			printf("\t\t<sub-title lang=\"%s\">%s</sub-title>\n", ll[i]->lang, xmlify(ll[i]->str));
		}
	}
	if((s = event_lang(event)))
	{
		printf("\t\t<language>%s</language>\n", s);
	}
	if(EA_INVALID != (aspect = event_aspect(event)))
	{
		printf("\t\t<video>\n"
			   "\t\t\t<aspect>%s</aspect>\n"
			   "\t\t</video>\n",
			   lookup(aspect_table, aspect));
	}			   
	if(EA_INVALID != (audio = event_audio(event)))
	{
		printf("\t\t<audio>\n"
			   "\t\t\t<stereo>%s</stereo>\n"
			   "\t\t</audio>\n",
			   lookup(audio_table, audio));
	}			   	
	printf("\t</programme>\n");
}
