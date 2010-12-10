#ifndef SERVICES_H_
# define SERVICES_H_                    1

typedef struct service_struct service_t;

typedef enum service_type {
	ST_RESERVED = 0,
	ST_TV,
	ST_RADIO,
	ST_TELETEXT,
	ST_NVOD_REF,
	ST_NVOD_TIMESHIFT,
	ST_MOSAIC,
	ST_FM,
	ST_DVB_SRM,
	ST_RESERVED_09,
	ST_ADVANCED_CODEC_RADIO,
	ST_ADVANCED_CODEC_MOSAIC,
	ST_DATA,
	ST_RESERVED_CIU,
	ST_RCS_MAP,
	ST_RCS_FLS,
	ST_DVB_MHP, /* 0x10 */
	ST_MPEG2,
	ST_RESERVED_12,
	ST_RESERVED_13,
	ST_RESERVED_14,
	ST_RESERVED_15,
	ST_ADVANCED_CODEC_SD,
	ST_ADVANCED_CODEC_NVOD_TIMESHIFT,
	ST_ADVANCED_CODEC_NVOD_REF,
	ST_ADVANCED_CODEC_HD,
	ST_ADVANCED_CODEC_HD_NVOD_TIMESHIFT,
	ST_ADVANCED_CODEC_HD_NVOD_REF,
	/* 0x1C .. 0x7F */
	ST_USER_DEFINED_MIN = 0x80,
	ST_USER_DEFINED_MAX = 0xFE,
	ST_RESERVED_FF = 0xFF
} service_type_t;

service_t *service_add(const char *uri);
service_t *service_add_dvb(int original_network_id, int transport_stream_id, int service_id);

service_t *service_locate(const char *uri);
service_t *service_locate_dvb(int original_network_id, int transport_stream_id, int service_id);

void service_set_type(service_t *service, service_type_t type);
service_type_t service_type(service_t *service);

void service_set_name(service_t *service, const char *name);
const char *service_name(service_t *service);

void service_set_provider(service_t *service, const char *provider);
const char *service_provider(service_t *service);

void service_set_authority(service_t *service, const char *authority);
const char *service_authority(service_t *service);

void service_debug_dump(void);

#endif /*!SERVICES_H_*/
