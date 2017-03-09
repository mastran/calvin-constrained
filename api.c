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

#include <string.h>
#include "api.h"
#include "platform.h"
#include "node.h"
#ifdef MICROPYTHON
#include "libmpy/calvin_mpy_port.h"
#endif


result_t api_runtime_init(node_t **node, char *name, char *proxy_uris)
{
	platform_init();

#ifdef MICROPYTHON
	if (!mpy_port_init(MICROPYTHON_HEAP_SIZE)) {
		log_error("Failed to initialize micropython lib");
		return FAIL;
	}
#endif

	if (platform_mem_alloc((void **)node, sizeof(node_t)) != SUCCESS) {
		log_error("Failed to allocate memory");
		return FAIL;
	}
	memset(*node, 0, sizeof(node_t));

	return node_init(*node, name, proxy_uris);
}

result_t api_runtime_start(node_t *node)
{
	return node_run(node);
}

result_t api_runtime_stop(node_t *node)
{
	node->state = NODE_STOP;
	return SUCCESS;
}