/*
 * Netmap support
 */

#include "packetio/packetio.h"

#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>

#include <poll.h>
#include <assert.h>

int packetio_iface_any(packetio_iface_t *iface)
{
	errno = EOPNOTSUPP;
	return -1;
}

int packetio_name_to_iface(const char *iface_name, packetio_iface_t *iface)
{
	*iface = (void *)iface_name;
	return 0;
}

int packetio_start(packetio_iface_t *iface, packetio_process_t process_fn)
{
	struct nm_pkthdr h;
	unsigned char *buf;
	struct nm_desc *d;
	struct pollfd fds;
	char ifname[128];

	snprintf(ifname, sizeof(ifname), "netmap:%s", *(const char **)iface);

	d = nm_open(ifname, NULL, 0, 0);
	if (!d) {
		return -1;
	}
	fds = (struct pollfd) {
		.fd	= NETMAP_FD(d),
		.events	= POLLIN,
	};
	for (;;) {
		poll(&fds, 1, -1);
		while ((buf = nm_nextpkt(d, &h))) {
			struct packetio_packet packet = {
				.data = buf,
				.size = h.len,
			};
			process_fn(&packet, 1);
		}
	}
	nm_close(d);
}
