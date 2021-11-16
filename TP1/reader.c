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
    
    /* Se crea la cola nombrada. -1 significa que ya existe por lo que no requiere realizar accion alguna */
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1  )
    {
        printf("Error al crear la cola nombrada: %d\n", returnCode);
        exit(1);
    }
    
    /* Se abre el archivo de cola nombrada. El programa se bloquea hasta que otro proceso realice su apertura */
	printf("Esperando por escritores...\n");
	if ( (fd = open(FIFO_NAME, O_RDONLY) ) < 0 )
    {
        printf("Error abriendo el archivo de cola nombrada: %d\n", fd);
        exit(1);
    }
    
    /* La syscall de apertura retorno sin error -> hay otro proceso vinculado a la cola nombrada */
	printf("Se obtuvo un escritor.\n");

    /* Loop hasta que la syscall devuelva un valor <= 0 */
	do
	{
        /* se leen los datos en un buffer local */
		if ((bytesRead = read(fd, inputBuffer, BUFFER_SIZE)) == -1)
        {
			perror("read");
        }
        else
		{
			inputBuffer[bytesRead] = '\0';
			printf("reader: read %d bytes: \"%s\"\n", bytesRead, inputBuffer);

            /* Se parsean los datos recibidos de acuerdo al prefijo */
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
