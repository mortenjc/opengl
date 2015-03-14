#include <stdlib.h>
#include <GL/glut.h>
#include <assert.h>
#include <stdio.h>
#include "../util/texture.h"

/*
** Demonstrate shadow textures
**/

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))


GLboolean dblbuf = GL_TRUE;

enum {SPHERE = 1, CONE, LIGHT, WALLS, FLOOR};

enum {X, Y, Z};

enum {SURFTEX, SHADTEX};

enum {NONE,
      LIGHTVIEW,
      SOFTLIGHTVIEW,
      BLACKLIGHTVIEW,
      SHADOW,
      SOFTSHAD,
      TOGFRUST,
      TOGTEX,
      EXIT};

int mode = NONE;
GLboolean showfrust = GL_FALSE;
GLboolean floortex = GL_FALSE;
char *progname;

GLfloat lightpos[] = {60.f, 50.f, -60.f, 1.f};

GLfloat floorcolor; /* used by draw_black() */

enum {LIGHTXY, LIGHTZ};
int active = LIGHTXY;


GLint winHeight = 1024, winWidth = 1024;

void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
}

/* set transforms to place viewer at light looking at floor */
void
setLightView(void)
{
    /* nearsize/10(= near plane dist) = 100/(light[Y] + 100) */
    GLfloat nearscale; /* nearplane scaling factor */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* from light height to floor = eye to far plane */
    /* near plane near light, far plane at floor */
	
    nearscale = 10.f/(lightpos[Y] + 100.f);
    glFrustum((-100. + lightpos[X]) * nearscale,
	      (100. + lightpos[X]) * nearscale,
	      (-100. - lightpos[Z]) * nearscale,
	      (100. - lightpos[Z]) * nearscale, 
	      10., lightpos[Y] + 101.); 
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* look straight at floor from light */
    gluLookAt(lightpos[X], lightpos[Y], lightpos[Z],
	      lightpos[X], -100., lightpos[Z],
	      0., 0., 1.);

}

void
setNormalView(void)
{
    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-100., 100., -100., 100., 320., 520.); 
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0., 0., 420., 0., 0., 0., 0., 1., 0.);
}

void
draw_black(GLfloat shadcolor)
{
    glDisable(GL_LIGHTING);

    /* normally black */
    glColor4f(shadcolor, shadcolor, shadcolor, 1.0);

    glCallList(CONE);
    glCallList(SPHERE);
    
    /* order dependent! Floor must be drawn last for soft shadows;
       (or blend differently, or use accum buffer, or copy each image
       separately)
       */
    /* 1 for single pass 1/pass for multisample light sources */
    glColor4f(floorcolor, floorcolor, floorcolor, 1.0);
    glCallList(FLOOR);

    glEnable(GL_LIGHTING);
}



void
motion(int x, int y)
{
    switch(active)
    {
    case LIGHTXY:
	lightpos[X] = (x - winWidth/2.f) * 100/(winWidth/2);
	lightpos[Y] = (winHeight/2.f - y) * 100/(winHeight/2);

	glutPostRedisplay();
	break;
    case LIGHTZ: 
	lightpos[Z] = (winHeight/2.f - y) * 100/(winHeight/2);

	glutPostRedisplay();
	break;
    }
    CHECK_ERROR("motion");
}

void
mouse(int button, int state, int x, int y)
{
    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN)
	switch(button)
	{
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = LIGHTXY;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = LIGHTZ;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* menu: never happens */
	    break;
	}
}


void
draw_frustum(void)
{
    /* draw edges of frustum from light */
    glDisable(GL_LIGHTING);
    glLineWidth(3.f);
    glBegin(GL_LINES);
    glColor4f(1.f, 0.f, 0.f, 1.0);
    glVertex3f(lightpos[X], lightpos[Y], lightpos[Z]);
    glVertex3f(-100.f, -100.f, -100.f);
    glVertex3f(lightpos[X], lightpos[Y], lightpos[Z]);
    glVertex3f( 100.f, -100.f, -100.f);
    glVertex3f(lightpos[X], lightpos[Y], lightpos[Z]);
    glVertex3f(-100.f, -100.f, 100.f);
    glVertex3f(lightpos[X], lightpos[Y], lightpos[Z]);
    glVertex3f( 100.f, -100.f, 100.f);
    glEnd();
    glEnable(GL_LIGHTING);
}

void
draw(void)
{
    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glCallList(WALLS);
    if(floortex)
	glEnable(GL_TEXTURE_2D);
    glCallList(FLOOR);
    if(floortex)
	glDisable(GL_TEXTURE_2D);
    glCallList(CONE);
    glCallList(SPHERE);
    
    /* move light around */
    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glCallList(LIGHT);
    glPopMatrix();
}


void 
redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    setNormalView();

    draw();
    if(showfrust)
	draw_frustum();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");

}

void 
redraw_light(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    setLightView();

    draw();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");

}

static int
max2pwr(int value)
{
  unsigned int rv = 0;

  assert(value > 0);

  value--;
  while (value != 0) {
    rv++;
    value = value >> 1;
  }
  return 1 << rv;
}

void 
redraw_shadow(void)
{
    int wid, ht;
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    setLightView(); /* put viewer at light */

    /* 
       don't exceed a reasonable texture size
       this should be done with proxys, but I'm guessing that 512 X 512
       is not too big
    */
    wid = max2pwr(MIN(512, winWidth)) >> 1;
    ht = max2pwr(MIN(512, winHeight)) >> 1;
    glViewport(0, 0, wid, ht);

    floorcolor = 1.f;
    if(floortex)
	draw_black(.25f);
    else
	draw_black(0.f);

    /* save shadow into texture */
    glBindTexture(GL_TEXTURE_2D, SHADTEX);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
		     wid, ht, 0);

    glViewport(0, 0, winWidth, winHeight);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    setNormalView();

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    if(showfrust)
	draw_frustum();

    glCallList(WALLS);
    glCallList(CONE);
    glCallList(SPHERE);
    
    /* move light around */
    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glCallList(LIGHT);
    glPopMatrix();


    glEnable(GL_TEXTURE_2D);

    if(floortex)
    {
	glBindTexture(GL_TEXTURE_2D, SURFTEX);
    
	glCallList(FLOOR); /* texture floor */

	/* for blending shadow tex with floor tex */
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);

	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_LIGHTING);
	glColor4f(1.f, 1.f, 1.f, 1.f);
    }
    glBindTexture(GL_TEXTURE_2D, SHADTEX);
    glCallList(FLOOR); /* modulate floor color with shadow texture */
    if(floortex)
    {
	glEnable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LESS);
    }

    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, SURFTEX);

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw_shadow()");
}

int count = 3;

void 
redraw_softshadow(void)
{
    int i, j;
    int wid, ht;
    GLfloat savelightpos[4];

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    /* draw from multiple light positions to build up image */

    savelightpos[X] = lightpos[X];
    savelightpos[Z] = lightpos[Z];
    
    glBlendFunc(GL_ONE, GL_ONE); /* for soft shadows */
    glEnable(GL_BLEND);

    /* 
       don't exceed a reasonable texture size
       this should be done with proxys, but I'm guessing that 512 X 512
       is not too big
    */
    wid = max2pwr(MIN(512, winWidth)) >> 1;
    ht = max2pwr(MIN(512, winHeight)) >> 1;
    glViewport(0, 0, wid, ht);
    for(j = 0; j < count; j++)
	for(i = 0; i < count; i++)
	{
	    lightpos[X] = savelightpos[X] + 4.f * i;
	    lightpos[Z] = savelightpos[Z] + 4.f * j;
	    setLightView(); /* put viewer at light */
	    floorcolor = 1.f/(count * count); /* 1/count^2 passes add to 1.f */
	    draw_black(0.f);
	    glClear(GL_DEPTH_BUFFER_BIT);
	}
    glDisable(GL_BLEND);
    lightpos[X] = savelightpos[X];
    lightpos[Z] = savelightpos[Z];

    /* save shadow into texture */
    glBindTexture(GL_TEXTURE_2D, SHADTEX);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
		     wid, ht, 0);

    glViewport(0, 0, winWidth, winHeight);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    setNormalView();

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    if(showfrust)
	draw_frustum();

    glCallList(WALLS);
    glCallList(CONE);
    glCallList(SPHERE);
    
    /* move light around */
    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glCallList(LIGHT);
    glPopMatrix();

    /* texture floor with shadow */
    glEnable(GL_TEXTURE_2D);
    if(floortex)
    {
	glBindTexture(GL_TEXTURE_2D, SURFTEX);
    
	glCallList(FLOOR); /* texture floor */

	/* for blending shadow tex with floor tex */
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);

	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, SHADTEX);
	glColor4f(1.f, 1.f, 1.f, 1.f);
    }
    glCallList(FLOOR);
    if(floortex)
    {
	glEnable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LESS);
    }


    glDisable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, SURFTEX);

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw_softshadow()");
}

int showSoft = 0;

void 
redraw_black(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    setLightView();

    if (showSoft) {
        int j, i;
        GLfloat savelightpos[4];

        glBlendFunc(GL_ONE, GL_ONE); /* for soft shadows */
        glEnable(GL_BLEND);

        savelightpos[X] = lightpos[X];
        savelightpos[Z] = lightpos[Z];

        for(j = 0; j < count; j++) {
    	    for(i = 0; i < count; i++) {
	        lightpos[X] = savelightpos[X] + 4.f * i;
	        lightpos[Z] = savelightpos[Z] + 4.f * j;
	        setLightView(); /* put viewer at light */
	        floorcolor = 1.f/(count * count); /* 1/count^2 passes add to 1.f */
	        draw_black(0.f);
	        glClear(GL_DEPTH_BUFFER_BIT);
	    }
        }
        lightpos[X] = savelightpos[X];
        lightpos[Z] = savelightpos[Z];
        glDisable(GL_BLEND);
    } else {
        floorcolor = 1.f;
        draw_black(0.f);
    }

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw_black()");
}


/*ARGSUSED1*/
void key(unsigned char key, int x, int y)
{

    switch(key)
    {
    case 'n': /* show scene with no shadow */
    case 'N':
	glutDisplayFunc(redraw);
	glutPostRedisplay();
	break;
    case 'l': /* show floor from light view */
    case 'L':
	glutDisplayFunc(redraw_light);
	glutPostRedisplay();
	break;
    case 'b': /* show floor from light view with shadow objs black */
    case 'B':
        showSoft = 0;
	glutDisplayFunc(redraw_black);
	glutPostRedisplay();
	break;
    case 'c': /* show floor from light view with soft shadow objs black */
    case 'C':
        showSoft = 1;
	glutDisplayFunc(redraw_black);
	glutPostRedisplay();
	break;
    case 'h': /* show scene with floor with shadow texture */
    case 'H':
	glutDisplayFunc(redraw_shadow);
	glutPostRedisplay();
	break;
    case 's': /* show scene with floor with soft shadow texture */
    case 'S':
	glutDisplayFunc(redraw_softshadow);
	glutPostRedisplay();
	break;
    case 'f':
    case 'F': /* toggle showing frustum from light to pgon */
	showfrust = !showfrust;
	glutPostRedisplay();
	break;
    case 't':
    case 'T': /* toggle surface texture on floor */
	floortex = !floortex;
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
    default:
	fprintf(stderr, "%s: Keyboard commands:\n"
		"n, N - scene with no shadow\n"
		"l, L - scene from lights point of view\n"
		"b, B - scene from lights point of view; obscuring objs black\n"
		"c, C - scene from lights point of view; obscuring objs soft\n"
		"h, H - scene with hard shadows\n"
		"s, S - scene with soft shadows\n"
		"f, F - toggle showing frustum from light view\n"
		"t, T - toggle surface texture on floor\n\n", progname);

	break;
    }
}

void
menu(int choice)
{
    switch(choice)
    {
    case NONE:
	key('n', 0, 0);
	break;
    case LIGHTVIEW:
	key('l', 0, 0);
	break;
    case BLACKLIGHTVIEW:
	key('b', 0, 0);
	break;
    case SOFTLIGHTVIEW:
	key('c', 0, 0);
	break;
    case SHADOW:
	key('h', 0, 0);
	break;
    case SOFTSHAD:
	key('s', 0, 0);
	break;
    case TOGFRUST:
	key('f', 0, 0);
	break;
    case TOGTEX:
	key('t', 0, 0);
	break;
    case EXIT:
	exit(0);
    }
    glutPostRedisplay();
}






/* material properties for objects in scene */
static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};

/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    GLUquadricObj *sphere, *cone, *base;
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};
    unsigned *floortex;
    int texcomps, texwid, texht;

    progname = argv[0];

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1)
    {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
	while(!done)
	{
	    switch(*args)
	    {
	    case 's': /* single buffer */
	    case 'S': /* single buffer */
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

    (void)glutCreateWindow("shadow textures");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

#if 0
    glutCreateMenu(menu);
    glutAddMenuEntry("No Shadows (n, N)", NONE);
    glutAddMenuEntry("Floor View from Light (l, L)", LIGHTVIEW);
    glutAddMenuEntry("Floor View with Black Objs (b, B)", BLACKLIGHTVIEW);
    glutAddMenuEntry("Floor View with Soft Objs (c, C)", SOFTLIGHTVIEW);
    glutAddMenuEntry("Hard Shadows (h, H)", SHADOW);
    glutAddMenuEntry("Soft Shadows (s, S)", SOFTSHAD);
    glutAddMenuEntry("Toggle showing frustum (f, F)", TOGFRUST);
    glutAddMenuEntry("Toggle floor texture (t, T)", TOGTEX);
    glutAddMenuEntry("Exit Program", EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
#endif


    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 320., 520.); 
    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0., 0., 420., 0., 0., 0., 0., 1., 0.);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glBindTexture(GL_TEXTURE_2D, SHADTEX);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, SURFTEX);

    /* make display lists for sphere and cone; for efficiency */

    glNewList(WALLS, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

    glBegin(GL_QUADS);

    /* left wall */
    glNormal3f(1.f, 0.f, 0.f);
    glVertex3f(-100.f, -100.f, 100.f);
    glVertex3f(-100.f, -100.f, -100.f);
    glVertex3f(-100.f,  100.f, -100.f);
    glVertex3f(-100.f,  100.f, 100.f);

    /* right wall */
    glNormal3f(-1.f, 0.f, 0.f);
    glVertex3f( 100.f, -100.f, 100.f);
    glVertex3f( 100.f,  100.f, 100.f);
    glVertex3f( 100.f,  100.f, -100.f);
    glVertex3f( 100.f, -100.f, -100.f);

    /* ceiling */
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(-100.f,  100.f, 100.f);
    glVertex3f(-100.f,  100.f, -100.f);
    glVertex3f( 100.f,  100.f, -100.f);
    glVertex3f( 100.f,  100.f, 100.f);

    /* back wall */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-100.f, -100.f, -100.f);
    glVertex3f( 100.f, -100.f, -100.f);
    glVertex3f( 100.f,  100.f, -100.f);
    glVertex3f(-100.f,  100.f, -100.f);
    glEnd();
    glEndList();

    glNewList(FLOOR, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

    glBegin(GL_QUADS);

    /* floor */
    glNormal3f(0.f, 1.f, 0.f);
    glTexCoord2i(1, 1);
    glVertex3f(-100.f, -100.f, 100.f);
    glTexCoord2i(0, 1);
    glVertex3f( 100.f, -100.f, 100.f);
    glTexCoord2i(0, 0);
    glVertex3f( 100.f, -100.f, -100.f);
    glTexCoord2i(1, 0);
    glVertex3f(-100.f, -100.f, -100.f);

    glEnd();
    glEndList();

    glNewList(SPHERE, GL_COMPILE);
    sphere = gluNewQuadric();
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
    glPushMatrix();
    glTranslatef(30.f, -50.f, -60.f);
    gluSphere(sphere, 20.f, 20, 20);
    glPopMatrix();
    gluDeleteQuadric(sphere);
    glEndList();

    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
    glPushMatrix(); /* place cone */
    glTranslatef(-40.f, -80.f, -20.f);
    glRotatef(-90.f, 1.f, 0.f, 0.f);
    gluDisk(base, 0., 20., 20, 1);
    gluCylinder(cone, 20., 0., 60., 20, 20);
    glPopMatrix();
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    glNewList(LIGHT, GL_COMPILE);
    sphere = gluNewQuadric();
    glDisable(GL_LIGHTING);
    glColor3f(.9f, .9f, .6f);
    gluSphere(sphere, 5.f, 20, 20);
    glEnable(GL_LIGHTING);
    gluDeleteQuadric(sphere);
    glEndList();

    floortex = read_texture("../data/plank.rgb",
			    &texwid, &texht, &texcomps);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, texwid, texht, GL_RGBA,
		      GL_UNSIGNED_BYTE, floortex);

    free(floortex);


    key('?', 0, 0);

    glutMainLoop();
    return 0;
}
