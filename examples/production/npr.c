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
enum { FALSE, TRUE };
#endif
enum { OBJ_ANGLE, OBJ_TRANSLATE, OBJ_PICK };
enum { X, Y, Z, W };
enum { MATTE, RUBBER, TWOTONE, METAL };

/*
 * window dimensions 
 */
int winWidth = 512;
int winHeight = 512;
int active;
int maxobject;
int edgelines = 0;
int lighting = 1;
int show_texture;

GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

GLfloat zero[4] = {0.f, 0.f, 0.f, 1.f};
GLfloat one[4] = {1.f, 1.f, 1.f, 1.f};

/*
 * Light colors 
 */
GLfloat warm[4] = {1.3f, 1.3f, 0.9f, 0.f};
GLfloat cool[4] = {0.0f, 0.0f, 0.7f, 0.f};
GLfloat lightambient[4] = {0.65f, 0.65f, 0.8f, 0.f};	/* (warm + cool)/2 */
GLfloat light1diffuse[4] = {0.65f, 0.65f, 0.1f, 0.f};	/* (warm - cool)/2 */
GLfloat light2diffuse[4] = {-.65f, -.65f, -.1f, 0.f};	/* (cool - warm)/2 */

GLfloat light0pos[4] = {1.f, 1.f, 1.f, 0.f};
#if 0
GLfloat light1pos[4] = {-1.f, -1.f, 1.f, 0.f};
GLfloat light2pos[4] = {1.f, 1.f, -1.f, 0.f};
#else
GLfloat light1pos[4] = {1.f, 1.f, 1.f, 0.f};
GLfloat light2pos[4] = {-1.f, -1.f, -1.f, 0.f};
#endif

static void material(int type);

void
reshape(int wid, int ht)
{
    float a;
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    a = ((float) winWidth) / winHeight;
    if (a > 1)
	glFrustum(-100. * a, 100. * a, -100., 100., 300., 600.);
    else
	glFrustum(-100., 100., -100. * a, 100. * a, 300., 600.);
    glMatrixMode(GL_MODELVIEW);
}

int ox, oy, gx, gy;

void
motion(int x, int y)
{
    int dx, dy;

    dx = x - ox; dy = y - oy;
    ox = x; oy = y;
    gx += dx; gy += dy;

    switch (active) {
    case OBJ_ANGLE:
	objangle[X] = (gx - winWidth / 2) * 360. / winWidth;
	objangle[Y] = (gy - winHeight / 2) * 360. / winHeight;
	glutPostRedisplay();
	break;
    case OBJ_PICK:
	glutPostRedisplay();
	break;
    case OBJ_TRANSLATE:
	objpos[X] = (gx - winWidth / 2) * 100. / winWidth;
	objpos[Y] = (winHeight / 2 - gy) * 100. / winHeight;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y)
{

    ox = x;
    oy = y;

    /*
     * hack for 2 button mouse 
     */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if (state == GLUT_DOWN)
	switch (button) {
	case GLUT_LEFT_BUTTON:	/* move the light */
	    active = OBJ_ANGLE;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = OBJ_PICK;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON:	/* move the polygon */
	    active = OBJ_TRANSLATE;
	    motion(x, y);
	    break;
	}
}

static void
fill_window(void)
{
    /*
     * set the matrix state
     */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);

    /*
     * draw a rectangle the size of the window
     */
    glDisable(GL_DEPTH_TEST);
    glRecti(0, 0, 1, 1);
    glEnable(GL_DEPTH_TEST);

    /*
     * pop the matrixes
     */
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static void
draw_edges(void)
{
    /*
     * render the offset depth image
     */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#if defined(GL_EXT_polygon_offset) || defined(GL_VERSION_1_2)
    glPolygonOffset(4.0, 1.0);
#else
    /* XXX should change to use glPolygonOffset in OpenGL 1.1 */
    glPolygonOffsetEXT(4.0, (1.0 / (1 << 22)));
#endif
    glEnable(GL_POLYGON_OFFSET_FILL);
    /* glColorMask(0,0,0,0); */
    glCallList(1);
    /* glColorMask(1,1,1,1); */
    glDisable(GL_POLYGON_OFFSET_FILL);

    /*
     *  make no further changes to the depth image
     */
    glDepthMask(0);
    glDisable(GL_DEPTH_TEST);	/* XXX */
    glEnable(GL_DEPTH_TEST);	/* XXX */

    /*
     * cull all facets of one (arbitrary) orientation.  render the
     * remaining facets in outline mode, toggling the stencil bit
     * at each pixel.
     */
    glDisable(GL_LIGHTING);
    glColor3f(0.f, 0.f, 0.f);
    glLineWidth(2.0);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
    glEnable(GL_CULL_FACE);
    glColorMask(0, 0, 0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glCallList(1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColorMask(1, 1, 1, 1);
    glDisable(GL_CULL_FACE);

    /*
     * color all pixels in the framebuffer with stencil value 1
     */
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glColor3f(0, 0, 0);
    fill_window();
    glDisable(GL_STENCIL_TEST);

    /*
     * draw all true edges, testing against the depth image
     */
    glColor3f(0, 0, 0);
    glCallList(2);

    /*
     * return state to default values
     */
    glDepthMask(1);
    glDisable(GL_DEPTH_TEST);	/* XXX */
    glEnable(GL_DEPTH_TEST);	/* XXX */
    if (lighting)
	glEnable(GL_LIGHTING);
    glLineWidth(1.0);
    glColor3f(1.f, 1.f, 1.f);
}

void
redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (show_texture) {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-2., 2., -2., 2., -1., 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex2f(-1.f, -1.f);
	glTexCoord2f(1.f, 0.f);
	glVertex2f(1.f, -1.f);
	glTexCoord2f(1.f, 1.f);
	glVertex2f(1.f, 1.f);
	glTexCoord2f(0.f, 1.f);
	glVertex2f(-1.f, 1.f);
	glEnd();
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	CHECK_ERROR("OpenGL Error in redraw()");
	glutSwapBuffers();
	return;
    }
    glPushMatrix();				/* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f);	/* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f);	/* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);

    /*
     * The object 
     */
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);
    glCallList(1);
    glDisable(GL_POLYGON_OFFSET_FILL);

    /*
     * The edges 
     */
    if (edgelines) {
#if 0
	glDisable(GL_LIGHTING);
	glColor3f(0.0, 0.0, 0.0);
	glLineWidth(2.0);
	glCallList(2);
	if (lighting)
	    glEnable(GL_LIGHTING);
	glColor3f(1.0, 1.0, 1.0);
	glLineWidth(1.0);
#else
	draw_edges();
#endif
    }
    glPopMatrix();		/* assuming modelview */

    CHECK_ERROR("OpenGL Error in redraw()");
    glutSwapBuffers();
}


static void 
material(int type)
{
    GLfloat ambient[] = {.19225f, .19225f, .19225f, 1.0f};
    GLfloat diffuse[] = {.50754f, .50754f, .50754f, 1.0f};
    GLfloat specular[] = {.508273f, .508273f, .508273f, 1.0f};

    switch (type) {
    case MATTE:
	glEnable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT2);
	glEnable(GL_LIGHTING);
	lighting = 1;
	glDisable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0);
	break;
    case RUBBER:
	glEnable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT2);
	glEnable(GL_LIGHTING);
	lighting = 1;
	glDisable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 51.2f);
	break;
    case TWOTONE:
	glDisable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	glEnable(GL_LIGHTING);
	lighting = 1;
	glDisable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0);
	break;
    case METAL:
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT2);
	glDisable(GL_LIGHTING);
	lighting = 0;
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glEnable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, one);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, one);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0);
	break;
    }
    show_texture = 0;
    CHECK_ERROR("material");
}



/*ARGSUSED1*/
void
key(unsigned char key, int x, int y)
{
    static int wire;

    switch (key) {
    case 'w':
	if (wire ^= 1)
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	break;

    case 'e':
	edgelines = !edgelines;
	break;

    case 'm':
	material(MATTE);
	break;

    case 'r':
	material(RUBBER);
	break;

    case 't':
	material(TWOTONE);
	break;

    case 'a':
	material(METAL);
	break;

    case 'x':
	show_texture = 1;
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	break;
    case 27:
    case 'q':
    case 'Q':
	exit(0);
	break;

    default:
	return;
    }

    glutPostRedisplay();
}

void
menu(int which)
{
    key((char) which, 0, 0);
}

void
create_menu(void)
{
    glutCreateMenu(menu);
    glutAddMenuEntry("OpenGL matte", 'm');
    glutAddMenuEntry("OpenGL rubber", 'r');
    glutAddMenuEntry("Two-tone shading", 't');
    glutAddMenuEntry("Metallic anisotropic reflection", 'a');
    glutAddMenuEntry("Show anisotropic texture", 'x');
    glutAddMenuEntry("Toggle edge lines", 'e');
    glutAddMenuEntry("Toggle wireframe", 'w');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

static void
cylinder_edges(int facets, float radius, float length)
{
    int facet;
    float x, y, z;

    glBegin(GL_LINE_LOOP);
    for (facet = 0; facet < facets; facet++) {
	double angle = facet * (2.0 * M_PI / facets);
	x = sin(angle) * radius;
	z = cos(angle) * radius;
	y = length;
	glVertex3f(x, y, z);
    }
    glEnd();
    glBegin(GL_LINE_LOOP);
    for (facet = 0; facet < facets; facet++) {
	double angle = facet * (2.0 * M_PI / facets);
	x = sin(angle) * radius;
	z = cos(angle) * radius;
	glVertex3f(x, 0, z);
    }
    glEnd();
}

static void
cylinder(int facets, int ribs, float radius, float length)
{
    int facet, rib;

    for (rib = 0; rib < ribs; rib++) {
	float x, y, z;
	glBegin(GL_TRIANGLE_STRIP);
	for (facet = 0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    y = rib * (length / ribs);
	    glNormal3f(sin(angle), 0.f, cos(angle));
	    glTexCoord2f(((float) facet) / facets, y);
	    glVertex3f(x, y, z);
	    glTexCoord2f(((float) facet) / facets, (rib + 1) * (length / ribs));
	    glVertex3f(x, (rib + 1) * (length / ribs), z);
	}
	x = sin(0.) * radius;
	z = cos(0.) * radius;
	y = rib * (length / ribs);
	glNormal3f(sin(0.), 0.f, cos(0.));
	glTexCoord2f(((float) facet) / facets, y);
	glVertex3f(x, y, z);
	glTexCoord2f(((float) facet) / facets, (rib + 1) * (length / ribs));
	glVertex3f(x, (rib + 1) * (length / ribs), z);
	glEnd();
    }
#if 0
    for (rib = 0; rib < 2; rib++) {
	float x, y, z;
	glNormal3f(0.f, rib == 0 ? -1.f : 1.f, 0.f);
	glBegin(GL_TRIANGLE_FAN);
	y = rib * length;
	glTexCoord2f(0.f, y);
	glVertex3f(0.f, rib * length, 0.f);
	for (facet = 0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    glTexCoord2f(((float) facet) / facets, y);
	    glVertex3f(x, y, z);
	}
	x = sin(0.) * radius;
	z = cos(0.) * radius;
	glTexCoord2f(((float) facet) / facets, y);
	glVertex3f(x, y, z);
	glEnd();
    }
#else
    for (rib = 0; rib < 2; rib++) {
	float x, y, z;
	glNormal3f(0.f, rib == 0 ? -1.f : 1.f, 0.f);
	glBegin(GL_TRIANGLES);
	y = rib * length;
	for (facet = 0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    int ff = (facet == facets - 1) ? 0 : facet + 1;
	    double angle2 = ff * (2.0 * M_PI / facets);
	    float x2 = sin(angle2) * radius;
	    float z2 = cos(angle2) * radius;
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    glTexCoord2f((facet + .5) / facets, y);
	    glVertex3f(0.f, y, 0.f);
	    glTexCoord2f(((float) facet) / facets, y);
	    glVertex3f(x, y, z);
	    glTexCoord2f(((float) (facet + 1)) / facets, y);
	    glVertex3f(x2, y, z2);
	}
	glEnd();
    }
#endif
}

void
make_stripes(void)
{
    int i, j;
    static GLubyte data[32 * 16];
    for (i = 0; i < 32; i++) {
	float f = drand48() * .5f;
	if (i == 1)
	    f = 1.f;
	for (j = 0; j < 16; j++) {
	    data[j * 32 + i] = f * 255;
	}
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 32, 16, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    CHECK_ERROR("make_stripes()");
}

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);
    (void) glutCreateWindow("npr");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    create_menu();

    glClearColor(.5f, .5f, .5f, 1.f);

    /*
     * draw a perspective scene 
     */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 300., 600.);
    glMatrixMode(GL_MODELVIEW);
    /*
     * look at scene from (0, 0, 450) 
     */
    gluLookAt(0., 0., 450., 0., 0., 0., 0., 1., 0.);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);

    /*
     * Normal light 
     */
    glLightfv(GL_LIGHT0, GL_POSITION, light0pos);

    /*
     * Two-tone lights 
     */
    glLightfv(GL_LIGHT1, GL_AMBIENT, lightambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, zero);
    glLightfv(GL_LIGHT1, GL_POSITION, light1pos);

    glLightfv(GL_LIGHT2, GL_AMBIENT, lightambient);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, light2diffuse);
    glLightfv(GL_LIGHT2, GL_SPECULAR, zero);
    glLightfv(GL_LIGHT2, GL_POSITION, light2pos);

    glCullFace(GL_BACK);
    glReadBuffer(GL_BACK);
    glDisable(GL_DITHER);

    /*
     * The cylinder body 
     */
    glNewList(1, GL_COMPILE);
    glPushMatrix();
    glTranslatef(0.f, -80.f, 0.f);
    cylinder(40, 10, 20, 160);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder(40, 3, 60, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder(40, 3, 30, 20);
    glTranslatef(0.f, 60.f, 0.f);
    cylinder(40, 3, 30, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder(40, 3, 60, 20);
    glPopMatrix();
    glEndList();

    /*
     * The cylinder edges 
     */
    glNewList(2, GL_COMPILE);
    glPushMatrix();
    glTranslatef(0.f, -80.f, 0.f);
    cylinder_edges(40, 20, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder_edges(40, 60, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder_edges(40, 30, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder_edges(40, 30, 40);
    glTranslatef(0.f, 40.f, 0.f);
    cylinder_edges(40, 30, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder_edges(40, 60, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder_edges(40, 20, 20);
    glPopMatrix();
    glEndList();

    make_stripes();
    material(TWOTONE);

    CHECK_ERROR("end of main");

    glutMainLoop();
    return 0;
}
