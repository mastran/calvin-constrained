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
#include "cc_calvinsys_ds18b20.h"
#include "runtime/north/cc_node.h"
#include "runtime/north/coder/cc_coder.h"
#include "runtime/south/platform/cc_platform.h"
#include "calvinsys/cc_calvinsys.h"
#include "ds18b20/ds18b20.h"

#define CC_DS18B20_SENSOR_GPIO 5

static bool cc_calvinsys_ds18b20_can_write(struct cc_calvinsys_obj_t *obj)
{
	return true;
}

static cc_result_t cc_calvinsys_ds18b20_write(cc_calvinsys_obj_t *obj, char *data, size_t size)
{
	return CC_SUCCESS;
}

static bool cc_calvinsys_ds18b20_can_read(struct cc_calvinsys_obj_t *obj)
{
	return true;
}

static cc_result_t cc_calvinsys_ds18b20_read(struct cc_calvinsys_obj_t *obj, char **data, size_t *size)
{
	float temp;
	char *w = NULL;
	int nsensors = 0;
	ds18b20_addr_t addrs[1];

	nsensors = ds18b20_scan_devices(CC_DS18B20_SENSOR_GPIO, addrs, 1);
	if (nsensors < 1) {
		cc_log_error("Failed to scan devices, count '%d'", nsensors);
		return CC_FAIL;
	}

	temp = ds18b20_measure_and_read(CC_DS18B20_SENSOR_GPIO, addrs[0]);

	*size = cc_coder_sizeof_float(temp);
	if (cc_platform_mem_alloc((void **)data, *size) != CC_SUCCESS) {
		cc_log_error("Failed to allocate memory");
		return CC_FAIL;
	}
	w = *data;
	w = cc_coder_encode_float(w, temp);

	return CC_SUCCESS;
}

static cc_result_t cc_calvinsys_ds18b20_open(cc_calvinsys_obj_t *obj, char *data, size_t len)
{
	obj->can_write = cc_calvinsys_ds18b20_can_write;
	obj->write = cc_calvinsys_ds18b20_write;
	obj->can_read = cc_calvinsys_ds18b20_can_read;
	obj->read = cc_calvinsys_ds18b20_read;

	return CC_SUCCESS;
}

static cc_result_t cc_calvinsys_ds18b20_deserialize(cc_calvinsys_obj_t *obj, char *buffer)
{
	return cc_calvinsys_ds18b20_open(obj, buffer, 0);
}

cc_result_t cc_calvinsys_ds18b20_create(cc_calvinsys_t **calvinsys, const char *name)
{
	return cc_calvinsys_create_capability(*calvinsys,
		name,
		cc_calvinsys_ds18b20_open,
		cc_calvinsys_ds18b20_deserialize,
		NULL);
}
