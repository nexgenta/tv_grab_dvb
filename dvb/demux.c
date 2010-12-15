/*
 * Copyright 2010 Mo McRoberts.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/dvb/dmx.h>

#include "dvb.h"

#include "p_dvb.h"

dvb_demux_t *
dvb_demux_open_fd(int fd, struct dmx_sct_filter_params *filter, struct dmx_pes_filter_params *pesfilter, size_t npesfilter)
{
	struct stat sbuf;
	size_t i;
	dvb_demux_t *p;

	if(-1 == fstat(fd, &sbuf))
	{
		return NULL;
	}
	if(S_ISCHR(sbuf.st_mode))
	{
		if(filter)
		{
			if(-1 == ioctl(fd, DMX_SET_FILTER, filter))
			{
				return NULL;
			}
		}
		if(pesfilter)
		{
			for(i = 0; i < npesfilter; i++)
			{
				if(-1 == ioctl(fd, DMX_SET_PES_FILTER, &(pesfilter[i])))
				{
					fprintf(stderr, "[dvb_demux_open: ioctl() failed on filter %d]: %s\n", i, strerror(errno));
					return NULL;
				}
			}
		}
	}
	if(!(p = dvb_demux_new(fd)))
	{
		return NULL;
	}
	return p;
}

dvb_demux_t *
dvb_demux_open_path(const char *path, struct dmx_sct_filter_params *filter, struct dmx_pes_filter_params *pesfilter, size_t npesfilter)
{
	int fd;
	dvb_demux_t *p;

	if(-1 == (fd = open(path, O_RDONLY|O_NONBLOCK)))
	{
		return NULL;
	}
	if(NULL == (p = dvb_demux_open_fd(fd, filter, pesfilter, npesfilter)))
	{
		close(fd);
		return NULL;
	}
	return p;
}

dvb_demux_t *
dvb_demux_open(int adapter, int demux, struct dmx_sct_filter_params *filter, struct dmx_pes_filter_params *pesfilter, size_t npes)
{
	char path[64];
	
	snprintf(path, 64, "/dev/dvb/adapter%d/demux%d", adapter, demux);
	return dvb_demux_open_path(path, filter, pesfilter, npes);
}

void
dvb_demux_close(dvb_demux_t *context)
{
	if(context->fd != -1)
	{
		close(context->fd);
	}
	context->fd = -1;
	dvb_demux_delete(context);
}

int
dvb_demux_start(dvb_demux_t *context)
{
	return ioctl(context->fd, DMX_START);
}

dvb_demux_t *
dvb_demux_new(int fd)
{
	dvb_demux_t *p;

	if(NULL == (p = calloc(1, sizeof(dvb_demux_t))))
	{
		return NULL;
	}
	p->fd = fd;
	return p;
}

void
dvb_demux_delete(dvb_demux_t *context)
{
	free(context);
}

void
dvb_demux_set_fd(dvb_demux_t *context, int fd)
{
	context->fd = fd;
}

int
dvb_demux_fd(dvb_demux_t *context)
{
	return context->fd;
}

void
dvb_demux_set_timeout(dvb_demux_t *context, time_t timeout)
{
	context->timeout = timeout;
}

time_t
dvb_demux_timeout(dvb_demux_t *context)
{
	return context->timeout;
}
			
