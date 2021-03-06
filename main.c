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
#include <stdio.h>
#include "cc_api.h"
#ifdef CC_GETOPT_ENABLED
#include <getopt.h>
#endif

int main(int argc, char **argv)
{
	char *attr = NULL, *proxy_uris = NULL;
	cc_node_t *node = NULL;
#ifdef CC_GETOPT_ENABLED
	int c = 0;
	static struct option long_options[] = {
		{"attr", required_argument, NULL, 'a'},
		{"proxy_uris", required_argument, NULL, 'p'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "a:p:", long_options, NULL)) != -1) {
		switch (c) {
		case 'a':
			attr = optarg;
			break;
		case 'p':
			proxy_uris = optarg;
			break;
		default:
			break;
		}
	}

#endif

	if (cc_api_runtime_init(&node, attr, proxy_uris, "./") != CC_SUCCESS)
		return EXIT_FAILURE;

	if (cc_api_runtime_start(node) != CC_SUCCESS)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
