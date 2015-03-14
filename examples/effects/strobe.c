#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <unistd.h> */
#include <math.h>
#include <GL/glut.h>

#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)(sqrt(x)))
#define floorf(x) ((float)(floor(x)))
#ifndef M_PI
#define M_PI 3.14159265
#define M_SQRT2 1.4142135
#endif
#endif

#if defined(_POSIX93)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#define _timeb timeb
#define _ftime ftime

typedef struct {
#if defined(_POSIX93)
    struct timespec _ts;
#else
    struct _timeb _ts;
#endif
} Timer;

void
timerReset(Timer *t)
{
#if defined(_POSIX93)
    clock_gettime(CLOCK_REALTIME, &t->_ts);
#else
    _ftime(&t->_ts);
#endif
}

double
timerGet(Timer *t)
{
#if defined(_POSIX93)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec - t->_ts.tv_sec + (ts.tv_nsec - t->_ts.tv_nsec)/1.e9;
#else
    struct _timeb ts;
    _ftime(&ts);
    return ts.time - t->_ts.time + 0.001 * (ts.millitm - t->_ts.millitm);
#endif
}

double
timeGet(void)
{
#if defined(_POSIX93)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1.e9;
#else
    struct _timeb ts;
    _ftime(&ts);
    return ts.time - + 0.001 * ts.millitm;
#endif
}

#undef USE_ACCUM 

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#define unitrand() (rand() / (double)RAND_MAX)

int windowWidth, windowHeight;

float oldProjection[16 * 20]; /* ugh */
int projStackTop = 0;

/* near, far name mangling because of x86 reserved words */
void pushOrthoView(float left, float right, float bottom, float top,
    float v_near, float v_far)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glGetFloatv(GL_PROJECTION_MATRIX, oldProjection + projStackTop);
    projStackTop += 16;
    glLoadIdentity();
    glOrtho(left, right, bottom, top, v_near, v_far);
}

void popView(void)
{
    glMatrixMode(GL_PROJECTION);
    projStackTop -= 16;
    glLoadMatrixf(oldProjection + projStackTop);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void sendHexVerts(void)
{
    glVertex2f(0, 0.5);
    glVertex2f(.5, 0.25);
    glVertex2f(.5, -0.25);
    glVertex2f(0, -0.5);
    glVertex2f(-0.5, -0.25);
    glVertex2f(-0.5, 0.25);
}

void wedge(double start, double end, double inc)
{
    double a;

    glBegin(GL_POLYGON);
    glVertex2f(0, 0);
    for(a = start; a < end; a += inc)
	glVertex2f(cos(a), sin(a));
    glVertex2f(0, 0);
    glEnd();
}

void
drawBlades(void)
{
    double inc;

    inc = M_PI / 120;
    wedge(0 * M_PI / 6, 2 * M_PI / 6, inc);
    wedge(4 * M_PI / 6, 6 * M_PI / 6, inc);
    wedge(8 * M_PI / 6, 10 * M_PI / 6, inc);
}

#define REGION_SIZE 40
#define JITTER_ELEMENTS 32

float jitter[JITTER_ELEMENTS];
int jitterCount = 8;

void makeDistribJitter(void)
{
    int i;
    double d;

    for(i = 0; i < jitterCount; i++){
	/* d = pow(unitrand(), 2) * M_SQRT2 / 2.0; */
	d = pow(unitrand(), 2);
	if(unitrand() < .5)
	    jitter[i] = .5 + d;
	else
	    jitter[i] = .5 - d;
    }
}

void makeGridJitter(void)
{
    int i;

    for(i = 0; i < jitterCount; i++)
	jitter[i] = (i + unitrand()) / jitterCount;
}

void makeNoJitter(void)
{
    int i;

    for(i = 0; i < jitterCount; i++)
	jitter[i] = .5;
}

void (*jitterFunc)(void) = makeNoJitter;

double lastFrameDuration;
double curRotation = 0;
double speed = .5;
static Timer frameTimer;

int doMagnify = 0;

void
display(void)
{
    int i;
    double nextRotation;
    char s[80];

    jitterFunc();
    pushOrthoView(0, windowWidth, 0, windowHeight, -1, 1);

    nextRotation = curRotation + lastFrameDuration * speed;

#ifndef USE_ACCUM
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#endif
    memset(s, '.', sizeof(s) - 1);
    memset(s + (sizeof(s) - 1) / 4, '-', (sizeof(s) - 1) / 2);
    for(i = 0; i < jitterCount; i++){
#ifdef USE_ACCUM
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
#endif
	glColor4f(1, 1, 1, 1.0 / (jitterCount));
	glPushMatrix();
	if(doMagnify) {
	    glTranslatef(REGION_SIZE / 2.0 + .5, REGION_SIZE / 2.0 + .5, 0);
	    glScalef(REGION_SIZE / 2.0, REGION_SIZE / 2.0, 1);
	}else{
	    glTranslatef(windowWidth / 2.0 + .5, windowWidth / 2.0 + .5, 0);
	    glScalef(windowWidth / 2.0, windowWidth / 2.0, 1);
	}
	glRotatef(curRotation * 360 * (1 - jitter[i]) + nextRotation * 360 *
	    jitter[i], 0, 0, 1);
	drawBlades();
	glPopMatrix();
#ifdef USE_ACCUM
	if(i == 0)
	    glAccum(GL_LOAD, 1.0 / (jitterCount));
	else
	    glAccum(GL_ADD, 1.0 / (jitterCount));
	/* goto foo; */
#endif
    }
/* foo: */
#ifdef USE_ACCUM
    glAccum(GL_RETURN, 1.0);
#endif

    if(doMagnify) {
	glDisable(GL_BLEND);
	glPixelZoom(windowWidth / REGION_SIZE, windowHeight / REGION_SIZE);
	glRasterPos2f(0, 0);
	glCopyPixels(0, 0, REGION_SIZE, REGION_SIZE, GL_COLOR);
    }

    popView();
    glutSwapBuffers();
    lastFrameDuration = lastFrameDuration * .8 + .2 * timerGet(&frameTimer);
    timerReset(&frameTimer);
    curRotation = nextRotation;
    /* printf("lastFrameDuration = %g\n", lastFrameDuration); */
}

/*ARGSUSED1*/
void key(unsigned char k, int x, int y)
{
     switch(k) {
     case 27: case 'q':
	 exit(0);

     case '-':
	 jitterCount--;
	 if(jitterCount < 1)
	     jitterCount = 1;
	 break;

     case '+':
	 jitterCount++;
	 if(jitterCount > JITTER_ELEMENTS)
	     jitterCount = JITTER_ELEMENTS;
	 break;

     case 'c':
	 jitterFunc = makeDistribJitter;
	 glutPostRedisplay();
	 break;

     case 'b':
	 jitterFunc = makeGridJitter;
	 glutPostRedisplay();
	 break;

     case 'a':
	 jitterFunc = makeNoJitter;
	 glutPostRedisplay();
	 break;

     case ',':
	 speed /= 1.1;
	 break;

     case '.':
	 speed *= 1.1;
	 break;
     }
}

void help(void)
{
    printf("This demonstrates how to reduce the effects of temporal aliasing.\n\n");

    printf("To start, press '+' until it is detectable (but not too obvious) that\n");
    printf("the rotation is occuring in discrete steps and not continuously.  The\n");
    printf("rotation speed can also be adjusted using '.' and ','.\n\n");

    printf("Pressing 'b' or 'c' will then render the scene using two different kinds\n");
    printf("of jitter, blurring the scene in order to reduce the aliasing.  Press 'a'\n");
    printf("to turn off the jitter.\n");
}

/* used to get current width and height of viewport */
void
reshape(int wid, int ht)
{
    glViewport(0, 0, wid, ht);
    windowWidth = wid;
    windowHeight = ht;
    glutPostRedisplay();
}

int
main(int argc, char **argv)
{
    makeNoJitter();
#ifdef _WIN32
    glutInitWindowSize(windowWidth = 128, windowHeight = 128);
#else
    glutInitWindowSize(windowWidth = 512, windowHeight = 512);
#endif
    glutInit(&argc, argv);
#ifdef USE_ACCUM
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_ACCUM|GLUT_DEPTH|GLUT_ALPHA);
#else
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH|GLUT_ALPHA);
#endif
    glutCreateWindow("Object recognition");
    glutKeyboardFunc(key);
    /* glutSpecialFunc(special); */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(display);
    /* glutMouseFunc(mouse); */
    /* glutMotionFunc(motion); */
    pushOrthoView(0, 512, 0, 512, 0, 1);
    help(); 
    timerReset(&frameTimer);
    glutMainLoop();
    return 0;
}
