#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#define gettimeofday(a) gettimeofday(a, NULL)
#endif

GLUquadricObj *cone, *base, *sphere;

void 
init(void)
{
    static GLfloat lightpos[] = {.5, .75, 1.5, 1};

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    cone = gluNewQuadric();
    base = gluNewQuadric();
    sphere = gluNewQuadric();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void 
reshape(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1, .01, 10);
    gluLookAt(0, 0, 2.577, 0, 0, -5, 0, 1, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void 
draw_room(void)
{
    /* material for the walls, floor, ceiling */
    static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

    glBegin(GL_QUADS);

    /* floor */
    glNormal3f(0, 1, 0);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, -1, -1);
    glVertex3f(-1, -1, -1);

    /* ceiling */
    glNormal3f(0, -1, 0);
    glVertex3f(-1, 1, -1);
    glVertex3f(1, 1, -1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);

    /* left wall */
    glNormal3f(1, 0, 0);
    glVertex3f(-1, -1, -1);
    glVertex3f(-1, -1, 1);
    glVertex3f(-1, 1, 1);
    glVertex3f(-1, 1, -1);

    /* right wall */
    glNormal3f(-1, 0, 0);
    glVertex3f(1, 1, -1);
    glVertex3f(1, 1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, -1, -1);

    /* far wall */
    glNormal3f(0, 0, 1);
    glVertex3f(-1, -1, -1);
    glVertex3f(1, -1, -1);
    glVertex3f(1, 1, -1);
    glVertex3f(-1, 1, -1);

    glEnd();
}

void 
draw_cone(void)
{
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, .5f};

    glPushMatrix();
    glTranslatef(0, -1, 0);
    glRotatef(-90, 1, 0, 0);

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);

    /* base is coplanar with floor, so turn off depth testing */
    glDisable(GL_DEPTH_TEST);
    gluDisk(base, 0., .3, 20, 1);
    glEnable(GL_DEPTH_TEST);

    gluCylinder(cone, .3, 0, 1.25, 20, 1);

    glPopMatrix();
}

void 
draw_sphere(GLdouble angle)
{
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, .5f};

    glPushMatrix();
    glTranslatef(0.f, -.3f, 0.f);
    glRotatef(angle, 0.f, 1.f, 0.f);
    glTranslatef(.6f, 0.f, 0.f);

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
    gluSphere(sphere, .3f, 20, 20);

    glPopMatrix();
}

GLdouble 
get_secs(void)
{
#ifdef _WIN32
    struct timeb t;
    ftime(&t);
    return (t.time % 60) + t.millitm / 1000.;
#else
    struct timeval t;
    gettimeofday(&t);
    return (t.tv_sec % 60) + t.tv_usec / 1000000.;
#endif
}

void 
draw(void)
{
    GLenum err;
    GLdouble secs, degrees;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* one revolution every 10 seconds... */
    secs = get_secs();
    secs = secs - 10.f * floor(secs / 10.f);
    degrees = (secs / 10.) * (360.);

    draw_room();

    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    if (degrees < 180) {
	/* sphere behind cone */
	glCullFace(GL_FRONT);
	draw_sphere(degrees);
	draw_cone();
	glCullFace(GL_BACK);
	draw_sphere(degrees);
	draw_cone();
    } else {
	/* cone behind sphere */
	glCullFace(GL_FRONT);
	draw_cone();
	draw_sphere(degrees);
	glCullFace(GL_BACK);
	draw_cone();
	draw_sphere(degrees);
    }
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    err = glGetError();
    if (err != GL_NO_ERROR)
	printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}

/*ARGSUSED1*/
void 
key(unsigned char key, int x, int y)
{
    static int idle = 1;
    if (key == 27)
	exit(0);
    idle = (idle == 0);
    if (idle) {
	glutIdleFunc(draw);
    } else {
	glutIdleFunc(0);
    }

}

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutIdleFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    init();

    printf("Press any key to pause the motion.\n");
    glutMainLoop();
    return 0;
}
