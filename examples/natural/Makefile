# overrride with: make "CFLAGS = <whatever>" <target>
SHELL = /bin/sh
CFLAGS = -g -I../util -I/usr/include/GL -Wall
LIBS = -L/usr/lib -lglut -lX11 -lm

PROGS = particle water

all: $(PROGS)

.c:	../util/texture.h ../util/texture.c
	cc $(CFLAGS) -o $@ $< ../util/texture.c $(LIBS)

campfire: campfire.c ../util/texture.h ../util/texture.c d.c sm.c
	cc $(CFLAGS) -o $@ $@.c d.c sm.c ../util/texture.c $(LIBS)

clean:
	- rm -f *.o
	@for file in $(PROGS) dummy_file ; do                 \
		if [ -x $$file ]; then rm -f $$file; fi;       \
	done

