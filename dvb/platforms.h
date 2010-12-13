#ifndef PLATFORMS_H_
# define PLATFORMS_H_                   1

typedef struct platform_struct platform_t;

platform_t *platform_add(const char *ident);
platform_t *platform_add_dvb(int original_network_id);

platform_t *platform_locate(const char *ident);
platform_t *platform_locate_dvb(int original_network_id);

platform_t *platform_locate_add(const char *ident);
platform_t *platform_locate_add_dvb(int original_network_id);

void platform_reset(platform_t *platform);

const char *platform_uri(platform_t *platform);

#endif /*!PLATFORMS_H_*/
