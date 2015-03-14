#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>

#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265f
#endif
#endif

GLfloat position[4] = {5.0f, 5.0f, 5.0f, 0.0f};
GLfloat red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
GLfloat blue[4] = {0.0f, 0.0f, 1.0f, 1.0f};
GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat yellow[4] = {1.0f, 1.0f, 0.0f, 1.0f};
GLfloat plane[4] = {1.0f, 0.0f, 0.0f, 0.5f};

int motionMode = 1, drawEdges = 0;
int px, py, moving = -1, model = 0;
float theta = M_PI, phi = M_PI / 3.f, distance = 4.0f, ts = 1.0f, tt = 0.0f;

/*
 * Display callback 
 */
void 
cbDisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*
     * Draw the object interior 
     */
    glCallList(1);

    /*
     * Draw the cutaway object shell 
     */
    if (drawEdges)
	glEnable(GL_POLYGON_OFFSET_FILL);
    glCallList(4);
    if (drawEdges)
	glDisable(GL_POLYGON_OFFSET_FILL);

    /*
     * Draw the shell edges 
     */
    if (drawEdges)
	glCallList(5);

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
	if (motionMode == 1) {
	    theta += (phi < M_PI) ? -0.01 * (x - px) : 0.01 * (x - px);
	    phi -= 0.01 * (y - py);
	} else
	    tt -= 0.01 * (x - px);
	break;

    default:
	return;
    }
    px = x;
    py = y;

    /*
     * Adjust the matrices 
     */
    if (motionMode == 1) {
	if (phi < 0.0)
	    phi += 2.f * M_PI;
	if (phi > 2.f * M_PI)
	    phi -= 2.f * M_PI;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(distance * sin(phi) * cos(theta), distance * sin(phi) * sin(theta), distance * cos(phi),
		  0.0, 0.0, 0.0, 0.0, 0.0, ((phi < M_PI) ? 1.0 : -1.0));
	glLightfv(GL_LIGHT0, GL_POSITION, position);
    } else {
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(tt, 0.0, 0.0);
    }
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
	break;

    case 'E':
    case 'e':
	glutChangeToMenuEntry(2, "Make cutaway stay with object ('o')", 'o');
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	break;
    case 'L':
    case 'l':
	drawEdges = !drawEdges;
	break;
    case 'O':
    case 'o':
	glutChangeToMenuEntry(2, "Make cutaway stay with eye ('e')", 'e');
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	break;
    case 'T':
    case 't':
	glutChangeToMenuEntry(1, "Make mouse ajust viewpoint ('v')", 'v');
	motionMode = 2;
	break;
    case 'V':
    case 'v':
	glutChangeToMenuEntry(1, "Make mouse adjust transparency ('t')", 't');
	motionMode = 1;
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

/*
 * Model geometry 
 */
void 
makeDisplayLists(void)
{
    /*
     * The interior 
     */
    glNewList(1, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, yellow);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    glMaterialf(GL_FRONT, GL_SHININESS, 25.0);
    glutSolidTorus(0.15, 0.6, 20, 20);
    glEndList();

    /*
     * The shell 
     */
    glNewList(2, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    glMaterialf(GL_FRONT, GL_SHININESS, 25.0);
    glutSolidTorus(0.25, 0.6, 20, 20);
    glEndList();

    /*
     * The shell rendered back->front 
     */
    glNewList(3, GL_COMPILE);
    glCullFace(GL_FRONT);
    glCallList(2);
    glCullFace(GL_BACK);
    glCallList(2);
    glEndList();


    /*
     * The cutaway shell 
     */
    glNewList(4, GL_COMPILE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_1D);

    glCallList(3);

    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_CULL_FACE);
    glEndList();

    /*
     * The shell edges 
     */
    glNewList(5, GL_COMPILE);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(0.5, 0.0, 0.0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_1D);

    glCallList(3);

    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glPopMatrix();
    glEndList();
}

#define TEXTURE_SIZE 256

void 
init(void)
{
#define USE_LA
#ifdef USE_LA
    GLfloat alphaTex[2 * TEXTURE_SIZE];
#else
    GLfloat alphaTex[TEXTURE_SIZE];
#endif
    GLuint alphaName;
    int i;

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
     * Texture 
     */
#ifdef USE_LA
    for (i = 0; i < TEXTURE_SIZE; i++) {
	alphaTex[2 * i + 0] = 1.f;
	alphaTex[2 * i + 1] = i / (TEXTURE_SIZE - 1.0);
    }
#else
    for (i = 0; i < TEXTURE_SIZE; i++)
	alphaTex[i] = i / (TEXTURE_SIZE - 1.0);
#endif
    glGenTextures(1, &alphaName);
    glBindTexture(GL_TEXTURE_1D, alphaName);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#ifdef USE_LA
    glTexImage1D(GL_TEXTURE_1D, 0, GL_LUMINANCE_ALPHA, TEXTURE_SIZE, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, alphaTex);
#else
    glTexImage1D(GL_TEXTURE_1D, 0, GL_ALPHA, TEXTURE_SIZE, 0, GL_ALPHA, GL_FLOAT, alphaTex);
#endif
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    /*
     * Texture coordinate generation 
     */
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, plane);

    /*
     * Lighting 
     */
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    /*
     * Geometry 
     */
    makeDisplayLists();

    /*
     * Misc 
     */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glLineWidth(2.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonOffset(1.0, 1.0);
    glClearColor(0.0f, 0.0f, 0.3f, 1.0f);

}

int i=1;

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Gradual cutaway");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    printf("glutinit %d\n", i++);
    int retval=999;
    //retval= glutCreateMenu(cbMenu);
    printf("retval: %d\n", retval);
    printf("glutinit %d\n", i++);
    glutAddMenuEntry("Make mouse adjust transparency ('t')", 't');
    printf("glutinit %d\n", i++);
    glutAddMenuEntry("Make cutaway stay with object ('o')", 'o');
    printf("glutinit %d\n", i++);
    glutAddMenuEntry("Toggle drawing shell edge lines ('l')", 'l');
    printf("glutinit %d\n", i++);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    printf("glutinit %d\n", i++);
    init();
    printf("glutinit %d\n", i++);
    glutMainLoop();
    return 0;
}
