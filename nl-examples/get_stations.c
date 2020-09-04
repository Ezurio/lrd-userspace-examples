/*   An example to get stations ifno

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include <string.h>
#include <asm/errno.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/genetlink.h>
#include <sys/select.h>

#include "genl.h"
#include "nl80211.h"


#define ETH_ALEN 6
#define MAC_ADDRESS_BUFFER_LEN (ETH_ALEN * 3) //xx:xx:xx:xx:xx:xx + '\0'
#define IP_ADDRESS_BUFFER_LEN 16              //xxx.xxx.xxx.xxx + '\0'
struct station_info
{
	char mac[MAC_ADDRESS_BUFFER_LEN];
	char ip[IP_ADDRESS_BUFFER_LEN];
};

//dhcp lease file
#define DNSMASQ_LEASE_FILE_FOR_WLAN0 "/var/lib/NetworkManager/dnsmasq-wlan0.leases"

//max 10 active stations
#define MAX_NUMBER_OF_ACTIVE_STATIONS 10
#define is_sta_valid(x) (x.mac[2] == ':')

struct nl80211_state
{
	struct nl_sock *nl_sock;
	int nl80211_id;
};

struct gen_nl_params
{
	struct nl80211_state nlstate;
	struct nl_cb *cb;
};


//Get stations' ip addresses from dnsmasq-wlan0.leases
static int get_ip_addresses(struct station_info *sta, int size)
{
	char epoch_time[256], mac[256], ip[68], name[256], client_id[768];

	FILE *fp = fopen(DNSMASQ_LEASE_FILE_FOR_WLAN0, "r");
	if (!fp)
		return -ENOENT;

	//Always update all clients' ip addresses in case of ip changes without events
	while (fscanf(fp, "%255s %255s %64s %255s %764s", epoch_time, mac, ip, name, client_id) >= 3)
	{
		for(int i=0; i<size; i++)
		{
			if(is_sta_valid(sta[i]))
			{
				if(memcmp(sta[i].mac, mac, MAC_ADDRESS_BUFFER_LEN))
					continue;

				memcpy(sta[i].ip, ip, IP_ADDRESS_BUFFER_LEN);
				sta[i].ip[IP_ADDRESS_BUFFER_LEN-1] = '\0';
			}
			break;
		}
	}

	fclose(fp);
	return 0;
}


//Append new item to the next available slot
static int add_station(struct station_info *sta, const char *mac)
{
	for(int i=0; i<MAX_NUMBER_OF_ACTIVE_STATIONS; i++)
	{
		if(is_sta_valid(sta[i]))
		{
			//In case a device disconnected without sending NL80211_CMD_DEL_STATION message and then reconnects
			if(memcmp(sta[i].mac, mac, MAC_ADDRESS_BUFFER_LEN) == 0)
				return 0;
		}
		else
		{
			memcpy(sta[i].mac, mac, MAC_ADDRESS_BUFFER_LEN);
			return 0;
		}
	}

	fprintf(stderr, "Too many active stations.\n");
	return -1;
}

//Delete item by overriding with subsequent items
static int del_station(struct station_info *sta, const char *mac)
{
	int i;

	for(i=0; i<MAX_NUMBER_OF_ACTIVE_STATIONS; i++)
	{
		if(memcmp(sta[i].mac, mac, MAC_ADDRESS_BUFFER_LEN) == 0)
		{
			break;
		}
	}

	if(i == MAX_NUMBER_OF_ACTIVE_STATIONS)
	{
		fprintf(stderr, "Unable to find device %s\n", mac);
		return -1;
	}

	for(; i<(MAX_NUMBER_OF_ACTIVE_STATIONS - 1); i++)
	{
		memcpy(&sta[i], &sta[i+1], sizeof(struct station_info));
	}

	memset(&sta[i], 0, sizeof(struct station_info));

	return 0;
}

static void mac_addr_n2a(char *mac, const unsigned char *arg)
{
	int i, l;

	l = 0;
	for (i = 0; i < ETH_ALEN ; i++)
	{
		if (i == 0)
		{
			sprintf(mac+l, "%02x", arg[i]);
			l += 2;
		}
		else
		{
			sprintf(mac+l, ":%02x", arg[i]);
			l += 3;
		}
	}
}

static int nl80211_init(struct nl80211_state *nlstate)
{
	int err;

	nlstate->nl_sock = nl_socket_alloc();
	if (!nlstate->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(nlstate->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	nl_socket_set_buffer_size(nlstate->nl_sock, 8192, 8192);

	/* try to set NETLINK_EXT_ACK to 1, ignoring errors */
	err = 1;
	setsockopt(nl_socket_get_fd(nlstate->nl_sock), SOL_NETLINK,
		   NETLINK_EXT_ACK, &err, sizeof(err));

	nlstate->nl80211_id = genl_ctrl_resolve(nlstate->nl_sock, "nl80211");
	if (nlstate->nl80211_id < 0) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_handle_destroy;
	}

	return 0;

 out_handle_destroy:
	nl_socket_free(nlstate->nl_sock);
	return err;
}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

static int nl80211_event_handle(struct nl_msg *msg, void *arg)
{
	char macbuf[MAC_ADDRESS_BUFFER_LEN];
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct station_info *sta = (struct station_info *)arg;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			  genlmsg_attrlen(gnlh, 0), NULL);

	switch (gnlh->cmd)
	{
		case NL80211_CMD_NEW_STATION:
			mac_addr_n2a(macbuf, nla_data(tb[NL80211_ATTR_MAC]));
			add_station(sta, macbuf);
		    break;
		case NL80211_CMD_DEL_STATION:
			mac_addr_n2a(macbuf, nla_data(tb[NL80211_ATTR_MAC]));
			del_station(sta, macbuf);
		    break;
		default:
			break;
	}

	return NL_SKIP;
}


int nl80211_listen(struct nl80211_state *nlstate)
{
	int mcid, ret;

	/* Listen to MLME multicast group for station NEW and DEL events */
	mcid = nl_get_multicast_id(nlstate->nl_sock, "nl80211", "mlme");
	if (mcid < 0)
		return mcid;

	ret = nl_socket_add_membership(nlstate->nl_sock, mcid);
	if (ret)
		return ret;

	return 0;
}

static void event_close(struct gen_nl_params *params)
{
	if(params->cb != NULL)
		nl_cb_put(params->cb);

	if(params->nlstate.nl_sock != NULL)
		nl_socket_free(params->nlstate.nl_sock);

	return;
}

static int event_init(struct gen_nl_params *params, void *arg)
{
	int rc;

	rc = nl80211_init(&params->nlstate);
	if (rc)
		return rc;

	rc = nl80211_listen(&params->nlstate);
	if (rc)
	{
		event_close(params);
		return rc;
	}

	params->cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!params->cb)
	{
		event_close(params);
		return -ENOMEM;
	}

	nl_socket_set_nonblocking(params->nlstate.nl_sock);

	// no sequence checking for multicast messages
	nl_cb_set(params->cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(params->cb, NL_CB_VALID, NL_CB_CUSTOM, nl80211_event_handle, arg);

	return 0;
}

static void event_process(struct gen_nl_params *params, struct station_info *sta)
{
	int fds;
	fd_set rx;
	struct timeval tv;

	while(1)
	{
		fds = nl_socket_get_fd(params->nlstate.nl_sock);
		FD_ZERO(&rx);
		FD_SET(fds, &rx);

		tv.tv_sec = 30;
		tv.tv_usec = 0;

		if(select(fds+1, &rx, NULL, NULL, &tv) > 0)
		{
			nl_recvmsgs(params->nlstate.nl_sock, params->cb);
		}
		else
		{
			get_ip_addresses(sta, MAX_NUMBER_OF_ACTIVE_STATIONS);

			for(int i=0; i<MAX_NUMBER_OF_ACTIVE_STATIONS; i++)
			{
				if(is_sta_valid(sta[i]))
				{
					printf("device-%d: mac: %s ip %s\n", i, sta[i].mac, sta[i].ip);
				}
			}
		}
	}

	return;
}


int main(int argc, char *argv[])
{
	int rc = 0;
	struct gen_nl_params params;
	struct station_info sta[MAX_NUMBER_OF_ACTIVE_STATIONS];

	memset(sta, 0, sizeof(sta));
	memset(&params, 0, sizeof(params));

	rc = event_init(&params, (void *)sta);
	if(rc < 0)
	{
		fprintf(stderr, "failed to init event\n");
		return rc;
	}

	event_process(&params, sta);

	event_close(&params);
	return rc;
}
