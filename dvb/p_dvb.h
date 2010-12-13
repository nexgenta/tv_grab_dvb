#ifndef P_DVB_H_
# define P_DVB_H_                       1

# include <time.h>
# include <stdint.h>
# include <sys/types.h>

# include "dvb.h"

# include "../debug.h"

struct dvb_demux_struct
{
	int fd;
	time_t timeout;
	uint8_t buf[8192];
	size_t ntables;
	dvb_table_t *tables;
};

#ifdef __cplusplus
extern "C" {
#endif

	dvb_demux_t *dvb_demux_new(int fd);
	void dvb_demux_delete(dvb_demux_t *context);

	uint32_t dvb_crc32(const uint8_t *data, size_t len);

#ifdef __cplusplus
};
#endif

#endif /*!P_DVB_H_*/
