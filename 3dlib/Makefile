
INCLUDE = -I. -I../nr
LIB=lib3dlib.a

MAC=y
ifdef MAC
  CFLAGS     += -DMAC=mac -Wno-deprecated-declarations
  LDFLAGS     = -framework GLUT -framework OpenGL -framework Cocoa 
  LDFLAGS    += -L"/System/Library/Frameworks/OpenGL.framework/Libraries"
else
  INCLUDE    += -I/usr/X11/include
  LDFLAGS     = -L/usr/X11/lib -lglut -lGLU -lGL -L../3dlib -l3dlib
endif


CFLAGS += $(INCLUDE)


SOURCES = 3dcomponents.cpp 3dgraphs.cpp imageloader.cpp

OBJECTS = $(SOURCES:%.cpp=%.o)

all: $(OBJECTS)
	ar -r $(LIB) $(OBJECTS) 

%.o: %.cpp
	g++ -c $< $(CFLAGS) -o $@

clean:
	rm -fr $(OBJECTS) $(LIB)
