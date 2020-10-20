# Makefile for CPP frame extraction

OBJS=frame_extractor.o
BIN=g++ frame_extraction.cpp

# OBJSTWO=frame_viewer.o
# BINTWO=g++ frame_viewer.cpp

BINTHREE=g++ test.cpp
OBJSTHREE=test.o

# CFLAGS+=-Wfatal-errors
# CFLAGS+=-Wall -g -O3
CFLAGS+=-std=c++17
LDFLAGS+=-L/opt/vc/lib/ -lpthread
LDFLAGS+=-L/usr/local/lib/ -lopencv_ximgproc
LDFLAGS+=`pkg-config --cflags --libs opencv4`

RPATH+=-Wl,-rpath=/usr/local/lib/libopencv_ximgproc.so.4.4

INCLUDES+=-I/opt/vc/include/
INCLUDES+=-I/opt/vc/include/interface/vcos/pthreads
INCLUDES+=-I/opt/vc/include/interface/vmcs_host/linux

all:
	@rm -f $(OBJS)
	$(BIN) $(CFLAGS) $(LDFLAGS) $(RPATH) $(INCLUDES) -o $(OBJS)
# 	@rm -f $(OBJSTWO)
# 	$(BINTWO) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJSTWO)
	@rm -f $(OBJSTHREE)
	$(BINTHREE) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJSTHREE)
