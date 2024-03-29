-include Makefile.user

DXROOT ?= $(HOME)/sdks/directx/dx7

ifeq (0,$(RELEASE))
 OPTLEVEL=-O0 -g
else
 OPTLEVEL=-W -Wall -Wimplicit -Wno-char-subscripts -Wno-unused -O2 -pipe \
	-fomit-frame-pointer -ffunction-sections -ffast-math -fsingle-precision-constant -G0 -mbranch-likely \
	-fno-pic -funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS 
endif

DXROOT ?= $(HOME)/sdks/directx/dx7

CC=$(CROSS_COMPILE)gcc
CFLAGS=$(OPTLEVEL) -Wall
CPPFLAGS=-Iinclude -Isrc

SOURCES=src/drivers.c \
        src/fx_man.c \
        src/cd.c \
        src/multivoc.c \
        src/mix.c \
        src/mixst.c \
        src/pitch.c \
        src/vorbis.c \
        src/music.c \
        src/midi.c \
        src/driver_nosound.c \
        src/asssys.c
		
include Makefile.shared

ifeq (1,$(JFAUDIOLIB_HAVE_SDL))
 CPPFLAGS+= -DHAVE_VORBIS $(shell pkg-config --cflags vorbisfile)
endif

ifneq (,$(findstring MINGW,$(shell uname -s)))
 CPPFLAGS+= -I$(DXROOT)/include -Ithird-party/mingw32/include
 SOURCES+= src/driver_directsound.c src/driver_winmm.c
else
 ifeq (1,$(JFAUDIOLIB_HAVE_SDL))
  CPPFLAGS+= -DHAVE_SDL $(shell pkg-config --cflags sdl)
  CPPFLAGS+= -DUSE_WILDMIDI
  ifeq (1,$(JFAUDIOLIB_USE_SDLMIXER))
   CPPFLAGS+= -DUSE_SDLMIXER
   SOURCES+= src/driver_sdlmixer.c
  else
   SOURCES+= src/driver_sdl.c
  endif
 endif
 ifeq (1,$(JFAUDIOLIB_HAVE_ALSA))
   CPPFLAGS+= -DHAVE_ALSA $(shell pkg-config --cflags alsa)
   SOURCES+= src/driver_alsa.c
 endif
 ifeq (1,$(JFAUDIOLIB_HAVE_FLUIDSYNTH))
  CPPFLAGS+= -DHAVE_FLUIDSYNTH $(shell pkg-config --cflags fluidsynth)
  SOURCES+= src/driver_fluidsynth.c
 endif
endif

OBJECTS=$(SOURCES:%.c=%.o)

$(JFAUDIOLIB): $(OBJECTS)
	ar cr $@ $^

$(OBJECTS): %.o: %.c
	$(CC) -c -flto $(CPPFLAGS) $(CFLAGS) $< -o $@
 
.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(JFAUDIOLIB)
