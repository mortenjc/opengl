#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include <math.h>
#include <texture.h>

#ifdef _WIN32
#define sqrtf(x)    ((float)sqrt(x))
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define floorf(x) ((float)(floor(x)))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

/*
 * For the blend equations 
 */
#if defined(GL_ARB_imaging) && !defined(GL_EXT_blend_minmax)
#define glBlendEquationEXT glBlendEquation
#define GL_MIN_EXT	   GL_MIN
#endif

#ifndef GL_EXT_blend_minmax
#define glBlendEquationEXT(arg) ;
#endif

/*
 * Check for extensions 
 */
int 
isExtensionSupported(const char *extension)
{
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;

    /*
     * Extension names should not have spaces 
     */
    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0')
	return 0;

    extensions = glGetString(GL_EXTENSIONS);

    start = extensions;
    for (;;) {
	where = (GLubyte *) strstr((const char *) start, extension);
	if (!where)
	    break;
	terminator = where + strlen(extension);
	if (where == start || *(where - 1) == ' ')
	    if (*terminator == ' ' || *terminator == '\0')
		return 1;
	start = terminator;
    }

    return 0;
}

/*
 * Point structure 
 */
typedef struct {
    int x, y;
    unsigned char r, g, b;
} Point;

/*
 * The image 
 */
static char mandrill[] = "../data/mandrill.rgb";
GLuint *img;
int w, h;
int comp;

int maxPoints = 2048, numPoints = 0, sampleMenu, strokeMenu;
int viewImage = 0, useStrokes = 0, drawEdges = 0, canDrawEdges;
int moving = 0, mouseX, mouseY, lastX, lastY;
Point *points;


void 
makepoints(void)
{
    GLubyte *bi = (GLubyte *) img;
    int i;

    free(points);
    points = (Point *) malloc(maxPoints * sizeof(Point));

    numPoints = maxPoints;
    for (i = 0; i < maxPoints; i++) {
	points[i].x = rand() % w;
	points[i].y = rand() % h;
	points[i].r = bi[4 * (points[i].y * w + points[i].x)];
	points[i].g = bi[4 * (points[i].y * w + points[i].x) + 1];
	points[i].b = bi[4 * (points[i].y * w + points[i].x) + 2];
    }
}

/*
 * cone with base at (0,0,0) and top at (0,0,1)
 * one unit wide and high.
 */
void
makecone(void)
{
#define SIDES 20
    int i;
    glNewList(1, GL_COMPILE);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.f, 0.f, 1.f);
    for (i = 0; i <= SIDES; i++) {
	float s = sinf((2.f * M_PI * i) / SIDES) * 25;
	float c = cosf((2.f * M_PI * i) / SIDES) * 25;
	glVertex3f(s, c, -4.0);
    }
    glEnd();
    glEndList();
}

GLfloat edgeKernel[] = {
    -0.50f, 0.25f, -0.50f,
     0.25f, 1.00f, 0.25f,
     -0.50f, 0.25f, -0.50f};

GLfloat sumMatrix[] = {
    0.33f, 0.33f, 0.33f, 0.0f,
    0.33f, 0.33f, 0.33f, 0.0f,
    0.33f, 0.33f, 0.33f, 0.0f,
    0.00f, 0.00f, 0.00f, 1.0f};

/*
 * Called when window needs to be redrawn 
 */
void 
redraw(void)
{
    int i, x, y;

    if (viewImage) {
	glReadBuffer(GL_FRONT);
	glDrawBuffer(GL_FRONT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte *) img);
    } else if (!drawEdges) {
	glDrawBuffer(useStrokes ? GL_BACK : GL_FRONT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
	 * Just draw the voronoi regions 
	 */
	for (i = 0; i < numPoints; i++) {
	    glPushMatrix();
	    glTranslatef(points[i].x, points[i].y, 0.f);
	    glColor3ub(points[i].r, points[i].g, points[i].b);
	    glCallList(1);
	    glColor3f(0.f, 0.f, 0.f);
	    glPopMatrix();
	}

	if (useStrokes)
	    glutSwapBuffers();
    } else {
	glClear(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);

	glutSetWindowTitle("Voronoi art (working...)");
	glutSetCursor(GLUT_CURSOR_WAIT);
	for (y = 0; y < 3; y++)
	    for (x = 0; x < 3; x++) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPushMatrix();
		glTranslatef(x - 1, y - 1, 0.0);

		for (i = 0; i < numPoints; i++) {
		    glPushMatrix();
		    glTranslatef(points[i].x, points[i].y, 0.f);
		    glColor3ub(points[i].r, points[i].g, points[i].b);
		    glCallList(1);
		    glPopMatrix();
		}
		glPopMatrix();

		glAccum(GL_ACCUM, edgeKernel[3 * y + x]);
	    }

	glAccum(GL_RETURN, 0.5);
	glutSetWindowTitle("Voronoi art");
	glutSetCursor(GLUT_CURSOR_INHERIT);

	/*
	 * Convert to grayscale 
	 */
	glMatrixMode(GL_COLOR);
	glLoadMatrixf(sumMatrix);
	glCopyPixels(0, 0, w, h, GL_COLOR);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	/*
	 * Threshold 
	 */
	glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
	glCopyPixels(0, 0, w, h, GL_COLOR);
	glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

	/*
	 * Draw the voronoi regions in the front buffer 
	 */
	glDrawBuffer(GL_FRONT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (i = 0; i < numPoints; i++) {
	    glPushMatrix();
	    glTranslatef(points[i].x, points[i].y, 0.f);
	    glColor3ub(points[i].r, points[i].g, points[i].b);
	    glCallList(1);
	    glColor3f(0.f, 0.f, 0.f);
	    glPopMatrix();
	}

	/*
	 * Blend in the edge lines 
	 */
	glClear(GL_DEPTH_BUFFER_BIT);
	glBlendEquationEXT(GL_MIN_EXT);
	glEnable(GL_BLEND);
	glCopyPixels(0, 0, w, h, GL_COLOR);
	glDisable(GL_BLEND);
    }

    glFlush();
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y)
{
    switch (key) {
    case '\033':
    case 'q':
    case 'Q':
	exit(EXIT_SUCCESS);

    case '+':
	if (maxPoints < 65536 && !useStrokes) {
	    maxPoints *= 2;
	    makepoints();
	}
	break;
    case '-':
	if (maxPoints > 64 && !useStrokes) {
	    maxPoints /= 2;
	    makepoints();
	}
	break;
    case 'e':
    case 'E':
	if (canDrawEdges)
	    drawEdges = !drawEdges;
	break;
    case 'i':
    case 'I':
	viewImage = !viewImage;
	glutChangeToMenuEntry(useStrokes ? 1 : 4, viewImage ? "View artistic image ('i')" : "View source image ('i')", 'i');
	break;
    case 'r':
    case 'R':
	if (!useStrokes)
	    makepoints();
	break;

    case 's':
    case 'S':
	useStrokes = !useStrokes;
	glutSetMenu(useStrokes ? strokeMenu : sampleMenu);
	if (viewImage)
	    glutChangeToMenuEntry(useStrokes ? 1 : 3, "View artistic image ('i')", 'i');

	if (useStrokes) {
	    maxPoints = 65536;
	    free(points);
	    points = (Point *) malloc(maxPoints * sizeof(Point));
	    numPoints = 0;
	} else {
	    maxPoints = 2048;
	    makepoints();
	}
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
mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
	moving = (state == GLUT_DOWN);
	mouseX = x;
	mouseY = y;
    }
}

void 
motion(int x, int y)
{
    mouseX = x;
    mouseY = y;
}


void 
idle(void)
{
    if (useStrokes && moving && (numPoints < maxPoints) && (lastX != mouseX || lastY != mouseY)) {
	int y = h - 1 - mouseY;
	GLubyte *bi = (GLubyte *) img;

	points[numPoints].x = mouseX;
	points[numPoints].y = y;
	points[numPoints].r = bi[4 * (y * w + mouseX)];
	points[numPoints].g = bi[4 * (y * w + mouseX) + 1];
	points[numPoints].b = bi[4 * (y * w + mouseX) + 2];
	numPoints++;

	lastX = mouseX;
	lastY = mouseY;
	glutPostRedisplay();
    }
}

void 
init(void)
{
    int colorMatrixExtension,
        blendMinMaxExtension;

    /*
     * Points 
     */
    points = (Point *) malloc(maxPoints * sizeof(Point));
    makepoints();
    makecone();

    /*
     * Matrices 
     */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -4.0, 4.0);
    glMatrixMode(GL_MODELVIEW);
    glScalef(2.0 / w, 2.0 / h, 1.0);
    glTranslatef(-w / 2.0, -h / 2.0, 0.0);

    /*
     * Misc 
     */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /*
     * Test for the required features 
     */
    colorMatrixExtension = isExtensionSupported("GL_SGI_color_matrix");
    blendMinMaxExtension = isExtensionSupported("GL_EXT_blend_minmax");
    canDrawEdges = (colorMatrixExtension && blendMinMaxExtension) ||
	(strncmp((const char *) glGetString(GL_VERSION), "1.2", 3) == 0);

    /*
     * Test for blend extension 
     */
    if (canDrawEdges) {
	GLfloat table[256];
	int i;

	/*
	 * Pixel transfer parameters 
	 */
	table[0] = 1.0;
	for (i = 1; i < 256; i++)
	    table[i] = 0.0;
	glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 256, table);
	glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 256, table);
	glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 256, table);
    } else {
	printf("This OpenGL implementation does not support the color matrix and/or\n");
	printf("the min/max blending equations, therefore the option to draw the\n");
	printf("Voronoi region edges is unavailable.\n\n");
	printf("The required features are available with OpenGL 1.2 or the GL_EXT_blend_minmax\n");
	printf("and GL_SGI_color_matrix extensions.\n");
    }
}

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(20, 20);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_ACCUM);
    (void) glutCreateWindow("Voronoi art");
    glutKeyboardFunc(key);
    glutDisplayFunc(redraw);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);

    /*
     * Load the image and sample it 
     */
    img = (GLuint *)read_texture((argc > 1) ? argv[1] : mandrill, &w, &h, &comp);
    if (!img) {
	fprintf(stderr, "Could not open image.\n");
	exit(1);
    }
    glutReshapeWindow(w, h);
    init();

    /*
     * Menu 
     */
    sampleMenu = glutCreateMenu(menu);
    glutAddMenuEntry("More samples ('+')", '+');
    glutAddMenuEntry("Fewer samples ('-')", '-');
    glutAddMenuEntry("Resample points ('r')", 'r');
    glutAddMenuEntry("View source image ('i')", 'i');
    if (canDrawEdges)
	glutAddMenuEntry("Toggle drawing region edges ('e')", 'e');
    glutAddMenuEntry("", ' ');
    glutAddMenuEntry("Paint with mouse strokes ('s')", 's');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    strokeMenu = glutCreateMenu(menu);
    glutAddMenuEntry("View source image ('i')", 'i');
    glutAddMenuEntry("", ' ');
    glutAddMenuEntry("Paint with random samples ('s')", 's');
    glutAddMenuEntry("exit", '\033');

    glutMainLoop();

    return 0;
}
