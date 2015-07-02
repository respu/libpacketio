#ifndef PACKETIO_H
#define PACKETIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

struct packetio_packet {
	void		*data;
	size_t		size;
};

typedef int packetio_iface_t;

typedef void (*packetio_process_t)(struct packetio_packet *packets, size_t count);

int packetio_iface_any(packetio_iface_t *iface);

int packetio_name_to_iface(const char *iface_name, packetio_iface_t *iface);

int packetio_start(packetio_iface_t *iface, packetio_process_t process_fn);

#ifdef __cplusplus
}
#endif

#endif
