/*
 * This program demonstrates animation using multiple texture objects
 * It draws a billboard polygon, animated with a flame texture. The
 * flame is a series of flame textures, bound in sequence, a new
 * on each frame. To increase the realism, a red/yellow lightsource
 * is placed at the center of the flame polygon, and flickers by
 * changing intensity each frame.
 *
 * Your job is to modify the display function to texture the flame
 * polygon with the proper flame texture, and to adjust the flame
 * light and position so the light appears to flicker and the light
 * follows the flame if it moves.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../util/texture.h"
#include <glut.h>

#ifdef _WIN32
/* Win32 math.h doesn't define float versions of the trig functions. */
#define sinf sin
#define cosf cos
#define atan2f atan2

/* nor does it define M_PI. */
#ifndef M_PI
#define M_PI 3.14159265
#endif

#define drand48() ((float)rand()/RAND_MAX)
#endif

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture glBindTextureEXT
#endif

int flames = 0; /* current flame image */
int flameCount = 32; /* total number of flame images */

static float transx = 1.0, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
GLboolean dblbuf = GL_TRUE;

enum {NO_DLIST, FLAME_BASE, GROUND};
enum {X, Y, Z};

typedef struct {
    GLfloat oldpos[3];
    GLfloat pos[3];
    GLfloat color[4]; /* RGBA */
    GLfloat Dr, Dg, Db, Da; /* Change in Color */
    GLfloat Vx, Vy, Vz;
    GLfloat Ax, Ay, Az;
} Particle;


#define PART_COUNT 20000
Particle parts[PART_COUNT]; /* array of particles */



#define PAN	1
#define ROT	2

#define RAD(x) (((x)*M_PI)/180.)

void
pan(const int x, const int y) {
    transx +=  (x-ox)/500.;
    transy -= (y-oy)/500.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
rotate(const int x, const int y) {
    rotx += x-ox;
    if (rotx > 360.) rotx -= 360.;
    else if (rotx < -360.) rotx += 360.;
    roty += y-oy;
    if (roty > 360.) roty -= 360.;
    else if (roty < -360.) roty += 360.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
motion(int x, int y) {
    if (mot == PAN) pan(x, y);
    else if (mot == ROT) rotate(x,y);
}

void
mouse(int button, int state, int x, int y) {

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;
    
	if(state == GLUT_DOWN) {
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    mot = PAN;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_RIGHT_BUTTON:
	    mot = ROT;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    break;
	}
    } else if (state == GLUT_UP) {
	mot = 0;
    }
}

void help(void) {
    printf("'h'           - help\n");
    printf("left mouse    - pan\n");
    printf("right mouse   - rotate\n");
    printf("e, E          - reexplode\n");
}

/* light to simulate flames effects */
GLfloat lightpos[] = {0.f, 0.f, 0.f, 1.f};
GLfloat firecolor[] = {1.f, 0.6f, 0.3f, 1.f};

enum {R, G, B, A};

/* initialize the particle data */
void
initpart(Particle parts[], int cnt)
{
    int i;
    GLfloat r, theta, phi;
    for(i = 0; i < cnt; i++) {
	
	phi = (.5 - drand48()) * M_PI;
	theta = (.5 - drand48()) * 2 * M_PI;
	r = drand48()/10.f * sinf(phi);

#if 0
	parts[i].Vx = (drand48() - .5f)/60.f;
	parts[i].Vz = (drand48() - .5f)/60.f;
	parts[i].Vy = drand48()/15.f;
#else
	parts[i].Vy = r * cosf(phi);
	parts[i].Vx = r * cosf(theta) * sinf(phi);
	parts[i].Vz = r * sinf(theta) * sinf(phi);
#endif

	parts[i].pos[X] = 0.f;
	parts[i].pos[Y] = -1.f;
	parts[i].pos[Z] = 0.f;

	parts[i].Ax = 0.f;
	parts[i].Ay = -.001f;
	parts[i].Az = 0.f;

	parts[i].color[R] = 1.f;
	parts[i].color[G] = 1.f;
	parts[i].color[B] = 1.f;
	parts[i].color[A] = 1.f;

	parts[i].Dr = (.5 - drand48())/50.f;
	parts[i].Dg = (.5 - drand48())/50.f;
	parts[i].Db = (.5- drand48())/50.f;
	parts[i].Da = (.5 - drand48())/50.f;

    }
}


void init(void)
{
    GLUquadricObj *cylinder;
    int i, j;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* turn on lighting, enable light 0, and turn on colormaterial */
    glMatrixMode(GL_PROJECTION);
    gluPerspective(50., 1., 1, 20.);

    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0., 0., -5.5);

    /* geometry for the base of the flame */
    glNewList(FLAME_BASE, GL_COMPILE);
    glColor3f(.2f, .2f, .2f);
    glPushMatrix();
    glTranslatef(0.f, -.9f, 0.f);
    glRotatef(90.f, 1.f, 0.f, 0.f);
    cylinder = gluNewQuadric();
    gluQuadricOrientation(cylinder, GLU_INSIDE);
    gluCylinder(cylinder, .5, .5, .1, 20, 20);
    gluDisk(cylinder, 0.f, .5, 20, 20);
    gluDeleteQuadric(cylinder);
    glPopMatrix();
    glEndList();


    /* geometry for the ground */
    glNewList(GROUND, GL_COMPILE);
    glColor4f(0.6f, 0.8f, 0.5f, 1.f);
    glBegin(GL_QUADS);
    glNormal3f(0.f, 1.f, 0.f);
    for(j = 0; j < 30; j++) /* z direction */
	for(i = 0; i < 30; i++) { /* x direction */
	    glVertex3f(-2.0 + i * 4.f/20, -1.0, -2.0 + j * 4.f/20);
	    glVertex3f(-2.0 + i * 4.f/20, -1.0, -2.0 + (j + 1) * 4.f/20);
	    glVertex3f(-2.0 + (i + 1) * 4.f/20, -1.0, -2.0 + (j + 1) * 4.f/20);
	    glVertex3f(-2.0 + (i + 1) * 4.f/20, -1.0, -2.0 + j * 4.f/20);
	}
    glEnd();
    glEndList();

    initpart(parts, PART_COUNT);

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glPointSize(2.0f);
    glLineWidth(2.0f);
}


/* update the particle data */
void
updatepart(Particle parts[], int cnt)
{
    int i;
    for(i = 0; i < cnt; i++) {
	memcpy(parts[i].oldpos, parts[i].pos, sizeof(parts[i].pos[0]) * 3);
	parts[i].pos[X] += parts[i].Vx;
	parts[i].pos[Y] += parts[i].Vy;
	parts[i].pos[Z] += parts[i].Vz;

	parts[i].Vx += parts[i].Ax;
	parts[i].Vy += parts[i].Ay;
	parts[i].Vz += parts[i].Az;

	parts[i].color[R] += parts[i].Dr;
	parts[i].color[G] += parts[i].Dg;
	parts[i].color[B] += parts[i].Db;
	parts[i].color[A] += parts[i].Da;

	if(parts[i].color[R] < 0.f) {
	    parts[i].color[R] = 0.f;
	    parts[i].Dr = 0.f;
	}

	if(parts[i].color[G] < 0.f) {
	    parts[i].color[G] = 0.f;
	    parts[i].Dg = 0.f;
	}

	if(parts[i].color[B] < 0.f) {
	    parts[i].color[B] = 0.f;
	    parts[i].Db = 0.f;
	}
	if(parts[i].color[A] < 0.f) {
	    parts[i].color[A] = 0.f;
	    parts[i].Da = 0.f;
	}

	if(parts[i].pos[Y] < -1.f) {
	    parts[i].pos[Y] = -1.f;
	    parts[i].Vx = 0.f;
	    parts[i].Vy = 0.f;
	    parts[i].Vz = 0.f;
	}

    }
}


void
display(void) 
{

    /*
     * save the viewing transform by saving a copy of the
     * modelview matrix at the right place, then undo the
     * rotation by calling calcMatrix() at the right time to
     * billboard the tree. Billboarding should only happen
     * if the billboard variable is not zero.
     */

    float mat[16];
    int i;
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gluLookAt(-sinf(RAD(rotx))*5.5,
	      transy,
	      cosf(RAD(rotx))*5.5,
	      0. ,0. ,0., 
	      0. ,1. ,0.);

    /* save the viewing tranform */
    glGetFloatv(GL_MODELVIEW_MATRIX, mat);

    /* ground */
    glDisable(GL_TEXTURE_2D);
/*    glCallList(GROUND);*/

    glPushMatrix();

    glTranslatef(0.f, 0.f, -transx);

    /* base of flame */
    glCallList(FLAME_BASE);

    glBegin(GL_POINTS);
    for(i = 0; i < PART_COUNT; i++) {
	glColor3fv(parts[i].color);
	glVertex3fv(parts[i].pos);
    }
    glEnd();
#if 1
    glBegin(GL_LINES);
    for(i = 0; i < PART_COUNT; i++) {
	glColor3fv(parts[i].color);
	glVertex3fv(parts[i].oldpos);
	glVertex3fv(parts[i].pos);
    }
    glEnd();
#endif

    updatepart(parts, PART_COUNT);

    glPopMatrix();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

/*ARGSUSED*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'e':
    case 'E':
	initpart(parts, PART_COUNT);
	break;
    case '\033':
	exit(EXIT_SUCCESS); 
	break;
    default:
	help();
	break;
    }
    glutPostRedisplay();
}

void
anim(void)
{

    glutPostRedisplay();
}

int
main(int argc, char** argv)
{
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    if(argc > 1) {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
	while(!done) {
	    switch(*args) {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = GL_FALSE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = GL_TRUE;
		break;
	    }
	    args++;
	}
    }
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);

    (void)glutCreateWindow("particle system");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutIdleFunc(anim);
    glutMotionFunc(motion);

    key('?', 0, 0); /* to print help message */

    glutMainLoop();
    return 0;
}
