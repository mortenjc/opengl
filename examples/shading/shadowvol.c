#include <stdlib.h>
#include <GL/glut.h>
#include <stdio.h>

/*
** Demonstrate shadow volumes
**/

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

GLboolean dblbuf = GL_TRUE;


/*
** Create a single component texture map
*/
GLfloat *make_texture(int maxs, int maxt)
{
    int s, t;
    static GLfloat *texture;

    texture = (GLfloat *)malloc(maxs * maxt * sizeof(GLfloat));
    for(t = 0; t < maxt; t++) {
	for(s = 0; s < maxs; s++) {
	    texture[s + maxs * t] = ((s >> 4) & 0x1) ^ ((t >> 4) & 0x1);
	}
    }
    return texture;
}

enum {SPHERE = 1, CONE, LIGHT, SHADOWVOL, SHADOWER};

typedef struct {
  GLfloat *verticies;
  GLfloat *normal;
  int n; /* number of verticies */
} ShadObj;

/* definition of shadowing object; this should be dynamically 
   generated.
*/
GLfloat shadVerts[] = { 30.f, 30.f, -350.f,
		        60.f, 20.f, -340.f,
		        40.f, 40.f, -400.f};

GLfloat shadNormal[] = {1.f, 1.f, 1.f};
ShadObj shadower;


enum {X, Y, Z};
enum {LIGHTXY, LIGHTZ};
int active = LIGHTXY;

GLfloat lightpos[] = {50.f, 50.f, -340.f, 1.f};

GLint winHeight = 512, winWidth = 512;

void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, winWidth, winHeight);
}





/* simple way to extend a point to build shadow volume */
void
extend(GLfloat new[3], GLfloat light[3], GLfloat vertex[3], GLfloat t)
{
  GLfloat delta[3];

  delta[X] = vertex[X] - light[X];
  delta[Y] = vertex[Y] - light[Y];
  delta[Z] = vertex[Z] - light[Z];

  new[X] = light[X] + delta[X] * t;
  new[Y] = light[Y] + delta[Y] * t;
  new[Z] = light[Z] + delta[Z] * t;
}

/* Create a shadow volume in a display list */
/* XXX light should have 4 components */
/* making display list all the time probably isn't efficient, but it
   is convenient
*/
void
makeShadowVolume(ShadObj *shadower, GLfloat light[3], 
		 GLfloat t, GLint dlist)
{
    int i;
    GLfloat newv[3];

    glNewList(dlist, GL_COMPILE);
    glBegin(GL_QUADS);
    /* color of shadow volume only used to visualize volume */
    glColor4f(.2f, .8f, .4f, .7f); 
    for(i = 0; i < shadower->n; i++) {
      glVertex3fv(&shadower->verticies[i * 3]);
      extend(newv,  light, &shadower->verticies[i * 3], t);
      glVertex3fv(newv);
      extend(newv,  light, &shadower->verticies[((i + 1) % shadower->n) * 3],
	     t);
      glVertex3fv(newv);
      glVertex3fv(&shadower->verticies[((i + 1) % shadower->n) * 3]);
    }
    glEnd();
    glEnable(GL_LIGHTING);
    glEndList();
}


void
motion(int x, int y)
{
    switch(active)
    {
    case LIGHTXY:
	lightpos[X] = (x - winWidth/2.f) * 100/(winWidth/2);
	lightpos[Y] = (winHeight/2.f - y) * 100/(winHeight/2);

	/* place light 0 in the right place */
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	makeShadowVolume(&shadower, lightpos, 10.f, SHADOWVOL);
	glutPostRedisplay();
	break;
    case LIGHTZ: 
	lightpos[Z] = -320.f -(winHeight/2.f - y) * 200/(winHeight/2);

	/* place light 0 in the right place */
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	makeShadowVolume(&shadower, lightpos, 10.f, SHADOWVOL);
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





enum {NONE, NOLIGHT, VOLUME, FRONTVOL, BACKVOL, SHADOW, STENCIL};




void
render(void)
{
    /*
    ** Note: wall verticies are ordered so they are all front facing
    ** this lets me do back face culling to speed things up.
    */
 
    glColor3f(1.f, 1.f, 1.f);

    /* floor */
    /* make the floor textured */
    glEnable(GL_TEXTURE_2D);

    /*
    ** Since we want to turn texturing on for floor only, we have to
    ** make floor a separate glBegin()/glEnd() sequence. You can't
    ** turn texturing on and off between begin and end calls
    */
    glBegin(GL_QUADS);
    glNormal3f(0.f, 1.f, 0.f);
    glTexCoord2i(0, 0);
    glVertex3f(-100.f, -100.f, -320.f);
    glTexCoord2i(1, 0);
    glVertex3f( 100.f, -100.f, -320.f);
    glTexCoord2i(1, 1);
    glVertex3f( 100.f, -100.f, -520.f);
    glTexCoord2i(0, 1);
    glVertex3f(-100.f, -100.f, -520.f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    /* walls */


    glBegin(GL_QUADS);
    /* left wall */
    glNormal3f(1.f, 0.f, 0.f);
    glVertex3f(-100.f, -100.f, -320.f);
    glVertex3f(-100.f, -100.f, -520.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glVertex3f(-100.f,  100.f, -320.f);

    /* right wall */
    glNormal3f(-1.f, 0.f, 0.f);
    glVertex3f( 100.f, -100.f, -320.f);
    glVertex3f( 100.f,  100.f, -320.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f( 100.f, -100.f, -520.f);

    /* ceiling */
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(-100.f,  100.f, -320.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f( 100.f,  100.f, -320.f);

    /* back wall */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-100.f, -100.f, -520.f);
    glVertex3f( 100.f, -100.f, -520.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glEnd();

    glCallList(CONE);
    glCallList(SPHERE);

    glCallList(SHADOWER);

    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glCallList(LIGHT);
    glPopMatrix();
}

void 
redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    render();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");
}


void 
redraw_dark(void)
{

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  /* turn off all but ambient light */
  glDisable(GL_LIGHT0);
  render();
  glEnable(GL_LIGHT0);

  if(dblbuf)
      glutSwapBuffers(); 
  else
      glFlush(); 
  
  CHECK_ERROR("OpenGL Error in redraw()");
}

void
redraw_volume(void)
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  render();
  glCallList(SHADOWVOL);

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw_volume()");
}

GLfloat one[] = {1.f, 1.f, 1.f, 1.f};
GLfloat dim[] = {.1f, .1f, .1f, 1.f};

void
redraw_front(void)
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);


  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  render(); /* render scene in depth buffer */

  glEnable(GL_STENCIL_TEST);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 0, 0);

  glEnable(GL_CULL_FACE);
  glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
  glCullFace(GL_BACK); /* increment using front face of shadow volume */
  glCallList(SHADOWVOL);


  glDepthMask(GL_TRUE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glDisable(GL_CULL_FACE);

  glStencilFunc(GL_EQUAL, 1, 1); /* draw what's covered by front face */
  render();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glCallList(SHADOWVOL);
  glDisable(GL_BLEND);

  glStencilFunc(GL_EQUAL, 0, 1); /* draw rest of scene */
  
  render();

  glEnable(GL_LIGHT0);
  glDisable(GL_STENCIL_TEST);


    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw_front()");
}


void
redraw_back(void)
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);


  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  render(); /* render scene in depth buffer */

  glEnable(GL_STENCIL_TEST);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 0, 0);

  glEnable(GL_CULL_FACE);
  glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
  glCullFace(GL_FRONT); /* increment using back face of shadow volume */
  glCallList(SHADOWVOL);


  glDepthMask(GL_TRUE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glDisable(GL_CULL_FACE);

  glStencilFunc(GL_EQUAL, 1, 1); /* draw what's covered by back face */
  render();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glCallList(SHADOWVOL);
  glDisable(GL_BLEND);

  glStencilFunc(GL_EQUAL, 0, 1); /* draw rest of scene */
  
  render();

  glEnable(GL_LIGHT0);
  glDisable(GL_STENCIL_TEST);


    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw_front()");
}

void
redraw_shadow(void)
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);


  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  render(); /* render scene in depth buffer */

  glEnable(GL_STENCIL_TEST);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 0, 0);

  glEnable(GL_CULL_FACE);
  glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
  glCullFace(GL_BACK); /* increment using front face of shadow volume */
  glCallList(SHADOWVOL);

  glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
  glCullFace(GL_FRONT); /* increment using front face of shadow volume */
  glCallList(SHADOWVOL);

  glDepthMask(GL_TRUE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glCullFace(GL_BACK);
  glDepthFunc(GL_LEQUAL);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glDisable(GL_CULL_FACE);

  glStencilFunc(GL_EQUAL, 1, 1); /* draw shadowed part */
  glDisable(GL_LIGHT0);
  render();

  glStencilFunc(GL_EQUAL, 0, 1); /* draw lit part */
  glEnable(GL_LIGHT0);
  render();

  glDepthFunc(GL_LESS);
  glDisable(GL_STENCIL_TEST);

  if(dblbuf)
      glutSwapBuffers(); 
  else
      glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw_shadow()");
}

void
redraw_stencil(void)
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);


  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  render(); /* render scene in depth buffer */

  glEnable(GL_STENCIL_TEST);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 0, 0);

  glEnable(GL_CULL_FACE);
  glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
  glCullFace(GL_BACK); /* increment using front face of shadow volume */
  glCallList(SHADOWVOL);

  glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
  glCullFace(GL_FRONT); /* increment using front face of shadow volume */
  glCallList(SHADOWVOL);

  glDepthMask(GL_TRUE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glCullFace(GL_BACK);
  glDepthFunc(GL_LEQUAL);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glDisable(GL_CULL_FACE);


  glDisable(GL_STENCIL_TEST);
  render(); /* draw entire scene */
  glEnable(GL_STENCIL_TEST);

  glStencilFunc(GL_EQUAL, 0, 1); /* draw lit part */

  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(0.f, 1.f, 0.f, .8f);
  glBegin(GL_QUADS);
  glVertex3f(-100.f, -100.f, -320.f);
  glVertex3f(100.f, -100.f, -320.f);
  glVertex3f(100.f, 100.f, -320.f);
  glVertex3f(-100.f, 100.f, -320.f);
  glEnd();

  glStencilFunc(GL_EQUAL, 1, 1); /* draw shadowed part */

  glColor4f(1.f, 0.f, 0.f, .8f);
  glBegin(GL_QUADS);
  glVertex3f(-100.f, -100.f, -320.f);
  glVertex3f(100.f, -100.f, -320.f);
  glVertex3f(100.f, 100.f, -320.f);
  glVertex3f(-100.f, 100.f, -320.f);
  glEnd();


  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glDisable(GL_BLEND);

  glDepthFunc(GL_LESS);
  glDisable(GL_STENCIL_TEST);

  if(dblbuf)
      glutSwapBuffers(); 
  else
      glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw_outstencil()");
}




/*ARGSUSED1*/
void key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 'n': /* no shadows */
    case 'N':
	glutDisplayFunc(redraw);
	glutPostRedisplay();
	break;
    case 'd': /* no lighting */
    case 'D':
	glutDisplayFunc(redraw_dark);
	glutPostRedisplay();
	break;
    case 'v': /* shadow volume */
    case 'V':
	glutDisplayFunc(redraw_volume);
	glutPostRedisplay();
	break;
    case 'f': /* front of shadow volume (transparency? stencil bits?) */
    case 'F':
	glutDisplayFunc(redraw_front);
	glutPostRedisplay();
	break;
    case 'b': /* back of shadow volume (transparency? stencil bits?) */
    case 'B':
	glutDisplayFunc(redraw_back);
	glutPostRedisplay();
	break;
    case 's': /* shadows */
    case 'S':
	glutDisplayFunc(redraw_shadow);
	glutPostRedisplay();
	break;
    case 'm': /* shadow stencil mask */
    case 'M':
	glutDisplayFunc(redraw_stencil);
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
    default:
	fprintf(stderr, "Keyboard commands\n"
		"n or N - scene with no shadows\n"
		"d or D - scene with no lighting\n"
		"v or V - scene with shadow volume visible\n"
		"f or F - scene covered by front of shadow volume\n"
		"b or B - scene covered by back of shadow volume\n"
		"m or M - stencil mask created by shadow volume\n"
		"s or S - scene with shadow volume shadow\n\n");
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
    case NOLIGHT:
	key('d', 0, 0);
	break;    
    case VOLUME:
	key('v', 0, 0);
	break;    
    case BACKVOL:
	key('b', 0, 0);
	break;    
    case FRONTVOL:
	key('f', 0, 0);
	break;    
    case SHADOW:
	key('s', 0, 0);
	break;    
    case STENCIL:
	key('m', 0, 0);
	break;    
    }
    glutPostRedisplay();
}


const int TEXDIM = 256;
/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    GLfloat *tex;
    GLUquadricObj *sphere, *cone, *base;
    int i;

    glutInit(&argc, argv);
    glutInitWindowSize(1024, 1024);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL|GLUT_DOUBLE);
    (void)glutCreateWindow("shadow volumes");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);

    glutCreateMenu(menu);
    glutAddMenuEntry("No Shadows (n, N)", NONE);
    glutAddMenuEntry("No Light (d, D)", NOLIGHT);
    glutAddMenuEntry("Show Volume (v, V)", VOLUME);
    glutAddMenuEntry("Show Front Volume (f, F)", FRONTVOL);
    glutAddMenuEntry("Show Back Volume (b, B)", BACKVOL);
    glutAddMenuEntry("Show Shadow Stencil Mask (m, M)", STENCIL);
    glutAddMenuEntry("Shadows (s, S)", SHADOW);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-10., 10., -10., 10., 32., 640.); 
    glMatrixMode(GL_MODELVIEW);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    /* make display lists for sphere and cone; for efficiency */

    glNewList(SHADOWER, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    glColor3f(1.f, 0.f, 0.f);
    glNormal3fv(shadNormal);
    for(i = 0; i < sizeof(shadVerts)/sizeof(GLfloat); i+= 3)
	glVertex3fv(&shadVerts[i]);
    glEnd();
    glEndList();


    glNewList(SPHERE, GL_COMPILE);
    sphere = gluNewQuadric();
    glColor3f(1.f, .5f, 0.f);
    glPushMatrix();
    glTranslatef(60.f, -50.f, -360.f);
    gluSphere(sphere, 20.f, 20, 20);
    glPopMatrix();
    gluDeleteQuadric(sphere);
    glEndList();

    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glColor3f(0.f, .5f, 1.f);
    glPushMatrix();
    glTranslatef(-40.f, -40.f, -400.f);
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

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    /* load pattern for current 2d texture */
    tex = make_texture(TEXDIM, TEXDIM);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    shadower.verticies = shadVerts;
    shadower.normal = shadNormal;
    shadower.n = sizeof(shadVerts)/(3 * sizeof(GLfloat));

    makeShadowVolume(&shadower, lightpos, 10.f, SHADOWVOL);

    key('?', 0, 0);
    glutMainLoop();
    return 0;
}
