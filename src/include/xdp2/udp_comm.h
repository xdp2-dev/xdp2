/*
 * Copyright (c) 2025 Tom Herbert
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __XDP2_UDP_COMM_H__
#define __XDP2_UDP_COMM_H__

/* UDP sockets send/receive communications handle interface */

#include <linux/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Communication handler for UDP */
struct xdp2_comm_handle {
	int info;
	ssize_t (*send_data)(struct xdp2_comm_handle *comm,
			     void *buff, size_t count, struct in_addr to,
			     unsigned short to_port);
	ssize_t (*recv_data)(struct xdp2_comm_handle *comm, void *buff,
			     size_t count, struct in_addr *from,
			     unsigned short *from_port);
};

/* Send UDP data on a socket */
static inline ssize_t xdp2_udp_socket_send(int sockfd, void *buff, size_t count,
					   struct in_addr to,
					   unsigned short to_port)
{
	struct sockaddr_in servaddr;
	ssize_t n;

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = to_port;
	servaddr.sin_addr = to;

	n = sendto(sockfd, buff, count, 0, (struct sockaddr *)&servaddr,
		   sizeof(servaddr));

	return n;
}

/* Receive UDP data on a socket */
static inline ssize_t xdp2_udp_socket_recv(int sockfd, void *buff, size_t len,
					   struct in_addr *from,
					   unsigned short *from_port)
{
	struct sockaddr_in cliaddr;
	socklen_t alen;
	ssize_t n;

	alen = sizeof(cliaddr);

	n = recvfrom(sockfd, buff, len,  MSG_WAITALL,
		     (struct sockaddr *) &cliaddr, &alen);

	*from = cliaddr.sin_addr;
	*from_port = cliaddr.sin_port;

	return n;
}

/* Create a UDP with a bind port */
static inline int xdp2_udp_socket_create(int port)
{
	struct sockaddr_in local_addr;
	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		return -1;
	}

	if (port < 0) {
		/* No bind (use epheneral port) */
		return sockfd;
	}

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(port);
	local_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *)&local_addr,
	    sizeof(local_addr)) < 0) {
		perror("Bind failed");
		close(sockfd);
		return -1;
	}

	return sockfd;
}

/* Send UDP data on the communications handle */
static inline ssize_t xdp2_udp_socket_send_data(
		struct xdp2_comm_handle *comm, void *buff, size_t count,
		struct in_addr to, unsigned short to_port)
{
	return xdp2_udp_socket_send(comm->info, buff, count, to, to_port);
}

/* Receive UDP data on the communications handle */
static inline ssize_t xdp2_udp_socket_recv_data(
		struct xdp2_comm_handle *comm, void *buff, size_t len,
		struct in_addr *from, unsigned short *from_port)
{
	return xdp2_udp_socket_recv(comm->info, buff, len, from, from_port);
}

/* Create a UDP server socket communications handler */
static inline struct xdp2_comm_handle *xdp2_udp_socket_start(int port)
{
	struct xdp2_comm_handle *handle;
	int sockfd;

	sockfd = xdp2_udp_socket_create(port);
	if (sockfd < 0)
		return NULL;

	handle = malloc(sizeof(*handle));
	if (!handle) {
		close(sockfd);
		return NULL;
	}

	handle->info = sockfd;
	handle->send_data = xdp2_udp_socket_send_data;
	handle->recv_data = xdp2_udp_socket_recv_data;

	return handle;
}

#endif /* __XDP2_UDP_COMM_H__ */
