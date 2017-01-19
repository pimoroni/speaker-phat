all:
	gcc -c phatmeter.c -fPIC -o phatmeter.o -lwiringPi -Wall
	gcc -shared phatmeter.o -lwiringPi -Wl,-soname -Wl,libphatmeter.so.0 -o libphatmeter.so.0.0.0

install:
	cp libphatmeter.so.0.0.0 /usr/share/lib/
	ln -s /usr/local/lib/libphatmeter.so.0.0.0 /usr/share/libphatmeter.so.0
	ln -s /usr/local/lib/libphatmeter.so.0.0.0 /usr/share/libphatmeter.so
