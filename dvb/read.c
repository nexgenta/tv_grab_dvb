#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "p_dvb.h"

static dvb_table_t *dvb_demux_read_section(dvb_demux_t *context, dvb_section_t *section);
static dvb_table_t *dvb_demux_section_add(dvb_demux_t *context, int table_id, int version, int secnum, int last, dvb_section_t *section);
static dvb_table_t *dvb_demux_table_alloc(dvb_demux_t *context, int table_id, int count);
static void dvb_demux_table_reset(dvb_demux_t *context, dvb_table_t *table, int count);

/* Read until either 'until', or the specified timeout is reached, or a complete
 * SI table (a fully-populated set of sections) is read. When it is, return it.
 * The memory allocated for the table is associated with the context, and is
 * only guaranteed to be valid until the next call to dvb_demux_read() on that
 * context.
 *
 * Do not use the same context on multiple threads without some kind of mutex
 * protecting them (separate threads can call dvb_demux_read() simultaneously
 * provided they each use a different context).
 */

dvb_table_t *
dvb_demux_read(dvb_demux_t *context, time_t until)
{
	time_t now;
	struct timeval tv, *tvp;
	fd_set fds;
	int r, nbytes, noioctl, need;
	size_t bufstart, bufend, bsize, l;
	uint8_t *p;
	dvb_section_t *section;
	dvb_table_t *s;
	
	need = 3;
	bufstart = bufend = 0;
	noioctl = 1;
	while(1)
	{
		if(bufstart >= 1024)
		{
			DBG(9, fprintf(stderr, "[dvb_read: buffer shifted far enough; moving back]\n"));
			memmove(context->buf, &(context->buf[bufstart]), bufend - bufstart);
			bufend -= bufstart;
			bufstart = 0;
		}
		p = &(context->buf[bufstart]);
		bsize = bufend - bufstart;
		if(bsize < sizeof(si_tab_t))
		{
			need = sizeof(si_tab_t) - bsize;
		}
		else
		{
			DBG(9, fprintf(stderr, "[dvb_read: have minimum number of required bytes (%d - %d = %d)]\n", bufend, bufstart, bufend - bufstart));
			DBG(9, fprintf(stderr, "[dvb_read: bytes: %02x %02x %02x %02x - %02x %02x %02x %02x]\n",
						   context->buf[bufstart] & 0xFF, context->buf[bufstart + 1] & 0xFF, 
						   context->buf[bufstart + 2] & 0xFF, context->buf[bufstart + 3] & 0xFF,
						   context->buf[bufstart + 4] & 0xFF, context->buf[bufstart + 5] & 0xFF,
						   context->buf[bufstart + 6] & 0xFF, context->buf[bufstart + 7] & 0xFF));
			if(p[0] == 0 && p[1] == 0 && p[2] == 1)
			{
				/* PES packet */
				DBG(9, fprintf(stderr, "[dvb_read: skipping PES packet]\n"));
				bufstart = bufend = 0;
				continue;
			}
			section = (void *) p;
			l = GetSectionLength(&section->si);
			DBG(9, fprintf(stderr, "[dvb_read: table_id = 0x%02x, section_length = %d]\n", GetTableId(&section->si), l));
			if(bsize < l + sizeof(si_tab_t))
			{
				need = sizeof(si_tab_t) + l - bsize;
			}
			else
			{
				DBG(9, fprintf(stderr, "[dvb_read: have sufficient bytes]\n"));
				if (dvb_crc32(p, l + sizeof(si_tab_t)) != 0)
				{
					DBG(5, fprintf(stderr, "Warning: dvb_read: CRC failed; shifting start\n"));
					bufstart++;
					continue;
				}
				s = dvb_demux_read_section(context, section);
				bufstart = bufend;
				if(s)
				{
					DBG(9, fprintf(stderr, "[dvb_read: have a complete section set]\n"));
					return s;
				}
				DBG(9, fprintf(stderr, "[dvb_read: read a section, discarded or incomplete set; looping]\n"));
				continue;
			}
		}
		if(context->timeout && until)
		{
			now = time(NULL);
			if(now >= until)
			{
				DBG(8, fprintf(stderr, "[dvb_read: end time reached]\n"));
				break;
			}
			tvp = &tv;
			if(until - now > context->timeout)
			{
				tv.tv_sec = context->timeout;
			}
			else
			{
				tv.tv_sec = until - now;
			}
		}
		else if(context->timeout)
		{
			tvp = &tv;
			tv.tv_sec = context->timeout;
		}
		else if(until)
		{
			now = time(NULL);
			if(now >= until)
			{
				break;
			}
			tvp = &tv;
			tv.tv_sec = until - now;
		}
		else
		{
			tvp = NULL;
		}
		tv.tv_usec = 0;
		FD_SET(context->fd, &fds);
		DBG(9, fprintf(stderr, "[dvb_read: waiting for data]\n"));
		r = select(context->fd + 1, &fds, NULL, NULL, tvp);
		DBG(9, fprintf(stderr, "[dvb_read: select r=%d]\n", r));
		if(r == -1 && errno == EINTR)
		{
			continue;
		}
		else if(r == -1)
		{
			break;
		}
		nbytes = need;
		DBG(9, fprintf(stderr, "[dvb_read: %d bytes to read]\n", nbytes));
		do
		{
			r = read(context->fd, &(context->buf[bufend]), nbytes);
		}
		while(r == -1 && errno == EINTR);
		if(r == -1)
		{
			if(errno == EWOULDBLOCK)
			{
				/* Out of data */
				DBG(9, fprintf(stderr, "[dvb_read: EWOULDBLOCK]\n"));
				continue;
			}
			perror("dvb_read: read()");
			break;
		}
		if(!r)
		{
			DBG(9, fprintf(stderr, "[dvb_read: descriptor closed]\n"));
			break;
		}
		DBG(9, fprintf(stderr, "[dvb_read: read %d bytes]\n", r));
		bufend += r;
	}
	DBG(9, fprintf(stderr, "[dvb_read: loop ended]\n"));
	return NULL;
}

static dvb_table_t *
dvb_demux_read_section(dvb_demux_t *context, dvb_section_t *section)
{
	int versioned;
	dvb_table_t *table;
	dvb_section_t *p;
	size_t i;
	
	DBG(8, fprintf(stderr, "[read_section: table_id is 0x%02x]\n", section->si.table_id));
	switch(GetTableId(section))
	{
	case 0x00: /* PAT */
	case 0x02: /* PMT */
	case 0x03: /* TSDT */
	case 0x40: /* NIT (this network) */
	case 0x41: /* NIT (other network) */
	case 0x42: /* SDT (this TS) */
	case 0x46: /* SDT (other TS) */
	case 0x4E: /* EIT (this TS, now & next) */
	case 0x4F: /* EIT (other TS, now & next) */
		versioned = 1;
		break;
	default:
		if(GetTableId(section) >= 0x50 && GetTableId(section) <= 0x6F)
		{
			versioned = 1;
		}
		else
		{
			versioned = 0;
		}
	}
	if(versioned)
	{
		/* Versioned tables all have the same set of leading bytes */
		DBG(8, fprintf(stderr, "[read_section: table is versioned; version=%02x, secno=%d, last=%d]\n",
					   section->pat.version_number, section->pat.section_number, section->pat.last_section_number));
		table = dvb_demux_section_add(context, GetTableId(section), section->pat.version_number, section->pat.section_number, section->pat.last_section_number, section);
		for(i = 0; i < table->nsections; i++)
		{
			if(!table->sections[i])
			{
				DBG(8, fprintf(stderr, "[read_section: section %d of %d is absent]\n", (int) i, (int) table->nsections - 1 ));
				break;
			}
		}
		if(i == table->nsections)
		{
			/* Complete set of sections */
			DBG(8, fprintf(stderr, "[read_section: all %d sections are present]\n", (int) table->nsections));
			return table;
		}
		/* Not ready yet */
		return NULL;
	}
	DBG(8, fprintf(stderr, "[read_section: table is not versioned]\n"));
	/* dvb_demux_section_replace() */
	table = dvb_demux_table_alloc(context, GetTableId(section), 1);
	if(NULL == (p = calloc(1, GetSectionLength(section) + sizeof(si_tab_t))))
	{
		return NULL;
	} 
	memcpy(p, section, GetSectionLength(section) + sizeof(si_tab_t));
	table->sections[0] = p;
	return table;
}

static dvb_table_t *
dvb_demux_section_add(dvb_demux_t *context, int table_id, int version, int secnum, int last, dvb_section_t *section)
{
	size_t i, n;
	dvb_table_t *table;
	dvb_section_t *p;

	for(i = 0; i < context->ntables; i++)
	{
		if(context->tables[i].table_id == table_id)
		{
			table = &(context->tables[i]);
			if(table->version_number > version)
			{
				/* Discard the new one */
				return table;
			}
			/* TODO: deal with current/next */
			else if(table->version_number < version)
			{
				/* Discard the previous one */
				dvb_demux_table_reset(context, table, last + 1);
				table->version_number = version;
			}
			for(n = 0; n < table->nsections; n++)
			{
				if(!table->sections[n])
				{
					break;
				}
			}
			if(n == table->nsections)
			{
				/* We've previously read this one; start again */
				dvb_demux_table_reset(context, table, last + 1);
				table->version_number = version;
			}
			else if(table->sections[secnum])
			{
				/* This section's already been read, but there are more to
				 * follow
				 */
				return table;
			}
			if(NULL == (p = calloc(1, GetSectionLength(section) + sizeof(si_tab_t))))
			{
				return NULL;
			} 
			memcpy(p, section, GetSectionLength(section) + sizeof(si_tab_t));
			table->sections[secnum] = p;
			return table;
		}
	}
	/* No match, add a new entry */
	DBG(9, fprintf(stderr, "[section_add: adding a new table for section with table_id = 0x%02x]\n", table_id));
	table = dvb_demux_table_alloc(context, table_id, last + 1);
	table->version_number = version;
	if(NULL == (p = calloc(1, GetSectionLength(section) + sizeof(si_tab_t))))
	{
		return NULL;
	} 
	memcpy(p, section, GetSectionLength(section) + sizeof(si_tab_t));
	table->sections[secnum] = p;
	return table;
}

static dvb_table_t *
dvb_demux_table_alloc(dvb_demux_t *context, int table_id, int count)
{
	size_t i;
	dvb_table_t *p, *l;
	
	for(i = 0; i < context->ntables; i++)
	{
		if(context->tables[i].table_id == table_id)
		{
			p = &(context->tables[i]);
			dvb_demux_table_reset(context, p, count);
			return p;
		}
	}
	if(NULL == (l = realloc(context->tables, sizeof(dvb_table_t) * (context->ntables + 1))))
	{
		return NULL;
	}
	context->tables = l;
	p = &(l[context->ntables]);
	context->ntables++;
	memset(p, 0, sizeof(dvb_table_t));
	p->table_id = table_id;
	dvb_demux_table_reset(context, p, count);
	return p;
}

static void
dvb_demux_table_reset(dvb_demux_t *context, dvb_table_t *table, int count)
{
	size_t i;

	(void) context;
	
	for(i = 0; i < table->nsections; i++)
	{
		free(table->sections[i]);
	}
	free(table->sections);
	table->version_number = -1;
	table->nsections = count;
	table->sections = calloc(count, sizeof(dvb_table_t *));
}
