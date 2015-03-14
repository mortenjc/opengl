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
static int round_joins, stencilling;
static int wider;

static void
draw(void) {
    int i;
    glBegin(GL_LINE_STRIP);
    for(i = 0; i < 100; i++) {
	float x = 1.5f*i/100.f;
	float y = sin(i*M_PI*3.f/100.f)*.75f;
	glVertex3f(-.75f+x, y, 0.f);
    }
    glEnd();
    if (round_joins) {
	glBegin(GL_POINTS);
	for(i = 0; i < 100; i++) {
	    float x = 1.5f*i/100.f;
	    float y = sin(i*M_PI*3.f/100.f)*.75f;
	    glVertex3f(-.75f+x, y, 0.f);
	}
	glEnd();
    }
}

/* Called when window needs to be redrawn */
void redraw(void)
{
    int i;
    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT); 
    glColor4f(1.f, .1f, 1.f, .7f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(wider ? 40 : 20);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(wider ? 20 : 10);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    if (stencilling) {
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_NOTEQUAL, 1, 1);
	glEnable(GL_ALPHA_TEST);

	for(i = 0; i < 100; i++) {
	    glAlphaFunc(GL_GEQUAL, (100.f-i)/100.f);
	    draw();
	}
	glDisable(GL_STENCIL_TEST);
    } else
	draw();


#if 0
    glBegin(GL_TRIANGLE_STRIP);
    for(i = 0; i < 100; i++) {
	float x = 1.5f*i/100.f;
	float y = sin(i*M_PI*3.f/100.f)*.75f;
	float x1 = 1.5f*(i+1)/100.f;
	float y1 = sin((i+1)*M_PI*3.f/100.f)*.75f;
	float m = -(x1-x)/(y1-y);
	float 
	glVertex3f(-.75f+x, y, 0.f);
    }
    glEnd();
#endif
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    CHECK_ERROR("redraw");
    glutSwapBuffers();
}

void
help(void) {
    printf("j    - round joins\n");
    printf("s    - stencil blending\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033': exit(EXIT_SUCCESS); break;
    case 's': stencilling ^= 1; break;
    case 'r': round_joins ^= 1; break;
    case 'w': wider ^= 1; break;
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
    glutAddMenuEntry("round joins", 'r');
    glutAddMenuEntry("stencil blend", 's');
    glutAddMenuEntry("line width", 'w');
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
    glutInitDisplayMode(GLUT_RGBA | GLUT_STENCIL | GLUT_DOUBLE);
    (void)glutCreateWindow("overlap");
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    create_menu();
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);


    glutDisplayFunc(redraw);
    CHECK_ERROR("main");
    glutMainLoop();
    return 0;
}
