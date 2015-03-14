/*
 *
 * Demonstrate multiple planar reflections using stencil buffer
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <GL/glut.h>
#include "../util/texture.h"

#ifdef WIN32
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

/* TEST PROGRAM */

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#define RAD(x) (((x)*M_PI)/180.)

enum {NODLIST, SPHERE, CONE, LIGHT};
enum {NORMAL, REFLECTION, MIRRORED, DARK, STENCIL, DEMO, EXIT};
enum {NONE, RIGHT, BACK, DUAL};

GLfloat viewangle[] = {0.f, 0.f};
GLfloat viewdist = 400.f;
GLfloat viewpos[]   = {0.f, 100.f, 400.f}; /* position of viewer */

GLfloat lightpos[]  = {50.f, 20.f, 0.f, 1.f};
GLfloat spherepos[] = {0.f, -80.f, -80.f};

enum {X, Y, Z};
enum {MOVE_ANGLE, MOVE_DIST, MOVE_SPHERE};
int active;
int dblbuf = GL_TRUE;
GLboolean darken = GL_TRUE;
GLboolean stencil = GL_FALSE;
GLboolean animate = GL_FALSE;


int winwid = 1024;
int winht = 1024;

void
updateMV(void)
{
    /* convert angles into a view position */
	viewpos[X] = viewdist * cos(RAD(viewangle[X])) * cos(RAD(viewangle[Y]));
	viewpos[Y] = viewdist * sin(RAD(viewangle[Y])) -100.f;
	viewpos[Z] = viewdist * sin(RAD(viewangle[X])) * cos(RAD(viewangle[Y]));

	/* assume GL_MODELVIEW matrix is current */
	glLoadIdentity();
	glTranslatef(0.f, -100.f, 0.f);
	gluLookAt(viewpos[X], viewpos[Y], viewpos[Z],
		  0., -100., 0., 
		  0., 1., 0.);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
}

void
idle(void)
{
    static GLfloat vel = -.5f;
    
    if(spherepos[Y] < -80.f || spherepos[Y] > 80.f)
	vel = -vel;

    spherepos[Y] += vel;
    glutPostRedisplay();
}

void
motion(int x, int y)
{

    switch(active)
    {
    case MOVE_ANGLE:
    {
	viewangle[X]  = (x - winwid) * 360.f/winwid;
	viewangle[Y]  = (winht - y) * 90.f/winht;
	if(viewangle[Y] < 0.f)
	    viewangle[Y] = 0.f;
	if(viewangle[Y] >= 90.f)
	    viewangle[Y] = 89.f;

	updateMV();
	glutPostRedisplay();
	break;
    }
    case MOVE_DIST:
	viewdist = 400.f + (winht/2 - y) * 400./winht;
	updateMV();
	glutPostRedisplay();
	break;
    case MOVE_SPHERE:
	spherepos[X] = (x - winwid/2) * 100.f/winwid;
	spherepos[Z] = (y - winht/5)  * 200.f/winht;
	glutPostRedisplay();
	break;
    }
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
	    active = MOVE_ANGLE;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = MOVE_DIST;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* move the sphere */
	    active = MOVE_SPHERE;
	    motion(x, y);
	    break;
	}
}

void
reshape(int wid, int ht)
{
    winwid = wid;
    winht = ht;
    glViewport(0, 0, wid, ht);
}


/* draw one of three mirrors on the walls */
void
draw_mirror(int mirror)
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    switch(mirror)
    {
    case NONE:
	break;
    case RIGHT:
	glBegin(GL_QUADS);
	glNormal3f(-1.f, 0.f, 0.f);
	glVertex3f( 100.f, -60.f,  40.f);
	glVertex3f( 100.f,  80.f,  40.f);
	glVertex3f( 100.f,  80.f, -40.f);
	glVertex3f( 100.f, -60.f, -40.f);
	glEnd();
	break;
    case BACK:
	glBegin(GL_QUADS);
	glNormal3f(0.f, 0.f, 1.f);
	glVertex3f(-70.f, -30.f, -100.f);
	glVertex3f( 70.f, -30.f, -100.f);
	glVertex3f( 70.f,  30.f, -100.f);
	glVertex3f(-70.f,  30.f, -100.f);
	glEnd();
	break;
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    CHECK_ERROR("draw_mirror()");
}


/* Called when window needs to be redrawn */
void
draw(GLfloat scale)
{
    glBegin(GL_QUADS);

    glColor3f(1.f * scale, 1.f * scale, 1.f * scale);

    /* left wall */
    glNormal3f(1.f, 0.f, 0.f);
    glVertex3f(-100.f, -100.f,  100.f);
    glVertex3f(-100.f, -100.f, -100.f);
    glVertex3f(-100.f,  100.f, -100.f);
    glVertex3f(-100.f,  100.f,  100.f);


    /* right wall */
    glNormal3f(-1.f, 0.f, 0.f);
    glVertex3f( 100.f, -100.f,  100.f);
    glVertex3f( 100.f,  100.f,  100.f);
    glVertex3f( 100.f,  100.f, -100.f);
    glVertex3f( 100.f, -100.f, -100.f);

    /* ceiling */
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(-100.f,  100.f,  100.f);
    glVertex3f(-100.f,  100.f, -100.f);
    glVertex3f( 100.f,  100.f, -100.f);
    glVertex3f( 100.f,  100.f,  100.f);


    /* back wall */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-100.f, -100.f, -100.f);
    glVertex3f( 100.f, -100.f, -100.f);
    glVertex3f( 100.f,  100.f, -100.f);
    glVertex3f(-100.f,  100.f, -100.f);


    /* front wall */
    glNormal3f(0.f, 0.f, -1.f);
    glVertex3f(-100.f, -100.f,  100.f);
    glVertex3f(-100.f,  100.f,  100.f);

    glVertex3f( 100.f,  100.f,  100.f);
    glVertex3f( 100.f, -100.f,  100.f);
    glEnd();

    /* floor */
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glNormal3f(0.f, 1.f, 0.f);
    glTexCoord2i(0, 0);
    glVertex3f(-100.f, -100.f,  100.f);
    glTexCoord2i(2, 0);
    glVertex3f( 100.f, -100.f,  100.f);
    glTexCoord2i(2, 2);
    glVertex3f( 100.f, -100.f, -100.f);
    glTexCoord2i(0, 2);
    glVertex3f(-100.f, -100.f, -100.f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    /* draw the sphere */
    glPushMatrix();
    glTranslatef(spherepos[X], spherepos[Y], spherepos[Z]);
    glColor3f(1.f * scale, .5f * scale, 0.f * scale);
    glCallList(SPHERE);
    glPopMatrix();

    /* draw the cone */
    glPushMatrix();
    glTranslatef(-60.f, -100.f, -5.f);
    glColor3f(0.f * scale, .5f * scale, 1.f * scale);
    glCallList(CONE);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glColor3f(.7 * scale, .2 * scale, .7 * scale);
    glCallList(LIGHT);
    glPopMatrix();

}



/* change modelview matrix to cause a reflection about the XZ axis */
/* need to change the viewing transform to do this */
void
reflection(int which)
{
    switch(which) /* which reflection to do */
    {
    case NONE: /* undo reflection */
	glPopMatrix();
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glCullFace(GL_BACK);
	break;
    case RIGHT: /* right mirror */
	glPushMatrix();
	glScalef(-1.f, 1.f, 1.f);
	glTranslatef(-200.f, 0.f, 0.f);

	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glCullFace(GL_FRONT);
	break;
    case BACK: /* back mirror */
	glPushMatrix();
	glScalef(1.f, 1.f, -1.f);
	glTranslatef(0.f, 0.f, 200.f);

	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glCullFace(GL_FRONT);
	break;
    case DUAL: /* second reflection (both 2nd reflections are the same) */
	glPushMatrix();
	glScalef(1.f, 1.f, -1.f);
	glTranslatef(0.f, 0.f, 200.f);

	glScalef(-1.f, 1.f, 1.f);
	glTranslatef(-200.f, 0.f, 0.f);

	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glCullFace(GL_BACK);
	break;
    }
}

/* Called when window needs to be redrawn */
void
redraw_normal(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    draw(1.f); /* draw the unreflected scene */

    glEnable(GL_POLYGON_OFFSET_FILL);
    glColor3f(0.f, 0.f, .7f);
    draw_mirror(BACK);
    draw_mirror(RIGHT);
    glDisable(GL_POLYGON_OFFSET_FILL);

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("redraw_normal()");
}


/* Called when window needs to be redrawn */
void
redraw_reflection(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    reflection(RIGHT);
    if(darken)
	draw(.6f);
    else
	draw(1.f);
    reflection(NONE);

    reflection(BACK);
    if(darken)
	draw(.6f);
    else
	draw(1.f);
    reflection(NONE);


    reflection(DUAL);
    if(darken)
	draw(.36f);
    else
	draw(1.f);
    reflection(NONE);


    /* restore unreflected light position */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("redraw_reflection()");
}

/* 
   decrement stencil buffer and punch a hole in depth buffer where stencil
   equals 2
*/
void
punch(void)
{
    /* we have to decrement the current reflection (1 -> 0) and the
       mirrors (2 -> 1). We do this to limit the number of stencil
       bits we need to 2.
    */

    glEnable(GL_STENCIL_TEST);
    glPushMatrix(); /* assuming GL_MODELVIEW */
    glLoadIdentity();
    glDisable(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_NOTEQUAL, 0, 3);
    /* mirror: 2 -> 1, rest of scene: 1 -> 0 */
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR); /* decrement stencil */
    /* there are better ways to do this */

    glBegin(GL_QUADS);
    glColor3f(1.f, 1.f, 1.f); /* for debugging */
    glVertex3i(-10, -10, -20);
    glVertex3i( 10, -10, -20);
    glVertex3i( 10,  10, -20);
    glVertex3i(-10,  10, -20);
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    /* clear z buffer where stencil == 1 (visible part of mirror) */

    glDepthFunc(GL_ALWAYS);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glBegin(GL_QUADS);
    glColor3f(0.f, 1.f, 1.f); /* for debugging */
    glVertex3i(-500, -500, -990);
    glVertex3i( 500, -500, -990);
    glVertex3i( 500,  500, -990);
    glVertex3i(-500,  500, -990);
    glEnd();
    glPopMatrix();

    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
}

/* draw the scene with the current reflection transform, masking so only
   mirrored parts are drawn. Stencil is 1 where mirror is.
*/
void
draw_mirrored(GLfloat scale, int which)
{
    glEnable(GL_STENCIL_TEST);

    /* only draw where stencil == 1 */
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); /* don't change stencil */
    draw(scale);

    /*			  
     * Draw mirror into stencil,depth,color buffer (stencil: 1->2)
     * mirrors are masked to where stencil is already 1
     */
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); /* arg 2 is no-op */
    glColor3f(0.f, 0.f, .7f); /* for debugging */
    draw_mirror(which);

     /* turn off the stencil test */

    glDisable(GL_STENCIL_TEST);
}


/* special first case of reflection iteration. Draw the unreflected scene
 *
 */
void
set_mirror(int which)
{
    glEnable(GL_STENCIL_TEST);

    /* set the stencil bits to 1 wherever we render */
    glStencilFunc(GL_ALWAYS, 1, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    /*			  
     * Draw mirrors into stencil,depth,color buffer (stencil = 1)
     */
    glColor3f(0.f, 0.f, .7f); /* for debugging */
    draw_mirror(which);

    /* now we have a good mask where mirror is visible */
    /* clear z buffer where mask is set */

    glDepthFunc(GL_ALWAYS);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); /* don't change stencil */

    /* there are better ways to do this */
    glPushMatrix(); /* assuming GL_MODELVIEW */
    glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(0.f, 1.f, 1.f); /* for debugging */
    glVertex3i(-500, -500, -990);
    glVertex3i( 500, -500, -990);
    glVertex3i( 500,  500, -990);
    glVertex3i(-500,  500, -990);
    glEnd();
    glPopMatrix();

    glDepthFunc(GL_LESS);

     /* turn off the stencil test */

    glDisable(GL_STENCIL_TEST);
}

void
show_stencil(void)
{
    glPushAttrib(GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glPushMatrix(); /* GL_MODELVIEW */
    glLoadIdentity();
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_STENCIL_TEST);

    glStencilFunc(GL_EQUAL, 0, 3); 
    glColor4f(1.f, 0.f, 0.f, .8f);
    glBegin(GL_QUADS);
    glVertex3f(-100.f, -100.f, -100.f);
    glVertex3f(100.f, -100.f, -100.f);
    glVertex3f(100.f, 100.f, -100.f);
    glVertex3f(-100.f, 100.f, -100.f);
    glEnd();


    glStencilFunc(GL_EQUAL, 1, 3); 
    glColor4f(0.f, 1.f, 0.f, .8f);
    glBegin(GL_QUADS);
    glVertex3f(-100.f, -100.f, -100.f);
    glVertex3f(100.f, -100.f, -100.f);
    glVertex3f(100.f, 100.f, -100.f);
    glVertex3f(-100.f, 100.f, -100.f);
    glEnd();

    glStencilFunc(GL_EQUAL, 2, 3); 
    glColor4f(0.f, 0.f, 1.f, .8f);
    glBegin(GL_QUADS);
    glVertex3f(-100.f, -100.f, -100.f);
    glVertex3f(100.f, -100.f, -100.f);
    glVertex3f(100.f, 100.f, -100.f);
    glVertex3f(-100.f, 100.f, -100.f);
    glEnd();

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glPopMatrix();
    glPopAttrib();
}


/* Called when window needs to be redrawn */

void
redraw_mirrored(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    draw(1.f); /* special case; unreflected scene */

    /* draw 1st level reflection */

    set_mirror(RIGHT); 

    reflection(RIGHT);
    if(darken)
	draw_mirrored(.6f, BACK);
    else
	draw_mirrored(1.f, BACK);
    reflection(NONE);

    punch(); /* decrement stencil, "cut holes" in depth buffer */

    /* draw 2nd level reflections */

    reflection(DUAL);
    if(darken)
	draw_mirrored(.36f, NONE);
    else
	draw_mirrored(1.f, NONE);
    reflection(NONE);


    glClear(GL_STENCIL_BUFFER_BIT);

    /* draw 1st level reflection */

    set_mirror(BACK); 

    reflection(BACK);
    if(darken)
	draw_mirrored(.6f, RIGHT);
    else
	draw_mirrored(1.f, RIGHT);
    reflection(NONE);

    if(stencil)
	show_stencil();
    
    punch(); /* decrement stencil, "cut holes" in depth buffer */

    /* draw 2nd level reflection */

    reflection(DUAL);
    if(darken)
	draw_mirrored(.36f, NONE);
    else
	draw_mirrored(1.f, NONE);
    reflection(NONE);

    /* don't bother punching, since no more reflections */

    /* restore unreflected light position */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    
    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("redraw_mirrored()");
}


/* show the steps to drawing reflections */
void
redraw_steps(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    /* UNREFLECTED: lower left */
    glViewport(0, 0, winwid/2, winht/2);
    draw(1.f); /* special case; unreflected scene */
    if(stencil)
	show_stencil();

    /* RIGHT REFLECTION lower right */
    glViewport(winwid/2, 0, winwid/2, winht/2);
    draw(1.f); /* special case; unreflected scene */

    /* draw 1st level reflection */

    set_mirror(RIGHT); 

    reflection(RIGHT);
    if(darken)
	draw_mirrored(.6f, BACK);
    else
	draw_mirrored(1.f, BACK);
    reflection(NONE);
    if(stencil)
	show_stencil();

    
    /* BACK_REFLECTION upper left */
    glViewport(0, winht/2, winwid/2, winht/2);

    glClear(GL_STENCIL_BUFFER_BIT);

    draw(1.f); /* special case; unreflected scene */

    /* draw 1st level reflection */

    set_mirror(BACK); 

    reflection(BACK);
    if(darken)
	draw_mirrored(.6f, RIGHT);
    else
	draw_mirrored(1.f, RIGHT);
    reflection(NONE);
    if(stencil)
	show_stencil();

    /* BOTH_REFLECTIONS upper right */
    glViewport(winwid/2, winht/2, winwid/2, winht/2);

    draw(1.f); /* special case; unreflected scene */

    /* draw 1st level reflection */

    set_mirror(RIGHT); 

    reflection(RIGHT);
    if(darken)
	draw_mirrored(.6f, BACK);
    else
	draw_mirrored(1.f, BACK);
    reflection(NONE);

    
    punch(); /* decrement stencil, "cut holes" in depth buffer */

    /* draw 2nd level reflections */

    reflection(DUAL);
    if(darken)
	draw_mirrored(.36f, NONE);
    else
	draw_mirrored(1.f, NONE);
    reflection(NONE);


    glClear(GL_STENCIL_BUFFER_BIT);

    /* draw 1st level reflection */

    set_mirror(BACK); 

    reflection(BACK);
    if(darken)
	draw_mirrored(.6f, RIGHT);
    else
	draw_mirrored(1.f, RIGHT);
    reflection(NONE);

    punch(); /* decrement stencil, "cut holes" in depth buffer */

    /* draw 2nd level reflection */

    reflection(DUAL);
    if(darken)
	draw_mirrored(.36f, NONE);
    else
	draw_mirrored(1.f, NONE);
    reflection(NONE);

    glClear(GL_STENCIL_BUFFER_BIT);

    if(stencil)
	show_stencil();

    
    glViewport(0, 0, winwid, winht);


    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("redraw_reflect()");
}


/*ARGSUSED1*/
void
key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 'n': /* unreflective floor */
    case 'N':
	glutDisplayFunc(redraw_normal);
	glutPostRedisplay();
	break;
    case 'r': /* show reflection */
    case 'R':
	glutDisplayFunc(redraw_reflection);
	glutPostRedisplay();
	break;
    case 'm': /* mirrored scene */
    case 'M':
	glutDisplayFunc(redraw_mirrored);
	glutPostRedisplay();
	break;
    case 'd':
    case 'D':
	darken = !darken; /* toggle darkening of reflections */
	glutPostRedisplay();
	break;
    case 's':
    case 'S':
	stencil = !stencil; /* toggle showing stencil */
	glutPostRedisplay();
	break;
    case 'a':
    case 'A':
	animate = !animate; /* toggle animation of bouncing sphere */
	if(animate)
	    glutIdleFunc(idle);
	else
	    glutIdleFunc(0);
	glutPostRedisplay();
	break;
    case 'b':
    case 'B':
	glutDisplayFunc(redraw_steps);
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
    }
}

void
menu(int choice)
{
    switch(choice)
    {
    case NORMAL:
	key('n', 0, 0);
	break;
    case REFLECTION:
	key('r', 0, 0);
	break;
    case MIRRORED:
	key('m', 0, 0);
	break;
    case DARK:
	key('d', 0, 0);
	break;
    case DEMO:
	key('b', 0, 0);
	break;
    case STENCIL:
	key('s', 0, 0);
	break;
    case EXIT:
	exit(0);
    }
}

GLfloat one[] = {1.f, 1.f, 1.f, 1.f};

int
main(int argc, char *argv[])
{
    unsigned *floortex;
    int texcomps, texwid, texht;
    GLint sbits;

    GLUquadricObj *sphere, *cone, *base;

    glutInitWindowSize(winwid, winht);
    glutInit(&argc, argv);
    if(argc > 1)
    {
	char *args = argv[1];
	int done = GL_FALSE;
	while(!done)
	{
	    switch(*args)
	    {
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
/* ensure we get 2 bits of stencil */
#if 0
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL|GLUT_DOUBLE);
#else
	glutInitDisplayString("stencil~2 rgb double depth");
#endif
    else
#if 0
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
#else
	glutInitDisplayString("stencil~2 rgb depth");
#endif

    (void)glutCreateWindow("multiple reflections demo");

    glPolygonOffset(-4.f, -8.f);

    glutDisplayFunc(redraw_normal);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);

    glutCreateMenu(menu);
    glutAddMenuEntry("No Reflections (n, N)", NORMAL);
    glutAddMenuEntry("Show reflections (r, R)", REFLECTION);
    glutAddMenuEntry("Scene with mirrors (m, M)", MIRRORED);
    glutAddMenuEntry("Toggle Darken Reflections (d, D)", DARK);
    glutAddMenuEntry("Show Reflection Steps (b, B)", DEMO);
    glutAddMenuEntry("Show Stencil Buffer (s, S)", STENCIL);
    glutAddMenuEntry("Exit Program", EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-5., 5., -5., 5., 10., 1000.); 
    glMatrixMode(GL_MODELVIEW);
    updateMV();

    /* turn on features */
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glLightfv(GL_LIGHT0, GL_SPECULAR, one);


    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, one);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 128);

    /* makes texturing faster, and looks better than GL_LINEAR */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /* make a display list containing a sphere */
    glNewList(SPHERE, GL_COMPILE);
    sphere = gluNewQuadric();
    gluSphere(sphere, 20.f, 20, 20);
    gluDeleteQuadric(sphere);
    glEndList();

    /* create a display list containing a cone */
    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glRotatef(-90.f, 1.f, 0.f, 0.f);
    gluQuadricOrientation(base, GLU_INSIDE);
    gluDisk(base, 0., 20., 20, 1);
    gluCylinder(cone, 20., 0., 60., 20, 20);
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    glNewList(LIGHT, GL_COMPILE);
    glDisable(GL_LIGHTING);
    sphere = gluNewQuadric();
    gluSphere(sphere, 5.f, 10, 10);
    gluDeleteQuadric(sphere);
    glEnable(GL_LIGHTING);
    glEndList();

    glEnable(GL_CULL_FACE);
    /* load pattern for current 2d texture */

    floortex = read_texture("../data/plank.rgb",
			    &texwid, &texht, &texcomps);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, texwid, texht, GL_RGBA,
		      GL_UNSIGNED_BYTE, floortex);

    free(floortex);

    glGetIntegerv(GL_STENCIL_BITS, &sbits);
    if(sbits < 2)
	printf("Warning: Not enough stencil bits (%d)"
	       "for multiple reflections\n", (int)sbits);
    else
	printf("Stencil bits = %d\n", (int)sbits);

    CHECK_ERROR("end of main");
    glutMainLoop();

    return 0;
}
