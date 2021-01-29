CFLAGS+=-g -Wall
INCLUDES+=-I./
LDFLAGS+=-pthread
CC=gcc

all: mserver

mserver: mserver.c shared.h
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $< -o $@

mclient: mclient.c shared.h
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $< -o $@

clean:
	rm -f mclient
	rm -f mserver

all: mclient mserver

