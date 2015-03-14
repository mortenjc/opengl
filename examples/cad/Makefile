# overrride with: make "CFLAGS = <whatever>" <target>
SHELL = /bin/sh
CFLAGS = -g -I../util -I/usr/include/GL -Wall
#LIBS = -L/usr/X11R6/lib -lglut -lGLU -lGL -lXmu -lXt -lX11 -lXi -lm
LIBS = -lX11 -lm -L/usr/lib -lglut

PROGS = capping cutaway locate

all: $(PROGS)

.c:	../util/texture.h ../util/texture.c
	cc $(CFLAGS) -o $@ $< ../util/texture.c $(LIBS)

hiddenline	\
lineaa 		\
shadows		\
solid_to_line:		common.o callbacks.o fileread.o
	cc $(CFLAGS) $@.c -o $@ common.o callbacks.o fileread.o $(LIBS)

frustum_z:	frustum_model.o frustum_view.o common.o
		cc $(CFLAGS) $@.c -o $@ common.o frustum_model.o frustum_view.o $(LIBS)

clean:
	- rm -f *.o
	@for file in $(PROGS) dummy_file ; do                 \
		if [ -x $$file ]; then rm -f $$file; fi;       \
	done

