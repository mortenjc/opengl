#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <imageloader.h>
#include "planet_params.h"
#include <aux_functions.h>
#include <3dcomponents.h>

using namespace std;

float _time = 0;			  //Time in 'ticks'
float _days = 0;			  //Time in days since 2000
float dupt = 1.0;		 	  // Update time in ms
int   update_freq = 10;       // 
float camera_z = -80.0;		  //Initial camera offset

int pan=0;                    // 
int tilt=0;					  //
int point_type = GL_LINE;

#define ANGINC 2  // Angular increment in degrees
void handleKeypress(unsigned char key, int x, int y) {
	switch (key) {
		case 27: //Escape key
			exit(0);
			break;
		case 107: // 'k'  rotate around green axis
			pan-=ANGINC;
			break;
		case 108: // 'l'  rotate around green axis
			pan+=ANGINC;
			break;
		case 113: // 'q'  rotate around blue axis
			tilt-=ANGINC;
			break;
		case 97:  // 'a'  rotate around blue axis
			tilt+=ANGINC;
			break;
		case 116: // 't' increase speed
			if (update_freq != 1)
				update_freq/=2;
			break;
		case 114: // 'r' reduce speed
			update_freq*=2;
			break;
		case 118: // 'v' Move back
			camera_z-= 2;
			break;
		case 102: // 'f' Move forward
			camera_z+= 2;
			break;
		case 112: // 'P' toggle polygon type
			if (point_type == GL_POINT)	{
				point_type = GL_LINE;
				break;
			}
			if (point_type == GL_LINE)	{
				point_type = GL_FILL;
				break;
			}	
			point_type = GL_POINT;
			break;
		default:
			printf("Key: %d\n", key);
			break;
	}
}

//Makes the image into a texture, and returns the id of the texture
GLuint loadTexture(Image* image) {
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D,	 0,	 GL_RGB, image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
	return textureId;
}



void drawAxes()
{
#define LEN 10
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(0.5);

	glBegin(GL_LINES);
		
	glColor3f(1.0, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(LEN, 0, 0);

	glColor3f(0.0, 1.0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0.0, LEN, 0);

	glColor3f(0.0, 0.0, 1.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0.0, 0.0, LEN);
	glEnd();

	glLineWidth(0.5);
}


void drawPlanets()
{
	int i=0;
	GLfloat x;
	plparm_t pt;
	GLfloat Ltot; //
	while (solar_system[i].name != NULL)
	{
		pt = solar_system[i];
		x = 10*pt.op.a;  // Exaggerate orbit

		glPushMatrix();
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.2, 0.2, 0.2);
		auxWireCircle(x, pt.op.i, 100); // Draw orbit 
		glColor3f(1.0, 1.0, 1.0);

		glRotatef(pt.op.i, 0.0, 0.0, 1.0);       // Inclination from the ecliptic
		Ltot = pt.op.L + pt.op.Lcy*_days/36500;
		glRotatef(Ltot, 0.0, 1.0, 0.0);
		glTranslatef(x, 0.0, 0.0);
		glRotatef(2.0*_days, 0.0, 1.0, 0.0);
        
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, pt.textureID);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		comp3dCreateSphere(pt.planet_radius, 20, point_type);
		glColor3f(1.0, 1.0, 1.0);
		glPopMatrix();
		i++;
	}
}


void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_TEXTURE_2D);

	
	Image* image;
	plparm_t * pt;
	int i=0;
	char texname[50];

	while (solar_system[i].name != NULL)
	{
		pt = &solar_system[i];
		
		snprintf(texname, sizeof(texname) , "%s.bmp", pt->name);
		image=loadBMP(texname);
		pt->textureID=loadTexture(image);
		printf("Loaded texture: %s with texture ID: %d\n", texname, pt->textureID);
		delete image;
		i++;
	}
}

void handleResize(int w, int h) 
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)w / (float)h, 1.0, 200.0);
}

void drawScene() 
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	glTranslatef(0.0f, 0.0f, camera_z);
	glRotatef(pan, 0.0, 1.0, 0.0);
	glRotatef(tilt, 0.0, 0.0, 1.0);
	
	GLfloat ambientLight[] = {0.7f, 0.7f, 0.7f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);
	GLfloat lightColor[] = {0.9f, 0.9f, 0.9f, 1.0f};
	GLfloat lightPos[] = {0.0, 200.0, 0.0, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	drawAxes();
	drawPlanets();
	
	glutSwapBuffers();
}


//Called continously from glutIdle state
void update() 
{
	_time+=dupt;
	if ( ((int)(_time/dupt) % update_freq) == 0) {
		_days+=2;
	}
	glutPostRedisplay();
}

int main(int argc, char** argv) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	
	glutCreateWindow("Solar System - by Morten Jagd Christensen");
	initRendering();
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_ARRAY );

	glutIdleFunc(update);
	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress);
	glutReshapeFunc(handleResize);
	
	glutMainLoop();
	return 0;
}
