#include <stdlib.h>

#ifdef MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <3dcomponents.h>
#include <nr3.h>


void plotBargraph3D(NRmatrix <double> data)
{
	int x,y;
	glPushMatrix();
	for (y=0; y<data.ncols(); y++)
	{
		for (x=0; x<data.nrows(); x++)
		{
			glTranslatef(1.0, 0, 0);
			comp3dMakeBox(0.5, data[x][y]);
		}
		glTranslatef(-1.0*data.nrows(), 0, 0);
		glTranslatef(0, 1.0, 0);
	}
	glPopMatrix();
}
