
TARGET = boot-001
BUILD_PRX = 1

OBJS = src/main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions
ASFLAGS = $(CFLAGS)

LIBS = -lpspgu -lpspdisplay -lpspkernel

EXTRA_TARGETS = EBOOT.PBP

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
