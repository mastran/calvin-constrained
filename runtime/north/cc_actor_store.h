/*
 * Copyright (c) 2016 Ericsson AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef CC_ACTOR_STORE_H
#define CC_ACTOR_STORE_H

#include <stdbool.h>
#include "cc_common.h"
#include "cc_actor.h"

typedef struct cc_actor_type_t {
	cc_result_t (*init)(cc_actor_t**actor, cc_list_t *managed_attributes);
	cc_result_t (*set_state)(cc_actor_t**actor, cc_list_t *managed_attributes);
	void (*free_state)(cc_actor_t*actor);
	bool (*fire_actor)(cc_actor_t*actor);
	cc_result_t (*get_managed_attributes)(cc_actor_t*actor, cc_list_t **attributes);
	void (*will_migrate)(cc_actor_t*actor);
	void (*will_end)(cc_actor_t*actor);
	void (*did_migrate)(cc_actor_t*actor);
} cc_actor_type_t;

cc_result_t cc_actor_store_init(cc_list_t **actor_types);

#endif /* CC_ACTOR_STORE_H */
