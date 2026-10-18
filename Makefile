
TARGET = DarkBot
BUILD_PRX = 1

PSP_EBOOT_TITLE = Dark Bot
EXTRA_TARGETS = EBOOT.PBP

OBJS = \
	src/main.o \
	src/png_loader.o

INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =

LIBS = \
	-lpspgu \
	-lpspgum \
	-lpspdisplay \
	-lpspctrl \
	-lpspkernel \
	-lpng \
	-lz \
	-lm

PSPSDK = $(shell psp-config --pspsdk-path)

include $(PSPSDK)/lib/build.mak
