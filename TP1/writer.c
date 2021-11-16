#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#define FIFO_NAME           "myfifo"
#define BUFFER_SIZE         300
#define DATA_PREFIX         "DATA:"
#define BUFFER_OFFSET       strlen(DATA_PREFIX)
#define SIGN_PREFIX         "SIGN:"
#define SIGN_MSG            "Signal recibida!\n"

volatile sig_atomic_t got_sigusr = 0;

void sigusr_handler(int sig)
{
    write(1, SIGN_MSG, strlen(SIGN_MSG));
    got_sigusr = sig;
}

int main(void)
{
    char outputBuffer[BUFFER_SIZE] = {DATA_PREFIX};
	uint32_t bytesWrote;
	int32_t returnCode, fd;

    struct sigaction sa;

    sa.sa_handler = sigusr_handler;
    sa.sa_flags = SA_SIGINFO;
    //sygemptyset(&sa.sa_mask);

    if(sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    if(sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }    

    /* Se crea la cola nombrada. -1 significa que ya existe por lo que no requiere realizar accion alguna */
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1 )
    {
        printf("Error al crear la cola nombrada: %d\n", returnCode);
        exit(1);
    }

    /* Se abre el archivo de cola nombrada. El programa se bloquea hasta que otro proceso realice su apertura */
	printf("Esperando por lectores...\n");
	if ( (fd = open(FIFO_NAME, O_WRONLY) ) < 0 )
    {
        printf("Error abriendo el archivo de cola nombrada: %d\n", fd);
        exit(1);
    }
    
    /* La syscall de apertura retorno sin error -> hay otro proceso vinculado a la cola nombrada */
	printf("Se ha conectado un lector, escribe algo...\n");

    /* Loop principal */
	while (1)
	{
        snprintf(outputBuffer, BUFFER_SIZE, "%s", DATA_PREFIX);     /* Se agrega el prefijo para datos ingresados por el usuario */
        if( fgets(outputBuffer+BUFFER_OFFSET, BUFFER_SIZE, stdin) == NULL) /* Se obtienen los datos ingresados por un usuario desde la consola */
        {
            perror("fgets");
        }

        if(got_sigusr)  /* Si se envio una señal de usuario, se agrega el prefijo de señal al buffer de salida */
        {            
            sprintf(outputBuffer, "%s%d\n", SIGN_PREFIX, got_sigusr-SIGUSR1+1);
            got_sigusr = 0;
        }      

        /* Se escribe el buffer a la cola nombrada. Strlen - 1 para evitar enviar el caracter \n */
		if ((bytesWrote = write(fd, outputBuffer, strlen(outputBuffer)-1)) == -1)
        {
			perror("write");
        }
        else
        {
			printf("Writer: wrote %d bytes, %s", bytesWrote, outputBuffer);
        }
	}
	return 0;
}
