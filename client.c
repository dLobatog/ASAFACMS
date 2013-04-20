#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <wordexp.h>
#include <sysexits.h>
#include "common.h"
#include "client.h"


int debug = 0;

void usage(char *program_name)
{
	printf("Usage: %s [-d] -s <server> -p <port>\n", program_name);
}

void f_put (char *title, char *xLabel, char *yLabel, char *style, char *src, int sd)
{
  struct put petition;
	petition.code = PUT_CODE;

  strncpy(petition.title, title, 20);
  strncpy(petition.xLabel, xLabel, 20);
  strncpy(petition.yLabel, yLabel, 20);
  strncpy(petition.style,style, 20);
  strncpy(petition.src, src, 20);

  int bytes_written;
  if((bytes_written = write(sd, (char *) &petition, sizeof(struct put))) == -1) {
		perror("c> Error sending put to server.\n");
		return;
	}

  //Send plot data file to server
  FILE *file;
  if ((file = fopen(src, "rb")) == 0){ 
		perror("c> Error opening file.\n");
		return;
	} 

  fseek(file, 0L, SEEK_END);
  int size = ftell(file);
  fseek(file, 0L, SEEK_SET);
  char filesize[10];
  sprintf(filesize, "%d", size);
         
  printf("c> File size - %d\n", size); 

  if((bytes_written = send(sd, filesize, 10, 0)) == -1) {
    perror("s> Could not send file size to server.\n");
    fclose(file);
    return;
  }
  fclose(file);

  if ((sendfile(sd, open(src, 0), 0, size)) == -1){
    perror("c> Error sending file.\n");
    fclose(file);
		return; 
  }

  // Read ack
  char bytes[1];
	ssize_t bytes_read;
	if((bytes_read = read(sd, bytes, 1)) == -1) {
		perror("c> Error receiving data from server.\n");
		return;
	} else if(bytes_read == 0) {
		printf("c> Server has closed the connection.\n");
		close(sd);
		exit(EXIT_SUCCESS);
	} 
  if(bytes[0] == PUT_CODE) {
    printf("c> OK\n");
	} 

  return;      
}

void f_get(char *dst, int sd)
{
  struct get petition;
	petition.code = GET_CODE;
  strncpy(petition.file, dst, 30); 
  int bytes_written;
  if((bytes_written = write(sd, (char *) &petition, sizeof(struct put))) == -1) {
		perror("c> Error sending get to server.\n");
		return;
	}

  char size[10];
	ssize_t bytes_read;
	if((bytes_read = read(sd, size, 10)) == -1) {
		perror("c> Error receiving data from server.\n");
		return;
	} else if(bytes_read == 0) {
		printf("c> Server has closed the connection.\n");
		close(sd);
		exit(EXIT_SUCCESS);
	} 

  int filesize = atoi(size);
  printf("c> File size - %d\n", filesize);

  char file_data[atoi(size)];
	if((bytes_read = read(sd, file_data, filesize)) == -1) {
		perror("c> Error receiving data from server.\n");
		return;
	} else if(bytes_read == 0) {
		printf("c> Server has closed the connection.\n");
		close(sd);
		exit(EXIT_SUCCESS);
	} 

  FILE *file = fopen(dst, "wb");
  fwrite(file_data, filesize, 1, file);
  fclose(file);

  return; 
}

void shell(int sd)
{
	char line[1024];
	char *pch;
	int exit = 0;
	
	wordexp_t p;
	char **w;
	int ret;

	memset(&p, 0, sizeof(wordexp_t));

	do
	{
		fprintf(stdout, "c> ");
		memset(line, 0, 1024);
		pch = fgets(line, 1024, stdin);

		if ((strlen(line)>1) && ((line[strlen(line)-1]=='\n') || (line[strlen(line)-1]=='\r')))
			line[strlen(line) - 1] = '\0';

		ret = wordexp((const char *)line, &p, 0);
		if(ret==0)
		{
			w = p.we_wordv;

			if((w != NULL) && (p.we_wordc > 0))
			{
				if(strcmp(w[0], "quit") == 0)
				{
					exit = 1;
				}else if(strcmp(w[0], "put") == 0){
					if(p.we_wordc == 6){
						f_put(w[1], w[2], w[3], w[4], w[5], sd);
					}else{
						printf("Syntax error. Usage: put <title> <xLabel> <yLabel> <style> <file_path>\n");
					}
				
				}else if(strcmp(w[0], "get") == 0){
					if(p.we_wordc == 2){
						f_get(w[1], sd);
					}else{
						printf("Syntax error. Usage: get <file_path>\n");
					}
				}else{
					fprintf(stderr, "Error: command '%s' not valid.\n", w[0]);
				}
			}
			wordfree(&p);
		}

	}while ((pch != NULL) && (!exit));
}

int main(int argc, char *argv[])
{
	char *program_name = argv[0];
	int opt, port = 0;
	char *server, *port_s;

	while((opt = getopt(argc, argv, "ds:p:")) != -1) 
	{
		switch(opt)
		{
			case 'd':
				debug = 1;
				break;
			case 's':
				server = optarg;
				if(debug) printf("Server %s\n", server);
				break;
			case 'p':
				port_s = optarg;
				port = strtol(optarg, NULL, 10);
				if(debug) printf("Port %s\n", port_s);
				break;
			case '?':
				if((optopt == 's') || (optopt == 'p'))
					fprintf(stderr, "Option -%c requieres an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			default:
				usage(program_name);
				exit(EX_USAGE);
		}
	}

	if((port < 1024) || (port > 65535))
	{
		fprintf(stderr, "Error: Port must be in the range 1024 <= port <= 65535");
		usage(program_name);
		exit(EX_USAGE);
	}
	
	if(debug)
	{
		printf("SERVER: %s PORT: %d\n", server, port);
	}

	int sd;
 	struct sockaddr_in address;
	struct hostent *hp;
	hp = gethostbyname(server);
	if(hp == NULL) {
		address.sin_addr.s_addr = inet_addr(server);
	}
	else {
		memcpy(&address.sin_addr.s_addr,*(hp->h_addr_list),sizeof(address.sin_addr.s_addr));
	}
	address.sin_family = AF_INET;
	address.sin_port = htons(port);

	if((sd = socket(address.sin_family, SOCK_STREAM, TCP)) == -1) {
		printf("c> Error connecting to server %s:%d.\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
		return -1;
	}

	if(connect(sd, (struct sockaddr *) &address, sizeof(address)) == -1) {
		printf("c> Error connecting to server %s:%d.\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
		return -1;
	}

	shell(sd);
	exit(EXIT_SUCCESS);
}
