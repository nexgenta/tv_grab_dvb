#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/dvb/dmx.h>

#include "dvb-demux.h"

int
dvb_demux_open_path(const char *path, struct dmx_sct_filter_params *filter)
{
	int fd;
	struct stat sbuf;

	if(-1 == (fd = open(path, O_RDONLY)))
	{
		return -1;
	}
	if(-1 == fstat(fd, &sbuf))
	{
		close(fd);
		return -1;
	}
	if(S_ISCHR(sbuf.st_mode))
	{
		if(filter)
		{
			if(-1 == ioctl(fd, DMX_SET_FILTER, filter))
			{
				close(fd);
				return -1;
			}
		}		
	}
	return fd;
}

int
dvb_demux_open(int adapter, int demux, struct dmx_sct_filter_params *filter)
{
	char path[64];
	
	snprintf(path, 64, "/dev/dvb/adapter%d/demux%d", adapter, demux);
	return dvb_demux_open_path(path, filter);
}

			
