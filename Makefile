# Makefile for CPP frame extraction

OBJS=frame_extractor.o
BIN=g++ frame_extraction.cpp
# BIN=g++ test.cpp

CFLAGS+=-Wall -g -O3
LDFLAGS+=-L/opt/vc/lib/ -lpthread
LDFLAGS+=`pkg-config --cflags --libs opencv`

INCLUDES+=-I/opt/vc/include/
INCLUDES+=-I/opt/vc/include/interface/vcos/pthreads
INCLUDES+=-I/opt/vc/include/interface/vmcs_host/linux

all:
	@rm -f $(OBJS)
	$(BIN) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJS)
