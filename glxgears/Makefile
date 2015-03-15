
CFLAGS=

LDFLAGS = -framework GLUT -framework OpenGL -framework Cocoa 
LDFPAGS+= -L"/System/Library/Frameworks/OpenGL.framework/Libraries"

all: glxgears

glxgears: glxgears.c
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@
