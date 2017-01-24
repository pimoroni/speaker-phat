all:
	gcc -c devices/speaker-phat.c -lwiringPi -Wall -O2
	gcc -c devices/blinkt.c -lwiringPi -Wall -O2
	gcc -c phatmeter.c -fPIC -o phatmeter.o -Wall -O2
	gcc -shared phatmeter.o blinkt.o speaker-phat.o -lwiringPi -Wl,-soname -Wl,libphatmeter.so.0 -o libphatmeter.so.0.0.0

install:
	cp libphatmeter.so.0.0.0 /usr/local/lib/
	ln -s /usr/local/lib/libphatmeter.so.0.0.0 /usr/local/lib/libphatmeter.so.0
	ln -s /usr/local/lib/libphatmeter.so.0.0.0 /usr/local/lib/libphatmeter.so

clean:
	rm phatmeter.o
	rm libphatmeter.so.0.0.0
