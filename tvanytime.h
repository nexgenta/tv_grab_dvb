#ifndef TVANYTIME_H_
# define TVANYTIME_H_                   1

# include <stdio.h>

# include "dvb/dvb.h"

typedef struct tva_options_struct tva_options_t;

struct tva_options_struct
{
	FILE *out;
};

void tva_preamble_service(tva_options_t *options);
void tva_postamble_service(tva_options_t *options);
int tva_write_service(service_t *service, void *data);
int tva_write_event(event_t *event, void *data);

#endif /*!TVANYTIME_H_ */
