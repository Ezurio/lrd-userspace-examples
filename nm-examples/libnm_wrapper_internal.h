#ifndef __LIBNM_WRAPPER_INTERNAL_H__
#define __LIBNM_WRAPPER_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <glib.h>
#include <libnm/NetworkManager.h>
#include "libnm_wrapper.h"

static inline void gbytes_to_string(GBytes *src, char *dst, int len)
{
	const char *ptr = nm_utils_ssid_to_utf8(g_bytes_get_data(src, NULL),
			g_bytes_get_size(src));
	safe_strncpy(dst, ptr, len);
	g_free(ptr);
}

#define nm_wrapper_assert(x, error) if(!x) return error;

typedef struct _libnm_wrapper_handle_st
{
	NMClient *client;
}libnm_wrapper_handle_st;

libnm_wrapper_handle libnm_wrapper_init();
void libnm_wrapper_destroy(libnm_wrapper_handle hd);

#ifdef __cplusplus
}
#endif

#endif //__LIBNM_WRAPPER_INTERNAL_H__
