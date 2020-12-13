#
# cebsocket is a lightweight WebSocket library for C
#
# https://github.com/rohanrhu/cebsocket
#
# Licensed under MIT
# Copyright (C) 2020, Oğuzhan Eroğlu (https://oguzhaneroglu.com/) <rohanrhu2@gmail.com>
#

CC = gcc
CL = ld

CFLAGS = -std=c99 \
		 -I. \
		 -g \
		 -lssl \
		 -lcrypto \
		 -lpthread \
		 -lm

SOURCES = $(shell find . -wholename "./src/*.c")
HEADERS = $(shell find . -wholename "./include/*.h")
OBJECTS = $(notdir $(SOURCES:.c=.o))
RM = rm -rf

.PHONY: clean

all: cebsocket.o

util.o:
	$(CC) -c -o $@ src/util.c $(CFLAGS)

cebsocket.o: util.o
	$(CC) -c -o _cebsocket.o src/cebsocket.c $(CFLAGS)
	$(CL) -r util.o _cebsocket.o -o cebsocket.o
	$(RM) _cebsocket.o
	$(RM) util.o

clean:
	make clean -C examples/hello
	$(RM) $(OBJECTS) _cebsocket.o