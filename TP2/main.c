#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include "SerialManager.h"
#include "SerialServiceSocket.h"

#define PORT_CIAA               1
#define CIAA_BAUDRATE           115200

#define RCV_DELAY               1       // s
#define RETRY_DELAY             1       // s

#define LOCAL_IP                "127.0.0.1"
#define LOCAL_PORT              10000

#define OUTS_STR_EXAMPLE        ">OUTS:X,Y,W,Z\r\n"
#define OUTS_STR_FORMAT         ">OUTS:%c,%c,%c,%c\r\n"
#define KEY_EVENT_OUT           ":LINE%cTG\n"
#define KEY_DELIMITER           ':'
#define STATES_CMD_FORMAT       ":STATES%c%c%c%c"
#define TOGGLE_STATE_CMD_FORMAT ">TOGGLE STATE:%c"
#define OUTPUTS_QTY             4

#define MAX_OUT_STR             (strlen(OUTS_STR_EXAMPLE)+1)
#define MAX_RX_BUFF             64
#define MAX_TX_BUFF             64

#define SIGN_MSG                "Signal recibida!\n"

int lockSign(int sign);
int unlockSign(int sign);
void onFailure(pthread_t* socket_thread);

int newFd = -1;
bool activeSocket = false;
volatile sig_atomic_t got_sigint = 0, got_sigterm = 0;
pthread_mutex_t mutexActiveSocket = PTHREAD_MUTEX_INITIALIZER;

void sigint_handler(int sig)
{
    write(1, SIGN_MSG, strlen(SIGN_MSG));
    got_sigint = 1;
}

void sigterm_handler(int sig)
{
    write(1, SIGN_MSG, strlen(SIGN_MSG));
    got_sigterm = 1;
}

void *socket_service(void *parameter)
{
    char RxBuff[MAX_RX_BUFF];
    int n; 

    while ( serial_service_socket_init(LOCAL_IP, LOCAL_PORT) )
    {
        sleep(RETRY_DELAY);        // Se reintenta cada RETRY_DELAY segundos hasta que se pueda iniciar el socket
    }

    newFd = serial_service_socket_accept();  // Se espera a que se conecte un cliente y se guarda el file descriptor de la conexión
    pthread_mutex_lock (&mutexActiveSocket);    // Abro sección crítica
    activeSocket = true;
    pthread_mutex_unlock (&mutexActiveSocket);  // Cierro sección crítica

    while(true)
    {   
        if( (n = serial_service_socket_read(newFd, RxBuff, MAX_RX_BUFF)) < 1 )
        {
            pthread_mutex_lock (&mutexActiveSocket);    // Abro sección crítica
            activeSocket = false;
            pthread_mutex_unlock (&mutexActiveSocket);  // Cierro sección crítica
            serial_service_socket_close(newFd);
            
            while( (newFd = serial_service_socket_accept()) == -1)
            {
                sleep(RETRY_DELAY);  // Reintento cada RETRY_DELAY segundos hasta que aparezca una nueva petición de conexión
            }

            pthread_mutex_lock (&mutexActiveSocket);    // Abro sección crítica
            activeSocket = true;
            pthread_mutex_unlock (&mutexActiveSocket);  // Cierro sección crítica
        }
        else
        {
            printf("Se recibieron %d bytes: %s\r\n", n, RxBuff);

            // Se chequea que haya llegado correctamente el comando para setear las salidas
            // y luego se envia por el puerto serial.
            char out_str[MAX_OUT_STR];
            char stateX, stateY, stateW, stateZ;
            if (sscanf(RxBuff, STATES_CMD_FORMAT, &stateX, &stateY, &stateW, &stateZ) == OUTPUTS_QTY)
            {
                snprintf(out_str, MAX_OUT_STR, OUTS_STR_FORMAT, stateX, stateY, stateW, stateZ);                
                serial_send(out_str, MAX_OUT_STR);
            }    
            else fprintf(stderr, "Error al recibir comando!");                
        }
    }

    pthread_exit(NULL);
}

int main(void)
{
    char RxBuff[MAX_RX_BUFF], TxBuff[MAX_TX_BUFF];
    int n;
    pthread_t socket_thread;

    printf("Inicio Serial Service\r\n");

    struct sigaction sa_sigint, sa_sigterm;

    sa_sigint.sa_handler = sigint_handler;
    sa_sigint.sa_flags = 0;
    sigemptyset(&sa_sigint.sa_mask);
    sa_sigterm.sa_handler = sigterm_handler;
    sa_sigterm.sa_flags = 0;
    sigemptyset(&sa_sigterm.sa_mask);

    if(sigaction(SIGINT, &sa_sigint, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }    

    if(sigaction(SIGTERM, &sa_sigterm, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    } 

    // Se bloquean las señales para que el thread que se creará
    // las descarte ya que las manejará el thread principal
    if ( lockSign(SIGINT) || lockSign(SIGTERM))
    {
        perror("pthread_sigmask lock");
        exit(EXIT_FAILURE);
    }       

    // Se abre el puerto serial
    if( serial_open(PORT_CIAA, CIAA_BAUDRATE) )
    {
        // El error se indica en rs232.c
        exit(EXIT_FAILURE);
    }

    pthread_create(&socket_thread, NULL, socket_service, NULL);
    pthread_detach(socket_thread);
    
    // Se desbloquean las señales para el thread principal
    if ( unlockSign(SIGINT) || unlockSign(SIGTERM) )
    {
        perror("pthread_sigmask unlock");
        onFailure(&socket_thread);
        exit(EXIT_FAILURE);
    }  

    while(true)
    {
        if(got_sigint || got_sigterm)  // Si llegó la señal SIGINT o SIGTERM finalizo el programa
        {
            bool error_flag = false;
            if(pthread_cancel(socket_thread))
            {
                perror("Error al finalizar el thread");   
                error_flag = true;          
            }

            pthread_mutex_lock (&mutexActiveSocket);    // Abro sección crítica
            if (activeSocket)
            {
                if( serial_service_socket_close(newFd) )
                {
                    perror("Error al cerrar el socket");
                    error_flag = true;
                }
                else activeSocket = false;
            }         
            pthread_mutex_unlock (&mutexActiveSocket);  // Cierro sección crítica
            
            if( serial_service_socket_fd() > -1 )
            {
                if( serial_service_socket_end() )
                {
                    perror("Error al cerrar main socket");
                    error_flag = true;
                }                
            }

            serial_close();

            if(error_flag)
            {
                exit(EXIT_FAILURE);
            }

            printf("SerialService ha terminado!\r\n");
            exit(EXIT_SUCCESS);  
        }
        else if( ( n = serial_receive(RxBuff, MAX_RX_BUFF) ) == -1 )
        {
            perror("Error al recibir por el puerto serie");
            onFailure(&socket_thread);            
            exit(EXIT_FAILURE);
        }
        else if( n > 0 )
        {
            RxBuff[n] = '\0';
            //printf("Se recibieron %d bytes: %s\r\n", n, RxBuff);

            char keyN;
            // Se chequea que haya llegado el comando de cambiar de estado
            // y se envia el comando correspondiente por el socket
            if (sscanf(RxBuff, TOGGLE_STATE_CMD_FORMAT, &keyN) == sizeof(char))
            {                
                n = snprintf(TxBuff, MAX_TX_BUFF, KEY_EVENT_OUT, keyN);
                
                pthread_mutex_lock (&mutexActiveSocket);  // Abro sección crítica
                if(activeSocket)
                {
                    if (serial_service_socket_send(newFd, TxBuff, n) == n)
                    {
                        //printf("Enviado: %s\n", TxBuff);
                    }  
                    else printf("Error al enviar!\n");              
                }
                pthread_mutex_unlock (&mutexActiveSocket);  // Cierro sección crítica
            }                 
        }
        sleep(RCV_DELAY);  // Delay para permitir que se carguen correctamente los datos al buffer del puerto serial
    }

    exit(EXIT_SUCCESS);
    return 0;
}

/**
 * @brief Bloquea una señal.
 * 
 * @param sign La señal a bloquear.
 * @return int 0 en caso de éxito. -1 o errorno en caso de falla.
 */
int lockSign(int sign)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sign);
    return( pthread_sigmask(SIG_BLOCK, &set, NULL) );
}

/**
 * @brief Desbloquea una señal.
 * 
 * @param sign La señal a desbloquear.
 * @return int 0 en caso de éxito. -1 o errorno en caso de falla.
 */
int unlockSign(int sign)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sign);
    return( pthread_sigmask(SIG_UNBLOCK, &set, NULL) );
}

/**
 * @brief Ante una falla libera los recursos utilizados.
 * 
 * @param pthread Puntero al thread creado.
 */
void onFailure(pthread_t* pthread)
{
    pthread_mutex_lock (&mutexActiveSocket);    // Abro sección crítica
    if(activeSocket)
    {
        serial_service_socket_close(newFd);
    }
    pthread_mutex_unlock (&mutexActiveSocket);  // Cierro sección crítica

    serial_service_socket_end();
    pthread_cancel(*pthread);
    serial_close();    
}
