#ifndef DVB_DEMUX_H_
# define DVB_DEMUX_H_                   1

# include <linux/dvb/dmx.h>

int dvb_demux_open_path(const char *path, struct dmx_sct_filter_params *filter);
int dvb_demux_open(int adapter, int demux, struct dmx_sct_filter_params *filter);

#endif /*!DVB_DEMUX_H_*/

