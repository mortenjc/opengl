#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

#include <3dcomponents.h>
#include "nr3.h"
#include <3dgraphs.h>

void drawScene();

// Controlling variables for global viewing options
int pan=-16;      // rotate around y axis
int tilt=-30;     // rotate around x axis
int min_height=1; // height filter
int color=0;	  // color vs. grey
int zoom=-90;     // z-coordinate zoom
int idlecount=0;  // simple time counter
int mod=0;        // cos modulate amplitudes
int draw_text =0; // To draw line numbers or not

NRmatrix <double> test(20, 10);

struct box_t {
   int  col[100][100];
   int  box[100][100];
   char txt[100][100][10];
} box;

GLfloat colors[5][3] ={{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0},{1.0,0.5,0.0},{1.0,1.0,0}};

float heightmod(int phase)
{
	if (mod == 0)
		return 1;
	return (cos(idlecount*18.0/500+phase)+1.0)/2;
}


#define ANGINC 2
void handleKeypress(unsigned char key, int x, int y) {
	switch (key) {
		case 27: //Escape key
			exit(0);
			break;
		case 107: //
			pan-=ANGINC;
			break;
		case 108:
			pan+=ANGINC;
			break;
		case 113: //
			tilt-=ANGINC;
			break;
		case 97:
			tilt+=ANGINC;
			break;
		case 49: //'1'
			min_height++;
			break;
		case 50: //'2'
			min_height--;
			break;
		case 102: //'f' zoom in
			zoom+=5;
			break;
		case 118: //'v' zoom out
			zoom-=5;
			break;
		case 103: //'g' toggle color vs. grey
			color = (color++)&0x01;
			break;
		case 109: //'m' toggle color vs. grey
			mod = (++mod)&0x01;
			printf("mod: %d\n", mod);
			break;
		case 116: //'t' toggle text
			draw_text = (++draw_text)&0x01;
			break;
		default:
			printf("Key: %d\n", key);
			break;
	}
}

void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	
    glEnable (GL_BLEND); 
	//glDisable(GL_DEPTH_TEST);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glBlendFunc(GL_ONE, GL_ONE);
}

void handleResize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)w / (float)h, 1.0, 200.0);
}

void drawBox(float x, float y, int height, int id, int phase)
{
	glPushMatrix();
	glTranslatef( x*1.5, -y*1.5, 0.2);
	
	if (color == 1)
		glColor3fv(colors[id]);
	else
		glColor3f(0.4, 0.4, 0.4);

	glScalef(1.0, 1.0, height*heightmod(phase));
	comp3dMakeBox(1.0, 1.0);

	if (draw_text && height != 0)
	{
		glColor3f(0.2, 0.2, 0.2);
		outputText(-0.25, -0.25, 0.3+height*heightmod(phase), "1");
	}
	glPopMatrix();
}

void drawFile(int x, int y)
{
	int wx=x*1.5;
	int wy=y*1.5;
	float d=0.2;
	glBegin(GL_QUADS); // Draw ground plane
		glColor3f(0.5, 0.5, 0.5);
		glNormal3f(   0,     0,   -1);
		glVertex3f(  -d,     d, -0.1);
		glVertex3f(  -d, -wy-d, -0.1);
		glVertex3f(wx+d, -wy-d, -0.1);
		glVertex3f(wx+d,     d, -0.1);
	glEnd();

	for (int i=1; i<x; i++)  // Draw rectangles
		for (int j=1; j<y;j++)
		{
			if (box.box[i][j] >= min_height)
			{
			    drawBox(i,j,box.box[i][j],box.col[i][j], j*y/5); // Normal height
			}
			else 
				drawBox(i,j, 0.0,box.col[i][j], j*y/5); // Zero height
		}
}



void drawScene() {
	idlecount++;
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
 
	glTranslatef(-45.0f, 10.0f, zoom);
	glRotatef(pan, 0.0, 1.0, 0.0);
	glRotatef(tilt, 1.0, 0.0, 0.0);
	
	GLfloat ambientLight[] = {0.8f, 0.8f, 0.8f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);

	GLfloat lightColor[] = {0.9f, 0.9f, 0.9f, 1.0f};
	GLfloat lightPos[] = {0, 0, 20, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

#if 1
	drawFile(10, 10);
	glTranslatef(16, 0, 0);
	drawFile(10, 30);
	glTranslatef(16, 0, 0);
	drawFile(10, 20);
	glTranslatef(16, 0, 0);
	drawFile(10, 20);
	glTranslatef(16, 0, 0);
	drawFile(10, 25);
#else
	glColor3f(1.0, 1.0, 0.0);
	plotBargraph3D(test);
#endif
	
	glutSwapBuffers();
}

void generateFile(int x, int y)
{
	for (int i=0; i<100; i++)
		for (int j=0; j<100; j++)
		{
			box.box[i][j]=-1;
			box.col[i][j]=-1;
		}
	for (int i=0; i<x; i++)
		for (int j=0; j<y; j++)
		{
			//sprintf_s(box.txt[i][j][0], sizeof(text), "%d", (int)((y-1)*10+x));
			if ( (10.0*rand()/RAND_MAX > 5) || i==0)
			{
				box.box[i][j]=1+8.0*rand()/RAND_MAX;
				box.col[i][j]=4*rand()/RAND_MAX;
			} else
			{
				box.box[i][j]=box.box[i-1][j];
				box.col[i][j]=box.col[i-1][j];
			}
		}
}
int main(int argc, char** argv) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(600, 400);
	glutCreateWindow("cvs visualization - By Morten Jagd Christensen");
	initRendering();
	
	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress);
	glutReshapeFunc(handleResize);
	glutIdleFunc(drawScene);

	generateFile(100, 100);
	
	glutMainLoop();
	return 0;
}
