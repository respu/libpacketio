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

typedef void (*packetio_process_t)(struct packetio_packet *packets, size_t count);

int packetio_start(packetio_process_t process_fn);

#ifdef __cplusplus
}
#endif

#endif
