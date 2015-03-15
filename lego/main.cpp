#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include "lego.h"

using namespace std;

int pan=0;          // rotate around y axis 
int tilt=-90;       // rotate around x axis

int objcount=0;
Lego * objarr[200];
int current=0;
int type =0;

Lego * addobject()
{
	Lego * tmp;
	objcount++;
	current = objcount; // always reset to new object
	tmp = new Lego(1, -4, -4, 0, 0); // add check for NULL
	objarr[objcount]=tmp;
	return tmp;
}

#define ANGINC 2
void handleKeypress(unsigned char key, int x, int y) {
	float tmp; // for color toggle
	Lego * cur = objarr[current];
	switch (key) {
		case 27: //Escape key
			exit(0);
			break;
		case 121: // 'y' toggle type
			type++;
			if (type > 2)
				type = 0;
			cur->type=type;
			break;
		case 116: //'t' toggle color   blue, red, green
			tmp = cur->color[0];
			cur->color[0]=cur->color[1];
			cur->color[1]=cur->color[2];
			cur->color[2]=tmp;
			break;
		case 118: //'v' skift vinkel
			if (cur->angle == 0)
				cur->angle = 90;
			else
				cur->angle = 0;
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
		case 43: // '+' add a lego object
			cur = addobject();
			break;

		case 122: // 'z' move left
			cur->x+=1;
			break;
		case 120: // 'x' move left
			cur->x-=1;
			break;

		case 99: //'c'
			cur->y+=1;
			break;
		case 100: //'d'
			cur->y-=1;
			break;

		case 44: //',' up
			cur->z+=1;
			break;
		case 46: //'.' down
			cur->z-=1;
			break;

		case 60: // '<' move to previous object
			if (current > 1)
				current-=1;
			else
				current = objcount;
			cur = objarr[current];
			break;

		default:
			printf("Key: %d\n", key);
			break;
	}
}


//Makes the image into a texture, and returns the id of the texture
void drawAxes()
{
	#define LEN 10
	glEnable(GL_LINE_SMOOTH);

	glBegin(GL_LINES);
	for (int i=0; i< LEN; i++)
	{
		glColor3f(1.0, 0, 0);
	    glVertex3f(0, i, 0);
		glVertex3f(LEN, i, 0);

		glColor3f(0.0, 1.0, 0);
		glVertex3f(i, 0, 0);
		glVertex3f(i, LEN, 0);
	}
	glColor3f(0.0, 0.0, 1.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0.0, 0.0, LEN);
	glEnd();
}


#define ENABLED(x) glIsEnabled(x) ? printf("%s enabled\n", #x) : printf("%s disabled\n", #x)

void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
}

void handleResize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)w / (float)h, 1.0, 200.0);
}

#define BOX_SIZE 7

void lego()
{
	glTranslatef(-5.0, -5.0, -20.0f);
	glRotatef(pan, 0.0, 1.0, 0.0);
	glRotatef(tilt, 1.0, 0.0, 0.0);
	
	GLfloat ambientLight[] = {0.3f, 0.3f, 0.3f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);
	
	GLfloat lightColor[] = {0.7f, 0.7f, 0.7f, 1.0f};
	GLfloat lightPos[] = {-2 * BOX_SIZE, BOX_SIZE, 4 * BOX_SIZE, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	drawAxes();

	int i;
	for (i=1; i<=objcount;i++)
	{
		
		if (i == current)
			objarr[i]->display(1);
		else
			objarr[i]->display(0);
	}
}
	
void drawScene() {
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	lego();
	glutSwapBuffers();
}


//Called every 25 milliseconds
void update(int value) {
	glutPostRedisplay();
	glutTimerFunc(5, update, 0);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(400, 400);
	
	glutCreateWindow("OpenGL testing- MJC");
	initRendering();
	
	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress);
	glutReshapeFunc(handleResize);
	glutTimerFunc(5, update, 0);
	
	glutMainLoop();
	return 0;
}
