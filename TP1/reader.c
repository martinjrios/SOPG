#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define FIFO_NAME           "myfifo"
#define SIGN_FILE_NAME      "Sign.txt"
#define DATA_FILE_NAME      "Log.txt"
#define BUFFER_SIZE         300
#define ERROR_STR_SIZE      64

#define DATA_PREFIX         "DATA:"
#define SIGN_PREFIX         "SIGN:"

int main(void)
{
	char inputBuffer[BUFFER_SIZE], errorStr[ERROR_STR_SIZE];
	int32_t bytesRead, returnCode, fd;
    FILE *pDF, *pSF;
    
    /* Create named fifo. -1 means already exists so no action if already exists */
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1  )
    {
        printf("Error al crear la cola nombrada: %d\n", returnCode);
        exit(1);
    }
    
    /* Open named fifo. Blocks until other process opens it */
	printf("Esperando por escritores...\n");
	if ( (fd = open(FIFO_NAME, O_RDONLY) ) < 0 )
    {
        printf("Error abriendo el archivo de cola nombrada: %d\n", fd);
        exit(1);
    }
    
    /* open syscalls returned without error -> other process attached to named fifo */
	printf("Se obtuvo un escritor.\n");

    /* Loop until read syscall returns a value <= 0 */
	do
	{
        /* read data into local buffer */
		if ((bytesRead = read(fd, inputBuffer, BUFFER_SIZE)) == -1)
        {
			perror("read");
        }
        else
		{
			inputBuffer[bytesRead] = '\0';
			printf("reader: read %d bytes: \"%s\"\n", bytesRead, inputBuffer);

            if(!strncmp(inputBuffer, DATA_PREFIX, strlen(DATA_PREFIX)))
            {            
                pDF = fopen( DATA_FILE_NAME, "a" );
                if ( pDF == NULL )
                {
                    strcpy(errorStr, "Error al abrir el archivo: ");
                    perror(strcat(errorStr, DATA_FILE_NAME));
                    exit(1);
                } 
                fprintf(pDF, "%s\n", inputBuffer);
                fclose(pDF);
            }

            else if(!strncmp(inputBuffer, SIGN_PREFIX, strlen(SIGN_PREFIX)))
            {            
                pSF = fopen( SIGN_FILE_NAME, "a" );
                if ( pSF == NULL )
                {
                    strcpy(errorStr, "Error al abrir el archivo: ");
                    perror(strcat(errorStr, SIGN_FILE_NAME));
                    exit(1);
                } 
                fprintf(pSF, "%s\n", inputBuffer);
                fclose(pSF);
            }            
		}
	}
	while (bytesRead > 0);

	return 0;
}
