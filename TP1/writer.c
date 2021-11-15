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
    sa.sa_flags = SA_SIGINFO;//SA_RESTART; //
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

    /* Create named fifo. -1 means already exists so no action if already exists */
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1 )
    {
        printf("Error al crear la cola nombrada: %d\n", returnCode);
        exit(1);
    }

    /* Open named fifo. Blocks until other process opens it */
	printf("Esperando por lectores...\n");
	if ( (fd = open(FIFO_NAME, O_WRONLY) ) < 0 )
    {
        printf("Error abriendo el archivo de cola nombrada: %d\n", fd);
        exit(1);
    }
    
    /* open syscalls returned without error -> other process attached to named fifo */
	printf("Se ha conectado un lector, escribe algo...\n");

    /* Loop forever */
	while (1)
	{
        /* Get some text from console */
        snprintf(outputBuffer, BUFFER_SIZE, "%s", DATA_PREFIX);
        if( fgets(outputBuffer+BUFFER_OFFSET, BUFFER_SIZE, stdin) == NULL)
        {
            perror("fgets");
        }

        if(got_sigusr)
        {            
            sprintf(outputBuffer, "%s%d\n", SIGN_PREFIX, got_sigusr-SIGUSR1+1);
            got_sigusr = 0;
        }      

        /* Write buffer to named fifo. Strlen - 1 to avoid sending \n char */
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
