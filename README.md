# ASAFACMS

## A Simple Attempt For A Concurrent Multithreaded Server

Drawing from Rick Hickey's talk "Simple Made Easy"*, **ASAFACMS** is an example of a straightforward connection-oriented concurrent multithreaded server and two sample clients in C and Java. 

The way the server works is extremely simple:

*   Opens a socket at the specified port
*   Whenever a client connects to the port, a thread is created and associated with this client.
*   The thread continuously loops sending and receiving data from the client
*   Meanwhile, if another client connects to the server, a new thread would be associated with it.

Your own request types can be easily created using a new struct on common.h. Example:

```c
struct put {
  char code;
  char title[20]; 
  char xLabel[20]; 
  char yLabel[20]; 
  char style[20]; 
  char src[20]; 
};
```

Its format need to contain at least a code so that the server understands the request. As an example, *get*'s code is 50.

```c
#define GET_CODE 50 // in common.h
```

The number of clients depends on the number of threads the operating system can assign to the server. if too many incoming connections happen at a point in time, they are queued up until a certain number is reached (MAX_QUEUE_CONN on server.h).

\* Watch the talk. If you don't have time for that, I meant "simple" as in the etymological meaning of it, "simplex" -&gt; "one thing, only one way of doing it..".  

### Clients and sample usage

I included two clients in C and Java that do the exact same thing. When you run them, a shell to connect to the server is opened, and you can issue commands to it. This example is simply meant to show how a client would work. There are two possible commands you can run, **get** and **put**. 

**get** pulls any file from the server to the directory where the client is running. 

**put** allows you to send a file with gnuplot data to generate a plot on the server.

The server includes get and put support by default, so that any client will open a thread function that allows you to do at least this. It is as simple as adding your own custom functions to do whatever you want on the server. 

The provided Makefile will compile the server and both clients. 
```
$> make  <-- compiles the source
$> ./server -p <port> <-- opens the server at port <port>
$> ./client -s <hostname> -p <port> <-- connects the C client to the server
$> java -cp . client -h <hostname> -p <port>  <-- connects the Java client to the server 
```

### License

See LICENSE. A copy of the Do What the Fuck You Want to Public License is attached to this project and you should take it seriously.
 
Disclaimer: Java's client uses GNU's GetOpt library which is GPL licensed. If you  create any derivative work including it, it must comply the terms of that license as well
