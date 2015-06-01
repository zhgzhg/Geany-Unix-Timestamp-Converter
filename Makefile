all: build

build:
	gcc -DLOCALEDIR=\"\" -DGETTEXT_PACKAGE=\"zhgzhg\" -c ./geany_unix_timestamp_converter.c -fPIC `pkg-config --cflags geany`
	gcc geany_unix_timestamp_converter.o -o unixtsconverter.so -shared `pkg-config --libs geany`

install: uninstall startinstall

startinstall:
	cp -f ./unixtsconverter.so /usr/lib/geany
	chmod 755 /usr/lib/geany/unixtsconverter.so

uninstall:
	rm -f /usr/lib/geany/unixtsconverter.so

clean:
	rm -f ./unixtsconverter.so
	rm -f ./geany_unix_timestamp_converter.o
