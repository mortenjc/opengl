


CFLAGS=

LDFLAGS = -framework GLUT -framework OpenGL -framework Cocoa 
LDFPAGS+= -L"/System/Library/Frameworks/OpenGL.framework/Libraries"

all: example

example: main.c
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@
