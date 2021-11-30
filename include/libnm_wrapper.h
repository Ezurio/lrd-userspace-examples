#ifndef __LIBNM_WRAPPER_H__
#define __LIBNM_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <arpa/inet.h>
#include <libnm/NetworkManager.h>
#include "libnm_wrapper_type.h"


#define LIBNM_DEFAULT_ANONYMOUSE_IDENTITY "summit"

typedef void * libnm_wrapper_handle;

typedef struct _NMWrapperDevice {
	int	autoconnect;
	int 	state;
	char	addr[LIBNM_WRAPPER_MAX_ADDR_NUM][LIBNM_WRAPPER_MAX_NAME_LEN];
	char	addr6[LIBNM_WRAPPER_MAX_ADDR_NUM][LIBNM_WRAPPER_MAX_NAME_LEN];
	char	mac[LIBNM_WRAPPER_MAX_MAC_ADDR_LEN];
} NMWrapperDevice;

typedef struct _NMWrapperAccessPoint {
	unsigned int mode;
	unsigned int frequency;
	unsigned int strength;
	unsigned int flags;
	unsigned int wpa_flags;
	unsigned int rsn_flags;
	char    ssid[LIBNM_WRAPPER_MAX_NAME_LEN];
	char    bssid[LIBNM_WRAPPER_MAX_MAC_ADDR_LEN];
} NMWrapperAccessPoint;

typedef struct _NMWrapperSettings {
	int		autoconnect;
	char	type[LIBNM_WRAPPER_MAX_NAME_LEN];
	char    id[LIBNM_WRAPPER_MAX_NAME_LEN];
	char    uuid[LIBNM_WRAPPER_MAX_UUID_LEN];
	char    interface[LIBNM_WRAPPER_MAX_NAME_LEN];
}NMWrapperSettings;

static inline void NMWrapperSettings_init(NMWrapperSettings *s)
{
	s->autoconnect = 0;
	s->type[0] = '\0';
	s->id[0] = '\0';
	s->uuid[0] = '\0';
	s->interface[0] = '\0';
}

typedef struct _NMWrapperWiredSettings {
	int speed;
	int wol;
    int auto_negotiate;
	char duplex[LIBNM_WRAPPER_MAX_NAME_LEN];
	char wol_password[LIBNM_WRAPPER_MAX_NAME_LEN];
}NMWrapperWiredSettings;

static inline void NMWrapperWiredSettings_init(NMWrapperWiredSettings *ws)
{
	ws->speed = ws->wol = ws->auto_negotiate = 0;
	ws->duplex[0] = ws->wol_password[0] = '\0';
}

typedef struct _NMWrapperWirelessSettings {
	int 	hidden;
	int		rate;
	int		tx_power;
	int		powersave;
	int 	channel;
	int 	wow;
	int     ccx;
	int 	scan_delay;
	int		scan_dwell;
	int		scan_passive_dwell;
	int 	scan_suspend_time;
	int 	scan_roam_delta;
	int		auth_timeout;
	int		frequency_dfs;
	int		max_scan_interval;
	char 	mode[LIBNM_WRAPPER_MAX_NAME_LEN];
	char 	frequency_list[LIBNM_WRAPPER_MAX_FREQUENCY_LIST_LEN];
	char 	bgscan[LIBNM_WRAPPER_MAX_NAME_LEN];
	char	ssid[LIBNM_WRAPPER_MAX_NAME_LEN];
	char	client_name[LIBNM_WRAPPER_MAX_NAME_LEN];
	char 	band[LIBNM_WRAPPER_MAX_NAME_LEN];
}NMWrapperWirelessSettings;

static inline void NMWrapperWirelessSettings_init(NMWrapperWirelessSettings *ws)
{
	ws->hidden = ws->rate = ws->tx_power = 0;
	ws->powersave = ws->channel = ws->wow = ws->ccx = 0;
	ws->scan_delay = ws->scan_dwell = ws->scan_passive_dwell = 0;
	ws->scan_suspend_time = ws->scan_roam_delta = ws->auth_timeout = 0;
	ws->frequency_dfs = ws->max_scan_interval = 0;
	ws->ssid[0] = ws->band[0] = ws->client_name[0] = '\0';
	ws->mode[0] = ws->bgscan[0] = ws->frequency_list[0] = '\0';
}

typedef struct _NMWrapperWirelessSecuritySettings{
	int pmf;
	int wep_key_type;
	int secret_flags;
	uint32_t wep_tx_keyidx;
	char auth_alg[LIBNM_WRAPPER_MAX_NAME_LEN];
	char key_mgmt[LIBNM_WRAPPER_MAX_NAME_LEN];
	char group[LIBNM_WRAPPER_MAX_NAME_LEN];
	char pairwise[LIBNM_WRAPPER_MAX_NAME_LEN];
	char proto[LIBNM_WRAPPER_MAX_NAME_LEN];
	char leap_username[LIBNM_WRAPPER_MAX_NAME_LEN];
	char leap_password[LIBNM_WRAPPER_MAX_NAME_LEN];
	char wepkey[4][LIBNM_WRAPPER_MAX_NAME_LEN];
	char psk[LIBNM_WRAPPER_MAX_NAME_LEN];
	char proactive_key_caching[LIBNM_WRAPPER_MAX_NAME_LEN];
}NMWrapperWirelessSecuritySettings;

static inline void NMWrapperWirelessSecuritySettings_init(NMWrapperWirelessSecuritySettings *wss)
{
	wss->wep_tx_keyidx = wss->pmf = wss->wep_key_type = wss->secret_flags = 0;
	wss->auth_alg[0] = wss->key_mgmt[0] = wss->proto[0] = wss->group[0] = wss->pairwise[0] = '\0';
	wss->leap_username[0] = wss->leap_password[0] = '\0';
	wss->wepkey[0][0] = wss->wepkey[1][0] = '\0';
	wss->wepkey[2][0] = wss->wepkey[3][0] = '\0';
	wss->psk[0] = wss->proactive_key_caching[0] = '\0';
}

typedef struct _NMWrapperWireless8021xSettings {
	int system_ca_certs;
	uint32_t auth_timeout;
	uint32_t p1_auth_flags;
	int  ca_cert_scheme;
	char ca_cert[LIBNM_WRAPPER_MAX_PATH_LEN];
	char ca_cert_password[LIBNM_WRAPPER_MAX_NAME_LEN];
	char ca_path[LIBNM_WRAPPER_MAX_PATH_LEN];
	int  cli_cert_scheme;
	char cli_cert[LIBNM_WRAPPER_MAX_PATH_LEN];
	char cli_cert_password[LIBNM_WRAPPER_MAX_NAME_LEN];
	///Valid methods are: "leap", "md5", "tls", "peap", "ttls", "pwd", and "fast".
	char eap[LIBNM_WRAPPER_MAX_NAME_LEN];
	char identity[LIBNM_WRAPPER_MAX_NAME_LEN];
	char pac_file[LIBNM_WRAPPER_MAX_PATH_LEN];
	char password[LIBNM_WRAPPER_MAX_NAME_LEN];
	char anonymous[LIBNM_WRAPPER_MAX_NAME_LEN];
	char p1_fast_provisioning[LIBNM_WRAPPER_MAX_NAME_LEN];
	char p1_peaplabel[LIBNM_WRAPPER_MAX_NAME_LEN];
	char p1_peapver[LIBNM_WRAPPER_MAX_NAME_LEN];
	///inner non-EAP authentication methods. Recognized non-EAP "phase 2" methods are "pap", "chap", "mschap", "mschapv2", "gtc", "otp", "md5", and "tls"
	char p2_auth[LIBNM_WRAPPER_MAX_NAME_LEN];
	///inner EAP-based authentication methods.  Recognized EAP-based "phase 2" methods are "md5", "mschapv2", "otp", "gtc", and "tls".
	char p2_autheap[LIBNM_WRAPPER_MAX_NAME_LEN];
	int  p2_ca_cert_scheme;
	char p2_ca_cert[LIBNM_WRAPPER_MAX_PATH_LEN];
	char p2_ca_cert_password[LIBNM_WRAPPER_MAX_NAME_LEN];
	char p2_ca_path[LIBNM_WRAPPER_MAX_PATH_LEN];
	int  p2_cli_cert_scheme;
	char p2_cli_cert[LIBNM_WRAPPER_MAX_PATH_LEN];
	char p2_cli_cert_password[LIBNM_WRAPPER_MAX_NAME_LEN];
	int  p2_private_key_scheme;
	int  p2_private_key_format;
	char p2_private_key[LIBNM_WRAPPER_MAX_PATH_LEN];
	char p2_private_key_password[LIBNM_WRAPPER_MAX_NAME_LEN];
	int  private_key_scheme;
	int  private_key_format;
	///Contains the private key when the “eap” property is set to "tls".
	char private_key[LIBNM_WRAPPER_MAX_PATH_LEN];
	char private_key_password[LIBNM_WRAPPER_MAX_NAME_LEN];
	char pin[LIBNM_WRAPPER_MAX_NAME_LEN];
	char tls_disable_time_checks[LIBNM_WRAPPER_MAX_NAME_LEN];
	char pac_file_password[LIBNM_WRAPPER_MAX_NAME_LEN];
}NMWrapperWireless8021xSettings;

static inline void NMWrapperWireless8021xSettings_init(NMWrapperWireless8021xSettings *wxs)
{
	wxs->auth_timeout = wxs->p1_auth_flags = wxs->system_ca_certs = 0;
	wxs->ca_cert[0] = wxs->ca_cert_password[0] = wxs->ca_path[0] = '\0';
	wxs->cli_cert[0] = wxs->cli_cert_password[0] = '\0';
	wxs->eap[0] = '\0';
	wxs->identity[0] = wxs->pac_file[0] = wxs->password[0] = wxs->anonymous[0] = '\0';
	wxs->p1_fast_provisioning[0] = wxs->p1_peaplabel[0] = wxs->p1_peapver[0] = '\0';
	wxs->p2_auth[0] = wxs->p2_autheap[0] = '\0';
	wxs->p2_ca_cert[0] = wxs->p2_ca_cert_password[0] = wxs->p2_ca_path[0] = '\0';
	wxs->p2_cli_cert[0] = wxs->p2_cli_cert_password[0] = '\0';
	wxs->p2_private_key[0] = wxs->p2_private_key_password[0] = '\0';
	wxs->private_key[0] = wxs->private_key_password[0] = wxs->pin[0] = '\0';
}

static inline const char* prefix_to_netmask(int prefix, char *buffer, int len)
{
	struct in_addr mask;
	mask.s_addr = nm_utils_ip4_prefix_to_netmask(prefix);
	return inet_ntop(AF_INET, &mask, buffer, len);
}

static inline int netmask_to_prefix(const char *mask)
{
	struct in_addr n;
	inet_pton(AF_INET, mask, &n);
	return nm_utils_ip4_netmask_to_prefix(n.s_addr);
}

typedef struct _NMWrapperIPRoute {
	uint32_t prefix;
	uint32_t window;
	uint32_t mtu;
	uint32_t pad;
	int64_t metric;
	char dest[LIBNM_WRAPPER_MAX_NAME_LEN];
}NMWrapperIPRoute;

/**
 * @name library management APIs
 * A handle MUST be initialized before calling any of other APIs, and it MUST
 * be destroyed at exit.
 */

/**@{*/
/**
 * Initialize library handle.
 *
 * Returns: pointer to library handle
 *          NULL if unsuccessful
 */
libnm_wrapper_handle libnm_wrapper_init();

/**
 * Destroy library handle.
 *
 * Returns: pointer to library handle
 */
void libnm_wrapper_destroy(libnm_wrapper_handle hd);
/**@}*/


/**
 * @name Connection Management APIs
 * Add, activate, deactivate, and remove a connection.
 */
/**@{*/
/**
 * Get connection settings.
 * @param hd: library handle
 * @param connection:  on which interface
 * @param id: connectin id
 * @param s : location to store general settings
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_settings(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperSettings *s);

/**
 * Get an array of settings from connections.
 * @param hd: library handle
 * @param interface:  on which interface
 * @param s : array to store general settings
 * @param size : the size of array
 *
 * Returns: number of connections actually parsed
 */
int libnm_wrapper_connections_get_settings(libnm_wrapper_handle hd, const char *interface, NMWrapperSettings *s, int size);

/**
 * Get active connection state.
 * @param hd: library handle
 * @param interface: on which device
 * @param active: active connection id
 *
 * Returns: NMActiveConnectionState
 */
int libnm_wrapper_active_connection_get_state(libnm_wrapper_handle hd, const char *interface, const char *active);

/**
 * Get active connection state reason.
 * @param hd: library handle
 * @param interface: on which device
 * @param active: active connection id
 *
 * Returns: NMActiveConnectionStateReason
 */
int libnm_wrapper_active_connection_get_state_reason(libnm_wrapper_handle hd, const char *interface, const char *active);

/**
 * Delete a connection by id.
 * @param hd: library handle
 * @param id: connection id
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_delete_connection(libnm_wrapper_handle hd, char *id);

/**
 * Enable/disable auto-start of a connection.
 * @param hd: library handle
 * @param id: connection id
 * @param autoconnect: true or false
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_set_autoconnect(libnm_wrapper_handle hd, const char *id, bool autoconnect);

/**
 * Get auto-start status of a connection.
 * @param hd: library handle
 * @param id: connection id
 * @param autoconnect: location to store auto-start
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_autoconnect(libnm_wrapper_handle hd, const char *id, bool *autoconnect);

/**
 * Activate the connection on the interface.
 * @param hd: library handle
 * @param interface: on which interface
 * @param id: connection id
 * @param wifi: whether is a wifi connection
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_activate_connection(libnm_wrapper_handle hd, const char *interface, char *id, bool wifi);

/**
 * Deactivate the connection on the interface.
 * @param hd: library handle
 * @param interface: on which interface
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_deactivate_connection(libnm_wrapper_handle hd, const char *interface);
/**@}*/

/**
 * @name Wifi connection management API
 */
/**@{*/
/**
 * Get wifi settings of a connection.
 * @param hd: library handle
 * @param interface: on which interface
 * @param id: if set, get the settings from the connection of the id, otherwise get the settings of the active connection.
 * @param ws: location to store wifi settings
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_wireless_settings(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperWirelessSettings *ws);

/**
 * Get wifi security settings of a connection.
 * @param hd: library handle
 * @param interface: on which interface
 * @param id: if set, get the settings from the connection of the id, otherwise get the settings of the active connection.
 * @param wss: location to store wifi security settings
 * @param wxs: location to store 8021x settings
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_wireless_security_settings(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperWirelessSecuritySettings *wss, NMWrapperWireless8021xSettings *wxs);

/**
 * Create a wifi connection profile.
 * @param hd: library handle
 * @param s: location to store general settings
 * @param ws: location to store wifi settings
 * @param wss: location to store wifi security settings
 * @param wxs: location to store 8021x settings
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_add_wireless_connection(libnm_wrapper_handle hd, NMWrapperSettings *s, NMWrapperWirelessSettings* ws, NMWrapperWirelessSecuritySettings *wss, NMWrapperWireless8021xSettings *wxs);

/**
 * Update a wifi connection profile.
 * @param hd: library handle
 * @param id: id of the connection to be updated
 * @param s: location to store general settings
 * @param ws: location to store wifi settings
 * @param wss: location to store wifi security settings
 * @param wxs: location to store 8021x settings
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_update_wireless_connection(libnm_wrapper_handle hd, const char *id, NMWrapperSettings *s, NMWrapperWirelessSettings* ws, NMWrapperWirelessSecuritySettings *wss, NMWrapperWireless8021xSettings *wxs);
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
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_add_wired_connection(libnm_wrapper_handle hd, NMWrapperSettings *s, NMWrapperWiredSettings *ws);

/**
 * Update a wired connection profile.
 * @param hd: library handle
 * @param id: connection id
 * @param s: location to store general settings
 * @param ws: location to store wired settings
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_update_wired_connection(libnm_wrapper_handle hd, const char *id, NMWrapperSettings *s, NMWrapperWiredSettings* ws);

/**
 * Update a wired connection profile.
 * @param hd: library handle
 * @param interface: on which interface
 * @param id: connection id
 * @param ws: location to store wired settings
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_connection_get_wired_settings(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperWiredSettings *ws);
/**@}*/

/**
 * @name AP Management API
 */
/**@{*/
/**
 * Get AP list.
 * @param hd: library handle
 * @param interface: on which interface
 * @param list: list of AP settings
 * @param size: size of list
 *
 * Returns: actual number of processed AP
 */
int libnm_wrapper_access_point_get_scanlist(libnm_wrapper_handle hd, const char *interface, NMWrapperAccessPoint *list, int size);

/**
 * Get active AP settings.
 * @param hd: library handle
 * @param ap: location to store active AP settings
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_access_point_get_active_settings(libnm_wrapper_handle hd, const char *interface, NMWrapperAccessPoint *ap);
/**@}*/

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
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_device_get_status(libnm_wrapper_handle hd, const char *interface, NMWrapperDevice* status);

/**
 * Enable/disable device auto-start.
 * @param hd: library handle
 * @param interface: which device
 * @param autoconnect: whether to enable auto-start
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_device_set_autoconnect(libnm_wrapper_handle hd, const char *interface, bool autoconnect);

/**
 * Get device auto-start flag.
 * @param hd: library handle
 * @param interface: which device
 * @param autoconnect: location to store auto-start
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_device_get_autoconnect(libnm_wrapper_handle hd, const char *interface, bool *autoconnect);

/**
 * Disconnects the device if currently connected, and prevents the device from automatically connecting to networks until the next manual network connection request..
 * @param hd: library handle
 * @param interface: which device
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_device_disconnect(libnm_wrapper_handle hd, const char *interface);

/**
 * Enables or disables wireless devices.
 * @param hd: library handle
 *
 * Returns: SDCERR_SUCCESS if successful
 */
int libnm_wrapper_device_enable_wireless(libnm_wrapper_handle hd , bool enable);

/**
 * Determines whether wireless devices are enabled
 * @param hd: library handle
 *
 * Returns: non-zero if wireless devices are enabled
 */
int libnm_wrapper_device_is_wireless_enabled(libnm_wrapper_handle hd);

/**
 * Get number of connections on interface.
 * @param hd: library handle
 * @param interface: which interface
 *
 * Returns: number of connections
 */
int libnm_wrapper_device_get_connection_num(libnm_wrapper_handle hd, char *interface);

/**
 * Get device state.
 * @param hd: library handle
 * @param interface: on which device
 *
 * Returns: NMDeviceState
 */
int libnm_wrapper_device_get_state(libnm_wrapper_handle hd, const char *interface);

/**
 * Get device state reason.
 * @param hd: library handle
 * @param interface: on which device
 *
 * Returns: NMDeviceStateReason
 */
int libnm_wrapper_device_get_state_reason(libnm_wrapper_handle hd, const char *interface);

/**
 * Monitor device state .
 * @param hd: library handle
 * @param interface: on which device
 * @param user: user callback function
 * Returns: LIBNM_WRAPPER_ERR_SUCCESS if successful
 */
int libnm_wrapper_device_state_monitor(libnm_wrapper_handle hd, const char *interface,
									LIBNM_WRAPPER_STATE_MONITOR_CALLBACK_ST *user);

/**@}*/


/**
 * @name IP Management API
 */
/**@{*/

/**
 * IPv4 Management API
 */
int libnm_wrapper_ipv4_set_method(libnm_wrapper_handle hd , const char *id, const char *value);
int libnm_wrapper_ipv4_get_method(libnm_wrapper_handle hd , const char *id, char *method, int len);
int libnm_wrapper_ipv4_get_address_num(libnm_wrapper_handle hd, const char *id, int *num);
int libnm_wrapper_ipv4_set_address(libnm_wrapper_handle hd, const char *id, const int index, const char *address, const char *netmask, const char *gateway);
int libnm_wrapper_ipv4_set_all_addresses(libnm_wrapper_handle hd, const char *id, const int index, const char *address, const char *netmask, const char *gateway, const char* dns);
int libnm_wrapper_ipv4_get_address(libnm_wrapper_handle hd, const char *id, const int index, char *address, int address_len, char *netmask, int netmask_len, char *gateway, int gateway_len);
int libnm_wrapper_ipv4_set_dns(libnm_wrapper_handle hd, const char *id, char *address);
int libnm_wrapper_ipv4_get_dns(libnm_wrapper_handle hd, const char *id, char *address, int buff_len);
int libnm_wrapper_ipv4_clear_address(libnm_wrapper_handle hd , const char *id);
int libnm_wrapper_ipv4_get_dhcp_information(libnm_wrapper_handle hd, const char *interface, const int size, const char *options[], const int len, char val[size][len]);
int libnm_wrapper_ipv4_get_route_information(libnm_wrapper_handle hd, const char *interface, const char *id, NMWrapperIPRoute *route, int size);
int libnm_wrapper_get_active_ipv4_addresses(libnm_wrapper_handle hd, const char *interface, char *ip, int ip_len, char *gateway, int gateway_len, char *subnet, int subnet_len, char *dns_1, int dns1_len, char *dns_2, int dns2_len);
/**
 * IPv6 Management API
 */
int libnm_wrapper_ipv6_set_method(libnm_wrapper_handle hd , const char *id, const char * value);
int libnm_wrapper_ipv6_get_method(libnm_wrapper_handle hd , const char *id, char *method, int len);
int libnm_wrapper_ipv6_get_address_num(libnm_wrapper_handle hd, const char *id, int *num);
int libnm_wrapper_ipv6_get_address(libnm_wrapper_handle hd, const char *id, const int index, char *address, int address_len, char *netmask, int netmask_len, char *gateway, int gateway_len);
int libnm_wrapper_ipv6_set_address(libnm_wrapper_handle hd, const char *id, const int index, const char *address, const char *netmask, const char *gateway);
int libnm_wrapper_ipv6_set_dns(libnm_wrapper_handle hd, const char *id, char *address);
int libnm_wrapper_ipv6_get_dns(libnm_wrapper_handle hd, const char *id, char *address, int buff_len);
int libnm_wrapper_ipv6_clear_address(libnm_wrapper_handle hd , const char *id);


/**
 * Deprecated IPv4/IPv6 Management API
 */
int libnm_wrapper_ipv4_get_broadcast_address(libnm_wrapper_handle hd , const char *id, char *address, int len);
int libnm_wrapper_ipv4_set_broadcast_address(libnm_wrapper_handle hd , const char *id, char *address);
int libnm_wrapper_ipv4_set_bridgeports(libnm_wrapper_handle hd , const char *id, int ports);
int libnm_wrapper_ipv4_get_bridgeports(libnm_wrapper_handle hd , const char *id, int *ports);
int libnm_wrapper_ipv4_disable_hostapd(libnm_wrapper_handle hd , const char *id);
int libnm_wrapper_ipv4_enable_hostapd(libnm_wrapper_handle hd , const char *id);
int libnm_wrapper_ipv4_disable_nat(libnm_wrapper_handle hd , const char *id);
int libnm_wrapper_ipv4_enable_nat(libnm_wrapper_handle hd , const char *id);
int libnm_wrapper_ipv4_get_nat(libnm_wrapper_handle hd , const char *id, int *nat);
int libnm_wrapper_ipv4_get_hostapd(libnm_wrapper_handle hd , const char *id, int *mode);
int libnm_wrapper_ipv6_set_nat(libnm_wrapper_handle hd , const char *id, int nat);
int libnm_wrapper_ipv6_get_nat(libnm_wrapper_handle hd , const char *id, int *nat);
int libnm_wrapper_ipv6_set_dhcp(libnm_wrapper_handle hd , const char *id, char *dhcp);
int libnm_wrapper_ipv6_get_dhcp(libnm_wrapper_handle hd , const char *id, char *dhcp, int len);
int libnm_wrapper_ipv6_disable_interface(libnm_wrapper_handle hd , const char *interface);
int libnm_wrapper_ipv6_enable_interface(libnm_wrapper_handle hd , const char *interface);
int libnm_wrapper_ipv6_disable_nat(libnm_wrapper_handle hd , const char *id);
int libnm_wrapper_ipv6_enable_nat(libnm_wrapper_handle hd , const char *id);
/**@}*/

/**
 * @name Misc API
 */
/**@{*/

/**
 * Set log level.
 * @param hd: library handle
 * @param level: log level
 *
 * Returns: SDCERR_SUCCESS if successful.
 */
int libnm_wrapper_set_log_level(libnm_wrapper_handle hd, int level);

/**
 * Get log level.
 * @param hd: library handle
 * @param level: location to store log level
 *
 * Returns: SDCERR_SUCCESS if successful.
 */
int libnm_wrapper_get_log_level(libnm_wrapper_handle hd, int *level);

/**
 * Get nm version.
 * @param hd: library handle
 * @param version: location to store version
 * @param len: version buffer size
 *
 * Returns: SDCERR_SUCCESS if successful.
 */
int libnm_wrapper_get_version(libnm_wrapper_handle hd, char *version, int len);

/**
 * Wifi frequency to channel
 * @param frequency:
 *
 * Returns: the channel represented by the frequency or 0.
 */
unsigned int libnm_wrapper_utils_wifi_freq_to_channel(unsigned int frequency);

/**
 * Wifi channel to frequency
 * @param channel: wifi channel
 * @param band: frequency band for wireless ("a" or "bg")
 *
 * Returns: the frequency represented by the channel of the band, or -1 when the freq is invalid,
 *          or 0 when the band is invalid
 */
unsigned int libnm_wrapper_utils_wifi_channel_to_freq(unsigned int channel, const char *band);
/**@}*/

#ifdef __cplusplus
}
#endif

#endif //__LIBNM_WRAPPEr_H__
