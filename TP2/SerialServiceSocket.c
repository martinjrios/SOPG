#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "SerialServiceSocket.h"

#define MAX_BACKLOG     10

int s = -1;
struct sockaddr_in clientaddr;
struct sockaddr_in serveraddr;

int serial_service_socket_init(const char *ip, uint16_t port)
{
    s = socket(AF_INET, SOCK_STREAM, 0);

    // Cargamos datos de IP:PORT del server
    bzero((char*) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);   

    if(inet_pton(AF_INET, ip, &(serveraddr.sin_addr))<=0)
    {
        fprintf(stderr,"ERROR invalid server IP\r\n");
        return 1;
    }  

    // Abrimos puerto con bind()
    if (bind(s, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
        close(s);
        s = -1;
        perror("listener: bind");
        return 1;
    }

    // Seteamos socket en modo Listening
    if (listen (s, MAX_BACKLOG) == -1) // backlog=10
    {
        perror("error en listen");
        return 1;
    }

    return 0;
}

/**
 * @brief Acepta la petición de conexión de un cliente.
 * 
 * @return int El file descriptor para esa conexión.
 */
int serial_service_socket_accept(void)
{
    int newfd = -1;

    // Ejecutamos accept() para recibir conexiones entrantes
    socklen_t addr_len = sizeof(struct sockaddr_in);
    if ( (newfd = accept(s, (struct sockaddr *)&clientaddr, &addr_len)) == -1)
    {
        perror("error en accept");
        return -1;
    }
    
    char ipClient[32];
    inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
    printf  ("server:  conexion desde:  %s\n",ipClient);

    return newfd;
}

/**
 * @brief Establece una conexión sobre un socket.
 * 
 * @return int 0 en caso de éxito. -1 en caso de error.
 */
int serial_service_socket_connect(void)
{
    if (connect(s, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        fprintf(stderr,"ERROR connecting\r\n");
        close(s);
        s = -1;
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Lee nbytes de un file descriptor asociado a un socket.
 * 
 * @param newfd El file descriptor asociado a la conexión entrante.
 * @param recvBuff El buffer donde se leerán los datos.
 * @param nbytes La cantidad de bytes máxima a leer.
 * @return int La cantidad de bytes leídos.
 */
int serial_service_socket_read(int newfd, char *recvBuff, size_t nbytes)
{
    int n;

    if( (n = read(newfd, recvBuff, nbytes)) == -1 )
    {
        perror("Error leyendo mensaje en socket");
        return -1;
    }

    recvBuff[n]=0x00;   

    return n;
}

/**
 * @brief Envía un mensaja a un socket.
 * 
 * @param newfd El file descriptor asociado a la conexión saliente.
 * @param sendBuff El mensaje a enviar.
 * @param nbytes La cantidad de bytes del mensaje.
 * @return int La cantidad de bytes enviados.
 */
int serial_service_socket_send(int newfd, char *sendBuff, size_t nbytes)
{
    int n;

    if( (n = send(newfd, sendBuff, nbytes, MSG_NOSIGNAL)) == -1 )
    {
        perror("No se pudo enviar el mensaje");
        return -1;
    }   

    return n;
}

/**
 * @brief Cierra el socket.
 * 
 * @param fd El número de socket.
 * @return int 0 si tuvo éxito.
 */
int serial_service_socket_close(int fd)
{
    return(close(fd));
}

/**
 * @brief Cierra el socket principal.
 * 
 * @return int 0 si tuvo éxito.
 */
int serial_service_socket_end(void)
{
    return(close(s));
}

/**
 * @brief Consulta el file descriptor del socket.
 * 
 * @return int El file descriptor para el socket o -1 si no fue creado.
 */
int serial_service_socket_fd(void)
{
    return s;
}