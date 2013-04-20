#ifndef COMMON_H
#define COMMON_H 1

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TRUE 1
#define FALSE 0

#define TCP 6

#define MSG_BUFFER 101
#define PUT_CODE 100
#define GET_CODE 50

struct put {
  char code;
  char title[20]; 
  char xLabel[20]; 
  char yLabel[20]; 
  char style[20]; 
  char src[20]; 
};

struct get {
  char code;
  char file[30]; 
};
  
#endif
