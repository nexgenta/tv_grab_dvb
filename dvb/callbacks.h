#ifndef CALLBACKS_H_
# define CALLBACKS_H_ 1

# include "services.h"
# include "events.h"
# include "networks.h"

typedef struct dvb_callbacks_struct dvb_callbacks_t;

struct dvb_callbacks_struct
{
	void (*service)(service_t *svc, void *data);
	void *service_data;

	void (*event)(event_t *event, void *data);
	void *event_data;

	void (*network)(network_t *network, void *data);
	void *network_data;
};

#endif /*!CALLBACKS_H_*/
