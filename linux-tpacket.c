/*
 * Linux TPACKET support
 */

#include "packetio/packetio.h"

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

struct tpacket_ring {
	int			sockfd;
	void			*mmap;
	size_t			mmap_size;
	struct iovec		*ring;
	size_t			ring_size;
	size_t			nr_blocks;
	size_t			block_size;
	struct tpacket_req3	req;
	packetio_process_t	process_fn;
};

int packetio_iface_any(packetio_iface_t *iface)
{
	*iface = 0;
	return 0;
}

int packetio_name_to_iface(const char *iface_name, packetio_iface_t *iface)
{
	unsigned int idx;

	idx = if_nametoindex(iface_name);
	if (!idx) {
		return -1;
	}
	*iface = idx;
	return 0;
}

static void tpacket_process_block(struct tpacket_ring *ring, struct tpacket_block_desc *block_desc)
{
	struct tpacket3_hdr *packet_desc = (void*)block_desc + block_desc->hdr.bh1.offset_to_first_pkt;
	size_t nr_packets = block_desc->hdr.bh1.num_pkts;
	struct packetio_packet packets[nr_packets];

	for (size_t i = 0; i < nr_packets; i++) {
		struct packetio_packet *packet = &packets[i];
		packet->data = (void*)packet_desc + packet_desc->tp_mac;
		packet->size = packet_desc->tp_snaplen;

		packet_desc = (void*)packet_desc + packet_desc->tp_next_offset;
		__sync_synchronize();
	}
	ring->process_fn(packets, nr_packets);
}

static int tpacket_process_ring(struct tpacket_ring *ring)
{
	size_t block_idx = 0;
	struct pollfd pfd;

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd		= ring->sockfd;
	pfd.events	= POLLIN | POLLERR;

	for (;;) {
		struct tpacket_block_desc *block_desc = ring->ring[block_idx].iov_base;

		while ((block_desc->hdr.bh1.block_status & TP_STATUS_USER) == 0) {
			poll(&pfd, 1, -1);
		}
		tpacket_process_block(ring, block_desc);
		block_desc->hdr.bh1.block_status = TP_STATUS_KERNEL;
		__sync_synchronize();
		block_idx = (block_idx + 1) % ring->nr_blocks;
	}
	return 0;
}

static int tpacket_bind_ring(struct tpacket_ring *ring, packetio_iface_t *iface)
{
	struct sockaddr_ll ll;

	memset(&ll, 0, sizeof(ll));
	ll.sll_family	= PF_PACKET;
	ll.sll_protocol	= htons(ETH_P_ALL);
	ll.sll_ifindex	= *iface;
	ll.sll_hatype	= 0;
	ll.sll_pkttype	= 0;
	ll.sll_halen	= 0;

	return bind(ring->sockfd, (struct sockaddr *) &ll, sizeof(ll));
}

static int tpacket_mmap_ring(struct tpacket_ring *ring)
{
	ring->mmap_size = ring->req.tp_block_nr * ring->req.tp_block_size;

	ring->mmap = mmap(0, ring->mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED|MAP_POPULATE, ring->sockfd, 0);
	if (ring->mmap == MAP_FAILED) {
		return -1;
	}
	memset(ring->ring, 0, ring->ring_size);
	for (size_t i = 0; i < ring->nr_blocks; i++) {
		ring->ring[i].iov_base	= ring->mmap + (i * ring->block_size);
		ring->ring[i].iov_len	= ring->block_size;
	}
	return 0;
}

static int tpacket_setup_ring(struct tpacket_ring *ring)
{
	size_t nr_blocks = 256;
	size_t frame_size;
	size_t page_size;

	page_size	= sysconf(_SC_PAGESIZE);
	frame_size	= TPACKET_ALIGNMENT << 7;

	ring->nr_blocks		= nr_blocks;
	ring->block_size	= page_size << 2;
	ring->ring_size		= nr_blocks * sizeof(*ring->ring);

	ring->req = (struct tpacket_req3) {
		.tp_retire_blk_tov	= 64,
		.tp_sizeof_priv		= 0,
		.tp_feature_req_word	= TP_FT_REQ_FILL_RXHASH,
		.tp_block_size		= ring->block_size,
		.tp_frame_size		= frame_size,
		.tp_block_nr		= nr_blocks,
		.tp_frame_nr		= ring->block_size / frame_size * nr_blocks,
	};
	if (setsockopt(ring->sockfd, SOL_PACKET, PACKET_RX_RING, &ring->req, sizeof(ring->req)) < 0) {
		return -1;
	}
	ring->ring = malloc(ring->ring_size);
	if (!ring->ring) {
		errno = ENOMEM;
		return -1;
	}
	return 0;
}

int packetio_start(packetio_iface_t *iface, packetio_process_t process_fn)
{
	struct tpacket_ring ring;
	int sockfd, version;

	sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockfd < 0) {
		return -1;
	}
	version = TPACKET_V3;
	if (setsockopt(sockfd, SOL_PACKET, PACKET_VERSION, &version, sizeof(version)) < 0) {
		goto error;
	}
	ring = (struct tpacket_ring) {
		.sockfd		= sockfd,
		.process_fn	= process_fn,
	};
	if (tpacket_setup_ring(&ring) < 0) {
		goto error;
	}
	if (tpacket_mmap_ring(&ring) < 0) {
		goto error;
	}
	if (tpacket_bind_ring(&ring, iface) < 0) {
		goto error;
	}
	if (tpacket_process_ring(&ring) < 0) {
		goto error;
	}
error:
	if (close(sockfd) < 0) {
		return -1;
	}
	return 0;
}
