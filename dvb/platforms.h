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
