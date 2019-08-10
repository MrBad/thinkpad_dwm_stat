INCLUDE=.
LIBS=-lX11
CC=gcc
OFLAGS=-c
CFLAGS=-g -Wall -Wextra -std=c99 -pedantic-errors -I$(INCLUDE)

TARGET=tkstat
OBJECTS=tkstat.o

all: $(OBJECTS) Makefile
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

%o: %.c Makefile *.h
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ $<

install:
	cp $(TARGET) /usr/local/bin/

clean:
	rm $(OBJECTS) $(TARGET)

