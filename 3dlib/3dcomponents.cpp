#include <math.h>
#include <string.h>

#ifdef MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glu.h>
#endif

void outputText(float x, float y, float z,  char *string)
{
  int len, i;
  glRasterPos3f(x, y, z);
  len = (int) strlen(string);
  for (i = 0; i < len; i++)
  {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
  }
}


// Various defines
#define PI  (3.1415926)
#define PI2 (2*PI)
#define PID2 (PI/2.0)


#define BOXLIST 128
void comp3dMakeBox(GLfloat width, GLfloat height)
{
	static int compiled = 0;
	GLfloat offset;
	offset = width/2;
	
	if (compiled == 0)
	{
		glNewList(BOXLIST, GL_COMPILE);
		glBegin(GL_QUADS);
			if (height != 0)
			{
				glNormal3f(0, -1, 0);
				glVertex3d(-offset, -offset, 0);
				glVertex3d(-offset, -offset, height);
				glVertex3d( offset, -offset, height);
				glVertex3d( offset, -offset, 0);

				glNormal3f(1, 0, 0);
				glVertex3d( offset, -offset, 0);
				glVertex3d( offset, -offset, height);
				glVertex3d( offset,  offset, height);
				glVertex3d( offset,  offset, 0);

				glNormal3f(-1, 0, 0);
				glVertex3d(-offset, -offset, 0);
				glVertex3d(-offset, -offset, height);
				glVertex3d(-offset,  offset, height);
				glVertex3d(-offset,  offset, 0);

				glNormal3f(0, 1, 0);
				glVertex3d(-offset,  offset, 0);
				glVertex3d(-offset,  offset, height);
				glVertex3d( offset,  offset, height);
				glVertex3d( offset,  offset, 0);
			}

			glNormal3f(0, 0, 1);
			glVertex3d(-offset, -offset, height);
			glVertex3d(-offset,  offset, height);
			glVertex3d( offset,  offset, height);
			glVertex3d( offset, -offset, height);
		glEnd();
		glEndList();
		compiled= 1;
	} else
	{
		glCallList(BOXLIST);
	}
}



typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
} XYZ;


// Create a sphere based on the current texture
// with radius r, detail n and style (point, line, solid) style
void comp3dCreateSphere(double r,int n, int style)
{
   int i,j;
   double theta1,theta2,theta3;
   XYZ e,p;

   glPolygonMode(GL_FRONT, style);
   glPolygonMode(GL_BACK,  style);

   for (j=0;j<n/2;j++) {
      theta1 = j * PI2 / n - PID2;
      theta2 = (j + 1) * PI2 / n - PID2;

      glBegin(GL_QUAD_STRIP);
      for (i=0;i<=n;i++) {
         theta3 = i * PI2 / n;

         e.x = cos(theta2) * cos(theta3);
         e.y = sin(theta2);
         e.z = cos(theta2) * sin(theta3);
         p.x = r * e.x;
         p.y = r * e.y;
         p.z = r * e.z;

         glNormal3f(e.x,e.y,e.z);
         glTexCoord2f(i/(double)n,2*(j+1)/(double)n);
         glVertex3f(p.x,p.y,p.z);

         e.x = cos(theta1) * cos(theta3);
         e.y = sin(theta1);
         e.z = cos(theta1) * sin(theta3);
         p.x = r * e.x;
         p.y = r * e.y;
         p.z = r * e.z;

         glNormal3f(e.x,e.y,e.z);
         glTexCoord2f(i/(double)n,2*j/(double)n);
         glVertex3f(p.x,p.y,p.z);
      }
      glEnd();
   }
}
