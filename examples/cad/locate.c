#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"
#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)sqrt(x))
#define floorf(x) ((float)floor(x))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#define drand48()   ((float)rand()/(float)RAND_MAX)
#endif

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#ifndef FALSE
enum {FALSE, TRUE};
#endif
enum {OBJ_ANGLE, OBJ_TRANSLATE, OBJ_PICK};
enum {X, Y, Z, W};
enum {XOR, COLOR};

/* window dimensions */
int winWidth = 512;
int winHeight = 512;
int active;
int mode = XOR;

int object = 2;
int proxy_object = 2;
int maxobject = 3;
#define OBJECTS 14
struct { float x, y, z; /* position */
	 float r, g, b; /* color */
         float a;
} objects[OBJECTS];
static int pickable, fast_pick;

GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

GLfloat color[4] = {1.f, 1.f, 1.f, 1.f};
GLfloat zero[4] = {0.f, 0.f, 0.f, 1.f};
GLfloat one[4] = {1.f, 1.f, 1.f, 1.f};

GLfloat lightpos[4] = {0.f, 0.f, 1.f, 0.f};

void unpick(void);

void
reshape(int wid, int ht) {
    float a;
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    a = ((float)winWidth)/winHeight;
    if (a > 1)
	glFrustum(-100.*a, 100.*a, -100., 100., 300., 600.); 
    else
	glFrustum(-100., 100., -100.*a, 100.*a, 300., 600.); 
    glMatrixMode(GL_MODELVIEW);
}

void pick_objects(int x, int y);

void
motion(int x, int y) {

    switch(active)
    {
    case OBJ_ANGLE:
	objangle[X] = (x - winWidth/2) * 360./winWidth;
	objangle[Y] = (y - winHeight/2) * 360./winHeight;
	glutPostRedisplay();
	break;
    case OBJ_PICK:
	pick_objects(x, winHeight-y);
	/*glutPostRedisplay();*/
	break;
    case OBJ_TRANSLATE:
	objpos[X] = (x - winWidth/2) * 100./winWidth;
	objpos[Y] = (winHeight/2 - y) * 100./winHeight;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y) {

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN) {
	switch(button) {
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = OBJ_ANGLE;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = OBJ_PICK;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    active = OBJ_TRANSLATE;
	    motion(x, y);
	    break;
	}
    } else {
	pickable = 0;
	if(button == GLUT_MIDDLE_BUTTON) unpick();
    }
}

void
draw_objects(void) {
    int i;
    for(i = 0; i < OBJECTS; i++) {
	glPushMatrix(); /* assuming modelview */
	glTranslatef(objects[i].x, objects[i].y, objects[i].z);
#if 0
	glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
	glRotatef(objangle[Y], 1.f, 0.f, 0.f);
#endif
	glColor3f(objects[i].r, objects[i].g, objects[i].b);
	glCallList(object);
	glPopMatrix();

    }
}

int the_pick;

void
highlight(int p) {
    if (mode == XOR) {
	glDepthFunc(GL_LEQUAL);
	glLogicOp(GL_XOR);
	glDrawBuffer(GL_FRONT);
	glEnable(GL_COLOR_LOGIC_OP);

	glPushMatrix();
	glTranslatef(objects[p].x, objects[p].y, objects[p].z);
	glColor3f(0x80, 0x80, 0x80);
	glCallList(object);
	glPopMatrix();

	glDisable(GL_COLOR_LOGIC_OP);
	glDrawBuffer(GL_BACK);
	glEnable(GL_LIGHTING);
    } else if (mode == COLOR) {
	glDepthFunc(GL_LEQUAL);
	glDrawBuffer(GL_FRONT);

	glPushMatrix();
	glTranslatef(objects[p].x, objects[p].y, objects[p].z);
	glColor3ub(0xff, 0xff, 0xff);
	glCallList(object);
	glPopMatrix();

	glDrawBuffer(GL_BACK);
	
    }
}
void
unhighlight(int p) {
    if (mode == XOR) {
	glDepthFunc(GL_LEQUAL);
	glLogicOp(GL_XOR);
	glDrawBuffer(GL_FRONT);
	glEnable(GL_COLOR_LOGIC_OP);

	glPushMatrix();
	glTranslatef(objects[p].x, objects[p].y, objects[p].z);
	glColor3f(0x80, 0x80, 0x80);
	glCallList(object);
	glPopMatrix();

	glDisable(GL_COLOR_LOGIC_OP);
	glDrawBuffer(GL_BACK);
	glEnable(GL_LIGHTING);
    } else if (mode == COLOR) {
	glDepthFunc(GL_LEQUAL);
	glDrawBuffer(GL_FRONT);

	glPushMatrix();
	glTranslatef(objects[p].x, objects[p].y, objects[p].z);
	glColor3f(objects[p].r, objects[p].g, objects[p].b);
	glCallList(object);
	glPopMatrix();

	glDrawBuffer(GL_BACK);
    }
}

void
unpick(void) {
    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);

    if (the_pick)
	unhighlight(the_pick - 1);

    glPopMatrix();
}

void
pick_objects(int x, int y) {
    int i;
    GLubyte buf[4];

    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);

    if (!pickable) {
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	for(i = 0; i < OBJECTS; i++) {
	    glPushMatrix(); /* assuming modelview */
	    glTranslatef(objects[i].x, objects[i].y, objects[i].z);
	    glColor3ub((GLubyte)((i+1)<<4), 0, 0);
	    glCallList(proxy_object);
	    glPopMatrix();
	}
	if (fast_pick) pickable = 1;
	glEnable(GL_LIGHTING);
    }
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    if (!fast_pick) printf("buf = %x %d %d\n", buf[0], x, y);

    i = buf[0] >> 4;
    if (the_pick && the_pick != i) {
	int p = the_pick - 1;
        /* unpick */
	unhighlight(p);
    }
    if (i && i != the_pick) {
	int p = i - 1;
        /* pick */
	highlight(p);
    }
    the_pick = i;
    glPopMatrix();
    glFlush();
    CHECK_ERROR("OpenGL Error in pick_objects()");
}

void
redraw(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    
#if 0
    if(lightchanged[UPDATE_OGL])
    {
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	lightchanged[UPDATE_OGL] = GL_FALSE;
    }
#endif

#if 0
    glCallList(object);
#endif
    draw_objects();

    glPopMatrix(); /* assuming modelview */

    CHECK_ERROR("OpenGL Error in redraw()");
    glutSwapBuffers(); 
}

void
help(void) {
    printf("ESC  - quit\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    static int wire;
    switch(key)
    {
    case 'p':
	fast_pick ^= 1;
	break;
    case 'o':
	/* toggle object type */
	object++;
	if(object > maxobject)
	    object = 2;
	glutPostRedisplay();
	break;
    case 'w':
	if (wire ^= 1)
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glutPostRedisplay();
	break;
    case 'x':
	if (mode == XOR) mode = COLOR;
	else mode = XOR;
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
	break;
    default:
	help();
	break;
    }

}

void
menu(int which) {
    key((char)which, 0, 0);
}

void
create_menu(void) {
    glutCreateMenu(menu);
    glutAddMenuEntry("toggle fast pick", 'p');
    glutAddMenuEntry("toggle highlight style", 'x');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int
main(int argc, char *argv[]) {
    GLUquadricObj *quad;
    int i;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    (void)glutCreateWindow("locate");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    create_menu();

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 300., 600.); 
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 450) */
    gluLookAt(0., 0., 450., 0., 0., 0., 0., 1., 0.);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    
    glCullFace(GL_BACK);
    glReadBuffer(GL_BACK);
    glDisable(GL_DITHER);

    CHECK_ERROR("end of main");

    glNewList(1, GL_COMPILE);
    glutSolidCube(15.f);
    glEndList();


    glNewList(2, GL_COMPILE);
    glutSolidTeapot(10.f);
    glEndList();

    quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);

    glNewList(3, GL_COMPILE);
    gluSphere(quad, 70., 20, 20);
    glEndList();

    gluDeleteQuadric(quad);
    maxobject = 3;

    for(i = 0; i < OBJECTS; i++) {
        objects[i].x = (drand48()-.5f)*200.f;
        objects[i].y = (drand48()-.5f)*200.f;
        objects[i].z = (drand48()-.5f)*200.f;
        objects[i].a = 0.f;
	objects[i].r = drand48();
	objects[i].g = drand48();
	objects[i].b = drand48();
    }

    glutMainLoop();
    return 0;
}
