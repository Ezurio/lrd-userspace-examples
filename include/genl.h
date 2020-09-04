#ifndef __GENL_H
#define __GENL_H

int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group);

#endif
