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

CEBSOCKET_SOURCES = $(filter-out $(shell find ../../ -path "*/examples/*"), $(shell find ../../ -name "*.c"))
CEBSOCKET_HEADERS = $(filter-out $(shell find ../../ -path "*/examples/*"), $(shell find ../../ -name "*.h"))
CEBSOCKET_OBJ = ../../cebsocket.o
SOURCE = chat.c
EXECUTABLE = $(SOURCE:.c=)

.PHONY: clean cebsocket

all: $(EXECUTABLE)

cebsocket:
	make -C ../../

clean:
	$(RM) $(EXECUTABLE)

$(EXECUTABLE): cebsocket $(CEBSOCKET_OBJ)
	$(CC) -o $@ $(SOURCE) $(CEBSOCKET_OBJ) $(CFLAGS)