#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265f
#endif
#endif

int px, py, moving = -1, model = 0, doCapping = 1;
float theta = M_PI, phi = M_PI / 3.f, distance = 2.0f;
GLdouble clip[4] = {1.0, 0.0, 0.0, 0.0};

/*
 * The capping polygon 
 */
void 
drawCap(void)
{
    glBegin(GL_QUADS);
    glNormal3f(-1.0, 0.0, 0.0);
    glVertex3f(-clip[3], -0.5, -0.5);
    glVertex3f(-clip[3], 0.5, -0.5);
    glVertex3f(-clip[3], 0.5, 0.5);
    glVertex3f(-clip[3], -0.5, 0.5);
    glEnd();
}

/*
 * The model 
 */
void 
drawModel(void)
{
    glPushMatrix();
    switch (model) {
    case 0:
	glutSolidTorus(0.1, 0.2, 32, 32);
	break;

    case 1:
	glutSolidSphere(0.2, 32, 32);
	break;

    case 2:
	glRotatef(45.0, 0.0, 1.0, 0.0);
	glRotatef(45.0, 1.0, 0.0, 0.0);
	glutSolidCube(0.35);
	break;
    }
    glPopMatrix();
}

/*
 * Display callback 
 */
void 
cbDisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClipPlane(GL_CLIP_PLANE0, clip);

    if (doCapping) {
	/*
	 * Draw the capping polygon into the depth buffer
	 */
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	drawCap();
	glDepthMask(0);

	/*
	 * Use the stencil buffer to find out where the cap cuts our model 
	 */
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	drawModel();
	glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
	glCullFace(GL_FRONT);
	drawModel();
	glDisable(GL_CULL_FACE);

	/*
	 * Draw the cap 
	 */
	glDepthMask(1);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilFunc(GL_EQUAL, 0x1, 0x1);
	drawCap();
	glDisable(GL_STENCIL_TEST);
    }
    /*
     * Draw the object 
     */
    glEnable(GL_CLIP_PLANE0);
    drawModel();
    glDisable(GL_CLIP_PLANE0);

    /*
     * Draw the outline of the clipping plane 
     */
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_LIGHTING);
    drawCap();
    glEnable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glutSwapBuffers();
}

/*
 * Mouse button callback 
 */
void 
cbMouse(int button, int state, int x, int y)
{
    /*
     * hack for 2 button mouse 
     */
    if ((button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT) ||
	(moving == GLUT_MIDDLE_BUTTON && button == GLUT_LEFT_BUTTON && state == GLUT_UP))
	button = GLUT_MIDDLE_BUTTON;

    if (moving == -1 && state == GLUT_DOWN) {
	moving = button;
	px = x;
	py = y;
    } else if (button == moving && state == GLUT_UP) {
	moving = -1;
    }
}

/*
 * Mouse motion callback 
 */
void 
cbMotion(int x, int y)
{
    switch (moving) {
    case GLUT_LEFT_BUTTON:
	theta -= 0.01 * (x - px);
	phi -= 0.01 * (y - py);
	break;
    case GLUT_MIDDLE_BUTTON:
	clip[3] += 0.01 * (x - px);
	break;
    }
    px = x;
    py = y;
    glLoadIdentity();
    gluLookAt(distance * sin(phi) * cos(theta), distance * sin(phi) * sin(theta), distance * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    glutPostRedisplay();
}

/*
 * Keyboard callback 
 */
/*ARGSUSED1*/
void 
cbKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
    case 'q':
	exit(0);

    case ' ':
	doCapping = !doCapping;
	break;
    case 'c':
    case 'C':
	model = 2;
	break;
    case 's':
    case 'S':
	model = 1;
	break;
    case 't':
    case 'T':
	model = 0;
	break;

    default:
	return;
    }

    glutPostRedisplay();
}

/*
 * Menu callback 
 */
void 
cbMenu(int option)
{
    cbKeyboard((unsigned char) option, 0, 0);
}

void 
init(void)
{
    GLfloat position[4] =
    {2.0, 2.0, 2.0, 1.0};
    GLfloat purple[4] =
    {1.0, 0.5, 1.0, 1.0};
    GLfloat white[4] =
    {1.0, 1.0, 1.0, 1.0};

    /*
     * Matrices 
     */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 1.0, 0.1, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(distance * sin(phi) * cos(theta), distance * sin(phi) * sin(theta), distance * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    /*
     * Lighting 
     */
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, purple);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    glMaterialf(GL_FRONT, GL_SHININESS, 25.0);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    /*
     * Misc 
     */
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

int 
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Stencil capping");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Show torus ('t')", 't');
    glutAddMenuEntry("Show sphere ('s')", 's');
    glutAddMenuEntry("Show cube ('c')", 'c');
    glutAddMenuEntry("Toggle stencil capping (<space>)", ' ');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
