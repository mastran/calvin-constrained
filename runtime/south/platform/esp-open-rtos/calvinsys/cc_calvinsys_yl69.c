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
#include <espressif/esp_common.h>
#include "cc_calvinsys_yl69.h"
#include "runtime/north/cc_node.h"
#include "runtime/north/coder/cc_coder.h"
#include "runtime/south/platform/cc_platform.h"
#include "calvinsys/cc_calvinsys.h"

static bool cc_calvinsys_yl69_can_read(struct cc_calvinsys_obj_t *obj)
{
	return true;
}

static cc_result_t cc_calvinsys_yl69_read(struct cc_calvinsys_obj_t *obj, char **data, size_t *size)
{
	uint16_t adc_value = sdk_system_adc_read();
	float humidity = 1024 - adc_value;
	char *w = NULL;

	humidity = humidity / 1024;
	humidity = humidity * 100;

	*size = cc_coder_sizeof_float(humidity);
	if (cc_platform_mem_alloc((void **)data, *size) != CC_SUCCESS) {
		cc_log_error("Failed to allocate memory");
		return CC_FAIL;
	}
	w = *data;
	w = cc_coder_encode_float(w, humidity);

	return CC_SUCCESS;
}

static cc_result_t cc_calvinsys_yl69_open(cc_calvinsys_obj_t *obj, char *data, size_t len)
{
	obj->can_read = cc_calvinsys_yl69_can_read;
	obj->read = cc_calvinsys_yl69_read;

	return CC_SUCCESS;
}

static cc_result_t cc_calvinsys_yl69_deserialize(cc_calvinsys_obj_t *obj, char *buffer)
{
	return cc_calvinsys_yl69_open(obj, buffer, 0);
}

cc_result_t cc_calvinsys_yl69_create(cc_calvinsys_t **calvinsys, const char *name)
{
	return cc_calvinsys_create_capability(*calvinsys,
		name,
		cc_calvinsys_yl69_open,
		cc_calvinsys_yl69_deserialize,
		NULL);
}
