

PROG=solarsystem
INCLUDE= -I../nr -I../3dlib

MAC=y
ifdef MAC
  CFLAGS     += -DMAC=mac -Wno-deprecated-declarations
  LDFLAGS     = -framework GLUT -framework OpenGL -framework Cocoa 
  LDFLAGS    += -L"/System/Library/Frameworks/OpenGL.framework/Libraries"
else
  INCLUDE    += -I/usr/X11/include
  LDFLAGS     = -L/usr/X11/lib -lglut -lGLU -lGL -L../3dlib -l3dlib
endif

  LDFLAGS += -L../3dlib -l3dlib

CFLAGS += $(INCLUDE)

all:
	g++ *.cpp $(CFLAGS) $(LDFLAGS) -o $(PROG)

clean:
	rm -f $(PROG)
