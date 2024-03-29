libdir.x86_64 := $(shell if [ -d "/usr/lib/x86_64-linux-gnu" ]; then echo "/usr/lib/x86_64-linux-gnu"; else echo "/usr/lib64"; fi )
libdir.i686   := $(shell if [ -d "/usr/lib/i386-linux-gnu" ]; then echo "/usr/lib/i386-linux-gnu"; else echo "/usr/lib"; fi )
libdir.arm    := $(shell if [ -d "/usr/lib/arm-linux-gnueabihf" ]; then echo "/usr/lib/arm-linux-gnueabihf"; else echo "/usr/lib"; fi )
libdir.macos  := /usr/local/lib

ISNOTMACOS := $(shell uname -a | grep "Darwin" >/dev/null ; echo $$? )

ifeq ($(ISNOTMACOS), 0)
	MACHINE := macos
	CFLAGS := -bundle
else
	MACHINE := $(shell uname -m)
	ifneq (, $(findstring armv, $(MACHINE)))
		 MACHINE := arm
	endif
	
	CFLAGS := -shared
	
	ifdef MINGW_PACKAGE_PREFIX
		CFLAGS := $(CFLAGS) -lucrtbase
	endif
endif

libdir = $(libdir.$(MACHINE))/geany

all: build

build:
	gcc -DLOCALEDIR=\"\" -DGETTEXT_PACKAGE=\"zhgzhg\" -c ./geany_unix_timestamp_converter.c -std=c99 -fPIC `pkg-config --cflags geany`
	gcc geany_unix_timestamp_converter.o -o unixtsconverter.so $(CFLAGS) `pkg-config --libs geany`

install: globaluninstall globalinstall localuninstall

uninstall: globaluninstall

globaluninstall:
	rm -f "$(libdir)/unixtsconverter.so"

localuninstall:
	rm -f "$(HOME)/.config/geany/plugins/unixtsconverter.so"

globalinstall:
	cp -f ./unixtsconverter.so "$(libdir)/unixtsconverter.so"
	chmod 755 "$(libdir)/unixtsconverter.so"

localinstall: localuninstall
	mkdir -p "$(HOME)/.config/geany/plugins"
	cp -f ./unixtsconverter.so "$(HOME)/.config/geany/plugins/unixtsconverter.so"
	chmod 755 "$(HOME)/.config/geany/plugins/unixtsconverter.so"

clean:
	rm -f ./unixtsconverter.so
	rm -f ./geany_unix_timestamp_converter.o
