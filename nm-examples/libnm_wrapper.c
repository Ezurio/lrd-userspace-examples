/**
 * Copyright (c) 2019, Laird
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include "libnm_wrapper_internal.h"

#define ADDR_LEN 16

libnm_wrapper_handle_st *st = NULL;
/**
 * This file provides C APIs to manage network devices and connections based on NetworkManager.
 */


/**
 * @name Library Initialization and Destroy APIs
 * A library handle MUST be initialized before calling any of other APIs, and the handle MUST
 * be destroyed at exit.
 */
/**@{*/
/**
 * Initialize library handle.
 *
 * Returns: pointer to library handle
 *          NULL if unsuccessful
 */
libnm_wrapper_handle libnm_wrapper_init()
{
	int i;

	if(!st) {
		st = malloc(sizeof(libnm_wrapper_handle_st));
		st->client = nm_client_new(NULL, NULL);
	} else {
		// Process any events that are pending on the main loop that have not
		// been processed since the last iteration
		for (i=0; i<10; i++) {
			if (!g_main_context_iteration(NULL, FALSE))
				break;
		}
	}

	return (libnm_wrapper_handle) st;
}

/**
 * Destroy library handle.
 *
 * Returns: pointer to library handle
 */
void libnm_wrapper_destroy(libnm_wrapper_handle hd)
{
	int i;

	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	if(!client)
		return;

	// Need to process any events that are still on the main loop so cleanup is successful
	// We should have processed all events we're interested in already but there are many cases
	// where we have never run the loop at all so internal events have not been processed
	// Typically only 1 event is still pending, fail-safe just in case
	for (i=0; i<10; i++) {
		if (!g_main_context_iteration(NULL, FALSE))
			break;
	}

	//g_object_unref (client);
	//free(hd);
}
/**@}*/


/**
 * @name Connection Management APIs
 * Add, activate, deactivate, and remove a connection.
 */
/**@{*/
/**
 * Adding/activating a connecton is async. Callbacks are needed to get the results.
 */
typedef struct _libnm_wrapper_cb_st
{
	GMainLoop *loop;
	int *result;
	int g_timer_id;
	NMActiveConnection *active;
}libnm_wrapper_cb_st;

static void added_cb(GObject *client, GAsyncResult *result, gpointer user_data)
{
	GError *error = NULL;
	NMRemoteConnection *remote;
	libnm_wrapper_cb_st *temp = (libnm_wrapper_cb_st *)user_data;
	GMainLoop *loop = temp->loop;
	int *ret = temp->result;

	remote = nm_client_add_connection_finish (NM_CLIENT (client), result, &error);
	if (error) {
		*ret = LIBNM_WRAPPER_ERR_FAIL;
		g_error_free (error);
	} else {
		*ret = LIBNM_WRAPPER_ERR_SUCCESS;
		g_object_unref (remote);
	}
	g_free(temp);
	g_main_loop_quit(loop);
}

/**
 * Create a new connection.
 * @param client: point to NetworkManager client
 * @param connection: connection to be added
 * @param result: location to store result
 *
 * Returns: result = LIBNM_WRAPPER_ERR_SUCCESS successful
 */
static void add_connection(NMClient *client, NMConnection *connection, int *result)
{
	GMainLoop *loop = g_main_loop_new (NULL, FALSE);
	libnm_wrapper_cb_st *temp = g_malloc0(sizeof(libnm_wrapper_cb_st));

	temp->loop = loop;
	temp->result = result;
	nm_client_add_connection_async(client, connection, TRUE, NULL, added_cb, temp);
	g_main_loop_run (loop);
	g_object_unref (connection);
}

/**
 * Add general settings to a connection.
 * @param connection: location to store connection settings
 * @param s         : general settings from user input
 * Returns:
 */
static void add_settings(NMConnection *connection, NMWrapperSettings *s)
{
	char *uuid = NULL;
	NMSettingConnection *s_con = NULL;

	s_con = (NMSettingConnection *) nm_setting_connection_new ();
	nm_connection_add_setting (connection, NM_SETTING (s_con));

	uuid = nm_utils_uuid_generate();
	g_object_set (s_con,
		NM_SETTING_CONNECTION_ID, s->id,
		NM_SETTING_CONNECTION_UUID, uuid,
		NM_SETTING_CONNECTION_AUTOCONNECT, s->autoconnect,
		NM_SETTING_CONNECTION_AUTOCONNECT_RETRIES, 0,
		NM_SETTING_CONNECTION_AUTH_RETRIES, 0,
		NM_SETTING_CONNECTION_INTERFACE_NAME, s->interface,
		NM_SETTING_CONNECTION_TYPE, s->type, NULL);
	g_free (uuid);
}

/**
 * Update general settings of a connection.
 * @param connection: location to store connection settings
 * @param s         : general settings from user input
 * Returns:
 */
static void update_settings(NMConnection *connection, NMWrapperSettings *s)
{
	NMSettingConnection *s_con = nm_connection_get_setting_connection(connection);

	g_object_set (s_con,
		NM_SETTING_CONNECTION_ID, s->id,
		NM_SETTING_CONNECTION_AUTOCONNECT, s->autoconnect,
		NM_SETTING_CONNECTION_INTERFACE_NAME, s->interface, NULL);
}

/**
 * Get general settings of a connection.
 * @param connection: location to store connection settings
 * @param s         : location to store general settings
 *
 * Returns:
 */
static void get_settings(NMConnection *connection, NMWrapperSettings *s)
{
	const char *ptr = NULL;
	NMSettingConnection *s_con = NULL;

	s_con = nm_connection_get_setting_connection(connection);

	ptr = nm_setting_connection_get_id(s_con);
	safe_strncpy(s->id, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	s->autoconnect = false;
	if(nm_setting_connection_get_autoconnect(s_con))
		s->autoconnect = true;

	ptr = nm_setting_connection_get_uuid(s_con);
	safe_strncpy(s->uuid, ptr, LIBNM_WRAPPER_MAX_UUID_LEN);

	ptr = nm_setting_connection_get_connection_type(s_con);
	safe_strncpy(s->type, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_connection_get_interface_name(s_con);
	safe_strncpy(s->interface, ptr, LIBNM_WRAPPER_MAX_UUID_LEN);
}

/**
 * Get Active connection.
 * @param client: point to NetworkManager client
 * @param connection:  on which interface
 *
 * Returns: NMRemoteConnection of the active connetion on the interface if successful,
 *          NULL if failed
 */
static NMRemoteConnection* get_active_connection(NMClient *client, const char *interface)
{
	NMDevice *dev = NULL;
	NMActiveConnection *active = NULL;

	dev = nm_client_get_device_by_iface(client, interface);
	active = nm_device_get_active_connection(dev);
	if(!active)
		return NULL;

	return nm_active_connection_get_connection(active);
}

/**
 * Get connection settings.
 * @param hd: library handle
 * @param connection:  on which interface
 * @param id: connectin id
 * @param s : location to store general settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_settings(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperSettings *s)
{
	NMConnection *connection = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	if(id)
		connection = NM_CONNECTION(nm_client_get_connection_by_id(client, id));
	else
		connection = NM_CONNECTION(get_active_connection(client, interface));

	nm_wrapper_assert(connection, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	get_settings(connection, s);
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

/**
 * Get an array of settings from connections.
 * @param hd: library handle
 * @param interface:  on which interface
 * @param s : array to store general settings
 * @param size : the size of array
 *
 * Returns: number of connections actually parsed
 */
int libnm_wrapper_connections_get_settings(libnm_wrapper_handle hd, const char *interface, NMWrapperSettings *s, int size)
{
	int i, j;
	const GPtrArray *connections;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	connections = nm_client_get_connections(client);
	for (i = j = 0; i < connections->len; i++)
	{
		NMConnection *connection = NM_CONNECTION(connections->pdata[i]);
		NMSettingConnection *s_con = nm_connection_get_setting_connection(connection);
		const char *ptr = nm_setting_connection_get_interface_name(s_con);
		if(ptr && !strncmp(ptr, interface, strlen(interface)))
		{
			if(j < size)
				get_settings(connection, &s[j]);
			++j;
			if(size && (j >= size))
				break;
		}
	}
	return j;
}

/**
 * Get active connection state.
 * @param hd: library handle
 * @param interface: on which device
 * @param active: active connection id
 *
 * Returns: NMActiveConnectionState
 */
int libnm_wrapper_active_connection_get_state(libnm_wrapper_handle hd, const char *interface, const char *active)
{
	NMConnection *connection = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	if(active)
		connection = NM_CONNECTION(nm_client_get_connection_by_id(client, active));
	else
		connection = NM_CONNECTION(get_active_connection(client, interface));

	return nm_active_connection_get_state(NM_ACTIVE_CONNECTION(connection));
}

/**
 * Get active connection state reason.
 * @param hd: library handle
 * @param interface: on which device
 * @param active: active connection id
 *
 * Returns: NMActiveConnectionStateReason
 */
int libnm_wrapper_active_connection_get_state_reason(libnm_wrapper_handle hd, const char *interface, const char *active)
{
	NMConnection *connection = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	if(active)
		connection = NM_CONNECTION(nm_client_get_connection_by_id(client, active));
	else
		connection = NM_CONNECTION(get_active_connection(client, interface));

	return nm_active_connection_get_state_reason(NM_ACTIVE_CONNECTION(connection));
}

/**
 * Delete a connection by id.
 * @param hd: library handle
 * @param id: connection id
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_delete_connection(libnm_wrapper_handle hd, char *id)
{
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	if(TRUE == nm_remote_connection_delete(remote, NULL, NULL))
		ret = LIBNM_WRAPPER_ERR_SUCCESS;

	return ret;
}

static void remote_commit_cb(GObject *remote, GAsyncResult *result, gpointer user_data)
{
	GError *error = NULL;
	libnm_wrapper_cb_st *temp = (libnm_wrapper_cb_st *)user_data;
	GMainLoop *loop = temp->loop;
	int *ret = temp->result;

	nm_remote_connection_commit_changes_finish (NM_REMOTE_CONNECTION(remote), result, &error);
	if (error) {
		*ret = LIBNM_WRAPPER_ERR_FAIL;
		g_error_free (error);
	} else {
		*ret = LIBNM_WRAPPER_ERR_SUCCESS;
	}
	g_free(temp);
	g_main_loop_quit(loop);
}

/**
 * Enable/disable auto-start of a connection.
 * @param hd: library handle
 * @param id: connection id
 * @param autoconnect: true or false
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_set_autoconnect(libnm_wrapper_handle hd, const char *id, bool autoconnect)
{
	NMRemoteConnection *remote = NULL;
	NMSettingConnection *s_con = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	GMainLoop *loop = g_main_loop_new (NULL, FALSE);
	libnm_wrapper_cb_st *temp = g_malloc0(sizeof(libnm_wrapper_cb_st));
	int result;

	temp->loop = loop;
	temp->result = &result;


	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER);

	s_con = nm_connection_get_setting_connection(NM_CONNECTION(remote));
	g_object_set(G_OBJECT(s_con), NM_SETTING_CONNECTION_AUTOCONNECT, autoconnect, NULL);
	nm_remote_connection_commit_changes_async(remote, TRUE, NULL, remote_commit_cb, temp);
	g_main_loop_run (loop);

	return result;
}


/**
 * Get auto-start status of a connection.
 * @param hd: library handle
 * @param id: connection id
 * @param autoconnect: location to store auto-start
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_autoconnect(libnm_wrapper_handle hd, const char *id, bool *autoconnect)
{
	NMRemoteConnection *remote = NULL;
	NMSettingConnection *s_con = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_con = nm_connection_get_setting_connection(NM_CONNECTION(remote));

	*autoconnect = false;
	if(nm_setting_connection_get_autoconnect(s_con))
		*autoconnect = true;

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static void active_connection_state_cb (NMActiveConnection *active,
								NMActiveConnectionState state,
								NMActiveConnectionStateReason reason,
								gpointer user_data);

static void activate_finish(libnm_wrapper_cb_st *temp, int result, bool fromTimer)
{
	GMainLoop *loop = temp->loop;
	int *ret = temp->result;

	*ret = result;

	if(temp->active)
	{
		if(fromTimer)
			g_signal_handlers_disconnect_by_func(temp->active, G_CALLBACK(active_connection_state_cb), temp);

		g_object_unref (temp->active);
	}

	if(!fromTimer && temp->g_timer_id > 0)
		g_source_remove(temp->g_timer_id);

	g_free(temp);
	temp = NULL;

	g_main_loop_quit(loop);
}

static void active_connection_state_cb (NMActiveConnection *active,
								NMActiveConnectionState state,
								NMActiveConnectionStateReason reason,
								gpointer user_data)
{
	libnm_wrapper_cb_st *temp = (libnm_wrapper_cb_st *)user_data;

	switch(state)
	{
		case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
			activate_finish(temp, LIBNM_WRAPPER_ERR_SUCCESS, false);
			break;

		case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
			break;

		default:
			activate_finish(temp, LIBNM_WRAPPER_ERR_FAIL, false);
			break;
	}
	return;
}

static gboolean activate_connection_timeout_cb (gpointer user_data)
{
	libnm_wrapper_cb_st *temp = (libnm_wrapper_cb_st *)user_data;

	if(!temp)
		return FALSE;

	activate_finish(temp, LIBNM_WRAPPER_ERR_FAIL, true);

	return FALSE;
}

static void activated_cb (GObject *client, GAsyncResult *result, gpointer user_data)
{
	int timeout = 20;
	GError *error = NULL;
	libnm_wrapper_cb_st *temp = (libnm_wrapper_cb_st *)user_data;
	GMainLoop *loop = temp->loop;
	int *ret = temp->result;

	*ret = LIBNM_WRAPPER_ERR_FAIL;
	temp->active = nm_client_activate_connection_finish(NM_CLIENT (client), result, &error);
	if (error)
	{
		g_error_free (error);
		g_free(temp);
		temp = NULL;
		g_main_loop_quit(loop);
	}
	else
	{
		activate_finish(temp, LIBNM_WRAPPER_ERR_SUCCESS, false);
	}
}

static void activate_connection(NMClient *client, NMConnection *connection,
					NMDevice *dev, const char *specific_object, int *result)
{
	GMainLoop *loop = g_main_loop_new (NULL, FALSE);
	libnm_wrapper_cb_st *temp = g_malloc0(sizeof(libnm_wrapper_cb_st));

	temp->loop = loop;
	temp->result = result;
	temp->active = NULL;
	temp->g_timer_id = 0;
	nm_client_activate_connection_async(client, connection, dev, specific_object, NULL, activated_cb, temp);
	g_main_loop_run (loop);
}

/**
 * Activate the connection on the interface.
 * @param hd: library handle
 * @param interface: on which interface
 * @param id: connection id
 * @param wifi: whether is a wifi connection
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_activate_connection(libnm_wrapper_handle hd, const char *interface, char *id, bool wifi)
{
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	NMDevice * dev = NULL;
	NMRemoteConnection *remote = NULL;
	const char * specific_object = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	remote = nm_client_get_connection_by_id (client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	dev = nm_client_get_device_by_iface(client, interface);
	if(!dev) {
		return LIBNM_WRAPPER_ERR_NO_HARDWARE;
	}

	activate_connection(client, NM_CONNECTION(remote), dev, NULL, &ret);
	return ret;
}

static void deactivate_connection_cb(GObject *client, GAsyncResult *result, gpointer user_data)
{
	GError *error = NULL;
	libnm_wrapper_cb_st *temp = (libnm_wrapper_cb_st *)user_data;
	GMainLoop *loop = temp->loop;
	int *ret = temp->result;

	nm_client_deactivate_connection_finish(NM_CLIENT(client), result, &error);
	if (error) {
		*ret = LIBNM_WRAPPER_ERR_FAIL;
		g_error_free (error);
	} else {
		*ret = LIBNM_WRAPPER_ERR_SUCCESS;
	}
	g_free(temp);
	g_main_loop_quit(loop);
}

/**
 * Deactivate the connection on the interface.
 * @param hd: library handle
 * @param interface: on which interface
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_deactivate_connection(libnm_wrapper_handle hd, const char *interface)
{
	NMDevice * dev = NULL;
	NMActiveConnection *active = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	GMainLoop *loop;
	libnm_wrapper_cb_st *temp;
	int result;

	dev = nm_client_get_device_by_iface(client, interface);
	active = nm_device_get_active_connection(dev);
	if(!active)
		return LIBNM_WRAPPER_ERR_SUCCESS;

	loop = g_main_loop_new (NULL, FALSE);
	temp = g_malloc0(sizeof(libnm_wrapper_cb_st));
	temp->loop = loop;
	temp->result = &result;

	nm_client_deactivate_connection_async (client, active, NULL, deactivate_connection_cb, temp);
	g_main_loop_run (loop);

	return result;
}

/**@}*/

/**
 * @name Wifi connection management API
 */
/**@{*/
/**
 * Add wifi settings to a connection.
 * @param connection: location to store connection settings
 * @param ws        : wifi settings from user input
 *
 * Returns:
 */
static void add_wireless_settings(NMConnection *connection, NMWrapperWirelessSettings *ws)
{
	GBytes* str = NULL;
	NMSettingWireless * s_wifi = NULL;

	s_wifi = nm_connection_get_setting_wireless(connection);
	if (s_wifi)
		nm_connection_remove_setting (connection, G_OBJECT_TYPE (s_wifi));

	s_wifi = (NMSettingWireless *) nm_setting_wireless_new();
	nm_connection_add_setting (connection, NM_SETTING(s_wifi));

	if (ws->mode[0])
		g_object_set (s_wifi,
				NM_SETTING_WIRELESS_MODE, ws->mode, NULL);

	if (ws->band[0])
		g_object_set (s_wifi, NM_SETTING_WIRELESS_BAND, ws->band, NULL);

	str = g_bytes_new(ws->ssid, strlen(ws->ssid));
	g_object_set (s_wifi,
		NM_SETTING_WIRELESS_SSID, str,
		NM_SETTING_WIRELESS_POWERSAVE, ws->powersave,
		NM_SETTING_WIRELESS_TX_POWER, ws->tx_power,
		NM_SETTING_WIRELESS_HIDDEN, (ws->hidden ? TRUE:FALSE),
		NM_SETTING_WIRELESS_WAKE_ON_WLAN, ws->wow,
		NM_SETTING_WIRELESS_RATE, ws->rate,
		NM_SETTING_WIRELESS_CCX, ws->ccx,
		NM_SETTING_WIRELESS_SCAN_DELAY, ws->scan_delay,
		NM_SETTING_WIRELESS_SCAN_DWELL, ws->scan_dwell,
		NM_SETTING_WIRELESS_SCAN_PASSIVE_DWELL, ws->scan_passive_dwell,
		NM_SETTING_WIRELESS_SCAN_SUSPEND_TIME, ws->scan_suspend_time,
		NM_SETTING_WIRELESS_SCAN_ROAM_DELTA, ws->scan_roam_delta,
		NM_SETTING_WIRELESS_AUTH_TIMEOUT, ws->auth_timeout,
		NM_SETTING_WIRELESS_FREQUENCY_DFS, ws->frequency_dfs,
		NM_SETTING_WIRELESS_MAX_SCAN_INTERVAL, ws->max_scan_interval, NULL);
	g_bytes_unref(str);

	if (ws->bgscan[0])
		g_object_set (s_wifi, NM_SETTING_WIRELESS_BGSCAN, ws->bgscan, NULL);

	if (ws->frequency_list[0])
		g_object_set (s_wifi, NM_SETTING_WIRELESS_FREQUENCY_LIST, ws->frequency_list, NULL);

	if (ws->client_name[0])
		g_object_set (s_wifi, NM_SETTING_WIRELESS_CLIENT_NAME, ws->client_name, NULL);
}

/**
 * Get wifi settings of a connection.
 * @param connection: location to store connection settings
 * @param ws        : location to store wifi settings
 *
 * Returns:
 */
static void get_wireless_settings(NMConnection *connection, NMWrapperWirelessSettings *ws)
{
	GBytes *tmp = NULL;
	const char *ptr = NULL;
	NMSettingWireless *s_wifi = NULL;

	s_wifi = nm_connection_get_setting_wireless(connection);

	ws->powersave = nm_setting_wireless_get_powersave(s_wifi);
	ws->tx_power = nm_setting_wireless_get_tx_power(s_wifi);
	ws->hidden = nm_setting_wireless_get_hidden(s_wifi);
	ws->wow = nm_setting_wireless_get_wake_on_wlan(s_wifi);
	ws->rate = nm_setting_wireless_get_rate(s_wifi);
	ws->ccx = nm_setting_wireless_get_ccx(s_wifi);
	ws->scan_delay = nm_setting_wireless_get_scan_delay(s_wifi);
	ws->scan_dwell = nm_setting_wireless_get_scan_dwell(s_wifi);
	ws->scan_passive_dwell = nm_setting_wireless_get_scan_passive_dwell(s_wifi);
	ws->scan_suspend_time = nm_setting_wireless_get_scan_suspend_time(s_wifi);
	ws->scan_roam_delta = nm_setting_wireless_get_scan_roam_delta(s_wifi);
	ws->auth_timeout = nm_setting_wireless_get_auth_timeout(s_wifi);
	ws->frequency_dfs = nm_setting_wireless_get_frequency_dfs(s_wifi);
	ws->max_scan_interval = nm_setting_wireless_get_max_scan_interval(s_wifi);

	tmp = nm_setting_wireless_get_ssid(s_wifi);
	ssid_gbytes_to_string(tmp, ws->ssid, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_wireless_get_mode(s_wifi);
	safe_strncpy(ws->mode, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_wireless_get_band(s_wifi);
	safe_strncpy(ws->band, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_wireless_get_bgscan(s_wifi);
	safe_strncpy(ws->bgscan, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_wireless_get_frequency_list(s_wifi);
	safe_strncpy(ws->frequency_list, ptr, LIBNM_WRAPPER_MAX_FREQUENCY_LIST_LEN);

	ptr = nm_setting_wireless_get_client_name(s_wifi);
	safe_strncpy(ws->client_name, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
}

/**
 * Get wifi settings of a connection.
 * @param hd: library handle
 * @param interface: on which interface
 * @param id: if set, get the settings from the connection of the id, otherwise get the settings of the active connection.
 * @param ws: location to store wifi settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_wireless_settings(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperWirelessSettings *ws)
{
	NMConnection *connection = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	if(id)
		connection = NM_CONNECTION(nm_client_get_connection_by_id(client, id));
	else
		connection = NM_CONNECTION(get_active_connection(client, interface));

	nm_wrapper_assert(connection, LIBNM_WRAPPER_ERR_INVALID_CONFIG)

	get_wireless_settings(connection, ws);
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static int get_wireless_security_settings_keymgmt_none(NMSettingWirelessSecurity * s_wsec, NMWrapperWirelessSecuritySettings *wss)
{
	int i = 0;
	const char *ptr = NULL;

	wss->wep_tx_keyidx = nm_setting_wireless_security_get_wep_tx_keyidx(s_wsec);
	for(i = 0; i < 4; i++)
	{
		ptr = nm_setting_wireless_security_get_wep_key(s_wsec, i);
		safe_strncpy(wss->wepkey[i], ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
	}

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static int set_wireless_security_settings_keymgmt_none(NMSettingWirelessSecurity * s_wsec, NMWrapperWirelessSecuritySettings *wss)
{
	int i;

	g_object_set (s_wsec,
		NM_SETTING_WIRELESS_SECURITY_WEP_TX_KEYIDX, wss->wep_tx_keyidx,
		NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, NM_WEP_KEY_TYPE_KEY, NULL);


	for (i = 0; i < 4; i++)
	{
		if (wss->wepkey[i][0])
		{
			nm_setting_wireless_security_set_wep_key(s_wsec, i, wss->wepkey[i]);
		}
	}
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static int get_wireless_security_settings_keymgmt_psk(NMSettingWirelessSecurity * s_wsec, NMWrapperWirelessSecuritySettings *wss)
{
	const char *ptr = NULL;

	ptr = nm_setting_wireless_security_get_psk(s_wsec);
	safe_strncpy(wss->psk, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static int set_wireless_security_settings_keymgmt_psk(NMSettingWirelessSecurity * s_wsec, NMWrapperWirelessSecuritySettings *wss)
{
	if (wss->psk[0])
		g_object_set(s_wsec, NM_SETTING_WIRELESS_SECURITY_PSK, wss->psk, NULL);

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static int get_wireless_security_settings_keymgmt_ieee8021x(NMSettingWirelessSecurity * s_wsec, NMWrapperWirelessSecuritySettings *wss)
{
	const char *ptr = NULL;

	ptr = nm_setting_wireless_security_get_leap_username(s_wsec);
	safe_strncpy(wss->leap_username, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_wireless_security_get_leap_password(s_wsec);
	safe_strncpy(wss->leap_password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static int set_wireless_security_settings_keymgmt_ieee8021x(NMSettingWirelessSecurity * s_wsec, NMWrapperWirelessSecuritySettings *wss)
{
	if (wss->leap_username[0])
		g_object_set (s_wsec, NM_SETTING_WIRELESS_SECURITY_LEAP_USERNAME, wss->leap_username, NULL);

	if (wss->leap_password[0])
		g_object_set (s_wsec, NM_SETTING_WIRELESS_SECURITY_LEAP_PASSWORD, wss->leap_password, NULL);

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static int get_wireless_security_settings_keymgmt_eap(NMConnection *connection, NMWrapperWireless8021xSettings *wxs)
{
	int i, nums;
	const char *ptr = NULL;
	GBytes *bptr = NULL;
	NMSetting8021x *s_8021x = NULL;

	s_8021x = nm_connection_get_setting_802_1x(connection);
	if (!s_8021x)
		return LIBNM_WRAPPER_ERR_FAIL;

	wxs->auth_timeout = nm_setting_802_1x_get_auth_timeout(s_8021x);
	wxs->system_ca_certs = nm_setting_802_1x_get_system_ca_certs(s_8021x);
	wxs->ca_cert_scheme = nm_setting_802_1x_get_ca_cert_scheme(s_8021x);
	wxs->cli_cert_scheme = nm_setting_802_1x_get_client_cert_scheme(s_8021x);
	wxs->private_key_scheme = nm_setting_802_1x_get_private_key_scheme(s_8021x);
	wxs->private_key_format = nm_setting_802_1x_get_private_key_format(s_8021x);
	wxs->p1_auth_flags = nm_setting_802_1x_get_phase1_auth_flags(s_8021x);
	wxs->p2_ca_cert_scheme = nm_setting_802_1x_get_phase2_ca_cert_scheme (s_8021x);
	wxs->p2_cli_cert_scheme = nm_setting_802_1x_get_phase2_client_cert_scheme (s_8021x);
	wxs->p2_private_key_scheme = nm_setting_802_1x_get_phase2_private_key_scheme(s_8021x);
	wxs->p2_private_key_format = nm_setting_802_1x_get_phase2_private_key_format(s_8021x);

	if(wxs->ca_cert_scheme == NM_SETTING_802_1X_CK_SCHEME_PATH)
	{
		ptr = nm_setting_802_1x_get_ca_cert_path(s_8021x);
		safe_strncpy(wxs->ca_cert, ptr, LIBNM_WRAPPER_MAX_PATH_LEN);
	}
	else
	{
		safe_strncpy(wxs->ca_cert, NULL, LIBNM_WRAPPER_MAX_PATH_LEN);
	}

	ptr = nm_setting_802_1x_get_ca_cert_password(s_8021x);
	safe_strncpy(wxs->ca_cert_password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_ca_path(s_8021x);
	safe_strncpy(wxs->ca_path, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	if(wxs->cli_cert_scheme == NM_SETTING_802_1X_CK_SCHEME_PATH)
	{
		ptr = nm_setting_802_1x_get_client_cert_path(s_8021x);
		safe_strncpy(wxs->cli_cert, ptr, LIBNM_WRAPPER_MAX_PATH_LEN);
	}
	else
	{
		safe_strncpy(wxs->cli_cert, NULL, LIBNM_WRAPPER_MAX_PATH_LEN);
	}

	ptr = nm_setting_802_1x_get_client_cert_password(s_8021x);
	safe_strncpy(wxs->cli_cert_password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	nums = nm_setting_802_1x_get_num_eap_methods(s_8021x);
	for(int i=0; i<nums; i++)
	{
		ptr = nm_setting_802_1x_get_eap_method(s_8021x, i);
		strncat(wxs->eap, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
		if(i != (nums-1))
			strncat(wxs->eap, " ", LIBNM_WRAPPER_MAX_NAME_LEN);
	}

	ptr = nm_setting_802_1x_get_identity(s_8021x);
	safe_strncpy(wxs->identity, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_pac_file(s_8021x);
	safe_strncpy(wxs->pac_file, ptr, LIBNM_WRAPPER_MAX_PATH_LEN);

	ptr = nm_setting_802_1x_get_pac_file_password(s_8021x);
	safe_strncpy(wxs->pac_file_password, ptr, LIBNM_WRAPPER_MAX_PATH_LEN);

	ptr = nm_setting_802_1x_get_anonymous_identity(s_8021x);
	safe_strncpy(wxs->anonymous, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_password(s_8021x);
	safe_strncpy(wxs->password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_phase1_peapver(s_8021x);
	safe_strncpy(wxs->p1_peapver, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_phase1_peaplabel(s_8021x);
	safe_strncpy(wxs->p1_peaplabel, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_phase1_peapver(s_8021x);
	safe_strncpy(wxs->p1_peapver, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_phase1_fast_provisioning(s_8021x);
	safe_strncpy(wxs->p1_fast_provisioning, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	nums = nm_setting_802_1x_get_num_phase2_auths(s_8021x);
	for(i=0; i<nums; i++)
	{
		ptr = nm_setting_802_1x_get_phase2_auth(s_8021x, i);
		strncat(wxs->p2_auth, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
		if(i != (nums-1))
			strncat(wxs->p2_auth, " ", LIBNM_WRAPPER_MAX_NAME_LEN);
	}

	nums = nm_setting_802_1x_get_num_phase2_autheaps(s_8021x);
	for(i=0; i<nums; i++)
	{
		ptr = nm_setting_802_1x_get_phase2_autheap(s_8021x, i);
		strncat(wxs->p2_autheap, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
		if(i != (nums-1))
			strncat(wxs->p2_autheap, " ", LIBNM_WRAPPER_MAX_NAME_LEN);
	}

	if(wxs->p2_ca_cert_scheme == NM_SETTING_802_1X_CK_SCHEME_PATH)
	{
		ptr = nm_setting_802_1x_get_phase2_ca_cert_path(s_8021x);
		safe_strncpy(wxs->p2_ca_cert, ptr, LIBNM_WRAPPER_MAX_PATH_LEN);
	}
	else
	{
		safe_strncpy(wxs->p2_ca_cert, NULL, LIBNM_WRAPPER_MAX_PATH_LEN);
	}

	ptr = nm_setting_802_1x_get_phase2_ca_cert_password (s_8021x);
	safe_strncpy(wxs->p2_ca_cert_password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_phase2_ca_path(s_8021x);
	safe_strncpy(wxs->p2_ca_path, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	if(wxs->p2_cli_cert_scheme == NM_SETTING_802_1X_CK_SCHEME_PATH)
	{
		ptr = nm_setting_802_1x_get_phase2_client_cert_path(s_8021x);
		safe_strncpy(wxs->p2_cli_cert, ptr, LIBNM_WRAPPER_MAX_PATH_LEN);
	}
	else
	{
		safe_strncpy(wxs->p2_cli_cert, NULL, LIBNM_WRAPPER_MAX_PATH_LEN);
	}

	ptr = nm_setting_802_1x_get_phase2_client_cert_password(s_8021x);
	safe_strncpy(wxs->p2_cli_cert_password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	if(wxs->p2_private_key_scheme == NM_SETTING_802_1X_CK_SCHEME_PATH)
	{
		ptr = nm_setting_802_1x_get_phase2_private_key_path(s_8021x);
		safe_strncpy(wxs->p2_private_key, ptr, LIBNM_WRAPPER_MAX_PATH_LEN);
	}
	else
	{
		safe_strncpy(wxs->p2_private_key, NULL, LIBNM_WRAPPER_MAX_PATH_LEN);
	}

	ptr = nm_setting_802_1x_get_phase2_private_key_password(s_8021x);
	safe_strncpy(wxs->p2_private_key_password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	if(wxs->private_key_scheme == NM_SETTING_802_1X_CK_SCHEME_PATH)
	{
		ptr = nm_setting_802_1x_get_private_key_path(s_8021x);
		safe_strncpy(wxs->private_key, ptr, LIBNM_WRAPPER_MAX_PATH_LEN);
	}
	else
	{
		safe_strncpy(wxs->private_key, NULL, LIBNM_WRAPPER_MAX_PATH_LEN);
	}

	ptr = nm_setting_802_1x_get_private_key_password(s_8021x);
	safe_strncpy(wxs->private_key_password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_802_1x_get_pin(s_8021x);
	safe_strncpy(wxs->pin, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

/**
 * Get wifi security settings of a connection.
 * @param hd: library handle
 * @param interface: on which interface
 * @param id: if set, get the settings from the connection of the id, otherwise get the settings of the active connection.
 * @param wss: location to store wifi security settings
 * @param wxs: location to store 8021x settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_wireless_security_settings(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperWirelessSecuritySettings *wss, NMWrapperWireless8021xSettings *wxs)
{
	int nums, i;
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	const char *ptr = NULL;
	NMConnection *conn = NULL;
	NMSettingWirelessSecurity * s_wsec = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	if (id)
		conn = NM_CONNECTION(nm_client_get_connection_by_id(client, id));
	else
		conn = NM_CONNECTION(get_active_connection(client, interface));

	nm_wrapper_assert(conn, LIBNM_WRAPPER_ERR_INVALID_CONFIG)

	s_wsec = nm_connection_get_setting_wireless_security(conn);
	if (!s_wsec)
		return LIBNM_WRAPPER_ERR_INVALID_WEP_TYPE;

	nums = nm_setting_wireless_security_get_num_groups(s_wsec);
	for (i = 0; i < nums; i++)
	{
		ptr = nm_setting_wireless_security_get_group(s_wsec, i);
		strncat(wss->group, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
		if(i != (nums-1))
			strncat(wss->group, " ", LIBNM_WRAPPER_MAX_NAME_LEN);
	}

	nums = nm_setting_wireless_security_get_num_protos(s_wsec);
	for (i = 0; i < nums; i++)
	{
		ptr = nm_setting_wireless_security_get_proto(s_wsec, i);
		strncat(wss->proto, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
		if(i != (nums-1))
			strncat(wss->proto, " ", LIBNM_WRAPPER_MAX_NAME_LEN);
	}

	nums = nm_setting_wireless_security_get_num_pairwise(s_wsec);
	for (i = 0; i < nums; i++)
	{
		ptr = nm_setting_wireless_security_get_pairwise(s_wsec, i);
		strncat(wss->pairwise, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
		if(i != (nums-1))
			strncat(wss->pairwise, " ", LIBNM_WRAPPER_MAX_NAME_LEN);
	}

	ptr = nm_setting_wireless_security_get_auth_alg(s_wsec);
	safe_strncpy(wss->auth_alg, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
	ptr = nm_setting_wireless_security_get_proactive_key_caching(s_wsec);
	safe_strncpy(wss->proactive_key_caching, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_wireless_security_get_key_mgmt(s_wsec);
	safe_strncpy(wss->key_mgmt, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
	if (ptr)
	{
		ret = get_wireless_security_settings_keymgmt_none(s_wsec, wss);
		if(ret == LIBNM_WRAPPER_ERR_SUCCESS)
			ret = get_wireless_security_settings_keymgmt_psk(s_wsec, wss);
		if(ret == LIBNM_WRAPPER_ERR_SUCCESS)
			ret = get_wireless_security_settings_keymgmt_ieee8021x(s_wsec, wss);
		if(ret == LIBNM_WRAPPER_ERR_SUCCESS)
			ret = get_wireless_security_settings_keymgmt_eap(conn, wxs);
	}

	return ret;
}

static void cert_to_utf8_path(int scheme, char *cert, char *outbuf, int len)
{
	snprintf(outbuf, len, "%s", cert);
	if (scheme == NM_SETTING_802_1X_CK_SCHEME_PATH)
	{
		gsize bytes_read, bytes_written;
		gchar *file = NULL;

		outbuf[0] = '\0';
		file = g_filename_to_utf8(cert, -1, &bytes_read, &bytes_written, NULL);

		if(file)
			snprintf(outbuf, len, "%s", file);
		g_free(file);
	}
}

static gchar* string_to_utf8(const char *src)
{
	gsize bytes_read, bytes_written;
	gchar *file = g_filename_to_utf8(src, -1, &bytes_read, &bytes_written, NULL);
	return file;
}

static int set_wireless_security_settings_keymgmt_eap(NMConnection *connection, NMWrapperWireless8021xSettings *wxs)
{
	int i, nums;
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	char buf[LIBNM_WRAPPER_MAX_PATH_LEN];
	NMSetting8021x *s_8021x = nm_connection_get_setting_802_1x(connection);
	GError *err = NULL;

	if (!s_8021x)
	{
		s_8021x = (NMSetting8021x *) nm_setting_802_1x_new();
		nm_connection_add_setting(connection, NM_SETTING(s_8021x));
	}

	g_object_set (s_8021x,
		NM_SETTING_802_1X_AUTH_TIMEOUT, wxs->auth_timeout,
		NM_SETTING_802_1X_PHASE1_AUTH_FLAGS, wxs->p1_auth_flags,
		NM_SETTING_802_1X_SYSTEM_CA_CERTS, wxs->system_ca_certs, NULL);

	if (wxs->ca_cert[0])
	{

		cert_to_utf8_path(wxs->ca_cert_scheme, wxs->ca_cert, buf, LIBNM_WRAPPER_MAX_PATH_LEN);

		if (FALSE == nm_setting_802_1x_set_ca_cert(s_8021x, buf, wxs->ca_cert_scheme, NULL, &err))
		{
			if (err)
			{
				g_error_free (err);
				return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
			}
			return ret;
		}
	}

	if (wxs->cli_cert[0])
	{
		cert_to_utf8_path(wxs->cli_cert_scheme, wxs->cli_cert, buf, LIBNM_WRAPPER_MAX_PATH_LEN);
		if (FALSE == nm_setting_802_1x_set_client_cert(s_8021x, buf, wxs->cli_cert_scheme, NULL, &err))
		{
			if(err)
			{
				g_error_free (err);
				return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
			}
			return ret;
		}
	}

	if (wxs->p2_ca_cert[0])
	{
		cert_to_utf8_path(wxs->p2_ca_cert_scheme, wxs->p2_ca_cert, buf, LIBNM_WRAPPER_MAX_PATH_LEN);
		if(FALSE == nm_setting_802_1x_set_phase2_ca_cert(s_8021x, buf, wxs->p2_ca_cert_scheme, NULL, NULL))
			return ret;
	}

	if (wxs->p2_cli_cert[0])
	{
		cert_to_utf8_path(wxs->p2_cli_cert_scheme, wxs->p2_cli_cert, buf, LIBNM_WRAPPER_MAX_PATH_LEN);
		if (FALSE == nm_setting_802_1x_set_phase2_client_cert(s_8021x, buf, wxs->p2_cli_cert_scheme, NULL, NULL))
			return ret;
	}

	if (wxs->private_key[0])
	{
		cert_to_utf8_path(wxs->private_key_scheme, wxs->private_key, buf, LIBNM_WRAPPER_MAX_PATH_LEN);
		if (FALSE == nm_setting_802_1x_set_private_key(s_8021x, buf, wxs->private_key_password, wxs->private_key_scheme, NULL, &err)) {
			if(err) {
				g_error_free (err);
				return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
			}
			return ret;
		}
	}

	if (wxs->p2_private_key[0])
	{
		cert_to_utf8_path(wxs->p2_private_key_scheme, wxs->p2_private_key, buf, LIBNM_WRAPPER_MAX_PATH_LEN);
		if(FALSE == nm_setting_802_1x_set_phase2_private_key(s_8021x, buf, wxs->p2_private_key_password, wxs->p2_private_key_scheme, NULL, NULL))
			return ret;
	}

	if (wxs->ca_cert_password[0])
	{
		g_object_set (s_8021x, NM_SETTING_802_1X_CA_CERT_PASSWORD, wxs->ca_cert_password, NULL);
	}

	if (wxs->ca_path[0])
	{
		gchar *file = string_to_utf8(wxs->ca_path);
		if (!file) return ret;
		g_object_set(s_8021x, NM_SETTING_802_1X_CA_PATH, file, NULL);
		g_free(file);
	}

	if (wxs->p2_ca_path[0])
	{
		gchar *file = string_to_utf8(wxs->p2_ca_path);
		if (!file) return ret;
		g_object_set(s_8021x, NM_SETTING_802_1X_PHASE2_CA_PATH, file, NULL);
		g_free(file);
	}

	if (wxs->pac_file[0])
	{
		gchar *file = string_to_utf8(wxs->pac_file);
		if (!file) return ret;
		g_object_set(s_8021x, NM_SETTING_802_1X_PAC_FILE, file, NULL);
		g_free(file);
	}

	if (wxs->pac_file_password[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_PAC_FILE_PASSWORD, wxs->pac_file_password, NULL);

	if (wxs->ca_path[0])
	{
		g_object_set(s_8021x, NM_SETTING_802_1X_CA_PATH, wxs->ca_path, NULL);
	}

	if (wxs->p2_ca_path[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_PHASE2_CA_PATH, wxs->p2_ca_path, NULL);

	if (wxs->cli_cert_password[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_PHASE2_CLIENT_CERT_PASSWORD, wxs->cli_cert_password, NULL);

	nums = nm_setting_802_1x_get_num_eap_methods(s_8021x);
	nm_setting_802_1x_clear_eap_methods(s_8021x);
	if (wxs->eap[0])
	{
		char *methods = wxs->eap;
		char *tokens = strtok(methods, " ");
		while (tokens != NULL )
		{
			nm_setting_802_1x_add_eap_method(s_8021x, tokens);
			tokens = strtok(NULL, " ");
		}
	}

	if (wxs->identity[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_IDENTITY, wxs->identity, NULL);

	if (wxs->anonymous[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_ANONYMOUS_IDENTITY, wxs->anonymous, NULL);

	if (wxs->password[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_PASSWORD, wxs->password, NULL);

	if (wxs->p1_peapver[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_PHASE1_PEAPVER, wxs->p1_peapver, NULL);

	if(wxs->p1_fast_provisioning[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_PHASE1_FAST_PROVISIONING, wxs->p1_fast_provisioning, NULL);

	if (wxs->p1_peaplabel[0])
		g_object_set(s_8021x, NM_SETTING_802_1X_PHASE1_PEAPLABEL, wxs->p1_peaplabel, NULL);

	nums = nm_setting_802_1x_get_num_phase2_auths(s_8021x);
	for (i=0; i<nums; i++)
	{
		nm_setting_802_1x_remove_phase2_auth(s_8021x, i);
	}

	if (wxs->p2_auth[0])
	{
		char *methods = wxs->p2_auth;
		char *tokens = strtok(methods," ");
		while( tokens != NULL )
		{
			nm_setting_802_1x_add_phase2_auth(s_8021x, tokens);
			tokens = strtok(NULL," ");
		}
	}

	nums = nm_setting_802_1x_get_num_phase2_autheaps(s_8021x);
	for (i=0; i<nums; i++)
	{
		nm_setting_802_1x_remove_phase2_autheap(s_8021x, i);
	}

	if (wxs->p2_autheap[0])
	{
		char *methods = wxs->p2_autheap;
		char *tokens = strtok(methods, " ");
		while (tokens != NULL)
		{
			nm_setting_802_1x_add_phase2_autheap(s_8021x, tokens);
			tokens = strtok(NULL, " ");
		}
	}

	if (wxs->p2_ca_cert_password[0])
	{
		g_object_set(s_8021x, NM_SETTING_802_1X_PHASE2_CA_CERT_PASSWORD, wxs->p2_ca_cert_password, NULL);
	}

	if (wxs->p2_cli_cert_password[0])
	{
		g_object_set(s_8021x, NM_SETTING_802_1X_PHASE2_CLIENT_CERT_PASSWORD, wxs->p2_cli_cert_password, NULL);
	}

	if (wxs->pin[0])
	{
		g_object_set(s_8021x, NM_SETTING_802_1X_PIN, wxs->pin, NULL);
	}

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

static int add_wireless_security_settings(NMConnection *connection, NMWrapperWirelessSecuritySettings *wss, NMWrapperWireless8021xSettings *wxs)
{
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	NMSettingWirelessSecurity * s_wsec = NULL;

	s_wsec = nm_connection_get_setting_wireless_security(connection);
	if (s_wsec)
	{
		NMSetting8021x *s_8021x = nm_connection_get_setting_802_1x(connection);
		if(s_8021x)
			nm_connection_remove_setting(connection, G_OBJECT_TYPE(s_8021x));
		nm_connection_remove_setting(connection, G_OBJECT_TYPE(s_wsec));
	}

	s_wsec = (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
	nm_connection_add_setting(connection, NM_SETTING (s_wsec));

	if (wss->group[0])
	{
		GStrv v = g_strsplit(wss->group, " ", -1);
		nm_setting_wireless_security_clear_groups(s_wsec);
		g_object_set (s_wsec, NM_SETTING_WIRELESS_SECURITY_GROUP, v, NULL);
		g_strfreev(v);
	}

	if (wss->proto[0])
	{
		GStrv v = g_strsplit(wss->proto, " ", -1);
		nm_setting_wireless_security_clear_protos(s_wsec);
		g_object_set (s_wsec, NM_SETTING_WIRELESS_SECURITY_PROTO, v, NULL);
		g_strfreev(v);
	}

	if (wss->pairwise[0])
	{
		GStrv v = g_strsplit(wss->pairwise, " ", -1);
		nm_setting_wireless_security_clear_pairwise(s_wsec);
		g_object_set (s_wsec, NM_SETTING_WIRELESS_SECURITY_PAIRWISE, v, NULL);
		g_strfreev(v);
	}

	g_object_set (s_wsec,
			NM_SETTING_WIRELESS_SECURITY_AUTH_ALG, wss->auth_alg,
			NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, wss->key_mgmt, NULL);

	if (wss->proactive_key_caching[0])
		g_object_set (s_wsec, NM_SETTING_WIRELESS_SECURITY_PROACTIVE_KEY_CACHING, wss->proactive_key_caching, NULL);

	ret = set_wireless_security_settings_keymgmt_none(s_wsec, wss);
	if(ret == LIBNM_WRAPPER_ERR_SUCCESS)
		ret = set_wireless_security_settings_keymgmt_psk(s_wsec, wss);
	if(ret == LIBNM_WRAPPER_ERR_SUCCESS)
		ret = set_wireless_security_settings_keymgmt_ieee8021x(s_wsec, wss);
	if(ret == LIBNM_WRAPPER_ERR_SUCCESS && wxs->eap[0])
		ret = set_wireless_security_settings_keymgmt_eap(connection, wxs);

	return ret;
}

/**
 * Create a wifi connection profile.
 * @param hd: library handle
 * @param s: location to store general settings
 * @param ws: location to store wifi settings
 * @param wss: location to store wifi security settings
 * @param wxs: location to store 8021x settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_add_wireless_connection(libnm_wrapper_handle hd, NMWrapperSettings *s, NMWrapperWirelessSettings* ws, NMWrapperWirelessSecuritySettings *wss, NMWrapperWireless8021xSettings *wxs)
{
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	GError *err = NULL;
	NMConnection *connection = NULL;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id (client, s->id);
	if (remote)
		return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;

	connection = nm_simple_connection_new();

	add_settings(connection, s);

	add_wireless_settings(connection, ws);

	if (wss->key_mgmt[0])
	{
		if(LIBNM_WRAPPER_ERR_SUCCESS != add_wireless_security_settings(connection, wss, wxs))
			return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;
	}

	nm_connection_normalize(connection, NULL, NULL, &err);
	if (err)
	{
		g_error_free (err);
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
	}

	add_connection(client, connection, &ret);
	return ret;
}

/**
 * Update a wifi connection profile.
 * @param hd: library handle
 * @param id: id of the connection to be updated
 * @param s: location to store general settings
 * @param ws: location to store wifi settings
 * @param wss: location to store wifi security settings
 * @param wxs: location to store 8021x settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_update_wireless_connection(libnm_wrapper_handle hd, const char *id, NMWrapperSettings *s, NMWrapperWirelessSettings* ws, NMWrapperWirelessSecuritySettings *wss, NMWrapperWireless8021xSettings *wxs)
{
	GError *err = NULL;
	NMConnection *connection = NULL;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	GMainLoop *loop;
	libnm_wrapper_cb_st *temp;
	int result;

	remote = nm_client_get_connection_by_id (client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)
	connection = NM_CONNECTION(remote);

	update_settings(connection, s);

	add_wireless_settings(connection, ws);

	if (wss->key_mgmt[0])
	{
		if(LIBNM_WRAPPER_ERR_SUCCESS != add_wireless_security_settings(connection, wss, wxs))
			return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;
	}

	nm_connection_normalize(NM_CONNECTION(remote), NULL, NULL, &err);
	if (err)
	{
		g_error_free (err);
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
	}

	loop = g_main_loop_new (NULL, FALSE);
	temp = g_malloc0(sizeof(libnm_wrapper_cb_st));
	temp->loop = loop;
	temp->result = &result;

	nm_remote_connection_commit_changes_async(remote, TRUE, NULL, remote_commit_cb, temp);
	g_main_loop_run(loop);

	return result;
}

/**@}*/

/**
 * @name Wired connection management API
 */
/**@{*/
/**
 * Create a wired connection profile.
 * @param hd: library handle
 * @param s: location to store general settings
 * @param ws: location to store wired settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
static void add_wired_settings(NMConnection *connection, NMWrapperWiredSettings *ws)
{
	NMSettingWired *s_wired = nm_connection_get_setting_wired(connection);
	if(s_wired)
		nm_connection_remove_setting (connection, G_OBJECT_TYPE (s_wired));

	s_wired = (NMSettingWired *) nm_setting_wireless_new();
	nm_connection_add_setting (connection, NM_SETTING(s_wired));

	g_object_set (s_wired,
		NM_SETTING_WIRED_SPEED, ws->speed,
		NM_SETTING_WIRED_AUTO_NEGOTIATE, ws->auto_negotiate,
		NM_SETTING_WIRED_WAKE_ON_LAN, ws->wol, NULL);

	if (ws->duplex[0])
		g_object_set (s_wired, NM_SETTING_WIRED_DUPLEX, ws->duplex, NULL);

	if (ws->wol_password[0])
		g_object_set (s_wired, NM_SETTING_WIRED_WAKE_ON_LAN_PASSWORD, ws->wol_password, NULL);
}

int libnm_wrapper_connection_add_wired_connection(libnm_wrapper_handle hd, NMWrapperSettings *s, NMWrapperWiredSettings *ws)
{
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	GError *err = NULL;
	NMConnection *connection = NULL;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id (client, s->id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	connection = nm_simple_connection_new();
	add_settings(connection, s);
	add_wired_settings(connection, ws);

	nm_connection_normalize(connection, NULL, NULL, &err);
	if(err)
	{
		g_error_free (err);
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
	}

	add_connection(client, connection, &ret);
	return ret;
}

/**
 * Update a wired connection profile.
 * @param hd: library handle
 * @param id: connection id
 * @param s: location to store general settings
 * @param ws: location to store wired settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_update_wired_connection(libnm_wrapper_handle hd, const char *id, NMWrapperSettings *s, NMWrapperWiredSettings* ws)
{
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	GError *err = NULL;
	NMConnection *connection = NULL;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id (client, s->id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	connection = nm_simple_connection_new();

	update_settings(connection, s);
	add_wired_settings(connection, ws);

	nm_connection_normalize(connection, NULL, NULL, &err);
	if(err)
	{
		g_error_free (err);
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
	}

	nm_remote_connection_commit_changes(remote, TRUE, NULL, &err);
	if(err)
	{
		g_error_free (err);
		return LIBNM_WRAPPER_ERR_FAIL;
	}

	return ret;
}

static void get_wired_settings(NMConnection *connection, NMWrapperWiredSettings *ws)
{
	const char *ptr;
	NMSettingWired *s_wired = nm_connection_get_setting_wired(connection);

	ws->speed = (int) nm_setting_wired_get_speed(s_wired);
	ws->auto_negotiate = nm_setting_wired_get_auto_negotiate(s_wired);
	ws->wol = (int)nm_setting_wired_get_wake_on_lan(s_wired);

	ptr = nm_setting_wired_get_wake_on_lan_password(s_wired);
	safe_strncpy(ws->wol_password, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);

	ptr = nm_setting_wired_get_duplex(s_wired);
	safe_strncpy(ws->duplex, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
}

/**
 * Update a wired connection profile.
 * @param hd: library handle
 * @param interface: on which interface
 * @param id: connection id
 * @param ws: location to store wired settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_wired_settings(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperWiredSettings *ws)
{
	NMConnection *connection = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	if(id)
		connection = NM_CONNECTION(nm_client_get_connection_by_id(client, id));
	else
		connection = NM_CONNECTION(get_active_connection(client, interface));

	nm_wrapper_assert(connection, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	get_wired_settings(connection, ws);
	return LIBNM_WRAPPER_ERR_SUCCESS;
}
/**@}*/


/**
 * @name AP Management API
 */
/**@{*/
void static get_access_point_settings(NMAccessPoint *ap, NMWrapperAccessPoint *dst)
{
	GBytes *tmp = NULL;
	const char *bssid = NULL;

	tmp = nm_access_point_get_ssid(ap);
	ssid_gbytes_to_string(tmp, dst->ssid, LIBNM_WRAPPER_MAX_NAME_LEN);

	bssid = nm_access_point_get_bssid(ap);
	if(bssid)
		nm_utils_hwaddr_aton(bssid, dst->bssid, LIBNM_WRAPPER_MAX_MAC_ADDR_LEN);

	dst->mode = nm_access_point_get_mode(ap);
	dst->frequency = nm_access_point_get_frequency(ap);
	dst->strength = nm_access_point_get_strength(ap);
	dst->wpa_flags = nm_access_point_get_wpa_flags(ap);
	dst->rsn_flags = nm_access_point_get_rsn_flags(ap);
	dst->flags = nm_access_point_get_flags(ap);
}

/**
 * Get AP list.
 * @param hd: library handle
 * @param interface: on which interface
 * @param list: list of AP settings
 * @param size: size of list
 *
 * Returns: actual number of processed AP
 */
int libnm_wrapper_access_point_get_scanlist(libnm_wrapper_handle hd, const char *interface, NMWrapperAccessPoint *list, int size)
{
	int i;
	NMDevice * dev = NULL;
	const GPtrArray *aps = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	dev = nm_client_get_device_by_iface(client, interface);
	aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(dev));
	for (i = 0; i < MIN(aps->len, size); i++)
	{
		NMAccessPoint *ap = g_ptr_array_index(aps, i);
		get_access_point_settings(ap, &list[i]);
	}

	return i;
}

/**
 * Get active AP settings.
 * @param hd: library handle
 * @param ap: location to store active AP settings
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_access_point_get_active_settings(libnm_wrapper_handle hd, const char *interface, NMWrapperAccessPoint *ap)
{
	NMDevice *dev = NULL;
	NMAccessPoint *activeAp = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	dev = nm_client_get_device_by_iface(client, interface);
	if(!dev)
		return LIBNM_WRAPPER_ERR_NO_HARDWARE;

	activeAp = nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(dev));
	if(!activeAp)
		return LIBNM_WRAPPER_ERR_INVALID_NAME;

	get_access_point_settings(activeAp, ap);
	return LIBNM_WRAPPER_ERR_SUCCESS;
}
/**@}*/

/**
 * @name IP Management API
 */
/**@{*/

#define GET_ATTR(name, dst, variant_type, type, dflt) \
			G_STMT_START { \
				GVariant *_variant = nm_ip_route_get_attribute (rt, ""name""); \
				if (_variant && g_variant_is_of_type (_variant, G_VARIANT_TYPE_ ## variant_type)) \
					(dst) = g_variant_get_ ## type (_variant); \
				else \
					(dst) = (dflt); \
			} G_STMT_END

int libnm_wrapper_ipv4_get_route_information(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperIPRoute *route, int size)
{
	NMIPConfig* cfg;
	GPtrArray *ptr_array;
	NMDevice *dev = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	dev = nm_client_get_device_by_iface(client, interface);
	if(!dev) return -1;

	cfg = nm_device_get_ip4_config(dev);
	if(!cfg) return -1;

	ptr_array = nm_ip_config_get_routes(cfg);
	if (!ptr_array || (ptr_array->len == 0))
		return -1;

	size = size < ptr_array->len ? size : ptr_array->len;
	for (int i = 0; i < size; i++)
	{
		NMIPRoute *rt = g_ptr_array_index(ptr_array, i);
		const char *ptr = nm_ip_route_get_dest(rt);
		safe_strncpy(route[i].dest, ptr, LIBNM_WRAPPER_MAX_NAME_LEN);
		route[i].prefix = nm_ip_route_get_prefix(rt);
		route[i].metric = nm_ip_route_get_metric(rt);
		GET_ATTR(NM_IP_ROUTE_ATTRIBUTE_MTU,            route[i].mtu,            UINT32,   uint32, 0);
		GET_ATTR(NM_IP_ROUTE_ATTRIBUTE_WINDOW,         route[i].window,         UINT32,   uint32, 0);
	}

	return size;
}

int libnm_wrapper_get_active_ipv4_addresses(libnm_wrapper_handle hd, const char *interface, char *ip, int ip_len, char *gateway, int gateway_len, char *subnet, int subnet_len, char *dns_1, int dns1_len, char *dns_2, int dns2_len)
{
	NMDevice *dev = NULL;
	NMIPConfig *ip4;
	NMActiveConnection *active = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	dev = nm_client_get_device_by_iface(client, interface);
	active = nm_device_get_active_connection(dev);

	if(!active)
		return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;

	ip4 = nm_active_connection_get_ip4_config (active);
	if(!ip4)
		return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;

	const GPtrArray *addresses;
	addresses = nm_ip_config_get_addresses(ip4);
	if (!addresses)
		return LIBNM_WRAPPER_ERR_FAIL;
	NMIPAddress * nm_ip = addresses->pdata[0];
	if (addresses->pdata[0] && ip != NULL){
		const char * addr = nm_ip_address_get_address(nm_ip);
		safe_strncpy(ip,addr,ip_len);
		int prefix = nm_ip_address_get_prefix (nm_ip);
		if (subnet != NULL && prefix > 0 ) {
			unsigned long mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
			snprintf(subnet, subnet_len, "%lu.%lu.%lu.%lu\n", mask >> 24, (mask >> 16) & 0xFF, (mask >> 8) & 0xFF, mask & 0xFF);
		}
	}

	const char * active_gateway = nm_ip_config_get_gateway(ip4);
	if (!active_gateway)
		return LIBNM_WRAPPER_ERR_FAIL;

	if (active_gateway[0] && gateway != NULL)
		safe_strncpy(gateway,active_gateway,gateway_len);

	const char *const* active_dns_addresses = nm_ip_config_get_nameservers(ip4);
	if (!active_dns_addresses)
		return LIBNM_WRAPPER_ERR_FAIL;

	if (active_dns_addresses[0] && dns_1 != NULL)
		safe_strncpy(dns_1,active_dns_addresses[0],dns1_len);

	if (active_dns_addresses[1] && dns_2 != NULL )
		safe_strncpy(dns_2,active_dns_addresses[1],dns2_len);

	return LIBNM_WRAPPER_ERR_SUCCESS;
}


int libnm_wrapper_ipv4_get_dhcp_information(libnm_wrapper_handle hd, const char *interface, const int size, const char *options[], const int len, char val[size][len])
{
	NMDevice *dev = NULL;
	NMDhcpConfig *dhcp4;
	NMActiveConnection *active = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	dev = nm_client_get_device_by_iface(client, interface);
	active = nm_device_get_active_connection(dev);
	if(!active)
		return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;

	dhcp4 = nm_active_connection_get_dhcp4_config(active);
	if(!dhcp4)
		return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;

	for(int i=0; i<size; i++)
	{
		const char *ptr = nm_dhcp_config_get_one_option(dhcp4, options[i]);
		if(ptr)
			safe_strncpy(val[i], ptr, len);
	}
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

int libnm_wrapper_ipv4_set_method(libnm_wrapper_handle hd, const char *id, const char *value)
{
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip4;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	//"manual" will be set when ip address is set;
	if(!strncmp(value, NM_SETTING_IP4_CONFIG_METHOD_MANUAL, strlen(NM_SETTING_IP4_CONFIG_METHOD_MANUAL)))
		return LIBNM_WRAPPER_ERR_SUCCESS;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config (NM_CONNECTION(remote));
	if(s_ip4)
	{
		g_object_set (G_OBJECT(NM_SETTING(s_ip4)), NM_SETTING_IP_CONFIG_METHOD, value, NULL);
		nm_setting_ip_config_clear_addresses(s_ip4);
		nm_setting_ip_config_clear_dns(s_ip4);
		nm_setting_ip_config_clear_routes(s_ip4);
		g_object_set (G_OBJECT(NM_SETTING(s_ip4)), NM_SETTING_IP_CONFIG_GATEWAY, NULL, NULL);

		if(TRUE == nm_remote_connection_commit_changes(remote, TRUE, NULL, NULL))
			return LIBNM_WRAPPER_ERR_SUCCESS;
	}

	return LIBNM_WRAPPER_ERR_FAIL;
}

int libnm_wrapper_ipv4_get_method(libnm_wrapper_handle hd , const char *id, char *method, int len)
{
	const char *ptr;
	int ret = LIBNM_WRAPPER_ERR_INVALID_CONFIG;
	NMSettingIPConfig *s_ip4;
	NMRemoteConnection *remote;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config(NM_CONNECTION(remote));
	if(!s_ip4)
		return ret;

	ptr = nm_setting_ip_config_get_method(s_ip4);
	if(ptr)
	{
		safe_strncpy(method, ptr, len);
		method[len - 1] = '\0';
		ret = LIBNM_WRAPPER_ERR_SUCCESS;
	}
	return ret;
}

int libnm_wrapper_ipv6_set_method(libnm_wrapper_handle hd , const char *id, const char * value){
	GError *err = NULL;
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip6;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	//"manual" will be set when ip address is set;
	if(!strncmp(value, NM_SETTING_IP6_CONFIG_METHOD_MANUAL, strlen(NM_SETTING_IP6_CONFIG_METHOD_MANUAL)))
		return LIBNM_WRAPPER_ERR_SUCCESS;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip6 = nm_connection_get_setting_ip6_config (NM_CONNECTION(remote));
	if(s_ip6)
	{
		g_object_set (s_ip6, NM_SETTING_IP_CONFIG_METHOD, value, NULL);
		nm_setting_ip_config_clear_addresses(s_ip6);
		nm_setting_ip_config_clear_dns(s_ip6);
		nm_setting_ip_config_clear_routes(s_ip6);
		g_object_set (G_OBJECT(NM_SETTING(s_ip6)), NM_SETTING_IP_CONFIG_GATEWAY, NULL, NULL);
		if(TRUE == nm_remote_connection_commit_changes(remote, TRUE, NULL, &err))
			return LIBNM_WRAPPER_ERR_SUCCESS;
		if(err)
		{
			g_error_free (err);
		}
	}
	return LIBNM_WRAPPER_ERR_FAIL;
}

int libnm_wrapper_ipv6_get_method(libnm_wrapper_handle hd , const char *id, char *method, int len)
{
	int ret = LIBNM_WRAPPER_ERR_INVALID_CONFIG;
	const char *ptr;
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip6;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip6 = nm_connection_get_setting_ip6_config(NM_CONNECTION(remote));
	if(!s_ip6)
		return ret;

	ptr = nm_setting_ip_config_get_method(s_ip6);
	if(ptr)
	{
		safe_strncpy(method, ptr, len);
		method[len - 1] = '\0';
		ret = LIBNM_WRAPPER_ERR_SUCCESS;
	}

	return ret;
}

int libnm_wrapper_ipv4_set_address(libnm_wrapper_handle hd, const char *id, const int index, const char *address, const char *netmask, const char *gateway)
{
	GError *err = NULL;
	NMIPAddress *addr;
	NMRemoteConnection *remote = NULL;
	NMSettingIPConfig *s_ip4 = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client , id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config(NM_CONNECTION(remote));
	if(!s_ip4)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	//Force to "manual" method to set ip address
	g_object_set (G_OBJECT(NM_SETTING(s_ip4)), NM_SETTING_IP_CONFIG_METHOD, "manual", NULL);

	if(nm_setting_ip_config_get_num_addresses(s_ip4) == 0)
	{
		addr = nm_ip_address_new(AF_INET, "192.168.1.1", 24, NULL);
		nm_setting_ip_config_add_address(s_ip4, addr);
		nm_ip_address_unref(addr);
	}

	if(index >= nm_setting_ip_config_get_num_addresses(s_ip4))
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	addr = nm_setting_ip_config_get_address(s_ip4, index);

	if (address && address[0])
		nm_ip_address_set_address(addr, address);

	if (netmask && netmask[0])
	{
		int prefix = atoi(netmask);
		if(prefix > 0 && prefix < 32)
			nm_ip_address_set_prefix(addr, atoi(netmask));
	}

	if (gateway && gateway[0])
		g_object_set (G_OBJECT(NM_SETTING(s_ip4)), NM_SETTING_IP_CONFIG_GATEWAY, gateway, NULL);

	if(FALSE == nm_connection_verify(NM_CONNECTION(remote), &err))
	{
		g_error_free (err);
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
	}

	if(FALSE == nm_remote_connection_commit_changes(remote, TRUE, NULL, NULL))
			return LIBNM_WRAPPER_ERR_FAIL;

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

int libnm_wrapper_ipv4_set_all_addresses(libnm_wrapper_handle hd, const char *id, const int index, const char *address, const char *netmask, const char *gateway, const char* dns)
{
	GError *err = NULL;
	NMIPAddress *addr;
	NMRemoteConnection *remote = NULL;
	NMSettingIPConfig *s_ip4 = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	GMainLoop *loop;
	libnm_wrapper_cb_st *temp;
	int result;

	remote = nm_client_get_connection_by_id(client , id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config(NM_CONNECTION(remote));
	if(!s_ip4)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	//Force to "manual" method to set ip address
	g_object_set (G_OBJECT(NM_SETTING(s_ip4)), NM_SETTING_IP_CONFIG_METHOD, "manual", NULL);

	if(nm_setting_ip_config_get_num_addresses(s_ip4) == 0)
	{
		addr = nm_ip_address_new(AF_INET, "192.168.1.1", 24, NULL);
		nm_setting_ip_config_add_address(s_ip4, addr);
		nm_ip_address_unref(addr);
	}

	if(index >= nm_setting_ip_config_get_num_addresses(s_ip4))
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	addr = nm_setting_ip_config_get_address(s_ip4, index);

	if(address && strlen(address))
		nm_ip_address_set_address(addr, address);

	if(netmask && strlen(netmask))
	{ 
		int prefix = atoi(netmask);
		if(prefix > 0 && prefix < 32)
			nm_ip_address_set_prefix(addr, atoi(netmask));
	}

	if(gateway && strlen(gateway))
		g_object_set (G_OBJECT(NM_SETTING(s_ip4)), NM_SETTING_IP_CONFIG_GATEWAY, gateway, NULL);

	nm_setting_ip_config_clear_dns(s_ip4);
	if(dns){
		char * tokens;
		tokens = strtok(dns," ");
		while( tokens != NULL )
		{
			if(nm_utils_ipaddr_valid(AF_INET, tokens) == FALSE)
				return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;
			if(FALSE == nm_setting_ip_config_add_dns(s_ip4, tokens))
				return LIBNM_WRAPPER_ERR_FAIL;
			tokens = strtok(NULL," ");
		}
	}

	if(FALSE == nm_connection_verify(NM_CONNECTION(remote), &err))
	{
		g_error_free (err);
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;
	}

	loop = g_main_loop_new (NULL, FALSE);
	temp = g_malloc0(sizeof(libnm_wrapper_cb_st));
	temp->loop = loop;
	temp->result = &result;

	nm_remote_connection_commit_changes_async(remote, TRUE, NULL, remote_commit_cb, temp);
	g_main_loop_run(loop);

	return result;
}


int libnm_wrapper_ipv4_get_address_num(libnm_wrapper_handle hd, const char *id, int *num)
{
	NMSettingIPConfig *s_ip4;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client , id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config(NM_CONNECTION(remote));
	if(!s_ip4)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	*num = nm_setting_ip_config_get_num_addresses(s_ip4);
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

int libnm_wrapper_ipv4_get_address(libnm_wrapper_handle hd, const char *id, const int index, char *address, int address_len, char *netmask, int netmask_len, char *gateway, int gateway_len)
{
	NMSettingIPConfig *s_ip4;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client , id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config(NM_CONNECTION(remote));
	if(!s_ip4)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	if(nm_setting_ip_config_get_num_addresses(s_ip4) > index){
		NMIPAddress *a = nm_setting_ip_config_get_address(s_ip4, index);
		const char *ptr = nm_setting_ip_config_get_gateway(s_ip4);
		int prefix = nm_ip_address_get_prefix(a);
		if(address)
			safe_strncpy(address, nm_ip_address_get_address(a), address_len);
		if(netmask)
			snprintf(netmask, netmask_len, "%d", prefix);
		if(gateway && ptr)
			safe_strncpy(gateway, ptr, gateway_len);
		return LIBNM_WRAPPER_ERR_SUCCESS;
	}

	return LIBNM_WRAPPER_ERR_FAIL;
}

int libnm_wrapper_ipv6_get_address_num(libnm_wrapper_handle hd, const char *id, int *num)
{
	NMSettingIPConfig *s_ip6;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client , id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip6 = nm_connection_get_setting_ip6_config(NM_CONNECTION(remote));
	if(!s_ip6)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	*num = nm_setting_ip_config_get_num_addresses(s_ip6);
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

int libnm_wrapper_ipv6_get_address(libnm_wrapper_handle hd, const char *id, const int index, char *address, int address_len, char *netmask, int netmask_len, char *gateway, int gateway_len)
{
	NMSettingIPConfig *s_ip6;
	NMRemoteConnection *remote = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client , id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip6 = nm_connection_get_setting_ip6_config(NM_CONNECTION(remote));
	if(!s_ip6)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	if(nm_setting_ip_config_get_num_addresses(s_ip6) > index){
		NMIPAddress *a = nm_setting_ip_config_get_address(s_ip6, index);
		const char *ptr = nm_setting_ip_config_get_gateway(s_ip6);
		int prefix = nm_ip_address_get_prefix (a);
		if(address)
			safe_strncpy(address, nm_ip_address_get_address(a), address_len);
		if(netmask)
			snprintf(netmask, netmask_len, "%d", prefix);
		if(gateway && ptr)
			safe_strncpy(gateway, ptr, gateway_len);
		return LIBNM_WRAPPER_ERR_SUCCESS;
	}

	return LIBNM_WRAPPER_ERR_FAIL;
}

int libnm_wrapper_ipv6_set_address(libnm_wrapper_handle hd, const char *id, const int index, const char *address, const char *netmask, const char *gateway)
{
	NMIPAddress *addr;
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip6;
	int ret = LIBNM_WRAPPER_ERR_SUCCESS;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip6 = nm_connection_get_setting_ip6_config(NM_CONNECTION(remote));
	if (!s_ip6)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	//Force to "manual" method to set ip address
	g_object_set (G_OBJECT(NM_SETTING(s_ip6)), NM_SETTING_IP_CONFIG_METHOD, "manual", NULL);

	if (nm_setting_ip_config_get_num_addresses(s_ip6) == 0)
	{
		addr = nm_ip_address_new(AF_INET6, "::", 128, NULL);
		nm_setting_ip_config_add_address(s_ip6, addr);
		nm_ip_address_unref(addr);
	}

	if (index >= nm_setting_ip_config_get_num_addresses(s_ip6))
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	addr = nm_setting_ip_config_get_address(s_ip6, index);

	if (address && address[0])
		nm_ip_address_set_address(addr, address);

	if (netmask && netmask[0])
	{
		int prefix = atoi(netmask);
		if(prefix > 0)
			nm_ip_address_set_prefix(addr, prefix);
	}

	if (gateway && gateway[0])
		g_object_set(G_OBJECT(NM_SETTING(s_ip6)), NM_SETTING_IP_CONFIG_GATEWAY, gateway, NULL);

	if (FALSE == nm_remote_connection_commit_changes(remote, TRUE, NULL, NULL))
			return LIBNM_WRAPPER_ERR_FAIL;

	return ret;
}

int libnm_wrapper_ipv4_set_dns(libnm_wrapper_handle hd, const char *id, char *address)
{
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip4;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config(NM_CONNECTION(remote));
	if(!s_ip4)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	nm_setting_ip_config_clear_dns(s_ip4);
	if(address){
		char * tokens;
		tokens = strtok(address," ");
		while( tokens != NULL )
		{
			if(nm_utils_ipaddr_valid(AF_INET, tokens) == FALSE)
				return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;
			if(FALSE == nm_setting_ip_config_add_dns(s_ip4, tokens))
				return LIBNM_WRAPPER_ERR_FAIL;
			tokens = strtok(NULL," ");
		}
	}

	if(FALSE == nm_remote_connection_commit_changes(remote, TRUE, NULL, NULL))
			return LIBNM_WRAPPER_ERR_FAIL;

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

int libnm_wrapper_ipv4_get_dns(libnm_wrapper_handle hd, const char *id, char *address, int buff_len)
{
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip4;
	int num_dns, i, len = 0;
	const char *dns = NULL;
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config (NM_CONNECTION(remote));
	if(!s_ip4)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	num_dns = nm_setting_ip_config_get_num_dns(s_ip4);
	for(i=0; i<num_dns; i++){
		dns = nm_setting_ip_config_get_dns (s_ip4, i);
		if(strlen(dns) + len + 1 > buff_len){
			ret = LIBNM_WRAPPER_ERR_INSUFFICIENT_MEMORY;
			break;
		}
		safe_strncpy(&address[len], dns, strlen(dns));
		len += strlen(dns);
		address[len] = ',';
		++len;
	}

	address[len] = '\0';

	if(len > 0 && address[len-1] == ',')
		address[len-1] = '\0';

	if(i > 0 && ret != LIBNM_WRAPPER_ERR_INSUFFICIENT_MEMORY)
		ret = LIBNM_WRAPPER_ERR_SUCCESS;

	return ret;
}

int libnm_wrapper_ipv6_set_dns(libnm_wrapper_handle hd, const char *id, char *address)
{
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip6;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip6 = nm_connection_get_setting_ip6_config (NM_CONNECTION(remote));
	if(!s_ip6)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	nm_setting_ip_config_clear_dns(s_ip6);
	if(address){
		char * tokens;
		tokens = strtok(address," ");
		while( tokens != NULL)
		{
			if(nm_utils_ipaddr_valid(AF_INET6, tokens) == FALSE)
				return LIBNM_WRAPPER_ERR_INVALID_PARAMETER;
			if(FALSE == nm_setting_ip_config_add_dns(s_ip6, tokens))
				return LIBNM_WRAPPER_ERR_FAIL;
			tokens = strtok(NULL," ");
		}
	}

	if(FALSE == nm_remote_connection_commit_changes(remote, TRUE, NULL, NULL))
			return LIBNM_WRAPPER_ERR_FAIL;
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

int libnm_wrapper_ipv6_get_dns(libnm_wrapper_handle hd, const char *id, char *address, int buff_len)
{
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip6;
	int num_dns, i, len = 0;
	const char *dns = NULL;
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip6 = nm_connection_get_setting_ip6_config (NM_CONNECTION(remote));
	if(!s_ip6)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	num_dns = nm_setting_ip_config_get_num_dns(s_ip6);
	for(i=0; i<num_dns; i++){
		dns = nm_setting_ip_config_get_dns (s_ip6, i);
		if(strlen(dns) + len + 1 > buff_len){
			ret = LIBNM_WRAPPER_ERR_INSUFFICIENT_MEMORY;
			break;
		}
		safe_strncpy(&address[len], dns, strlen(dns));
		len += strlen(dns);
		address[len] = ',';
		++len;
	}
	address[len] = '\0';
	if(len > 0 && address[len-1] == ',')
		address[len-1] = '\0';
	if(i > 0 && ret != LIBNM_WRAPPER_ERR_INSUFFICIENT_MEMORY)
		ret = LIBNM_WRAPPER_ERR_SUCCESS;

	return ret;
}

int libnm_wrapper_ipv4_clear_address(libnm_wrapper_handle hd , const char *id)
{
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip4;
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip4 = nm_connection_get_setting_ip4_config (NM_CONNECTION(remote));
	if(!s_ip4)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	nm_setting_ip_config_clear_addresses(s_ip4);
	ret = LIBNM_WRAPPER_ERR_SUCCESS;
	return ret;
}

int libnm_wrapper_ipv6_clear_address(libnm_wrapper_handle hd , const char *id)
{
	NMRemoteConnection *remote;
	NMSettingIPConfig *s_ip6;
	int ret = LIBNM_WRAPPER_ERR_FAIL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	remote = nm_client_get_connection_by_id(client, id);
	nm_wrapper_assert(remote, LIBNM_WRAPPER_ERR_INVALID_PARAMETER)

	s_ip6 = nm_connection_get_setting_ip6_config(NM_CONNECTION(remote));
	if(!s_ip6)
		return LIBNM_WRAPPER_ERR_INVALID_CONFIG;

	nm_setting_ip_config_clear_addresses(s_ip6);
	ret = LIBNM_WRAPPER_ERR_SUCCESS;
	return ret;
}

int libnm_wrapper_ipv4_disable_nat(libnm_wrapper_handle hd , const char *id)
{
	return libnm_wrapper_ipv4_set_method(hd, id, NM_SETTING_IP4_CONFIG_METHOD_DISABLED);
}

int libnm_wrapper_ipv4_enable_nat(libnm_wrapper_handle hd , const char *id)
{
	return libnm_wrapper_ipv4_set_method(hd, id, NM_SETTING_IP4_CONFIG_METHOD_SHARED);
}

int libnm_wrapper_ipv4_get_nat(libnm_wrapper_handle hd , const char *id, int *nat)
{
	char buffer[16];
	int ret = libnm_wrapper_ipv4_get_method(hd, id, buffer, 16);
	*nat = 0;
	if(!ret && !strncmp(buffer,  NM_SETTING_IP4_CONFIG_METHOD_SHARED, sizeof(NM_SETTING_IP4_CONFIG_METHOD_SHARED)))
		*nat = 1;
	return ret;
}

/**
 * @name Deprecated IP API
 */
int libnm_wrapper_ipv4_get_broadcast_address(libnm_wrapper_handle hd , const char *id, char *address, int len)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv4_set_broadcast_address(libnm_wrapper_handle hd , const char *id, char *address)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv4_set_bridgeports(libnm_wrapper_handle hd , const char *id, int ports)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv4_get_bridgeports(libnm_wrapper_handle hd , const char *id, int *ports)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv4_disable_hostapd(libnm_wrapper_handle hd , const char *id)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv4_enable_hostapd(libnm_wrapper_handle hd , const char *id)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv4_get_hostapd(libnm_wrapper_handle hd , const char *id, int *mode)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv6_set_nat(libnm_wrapper_handle hd , const char *id, int nat)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv6_get_nat(libnm_wrapper_handle hd , const char *id, int *nat)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv6_set_dhcp(libnm_wrapper_handle hd , const char *id, char *dhcp)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv6_get_dhcp(libnm_wrapper_handle hd , const char *id, char *dhcp, int len)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv6_disable_interface(libnm_wrapper_handle hd , const char *interface)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv6_enable_interface(libnm_wrapper_handle hd , const char *interface)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv6_disable_nat(libnm_wrapper_handle hd , const char *id)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}

int libnm_wrapper_ipv6_enable_nat(libnm_wrapper_handle hd , const char *id)
{
	return LIBNM_WRAPPER_ERR_NOT_IMPLEMENTED;
}
/**@}*/

/**
 * @name Misc API
 */
/**@{*/

static const char * logLevelStr[] = {"OFF", "ERR", "WARN", "INFO", "DEBUG", "TRACE", "KEEP"};

/**
 * Set log level.
 * @param hd: library handle
 * @param level: log level
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful.
 */
int libnm_wrapper_set_log_level(libnm_wrapper_handle hd, int level)
{
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	if(level >= 0 && level <= 6)
		if(TRUE == nm_client_set_logging(client, logLevelStr[level], NULL, NULL))
			return LIBNM_WRAPPER_ERR_SUCCESS;

	return LIBNM_WRAPPER_ERR_FAIL;
}

/**
 * Get log level.
 * @param hd: library handle
 * @param level: location to store log level
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful.
 */
int libnm_wrapper_get_log_level(libnm_wrapper_handle hd, int *level)
{
	char *str = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	if(FALSE == nm_client_get_logging(client, &str, NULL, NULL))
		return LIBNM_WRAPPER_ERR_FAIL;

	for(int i = 0; i < 7; i++){
		if(!strncmp(str, logLevelStr[i], strlen(logLevelStr[i]))){
			*level = i;
			return LIBNM_WRAPPER_ERR_SUCCESS;
		}
	}

	return LIBNM_WRAPPER_ERR_FAIL;
}

/**
 * Get nm version.
 * @param hd: library handle
 * @param version: location to store version
 * @param len: version buffer size
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful.
 */
int libnm_wrapper_get_version(libnm_wrapper_handle hd, char *version, int len)
{
	const char *ptr = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	ptr = nm_client_get_version(client);
	safe_strncpy(version, ptr, len);
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

/**
 * Wifi frequency to channel
 * @param frequency:
 *
 * Returns: the channel represented by the frequency or 0.
 */
unsigned int libnm_wrapper_utils_wifi_freq_to_channel(unsigned int frequency)
{
	return nm_utils_wifi_freq_to_channel(frequency);
}

/**
 * Wifi channel to frequency
 * @param channel: wifi channel
 * @param band: frequency band for wireless ("a" or "bg")
 *
 * Returns: the frequency represented by the channel of the band, or -1 when the freq is invalid,
 *          or 0 when the band is invalid
 */
unsigned int libnm_wrapper_utils_wifi_channel_to_freq(unsigned int channel, const char *band)
{
	return nm_utils_wifi_channel_to_freq(channel, band);
}
/**@}*/
