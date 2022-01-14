#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include "libnm_wrapper_internal.h"

/**
 * @name Device Management API
 */
/**@{*/
/**
 * Get device status.
 * @param hd: library handle
 * @param interface: which device
 * @param status: location to store status
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_device_get_status(libnm_wrapper_handle hd, const char *interface, NMWrapperDevice* status)
{
	const char *ptr = NULL;
	GPtrArray *ptr_array = NULL;
	NMIPConfig *s_ip = NULL;
	int num_ips;

	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;

	NMDevice *dev = nm_client_get_device_by_iface(client, interface);
	if(!dev)
		return LIBNM_WRAPPER_ERR_NO_HARDWARE;

	status->autoconnect = false;
	if( nm_device_get_autoconnect(dev))
		status->autoconnect = true;

	status->state = nm_device_get_state(dev);

	ptr = nm_device_get_hw_address(dev);
	if(ptr)
		nm_utils_hwaddr_aton(ptr, status->mac, LIBNM_WRAPPER_MAX_MAC_ADDR_LEN);

	s_ip = nm_device_get_ip4_config(dev);
	if(s_ip)
	{
		num_ips = 0;
		ptr_array = nm_ip_config_get_addresses(s_ip);
		if(ptr_array)
			num_ips = MIN(ptr_array->len, LIBNM_WRAPPER_MAX_ADDR_NUM);

		for(int i=0; i < num_ips; i++)
		{
			NMIPAddress *a = g_ptr_array_index (ptr_array, i);
			safe_strncpy(status->addr[i], nm_ip_address_get_address(a), LIBNM_WRAPPER_MAX_NAME_LEN);
		}
	}

	s_ip = nm_device_get_ip6_config(dev);
	if(s_ip)
	{
		num_ips = 0;
		ptr_array = nm_ip_config_get_addresses(s_ip);
		if(ptr_array)
			num_ips = MIN(ptr_array->len, LIBNM_WRAPPER_MAX_ADDR_NUM);
		for(int i=0; i < num_ips; i++)
		{
			NMIPAddress *a = g_ptr_array_index (ptr_array, i);
			safe_strncpy(status->addr6[i], nm_ip_address_get_address(a), LIBNM_WRAPPER_MAX_NAME_LEN);
		}
	}

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

/**
 * Enable/disable device auto-start.
 * @param hd: library handle
 * @param interface: which device
 * @param autoconnect: whether to enable auto-start
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_device_set_autoconnect(libnm_wrapper_handle hd, const char *interface, bool autoconnect)
{
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	NMDevice *dev = nm_client_get_device_by_iface(client, interface);
	if(!dev)
		return LIBNM_WRAPPER_ERR_NO_HARDWARE;

	nm_device_set_autoconnect(dev, autoconnect);

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

/**
 * Get device auto-start flag.
 * @param hd: library handle
 * @param interface: which device
 * @param autoconnect: location to store auto-start
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_device_get_autoconnect(libnm_wrapper_handle hd, const char *interface, bool *autoconnect)
{
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	NMDevice *dev = nm_client_get_device_by_iface(client, interface);
	if(!dev)
		return LIBNM_WRAPPER_ERR_NO_HARDWARE;

	*autoconnect = false;
	if( nm_device_get_autoconnect(dev))
		*autoconnect = true;

	return LIBNM_WRAPPER_ERR_SUCCESS;
}

/**
 * Disconnects the device if currently connected, and prevents the device from automatically connecting to networks until the next manual network connection request..
 * @param hd: library handle
 * @param interface: which device
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_device_disconnect(libnm_wrapper_handle hd, const char *interface)
{
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	NMDevice * dev = nm_client_get_device_by_iface(client, interface);
	if(TRUE == nm_device_disconnect(dev, NULL, NULL))
		return LIBNM_WRAPPER_ERR_SUCCESS;
	return LIBNM_WRAPPER_ERR_FAIL;
}

/**
 * Enables or disables wireless devices.
 * @param hd: library handle
 *
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_device_enable_wireless(libnm_wrapper_handle hd , bool enable)
{
	int state;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	int count = 0;
	bool processed;

	nm_client_wireless_set_enabled(client, enable);
	// Ensure state change is reflected in nm_client_wireless_get_enabled() before returning
	do {
		processed = g_main_context_iteration(NULL, FALSE);
		state = nm_client_wireless_get_enabled(client);
		if (state != enable && !processed) {
			// Yield if there were no events ready to be processed and state has not yet changed
			usleep(50*1000);
		}
	} while ((state != enable) && (count++ < 200));

	if (state != enable) {
		printf("%s: WARNING! wireless state remains %d!\n", __func__, state);
		return LIBNM_WRAPPER_ERR_FAIL;
	}
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

/**
 * Determines whether wireless devices are enabled
 * @param hd: library handle
 *
 * Returns: non-zero if wireless devices are enabled
 */
int libnm_wrapper_device_is_wireless_enabled(libnm_wrapper_handle hd)
{
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	return nm_client_wireless_get_enabled(client);
}

/**
 * Get number of connections on interface.
 * @param hd: library handle
 * @param interface: which interface
 *
 * Returns: number of connections
 */
int libnm_wrapper_device_get_connection_num(libnm_wrapper_handle hd, char *interface)
{
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	NMDevice * dev = nm_client_get_device_by_iface(client, interface);
	const GPtrArray *connections = nm_device_get_available_connections(dev);
	return connections->len;
}

/**
 * Get device state.
 * @param hd: library handle
 * @param interface: on which device
 *
 * Returns: NMDeviceState
 */
int libnm_wrapper_device_get_state(libnm_wrapper_handle hd, const char *interface)
{
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	NMDevice * dev = nm_client_get_device_by_iface(client, interface);
	return nm_device_get_state(dev);
}

/**
 * Get device state reason.
 * @param hd: library handle
 * @param interface: on which device
 *
 * Returns: NMDeviceStateReason
 */
int libnm_wrapper_device_get_state_reason(libnm_wrapper_handle hd, const char *interface)
{
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	NMDevice * dev = nm_client_get_device_by_iface(client, interface);
	return nm_device_get_state_reason(dev);
}

/**
 * Monitor device state .
 * @param hd: library handle
 * @param interface: on which device
 * @param user: user callback function
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */

static void device_state (NMDevice *dev, GParamSpec *pspec,
						LIBNM_WRAPPER_STATE_MONITOR_CALLBACK_ST *user)
{
	int state = nm_device_get_state (dev);
	int reason = nm_device_get_state_reason(dev);
	int noExit = user->callback(state, reason);
	GMainLoop *loop = (GMainLoop *)user->arg;
	if(!noExit) g_main_loop_quit(loop);
}

int libnm_wrapper_device_state_monitor(libnm_wrapper_handle hd, const char *interface,
									LIBNM_WRAPPER_STATE_MONITOR_CALLBACK_ST *user)
{
	GMainLoop *loop;
	GError *error = NULL;
	NMClient *client = ((libnm_wrapper_handle_st *)hd)->client;
	NMDevice *dev = nm_client_get_device_by_iface(client, interface);
	g_signal_connect (dev, "notify::" NM_DEVICE_STATE, G_CALLBACK (device_state), user);
	loop = g_main_loop_new (NULL, FALSE);
	user->arg = (void *)loop;
	g_main_loop_run (loop);
	return LIBNM_WRAPPER_ERR_SUCCESS;
}

/**@}*/
