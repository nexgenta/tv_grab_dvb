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

#ifndef DVB_H_
# define DVB_H_                         1

# include "services.h"
# include "events.h"
# include "networks.h"
# include "multiplexes.h"
# include "platforms.h"

# include "callbacks.h"

# include "si_tables.h"

# include <sys/types.h>
# include <time.h>
# include <linux/dvb/dmx.h>

typedef struct dvb_table_struct dvb_table_t;
typedef union dvb_section_union dvb_section_t;
typedef struct dvb_demux_struct dvb_demux_t;

struct dvb_table_struct
{
	int table_id;
	int version_number;
	int current_next_indicator;
	uint32_t identifier;
	size_t nsections;
	dvb_section_t **sections;
};

union dvb_section_union
{
	si_tab_t si;
	pat_t pat;
	cat_t cat;
	pmt_t pmt;
	nit_t nit;
	sdt_t sdt;
	eit_t eit;
};

# ifdef __cplusplus
extern "C" {
# endif

	dvb_demux_t *dvb_demux_open_fd(int fd, struct dmx_sct_filter_params *filter, struct dmx_pes_filter_params *pesfilter, size_t npesfilter);
	dvb_demux_t *dvb_demux_open_path(const char *path, struct dmx_sct_filter_params *filter, struct dmx_pes_filter_params *pesfilter, size_t npesfilter);
	dvb_demux_t *dvb_demux_open(int adapter, int demux, struct dmx_sct_filter_params *filter, struct dmx_pes_filter_params *pesfilter, size_t npesfilter);

	void dvb_demux_close(dvb_demux_t *context);

	void dvb_demux_set_fd(dvb_demux_t *context, int fd);
	int dvb_demux_fd(dvb_demux_t *context);
	
	void dvb_demux_set_timeout(dvb_demux_t *context, time_t timeout);
	time_t dvb_demux_timeout(dvb_demux_t *context);

	int dvb_demux_start(dvb_demux_t *context);

	dvb_table_t *dvb_demux_read(dvb_demux_t *context, time_t until);

	int dvb_parse_si(dvb_table_t *table, dvb_callbacks_t *callbacks);

	int dvb_parse_pat(dvb_table_t *table, dvb_callbacks_t *callbacks);
	int dvb_parse_nit(dvb_table_t *table, dvb_callbacks_t *callbacks);
	int dvb_parse_sdt(dvb_table_t *table, dvb_callbacks_t *callbacks);

# ifdef __cplusplus
};
# endif

#endif /*!DVB_H_*/
