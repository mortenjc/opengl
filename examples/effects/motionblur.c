#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#define unitrand() (rand() / (double)RAND_MAX)
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

const GLdouble FRUSTDIM = 30.f; /* was 100 */
const GLdouble FRUSTNEAR = 320.f;
const GLdouble FRUSTFAR = 660.f;

float decayFactor;

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

enum {SPHERE = 1, CONE};

void
render(GLfloat dx, GLfloat dy, GLfloat dz)
{
#if 0
    /* material properties for objects in scene */
    static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};
#endif

    /* glClearColor(1, 1, 1, 1); */
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

#if 0
    /*
    ** Note: wall verticies are ordered so they are all front facing
    ** this lets me do back face culling to speed things up.
    */
 
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

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
    glVertex3f( 100.f, -100.f, -640.f);
    glTexCoord2i(0, 1);
    glVertex3f(-100.f, -100.f, -640.f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    /* walls */

    glBegin(GL_QUADS);
    /* left wall */
    glNormal3f(1.f, 0.f, 0.f);
    glVertex3f(-100.f, -100.f, -320.f);
    glVertex3f(-100.f, -100.f, -640.f);
    glVertex3f(-100.f,  100.f, -640.f);
    glVertex3f(-100.f,  100.f, -320.f);

    /* right wall */
    glNormal3f(-1.f, 0.f, 0.f);
    glVertex3f( 100.f, -100.f, -320.f);
    glVertex3f( 100.f,  100.f, -320.f);
    glVertex3f( 100.f,  100.f, -640.f);
    glVertex3f( 100.f, -100.f, -640.f);

    /* ceiling */
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(-100.f,  100.f, -320.f);
    glVertex3f(-100.f,  100.f, -640.f);
    glVertex3f( 100.f,  100.f, -640.f);
    glVertex3f( 100.f,  100.f, -320.f);

    /* back wall */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-100.f, -100.f, -640.f);
    glVertex3f( 100.f, -100.f, -640.f);
    glVertex3f( 100.f,  100.f, -640.f);
    glVertex3f(-100.f,  100.f, -640.f);
    glEnd();
#endif

    glPushMatrix();
    glTranslatef(-80.f + dx, -10.f + dy, -420.f + dz);
    glCallList(SPHERE);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.f, -30.f, -500.f);
    glCallList(CONE);
    glPopMatrix();

    if(glGetError()) /* to catch programming errors */
       printf("Oops! I screwed up my OpenGL calls somewhere\n");

    glFlush(); /* high end machines may need this */
}

enum {NONE, SAMPLE, DECAY};

int rendermode = NONE;

int frameCount;

void
menu(int selection)
{
    rendermode = selection;
}

GLdouble focus = 420.;

/* Called when window needs to be redrawn */
void redraw(void)
{
    int i;
    int max;
    GLfloat dx, dy, dz;

    glPushMatrix();

    gluLookAt(400, 200, -300, 0, 0, -500, 0, 1, 0);
    switch(rendermode) {
    case NONE:
	dx = 80 + sin(frameCount/10.f * M_PI / 50) * 50;
	dy = 0;
	dz = -80 + cos(frameCount/10.f * M_PI / 50) * 50;
        render(dx, dy, dz);
        break;
    case SAMPLE:
        max = 4;
  
        glClear(GL_ACCUM_BUFFER_BIT);
  
        for(i = 0; i < max; i++) {
	    dx = 80 + sin((frameCount + i / (float)max) * M_PI / 50) * 50;
	    dy = 0;
	    dz = -80 + cos((frameCount + i / (float)max) * M_PI / 50) * 50;
	    render(dx, dy, dz);
	    glAccum(GL_ACCUM, 1.f/max);
        } 
        glAccum(GL_RETURN, 1.f);
        break;
    case DECAY:
	dx = 80 + sin(frameCount * M_PI / 50) * 50;
	dy = 0;
	dz = -80 + cos(frameCount * M_PI / 50) * 50;
        render(dx, dy, dz);
        if(frameCount == 0) {
	    glAccum(GL_LOAD, 1.0);
        }else{
	    glAccum(GL_MULT, decayFactor);
	    glAccum(GL_ACCUM, 1 - decayFactor);
        }
        glAccum(GL_RETURN, 1.f);
        break;
    }
    frameCount++;

    glPopMatrix();
    glutSwapBuffers();
}

/*ARGSUSED1*/
void key(unsigned char key, int x, int y)
{
    if(key == '\033')
	exit(0);
}


const int TEXDIM = 256;
/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    GLfloat *tex;
    static GLfloat lightpos[] = {50.f, 50.f, -320.f, 1.f};
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
    /* static GLfloat sphere_mat[] = {0.f, 0.f, 0.f, 1.f}; */
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};
    GLUquadricObj *sphere, *cone, *base;

    if(argc >= 2)
	decayFactor = atof(argv[1]);
    else
	decayFactor = .8f;

    glutInit(&argc, argv);
#ifdef _WIN32
    glutInitWindowSize(1024, 1024);
#else
    glutInitWindowSize(1024, 1023);
#endif
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_ACCUM|GLUT_DOUBLE);
    (void)glutCreateWindow("motion blur");
    glutDisplayFunc(redraw);
    glutIdleFunc(redraw);
    glutKeyboardFunc(key);

    glutCreateMenu(menu);
    glutAddMenuEntry("Normal", NONE);
    glutAddMenuEntry("Sampled Motion Blur", SAMPLE);
    glutAddMenuEntry("Decay Motion Blur", DECAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-FRUSTDIM, FRUSTDIM, -FRUSTDIM, FRUSTDIM, FRUSTNEAR, FRUSTFAR); 
    glMatrixMode(GL_MODELVIEW);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glNewList(SPHERE, GL_COMPILE);
    /* make display lists for sphere and cone; for efficiency */
    sphere = gluNewQuadric();
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
    gluSphere(sphere, 20.f, 20, 20);
    gluDeleteQuadric(sphere);
    glEndList();

    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glRotatef(-90.f, 1.f, 0.f, 0.f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
    gluDisk(base, 0., 20., 20, 1);
    gluCylinder(cone, 20., 0., 60., 20, 20);
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    /* load pattern for current 2d texture */
    tex = make_texture(TEXDIM, TEXDIM);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    glutMainLoop();
    return(0);
}

