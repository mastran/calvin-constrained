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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cc_node.h"
#include "scheduler/cc_scheduler.h"
#include "cc_proto.h"
#include "cc_transport.h"
#include "cc_msgpack_helper.h"
#include "../../msgpuck/msgpuck.h"
#include "../south/platform/cc_platform.h"
#ifdef USE_TLS
#include "../../crypto/cc_crypto.h"
#endif

#ifdef USE_PERSISTENT_STORAGE
#define NODE_STATE_BUFFER_SIZE			10000
#endif

static void node_reset(node_t *node, bool remove_actors)
{
	int i = 0;
	list_t *tmp_list = NULL;

	log("Resetting node");

	while (node->actors != NULL) {
		tmp_list = node->actors;
		node->actors = node->actors->next;
		if (remove_actors)
			actor_free(node, (actor_t *)tmp_list->data, true);
		else
			actor_disconnect(node, (actor_t *)tmp_list->data);
	}
	if (remove_actors)
		node->actors = NULL;

	while (node->tunnels != NULL) {
		tmp_list = node->tunnels;
		node->tunnels = node->tunnels->next;
		tunnel_free(node, (tunnel_t *)tmp_list->data);
	}
	node->tunnels = NULL;
	node->storage_tunnel = NULL;

	while (node->links != NULL) {
		tmp_list = node->links;
		node->links = node->links->next;
		link_free(node, (link_t *)tmp_list->data);
	}
	node->links = NULL;
	node->proxy_link = NULL;

	for (i = 0; i < MAX_PENDING_MSGS; i++) {
		node->pending_msgs[i].handler = NULL;
		node->pending_msgs[i].msg_data = NULL;
	}
}

#ifdef USE_PERSISTENT_STORAGE
static bool node_get_state(node_t *node)
{
	result_t result = FAIL;
	char buffer[NODE_STATE_BUFFER_SIZE], *value = NULL, *array_value = NULL;
	uint32_t i = 0, value_len = 0, array_size = 0, state = 0;
	link_t *link = NULL;
	tunnel_t *tunnel = NULL;
	actor_t *actor = NULL;

	if (platform_read_node_state(node, buffer, NODE_STATE_BUFFER_SIZE) == SUCCESS) {
		result = decode_uint_from_map(buffer, "state", &state);
		if (result == SUCCESS)
			node->state = (node_state_t)state;

		result = decode_string_from_map(buffer, "id", &value, &value_len);
		if (result == SUCCESS)
			strncpy(node->id, value, value_len);

		result = decode_string_from_map(buffer, "name", &value, &value_len);
		if (result == SUCCESS)
			strncpy(node->name, value, value_len);

		if (result == SUCCESS) {
			if (get_value_from_map(buffer, "links", &array_value) == SUCCESS) {
				array_size = get_size_of_array(array_value);
				for (i = 0; i < array_size; i++) {
					if (get_value_from_array(array_value, i, &value) == SUCCESS) {
						link = link_deserialize(node, value);
						if (link == NULL) {
							result = FAIL;
							break;
						}
						if (link->is_proxy)
							node->proxy_link = link;
					}
				}
			}
		}

		if (result == SUCCESS) {
			if (get_value_from_map(buffer, "tunnels", &array_value) == SUCCESS) {
				array_size = get_size_of_array(array_value);
				for (i = 0; i < array_size; i++) {
					if (get_value_from_array(array_value, i, &value) == SUCCESS) {
						tunnel = tunnel_deserialize(node, value);
						if (tunnel == NULL) {
							result = FAIL;
							break;
						}
						if (tunnel->type == TUNNEL_TYPE_STORAGE)
							node->storage_tunnel = tunnel;
					}
				}
			}
		}

		if (result == SUCCESS) {
			if (get_value_from_map(buffer, "actors", &array_value) == SUCCESS) {
				array_size = get_size_of_array(array_value);
				for (i = 0; i < array_size; i++) {
					if (get_value_from_array(array_value, i, &value) == SUCCESS) {
						actor = actor_create(node, value);
						if (actor == NULL) {
							result = FAIL;
							break;
						}
					}
				}
			}
		}

		if (result == FAIL) {
			log_error("Failed to decode runtime state");
			node_reset(node, true);
		}
	}

	return result == SUCCESS ? true : false;
}

void node_set_state(node_t *node)
{
	char buffer[NODE_STATE_BUFFER_SIZE];
	char *tmp = buffer;
	int nbr_of_items = 0;
	list_t *item = NULL;

	tmp = mp_encode_map(tmp, 7);
	{
		tmp = encode_uint(&tmp, "state", node->state);
		tmp = encode_str(&tmp, "id", node->id, strlen(node->id));
		tmp = encode_str(&tmp, "name", node->name, strlen(node->name));

		nbr_of_items = list_count(node->links);
		tmp = encode_array(&tmp, "links", nbr_of_items);
		{
			item = node->links;
			while (item != NULL) {
				tmp = link_serialize((link_t *)item->data, &tmp);
				item = item->next;
			}
		}

		nbr_of_items = list_count(node->tunnels);
		tmp = encode_array(&tmp, "tunnels", nbr_of_items);
		{
			item = node->tunnels;
			while (item != NULL) {
				tmp = tunnel_serialize((tunnel_t *)item->data, &tmp);
				item = item->next;
			}
		}

		nbr_of_items = list_count(node->actors);
		tmp = encode_array(&tmp, "actors", nbr_of_items);
		{
			item = node->actors;
			while (item != NULL) {
				tmp = mp_encode_map(tmp, 1);
				tmp = actor_serialize(node, (actor_t *)item->data, &tmp, true);
				item = item->next;
			}
		}
	}

	platform_write_node_state(node, buffer, tmp - buffer);
}
#endif

result_t node_add_pending_msg(node_t *node, char *msg_uuid, uint32_t msg_uuid_len, result_t (*handler)(node_t *node, char *data, void *msg_data), void *msg_data)
{
	int i = 0;

	for (i = 0; i < MAX_PENDING_MSGS; i++) {
		if (node->pending_msgs[i].handler == NULL) {
			strncpy(node->pending_msgs[i].msg_uuid, msg_uuid, msg_uuid_len);
			node->pending_msgs[i].handler = handler;
			node->pending_msgs[i].msg_data = msg_data;
			return SUCCESS;
		}
	}

	log_error("Pending msg queue is full");
	return FAIL;
}

result_t node_remove_pending_msg(node_t *node, char *msg_uuid, uint32_t msg_uuid_len)
{
	int i = 0;

	for (i = 0; i < MAX_PENDING_MSGS; i++) {
		if (node->pending_msgs[i].handler != NULL) {
			if (strncmp(node->pending_msgs[i].msg_uuid, msg_uuid, msg_uuid_len) == 0) {
				node->pending_msgs[i].handler = NULL;
				node->pending_msgs[i].msg_data = NULL;
				return SUCCESS;
			}
		}
	}

	log_error("No pending msg with id '%s'", msg_uuid);
	return FAIL;
}

result_t node_get_pending_msg(node_t *node, const char *msg_uuid, uint32_t msg_uuid_len, pending_msg_t *pending_msg)
{
	int i = 0;

	for (i = 0; i < MAX_PENDING_MSGS; i++) {
		if (node->pending_msgs[i].handler != NULL) {
			if (strncmp(node->pending_msgs[i].msg_uuid, msg_uuid, msg_uuid_len) == 0) {
				*pending_msg = node->pending_msgs[i];
				return SUCCESS;
			}
		}
	}

	log_error("No pending msg with id '%s'", msg_uuid);
	return FAIL;
}

bool node_can_add_pending_msg(const node_t *node)
{
	int i = 0;

	for (i = 0; i < MAX_PENDING_MSGS; i++) {
		if (node->pending_msgs[i].handler == NULL)
			return true;
	}

	return false;
}

static result_t node_setup_reply_handler(node_t *node, char *data, void *msg_data)
{
	uint32_t status;
	char *value = NULL;

	if (get_value_from_map(data, "value", &value) == SUCCESS) {
		if (decode_uint_from_map(value, "status", &status) == SUCCESS) {
			if (status == 200) {
				log("Node started with proxy '%s'", node->transport_client->peer_id);
				node->state = NODE_STARTED;
				platform_node_started(node);
				return SUCCESS;
			}
			log_error("Failed to setup node, status '%d'", (int)status);
		}
	}

	return FAIL;
}

result_t node_handle_token(port_t *port, const char *data, const size_t size, uint32_t sequencenbr)
{
	if (port->actor->state == ACTOR_ENABLED)
		return fifo_com_write(&port->fifo, data, size, sequencenbr);
	return FAIL;
}

void node_handle_token_reply(node_t *node, char *port_id, uint32_t port_id_len, port_reply_type_t reply_type, uint32_t sequencenbr)
{
	port_t *port = port_get(node, port_id, port_id_len);

	if (port != NULL) {
		if (reply_type == PORT_REPLY_TYPE_ACK)
			fifo_com_commit_read(&port->fifo, sequencenbr);
		else if (reply_type == PORT_REPLY_TYPE_NACK)
			fifo_com_cancel_read(&port->fifo, sequencenbr);
		else if (reply_type == PORT_REPLY_TYPE_ABORT)
			log_debug("TODO: handle ABORT");
	}
}

result_t node_handle_message(node_t *node, char *buffer, size_t len)
{
	if (proto_parse_message(node, buffer) == SUCCESS) {
#ifdef USE_PERSISTENT_STORAGE
#ifdef PERSISTENT_STORAGE_CHECKPOINTING
		// message successfully handled == state changed -> serialize the node
		if (node->state == NODE_STARTED)
			node_set_state(node);
#endif
#endif
		return SUCCESS;
	}

	log_error("Failed to handle message");
	return FAIL;
}

static result_t node_setup(node_t *node, char *name)
{
#ifdef USE_PERSISTENT_STORAGE
	if (node_get_state(node)) {
		log("Node created from previous state, id '%s' name '%s'",
				node->id,
				node->name);
		return SUCCESS;
	}
#endif

	node->state = NODE_DO_START;

#ifdef USE_TLS
	char domain[50];

	if (crypto_get_node_info(domain, node->name, node->id) == SUCCESS) {
		log("Node created from certificate, domain: '%s' id '%s' name '%s'",
				domain,
				node->id,
				node->name);
		return SUCCESS;
	}
#endif

	gen_uuid(node->id, NULL);

	if (name != NULL)
		strncpy(node->name, name, strlen(name) + 1);
	else
		strncpy(node->name, "constrained", 12);

	log("Node created, id '%s' name '%s'", node->id, node->name);
	return SUCCESS;
}

static result_t node_connect_to_proxy(node_t *node, char *uri)
{
	char *peer_id = NULL;
	size_t peer_id_len = 0;

	if (node->transport_client == NULL) {
		node->transport_client = transport_create(node, uri);
		if (node->transport_client == NULL)
			return FAIL;
	}

	while (node->state != NODE_STOP && node->transport_client->state == TRANSPORT_INTERFACE_DOWN)
		platform_evt_wait(node, NULL);

	if (node->state == NODE_STOP || node->transport_client->connect(node, node->transport_client) != SUCCESS)
		return FAIL;

	while (node->state != NODE_STOP && node->transport_client->state == TRANSPORT_PENDING)
		platform_evt_wait(node, NULL);

	if (transport_join(node, node->transport_client) != SUCCESS)
		return FAIL;

	while (node->state != NODE_STOP && node->transport_client->state == TRANSPORT_PENDING)
		platform_evt_wait(node, NULL);

	if (node->state == NODE_STOP || node->transport_client->state != TRANSPORT_ENABLED)
		return FAIL;

	peer_id = node->transport_client->peer_id;
	peer_id_len = strlen(peer_id);

	if (node->proxy_link != NULL && strncmp(node->proxy_link->peer_id, peer_id, peer_id_len) != 0)
		node_reset(node, false);

	if (node->proxy_link == NULL) {
		node->proxy_link = link_create(node, peer_id, peer_id_len, true);
		if (node->proxy_link == NULL) {
			log_error("Failed to create proxy link");
			return FAIL;
		}
	}

	if (node->storage_tunnel == NULL) {
		node->storage_tunnel = tunnel_create(node, TUNNEL_TYPE_STORAGE, TUNNEL_DISCONNECTED, peer_id, peer_id_len, NULL, 0);
		if (node->storage_tunnel == NULL) {
			log_error("Failed to create storage tunnel");
			return FAIL;
		}
		tunnel_add_ref(node->storage_tunnel);
	}

	if (proto_send_node_setup(node, node_setup_reply_handler) != SUCCESS)
		return FAIL;

	while (node->state != NODE_STARTED && node->state != NODE_STOP)
		platform_evt_wait(node, NULL);

	if (node->state != NODE_STARTED) {
		log_error("Failed to setup proxy");
		return FAIL;
	}

	return SUCCESS;
}

result_t node_init(node_t *node, char *name, char *proxy_uris)
{
	int i = 0;
	char *uri = NULL;

	node->fire_actors = fire_actors;
	node->transport_client = NULL;
	node->proxy_link = NULL;
	node->platform = NULL;
	node->attributes = NULL;
	node->links = NULL;
	node->storage_tunnel = NULL;
	node->tunnels = NULL;
	node->actors = NULL;
	node->calvinsys = NULL;

	log("Initializing node");

	if (platform_create(node) != SUCCESS) {
		log_error("Failed to create platform object");
		return FAIL;
	}

	if (platform_create_calvinsys(node) != SUCCESS) {
		log_error("Failed to create calvinsys object");
		return FAIL;
	}

	if (node_setup(node, name) != SUCCESS) {
		log_error("Failed to setup node");
		return FAIL;
	}

	if (proxy_uris != NULL) {
		uri = strtok(proxy_uris, " ");
		while (uri != NULL && i < MAX_URIS) {
			node->proxy_uris[i] = uri;
			uri = strtok(NULL, " ");
			i++;
		}
	}

	log("Node initialized");

	return SUCCESS;
}

result_t node_run(node_t *node)
{
	int i = 0;
	struct timeval reconnect_timeout;

	if (node->fire_actors == NULL) {
		log_error("No actor scheduler set");
		return FAIL;
	}

	while (node->state != NODE_STOP) {
		for (i = 0; i < MAX_URIS && node->state != NODE_STOP; i++) {
			if (node->proxy_uris[i] != NULL) {
				log("Connecting to '%s'", node->proxy_uris[i]);
				node->state = NODE_DO_START;
				if (node_connect_to_proxy(node, node->proxy_uris[i]) == SUCCESS) {
					log("Connected to '%s'", node->proxy_uris[i]);
					while (node->state != NODE_STOP && node->transport_client->state == TRANSPORT_ENABLED) {
						if (node->state == NODE_STARTED) {
							// fire actors and transfer data from in- and out-ports§
							node->fire_actors(node);
						}
						platform_evt_wait(node, NULL);
					}
					log("Disconnected from '%s'", node->proxy_uris[i]);
				}

				if (node->transport_client != NULL) {
					node->transport_client->disconnect(node, node->transport_client);
					node->transport_client->free(node->transport_client);
					node->transport_client = NULL;
				}
			}
		}

		if (node->state != NODE_STOP) {
			reconnect_timeout.tv_sec = 5;
			reconnect_timeout.tv_usec = 0;
			platform_evt_wait(node, &reconnect_timeout);
		}
	}

	log("Node stopped");
	return SUCCESS;
}