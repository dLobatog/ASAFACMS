.SUFFIXES: .java .class 
C_BINARIES=client server
JAVA_BINARIES=Client.class
BINARIES=$(C_BINARIES) $(JAVA_BINARIES)

CC=gcc
JC=javac -cp .

CPPFLAGS=-DDEBUG
CFLAGS=-Wall -g -O0

LDLIBS=-lpthread
LDFLAGS=-lpthread


all: $(BINARIES)

.java.class:
	$(JC) $<

clean:
	rm -f $(BINARIES)
