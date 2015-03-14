#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#ifdef _WIN32
#define sqrtf(x)    ((float)sqrt(x))
#define drand48()   ((float)rand()/(float)RAND_MAX)
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define floorf(x) ((float)(floor(x)))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

#define CHECK_ERROR(string)                                              \
{                                                                        \
    GLenum error_value;                                                  \
    while((error_value = glGetError()) != GL_NO_ERROR)                   \
	printf("Error Encountered: %s (%s)\n", string,                   \
	       gluErrorString(error_value));                             \
}

static int winWidth, winHeight;

#define SIZE	64
struct point { float x, y; } points[SIZE];

void
makepoints(void) {
    int i;
    for(i = 0; i < SIZE; i++) {
	points[i].x = drand48()*2.f-1.f;
	points[i].y = drand48()*2.f-1.f;
    }
}

/*
 * cone with base at (0,0,0) and top at (0,0,1)
 * one unit wide and high.
 */
void
makecone(void)
{
#define SIDES 20
    int i;
    glNewList(1, GL_COMPILE);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.f, 0.f, 1.f);
    for(i = 0; i <= SIDES; i++) {
	float s = sinf((2.f*M_PI*i)/SIDES)*.5f;
	float c = cosf((2.f*M_PI*i)/SIDES)*.5f;
	glVertex3f(s, c, 0.f);
    }
    glEnd();
    glEndList();
    CHECK_ERROR("makecone");
}


/* Called when window needs to be redrawn */
void redraw(void)
{
    int i;
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 
    glPointSize(3.f);

    for(i = 0; i < SIZE; i++) {
	glPushMatrix();
	glTranslatef(points[i].x, points[i].y, 0.f);
#if  0
	glColor3f((points[i].x+1.f)/2.f, (points[i].y+1.f)/2.f, drand48());
#endif
	glColor3f(drand48(), drand48(), drand48());
	glCallList(1);
	glColor3f(0.f, 0.f, 0.f);
	glBegin(GL_POINTS);
	glVertex3f(0.f, 0.f, 1.f);
	glEnd();
	glPopMatrix();
    }
    CHECK_ERROR("redraw");
}

void
help(void) {
/*    printf("S    - increase shift scale\n");
    printf("s    - decrease shift scale\n");	*/
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033': exit(EXIT_SUCCESS); break;
    default: help(); return;
    }
    glutPostRedisplay();
}

void
menu(int which) {
    key((char)which, 0, 0);
}

void
create_menu(void) {
    glutCreateMenu(menu);
    glutAddMenuEntry("increase shift scale", 'S');
    glutAddMenuEntry("decrease shift scale", 's');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void
reshape(int wid, int ht)
{
    float a;
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    a = ((float)winWidth)/winHeight;
    if (a > 1)
	glOrtho(-1.f*a, 1.f*a, -1.f, 1.f, -1.f, 1.f);
    else
	glOrtho(-1.f, 1.f, -1.f*a, 1.f*a, -1.f, 1.f);
    glMatrixMode(GL_MODELVIEW);
}

/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);
    (void)glutCreateWindow("voronoi");
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
/*    create_menu();	*/

    glEnable(GL_DEPTH_TEST);
    makepoints();
    makecone();
    /*glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);*/

    glutDisplayFunc(redraw);
    CHECK_ERROR("main");
    glutMainLoop();
    return 0;
}
