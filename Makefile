libdir.x86_64 := $(shell if [ -d "/usr/lib/x86_64-linux-gnu" ]; then echo "/usr/lib/x86_64-linux-gnu"; else echo "/usr/lib64"; fi )
libdir.i686   := $(shell if [ -d "/usr/lib/i386-linux-gnu" ]; then echo "/usr/lib/i386-linux-gnu"; else echo "/usr/lib"; fi )


MACHINE := $(shell uname -m)

libdir = $(libdir.$(MACHINE))

all: build

build:
	gcc -DLOCALEDIR=\"\" -DGETTEXT_PACKAGE=\"zhgzhg\" -c ./geany_unix_timestamp_converter.c -fPIC `pkg-config --cflags geany`
	gcc geany_unix_timestamp_converter.o -o unixtsconverter.so -shared `pkg-config --libs geany`

install: uninstall startinstall

startinstall:
	cp -f ./unixtsconverter.so $(libdir)/geany
	chmod 755 $(libdir)/geany/unixtsconverter.so

uninstall:
	rm -f $(libdir)/geany/unixtsconverter.so

clean:
	rm -f ./unixtsconverter.so
	rm -f ./geany_unix_timestamp_converter.o
