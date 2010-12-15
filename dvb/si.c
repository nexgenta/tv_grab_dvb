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

/* Parse DVB Service Information tables */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "p_dvb.h"

int
dvb_parse_si(dvb_table_t *table, dvb_callbacks_t *callbacks)
{
	DBG(5, fprintf(stderr, "[dvb_parse_si: table_id=0x%02x, version=%02d, sections=%d]\n",
				   table->table_id, table->version_number, table->nsections));
	switch(table->table_id)
	{
	case 0x00:
		return dvb_parse_pat(table, callbacks);		
	case 0x40:
		return dvb_parse_nit(table, callbacks);
	case 0x42:
	case 0x46:
		return dvb_parse_sdt(table, callbacks);
	case 0x4a:
		fprintf(stderr, "Warning: parse_dvb_si: bouquet_association_table (0x%02x) is not yet handled\n", table->table_id);
		break;
	}
	fprintf(stderr, "Warning: parse_dvb_si: Unknown SI table 0x%02x (version=%02d)\n",
			table->table_id, table->version_number);
	return 0;
}
