/* DVB/MPEG TS Programe Association Table */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "dvb.h"

/* Parse a Programe Association Table; invoked by parse_dvb_si() */
int
dvb_parse_pat(dvb_table_t *table, dvb_callbacks_t *callbacks)
{
	(void) table;
	(void) callbacks;

	return 0;
}


