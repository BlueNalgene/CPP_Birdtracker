# Makefile for CPP frame extraction

OBJS=frame_extractor.o
BIN=g++ frame_extraction.cpp
# BIN=g++ test.cpp

OBJSTWO=frame_viewer.o
BINTWO=g++ frame_viewer.cpp

# CFLAGS+=-Wall -g -O3
LDFLAGS+=-L/opt/vc/lib/ -lpthread
LDFLAGS+=`pkg-config --cflags --libs opencv`

INCLUDES+=-I/opt/vc/include/
INCLUDES+=-I/opt/vc/include/interface/vcos/pthreads
INCLUDES+=-I/opt/vc/include/interface/vmcs_host/linux

all:
	@rm -f $(OBJS)
	$(BIN) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJS)
	@rm -f $(OBJSTWO)
	$(BINTWO) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJSTWO)
