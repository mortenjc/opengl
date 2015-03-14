#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"
#ifdef _WIN32
#ifdef GL_EXT_blend_minmax
#include <windows.h>  /* for wglGetProcAddress */
PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT;
#endif
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)sqrt(x))
#define floorf(x) ((float)floor(x))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#define drand48()   ((float)rand()/(float)RAND_MAX)
#endif
#ifdef GL_EXT_texture
#define glBindTexture	glBindTextureEXT
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
enum {NONE, TWO_PASS, SCREEN };
static int mode = NONE;

/* window dimensions */
int winWidth = 512;
int winHeight = 512;
int active;
int object = 4;
int maxobject;

int hasBlendMinmax = 0;

GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[3] = {0.f, 0.f, 0.f};

GLfloat color[4] = {1.f, 1.f, 1.f, 1.f};
GLfloat zero[4] = {0.f, 0.f, 0.f, 1.f};
GLfloat one[4] = {1.f, 1.f, 1.f, 1.f};

GLboolean lightchanged = GL_TRUE;
GLfloat lightpos[4] = {.57735f, .57735f, .57735f, 0.f};
GLfloat sscale = 72.0f, tscale = 30.0f;
int ox, oy;

static int texturing;

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
	glutPostRedisplay();
	break;
    case OBJ_TRANSLATE:
#if 0
	objpos[X] = (x - winWidth/2) * 100./winWidth;
	objpos[Y] = (winHeight/2 - y) * 100./winHeight;
#else
	objpos[Z] += x - ox;
	ox = x; oy = y;
#endif
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y) {

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN)
	switch(button)
	{
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = OBJ_ANGLE;
	    /*motion(x, y);*/
	    ox = x; oy = y;
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    active = OBJ_PICK;
	    /*motion(x, y);*/
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = OBJ_TRANSLATE;
	    /*motion(x, y);*/
	    ox = x; oy = y;
	    break;
	}
}

void
redraw(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glScalef(sscale, tscale, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], objpos[Z]); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    
    if(lightchanged)
    {
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	lightchanged = GL_FALSE;
    }

    if (mode == TWO_PASS) {
	glDisable(GL_TEXTURE_2D);
	glCallList(object);
#ifdef GL_EXT_blend_minmax
        if (hasBlendMinmax) {
	  glColor4f(.5f, .5, .5, 1.f);
	  glBlendEquationEXT(GL_MAX_EXT);
	  glEnable(GL_BLEND);
	  glEnable(GL_TEXTURE_2D);
	  glDisable(GL_LIGHTING);
	  glCallList(object);
	  glDisable(GL_BLEND);
	  glEnable(GL_LIGHTING);
	  glColor4f(1.f, 1.f, 1.f, 1.f);
	} else
#endif
        {
	  glColor4f(.5f, .5f, .5f, .5f);
	  glEnable(GL_COLOR_LOGIC_OP);
	  glLogicOp(GL_OR);
	  glDisable(GL_LIGHTING);
	  glEnable(GL_TEXTURE_2D);
	  glCallList(object);
	  glDisable(GL_COLOR_LOGIC_OP);
	  glEnable(GL_LIGHTING);
	}
    } else if (mode == SCREEN) {
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glColor4f(.5f, .5, .5, 1.f);
	glCallList(object);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
    } else {
	glCallList(object);
    }

    glPopMatrix(); /* assuming modelview */
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);

    CHECK_ERROR("OpenGL Error in redraw()");
    glutSwapBuffers(); 
}

void
help(void) {
    printf("g    - generate texture coordinate s\n");
    printf("k    - generate texture coordinates s,t\n");
    printf("o    - next object\n");
    printf("t    - next texture\n");
    printf("w    - toggle wireframe/solid\n");
    printf("s,S  - scale texture up/down in s direction\n");
    printf("y,Y  - scale texture up/down in t direction\n");
    printf("ESC  - quit\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    static int wire;
    static int texgen, stipple;
    static int lighting = 1;
    switch(key)
    {
    case '0':
	mode = NONE;
	break;
    case '2':
	mode = TWO_PASS;
	break;
    case '1':
	mode = SCREEN;
	break;
    case 'p':
    	if (stipple ^= 1)
	    glEnable(GL_POLYGON_STIPPLE);
	else
	    glDisable(GL_POLYGON_STIPPLE);
	break;
    case 'o':
	/* toggle object type */
	object++;
	if(object > maxobject)
	    object = 1;
	break;
    case 'g':
    	if (texgen ^= 1)
	    glEnable(GL_TEXTURE_GEN_S);
	else
	    glDisable(GL_TEXTURE_GEN_S);
	break;
    case 'k':
    	if (texgen ^= 1) {
	    glEnable(GL_TEXTURE_GEN_S);
	    glEnable(GL_TEXTURE_GEN_T);
	} else {
	    glDisable(GL_TEXTURE_GEN_S);
	    glDisable(GL_TEXTURE_GEN_T);
	}
	glutPostRedisplay();
	break;
    case 't':
    	texturing += 1;
	if (texturing > 3) texturing = 0;
	if (texturing) {
	    glBindTexture(GL_TEXTURE_2D, texturing);
	    glEnable(GL_TEXTURE_2D);
	} else {
	    glDisable(GL_TEXTURE_2D);
	}
	break;
    case 'w':
	if (wire ^= 1)
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	break;
    case 's':
    	sscale *=.9f; printf("sscale %f\n", sscale); break;
    case 'S':
    	sscale *= 1.1f; printf("sscale %f\n", sscale); break;
    case 'y':
    	tscale *= .9f; printf("tscale %f\n", tscale); break;
    case 'Y':
    	tscale *= 1.1f; printf("tscale %f\n", tscale); break;
    case 'l':
	lighting ^= 1;
	if (lighting) glEnable(GL_LIGHTING);
	else glDisable(GL_LIGHTING);
	break;
    case '\033':
	exit(0);
	break;
    default:
	help();
	break;
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
    glutAddMenuEntry("show regular", '0');
    glutAddMenuEntry("two pass", '2');
    glutAddMenuEntry("show screen", '1');
    glutAddMenuEntry("texturing", 't');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

static void
torus(float r, float R, GLint nsides, GLint rings)
{
    int i, j;
    float theta, phi, theta1, phi1;
    float p0[03], p1[3], p2[3], p3[3];
    float n0[3], n1[3], n2[3], n3[3];

    for (i = 0; i < rings; i++) {
	theta = (float) i *2.0 * M_PI / rings;
	theta1 = (float) (i + 1) * 2.0 * M_PI / rings;
	for (j = 0; j < nsides; j++) {
	    phi = (float) j *2.0 * M_PI / nsides;
	    phi1 = (float) (j + 1) * 2.0 * M_PI / nsides;

	    p0[0] = cos(theta) * (R + r * cos(phi));
	    p0[1] = -sin(theta) * (R + r * cos(phi));
	    p0[2] = r * sin(phi);

	    p1[0] = cos(theta1) * (R + r * cos(phi));
	    p1[1] = -sin(theta1) * (R + r * cos(phi));
	    p1[2] = r * sin(phi);

	    p2[0] = cos(theta1) * (R + r * cos(phi1));
	    p2[1] = -sin(theta1) * (R + r * cos(phi1));
	    p2[2] = r * sin(phi1);

	    p3[0] = cos(theta) * (R + r * cos(phi1));
	    p3[1] = -sin(theta) * (R + r * cos(phi1));
	    p3[2] = r * sin(phi1);

	    n0[0] = cos(theta) * (cos(phi));
	    n0[1] = -sin(theta) * (cos(phi));
	    n0[2] = sin(phi);

	    n1[0] = cos(theta1) * (cos(phi));
	    n1[1] = -sin(theta1) * (cos(phi));
	    n1[2] = sin(phi);

	    n2[0] = cos(theta1) * (cos(phi1));
	    n2[1] = -sin(theta1) * (cos(phi1));
	    n2[2] = sin(phi1);

	    n3[0] = cos(theta) * (cos(phi1));
	    n3[1] = -sin(theta) * (cos(phi1));
	    n3[2] = sin(phi1);

	    glBegin(GL_QUADS);
	    glNormal3fv(n3);
	    glTexCoord2f(((float)i)/rings, (j+1.f)/nsides);
	    glVertex3fv(p3);
	    glNormal3fv(n2);
	    glTexCoord2f((i+1.f)/rings, (j+1.f)/nsides);
	    glVertex3fv(p2);
	    glNormal3fv(n1);
	    glTexCoord2f((i+1.f)/rings, ((float)j)/nsides);
	    glVertex3fv(p1);
	    glNormal3fv(n0);
	    glTexCoord2f(((float)i)/rings, ((float)j)/nsides);
	    glVertex3fv(p0);
	    glEnd();
	}
    }
}

static void
cylinder(int facets, int ribs, float radius, float length) {
    int facet, rib;

    for (rib=0; rib < ribs; rib++) {
	float x, y, z;
	glBegin(GL_TRIANGLE_STRIP);
	for (facet=0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    y = rib * (length / ribs);
	    glNormal3f(sin(angle), 0.f, cos(angle));
	    glTexCoord2f(((float)facet)/facets, y);
	    glVertex3f(x, y, z);
	    glTexCoord2f(((float)facet)/facets, (rib+1)*(length/ribs));
	    glVertex3f(x, (rib+1)*(length/ribs), z);
	}
	x = sin(0.) * radius;
	z = cos(0.) * radius;
	y = rib * (length / ribs);
	glNormal3f(sin(0.), 0.f, cos(0.));
	glTexCoord2f(((float)facet)/facets, y);
	glVertex3f(x, y, z);
	glTexCoord2f(((float)facet)/facets, (rib+1)*(length/ribs));
	glVertex3f(x, (rib+1)*(length/ribs), z);
	glEnd();
    }
#if 0
    for(rib = 0; rib < 2; rib++) {
        float x, y, z;
	glNormal3f(0.f, rib == 0 ? -1.f : 1.f, 0.f);
	glBegin(GL_TRIANGLE_FAN);
	y = rib * length;
	glTexCoord2f(0.f, y);
	glVertex3f(0.f, rib*length, 0.f);
	for (facet=0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    glTexCoord2f(((float)facet)/facets, y);
	    glVertex3f(x, y, z);
	}
	x = sin(0.) * radius;
	z = cos(0.) * radius;
	glTexCoord2f(((float)facet)/facets, y);
	glVertex3f(x, y, z);
	glEnd();
    }
#else
    for(rib = 0; rib < 2; rib++) {
        float x, y, z;
	glNormal3f(0.f, rib == 0 ? -1.f : 1.f, 0.f);
	glBegin(GL_TRIANGLES);
	y = rib * length;
	for (facet=0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    int ff = (facet == facets-1) ? 0 : facet+1;
	    double angle2 = ff * (2.0 * M_PI / facets);
	    float x2 = sin(angle2) * radius;
	    float z2 = cos(angle2) * radius;
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    glTexCoord2f((facet+.5)/facets, y);
	    glVertex3f(0.f, y, 0.f);
	    glTexCoord2f(((float)facet)/facets, y);
	    glVertex3f(x, y, z);
	    glTexCoord2f(((float)(facets+1))/facets, y);
	    glVertex3f(x2, y, z2);
	}
	glEnd();
    }
#endif
}

void
make_stripes(void) {
    int i, j;
    static GLubyte data[16*8];
    static GLubyte data1[16*8];
    static GLubyte data2[16*8];
    double d[] = { 0.f, 1.f, 0.f, 1.f};
    for(i = 0; i < 8; i++) {
        for(j = 0; j < 16; j++) {
	    float f = 1.;
	    if (i < 2 || j < 4 ) f = 0.f;
	    data[j*8+i] = f*255;

	    f = 0.f;
	    if (i < 4) f = 1.f;
	    data1[j*8+i] = f*255;

	    f = 0.f;
	    if (j < 8) f = 1.f;
	    data2[j*8+i] = f*255;
	}
    }
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 8, 16, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 8, 16, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 8, 16, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#if 0
    glMatrixMode(GL_TEXTURE);
    glScalef(50.f, 1.f, 1.f);
    glScalef(.5f, .5f, 1.f);
    glMatrixMode(GL_MODELVIEW);
#endif
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGendv(GL_T, GL_OBJECT_PLANE, d);
    CHECK_ERROR("make_stripes()");
}

void
make_stipple(void) {
    GLuint stipple[32];
    int i;
    for(i = 0; i < 32; i++) {
    	stipple[i] = 0xff00ff00;
    }
    glPolygonStipple((GLubyte *)stipple);
}

int
main(int argc, char *argv[]) {
    GLUquadricObj *quad;
    GLfloat zero[] = { 0.f, 0.f, 0.f, 0.f };

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    (void)glutCreateWindow("hatch");

#ifdef GL_EXT_blend_minmax
    hasBlendMinmax = glutExtensionSupported("GL_EXT_blend_minmax");

# ifdef _WIN32
    /* grab extension function pointer for glBlendEquationEXT */
    if (hasBlendMinmax) {
      glBlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC) wglGetProcAddress("glBlendEquationEXT");
      if (glBlendEquationEXT == NULL) {
        hasBlendMinmax = 0;
      }
    }
# endif
#endif

    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    //create_menu();

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
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, zero);
    
    glCullFace(GL_BACK);
    glReadBuffer(GL_BACK);
    glDisable(GL_DITHER);

    glNewList(1, GL_COMPILE);
    glutSolidCube(50.f);
    glEndList();

    glNewList(2, GL_COMPILE);
    glutSolidTeapot(50.f);
    glEndList();

    quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);

    glNewList(3, GL_COMPILE);
    gluSphere(quad, 70., 20, 20);
    glEndList();

    gluDeleteQuadric(quad);

    glNewList(4, GL_COMPILE);
#if 0
    glutSolidTorus(20, 50, 50, 50);
#endif
    torus(20, 50, 50, 100);
    glEndList();

    glNewList(5, GL_COMPILE);
    glPushMatrix();
    glTranslatef(0.f, -80.f, 0.f);
    cylinder(40, 3, 20, 160);
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

    maxobject = 5;

    make_stripes();
    make_stipple();

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClearColor(1.f, 1.f, 1.f, 1.0f);
    glDepthFunc(GL_LEQUAL);
    CHECK_ERROR("end of main");

    help();
    glutMainLoop();
    return 0;
}
