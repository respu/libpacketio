#include <packetio/packetio.h>

#include <linux/if_ether.h>
#include <linux/icmp.h>
#include <net/if_arp.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <arpa/inet.h>
#include <linux/ip.h>

#include <getopt.h>
#include <libgen.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

struct trace_config {
	const char	*iface_name;
};

static const char *program;

void trace_tcp(struct iphdr *ip)
{
	struct tcphdr *tcp;

	tcp = (void *) ip + ip->ihl * sizeof(uint32_t);

	printf("TCP %d -> %d [seq = %u, ack = %u, window = %u]\n", ntohs(tcp->source), ntohs(tcp->dest), ntohl(tcp->seq), ntohl(tcp->ack_seq), ntohs(tcp->window));
}

void trace_udp(struct iphdr *ip)
{
	struct udphdr *udp;

	udp = (void *) ip + ip->ihl * sizeof(uint32_t);

	printf("UDP %d -> %d [len = %d]\n", ntohs(udp->source), ntohs(udp->dest), ntohs(udp->len));
}

const char *icmp_type(uint8_t type)
{
	switch (type) {
	case 0:
		return "Echo Reply";
	case 8:
		return "Echo Request";
	default:
		return "Unknown";
	}
}

void trace_icmp(struct iphdr *ip)
{
	struct icmphdr *icmp;

	icmp = (void *) ip + ip->ihl * sizeof(uint32_t);

	printf("ICMP %-2d %s [code = %d]\n", icmp->type, icmp_type(icmp->type), icmp->code);
}

void trace_ip(void *packet, size_t len)
{
	struct iphdr *ip;

	ip = packet + sizeof(struct ethhdr);

	switch (ip->protocol) {
	case IPPROTO_ICMP:
		trace_icmp(ip);
		break;
	case IPPROTO_UDP:
		trace_udp(ip);
		break;
	case IPPROTO_TCP:
		trace_tcp(ip);
		break;
	default:
		break;
	}
}

void trace_arp(void *packet, size_t len)
{
	struct arphdr *arp;

	arp = packet + sizeof(struct ethhdr);

	printf("ARP [op = %d]\n", ntohs(arp->ar_op));
}

void trace_packet(void *packet, size_t len)
{
	struct ethhdr *eth;

	eth = packet;

	switch (ntohs(eth->h_proto)) {
	case ETH_P_ARP:
		trace_arp(packet, len);
		break;
	case ETH_P_IP:
		trace_ip(packet, len);
		break;
	default:
		break;
	}
}

static void trace_packets(struct packetio_packet *packets, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		struct packetio_packet *packet = &packets[i];

		trace_packet(packet->data, packet->size);
	}
}

static void usage(void)
{
	fprintf(stdout,
		"usage: %s [options]\n"
		"  options:\n"
		"    -i, --interface <name>  Listen on interface. If not specified, listen to all interfaces.\n"
		"    -h, --help              Display this help and exit.\n",
		program);
	exit(1);
}

void parse_options(struct trace_config *cfg, int argc, char *argv[])
{
	static struct option trace_options[] = {
		{"interface",	required_argument,	0,	'i'},
		{"help",	no_argument,		0,	'h'},
		{0, 0, 0, 0}
	};

	for (;;) {
		int opt_idx = 0;
		int c;

		c = getopt_long(argc, argv, "i:h", trace_options, &opt_idx);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			cfg->iface_name = optarg;
			break;
		case 'h':
			usage();
		default:
			usage();
		}
	}
}

int main(int argc, char* argv[])
{
	struct trace_config cfg = {
	};
	packetio_iface_t iface;
	int err;

	program = basename(argv[0]);

	parse_options(&cfg, argc, argv);

	if (cfg.iface_name) {
		err = packetio_name_to_iface(cfg.iface_name, &iface);
		if (err < 0) {
			fprintf(stderr, "error: %s: %s\n", cfg.iface_name, strerror(errno));
			return 1;
		}
	} else {
		err = packetio_iface_any(&iface);
		if (err < 0) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return 1;
		}
	}

	err = packetio_start(&iface, trace_packets);
	if (err < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}
