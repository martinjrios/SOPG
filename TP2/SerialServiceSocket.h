#ifndef _SERIAL_SERVICE_SOCKET_H_
#define _SERIAL_SERVICE_SOCKET_H_

#include <stddef.h>
#include <stdint.h>

int serial_service_socket_init(const char *ip, uint16_t port);
int serial_service_socket_accept(void);
int serial_service_socket_connect(void);
int serial_service_socket_read(int newfd, char *recvBuff, size_t nbytes);
int serial_service_socket_send(int newfd, char *sendBuff, size_t nbytes);
int serial_service_socket_close(int fd);
int serial_service_socket_end(void);
int serial_service_socket_fd(void);

#endif /*_SERIAL_SERVICE_SOCKET_H_*/