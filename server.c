#include <strings.h>
#include <unistd.h>
#include <sysexits.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include "common.h"
#include "server.h"

#define GNUPLOT_PATH "/usr/bin/gnuplot"

int debug = 0;
int busy = TRUE;

pthread_mutex_t mutex;
pthread_cond_t thread_ready;
 

int generate_graphics(char * title, char * xLabel, char * yLabel, char * style, char * file)
{
	FILE *gp;
	gp = popen(GNUPLOT_PATH, "w");
	if(gp == NULL)
	{
		fprintf(stderr, "We can't find %s", GNUPLOT_PATH);
		return -1;
	}
	fprintf(gp, "set title \"%s\" \n", title);
	fprintf(gp, "set term png \n");
	fprintf(gp, "set output \"%s.png\" \n", title);
	fprintf(gp, "set xlabel \"%s\" \n", xLabel);
	fprintf(gp, "set ylabel \"%s\" \n", yLabel);
	fprintf(gp, "plot \"%s\" with %s \n", file, style);
	fflush(gp);
	pclose(gp);
	return (0);
}

int run(int sc, char * ipClient, int port)
{
	return 0;
}

int main (int argc, char *argv[])
{
	int port, opt;
	char *port_s;
	
	while((opt = getopt(argc, argv, "dp:")) != -1)
	{
		switch (opt)
		{
			case 'd':
				debug = 1;
				break;
			case 'p':
				port_s= optarg;
				port = strtol(optarg, NULL, 10);
				break;
			case '?':
				if(optopt == 'p')
					fprintf(stderr, "Option -%c requires an anrgument.\n", optopt);
				else if(isprint(optopt))
					fprintf(stderr, "Unknown option -%c.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			default:
				printf("Usage: ./server [-d] -p <port>\n");
				exit(EX_USAGE);
		}	
	}

	if((port < 1024) || (port > 65535))
	{
		fprintf(stderr, "Error: Port must be in the range 1024 <= port <= 65535\n");
		printf("Usage: ./server [-d] -p <port>\n");
		exit(EX_USAGE);
	}

#ifdef DEBUG
	fprintf(stderr, "PORT %d\n", port);
	if(debug)
		fprintf(stderr, "DEBUG ON\n");
	else
		fprintf(stderr, "DEBUG OFF\n");
#endif

  int server_sd;
	struct sockaddr_in server_addr;
	if((server_sd = socket(AF_INET, SOCK_STREAM, TCP)) == -1) {
		perror("s> Server could not create socket.\n");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);
	if(bind(server_sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
		perror("s> Server cannot bind.\n");
		close(server_sd);
		return -1;
	}

	printf("s> init server %s:%d\n", inet_ntoa(server_addr.sin_addr), port);

	if(listen(server_sd, MAX_QUEUE_CONN) == -1) {
		perror("s> Error listening.\n");
		close(server_sd);
		return -1;
	}

	printf("s> waiting\n");

	pthread_t client_thread;
	pthread_attr_t client_attr;
	pthread_attr_init(&client_attr);
	pthread_attr_setdetachstate(&client_attr, PTHREAD_CREATE_DETACHED);

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&thread_ready, NULL);

	int client_sd;
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	while(TRUE) {
		if((client_sd = accept(server_sd, (struct sockaddr *)  &client_addr, &addr_len)) == -1) {
			perror("s> Cannot accept inbound connection.\n");
			continue;
		}

		int* arguments = malloc(sizeof(int) * 1);
		arguments[0] = client_sd;
		if(pthread_create(&client_thread, &client_attr, client_func, (void*) arguments) == -1) {
			perror("s> Cannot create thread for client.\n");
			continue;
		}

		pthread_mutex_lock(&mutex);
		while(busy == TRUE) {
			pthread_cond_wait(&thread_ready, &mutex);
		}
		pthread_mutex_unlock(&mutex);

		busy = TRUE;
	}

	close(server_sd);

	exit(0);
}

 
void* client_func(void* p) {
	pthread_mutex_lock(&mutex);
	int* arguments = (int*) p;
	int client_sd = arguments[0];
	busy = FALSE;
	pthread_cond_signal(&thread_ready);
	pthread_mutex_unlock(&mutex);

	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	getpeername(client_sd, (struct sockaddr*) &client_addr, &addr_len);

	printf("s> accept %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	char bytes[MSG_BUFFER];
	ssize_t bytes_read, bytes_written;
	while(TRUE) {
		if((bytes_read = read(client_sd, bytes, MSG_BUFFER)) == -1) {
			perror("s> Could not receive data from client.\n");
			break;
		} else if(bytes_read == 0) {
			printf("s> %s:%d quit\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
			close(client_sd);
			pthread_exit(EXIT_SUCCESS);
		}

		if(bytes[0] == PUT_CODE) {
      struct put *petition = (struct put *) &bytes;

      // Create file from received data

      // receive size
      char size[10];
      ssize_t bytes_read;
      if((bytes_read = read(client_sd, size, 10)) == -1) {
        perror("c> Error receiving data from client.\n");
        break;
      } else if(bytes_read == 0) {
        printf("c> Client has closed the connection.\n");
        close(client_sd);
        break;
      }                         

      int filesize = atoi(size);
      printf("c> File size - %d\n", filesize);

      //receive actual file data

      char file_data[filesize];
      if((bytes_read = read(client_sd, file_data, filesize)) == -1) {
        perror("c> Error receiving data from client.\n");
        break;
      } else if(bytes_read == 0) {
        printf("c> Client has closed the connection.\n");
        close(client_sd);
        break;
      } 

      //create file
      FILE *file = fopen(petition->src, "wb");
      fwrite(file_data, filesize, 1, file);
      fclose(file);

      // Create graphic
      generate_graphics(petition->title, 
                        petition->xLabel,
                        petition->yLabel,
                        petition->style,
                        petition->src);

      printf("s> %s:%d OK gnuplot %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), petition->title);
   
      // Send ACK to client.
      char byte[1];
      byte[0] = PUT_CODE;
			if((bytes_written = write(client_sd, byte, 1)) == -1) {
				perror("s> Could not send data to client.\n");
				break;
			} 

    } else if (bytes[0] == GET_CODE) {
      struct get *petition = (struct get *) &bytes;
      printf("s> %s:%d init get graphic %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), petition->file);

      //Open file
      FILE *file;
      char *file_data;
      if ((file = fopen(petition->file, "rb")) != 0){
        fseek(file, 0L, SEEK_END);
        int size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        //Allocate memory
        file_data=(char *)malloc(size+1);

        //Read file contents into buffer
        fread(file_data, size, 1, file);
        fclose(file);

        char filesize[10];
        sprintf(filesize, "%d", size);

        // SEND size
        if((bytes_written = send(client_sd, filesize, 10, 0)) == -1) {
          perror("s> Could not send file size to client.\n");
          break;
        }

        //SEND file
        if((bytes_written = sendfile(client_sd, open(petition->file, 0), 0, size)) == -1) {
          perror("s> Could not send data to client.\n");
          break;
        }

        printf("s> %s:%d end get graphic %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), petition->file); 
      } else {
        printf("s> %s:%d error sending file %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), petition->file);  
      }
     
    } 

		memset(bytes, 0, sizeof(bytes));
	}

	close(client_sd);
	pthread_exit(0);
} 
