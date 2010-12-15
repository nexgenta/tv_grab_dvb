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
