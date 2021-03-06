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
#include <stdlib.h>
#include <string.h>
#include "cc_actor_light.h"
#include "runtime/north/cc_actor_store.h"
#include "runtime/north/coder/cc_coder.h"
#include "runtime/north/cc_fifo.h"

static cc_result_t cc_actor_light_init(cc_actor_t **actor, cc_list_t *managed_attributes)
{
	char *obj_ref = NULL;

	obj_ref = cc_calvinsys_open(*actor, "io.light", NULL, 0);
	if (obj_ref == NULL) {
		cc_log_error("Failed to open 'io.light'");
		return CC_FAIL;
	}

	(*actor)->instance_state = (void *)obj_ref;

	return CC_SUCCESS;
}

static cc_result_t cc_actor_light_set_state(cc_actor_t **actor, cc_list_t *managed_attributes)
{
	cc_list_t *item = NULL;

	item = cc_list_get(managed_attributes, "io.light");
	if (item == NULL) {
		cc_log_error("Failed to get 'io.light'");
		return CC_FAIL;
	}

	(*actor)->instance_state = (void *)item->id;

	return CC_SUCCESS;
}

static bool cc_actor_light_fire(struct cc_actor_t *actor)
{
	cc_port_t *inport = (cc_port_t *)actor->in_ports->data;
	char *obj_ref = (char *)actor->instance_state;
	cc_token_t *token = NULL;

	if (!cc_fifo_tokens_available(inport->fifo, 1))
		return false;

	token = cc_fifo_peek(inport->fifo);
	if (cc_calvinsys_write(actor->calvinsys, obj_ref, token->value, token->size) != CC_SUCCESS) {
		cc_fifo_cancel_commit(inport->fifo);
		return false;
	}

	cc_fifo_commit_read(inport->fifo, true);

	return true;
}

static cc_result_t cc_actor_light_get_attributes(cc_actor_t *actor, cc_list_t **managed_attributes)
{
	char *obj_ref = NULL, *buffer = NULL, *w = NULL;
	uint32_t buffer_len = 0;

	if (actor->instance_state == NULL) {
		cc_log_error("Actor does not have a state");
		return CC_FAIL;
	}

	obj_ref = (char *)actor->instance_state;
	buffer_len = cc_coder_sizeof_str(strlen(obj_ref));

	if (cc_platform_mem_alloc((void **)&buffer, buffer_len) != CC_SUCCESS) {
		cc_log_error("Failed to allocate memory");
		return CC_FAIL;
	}

	w = buffer;
	w = cc_coder_encode_str(w, obj_ref, strlen(obj_ref));

	if (cc_list_add_n(managed_attributes, "light", 6, buffer, w - buffer) == NULL) {
		cc_log_error("Failed to add 'light' to managed attributes");
		cc_platform_mem_free(buffer);
		return CC_FAIL;
	}

	return CC_SUCCESS;
}

cc_result_t cc_actor_light_register(cc_list_t **actor_types)
{
	cc_actor_type_t *type = NULL;

	if (cc_platform_mem_alloc((void **)&type, sizeof(cc_actor_type_t)) != CC_SUCCESS) {
		cc_log_error("Failed to allocate memory");
		return CC_FAIL;
	}

	memset(type, 0, sizeof(cc_actor_type_t));
	type->init = cc_actor_light_init;
	type->set_state = cc_actor_light_set_state;
	type->fire_actor = cc_actor_light_fire;
	type->get_managed_attributes = cc_actor_light_get_attributes;

	if (cc_list_add_n(actor_types, "io.Light", 8, type, sizeof(cc_actor_type_t *)) == NULL) {
		cc_log_error("Failed to register 'io.Light'");
		return CC_FAIL;
	}

	return CC_SUCCESS;
}
