#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

static char defaultFile[] = "../data/mandrill.rgb";

GLuint *img;
int w0, h0;
int w, h;
int comp;

#define MAX_MOVE_POINTS	64
typedef struct {
    GLfloat old[2];
    GLfloat new[2];
    GLfloat vec[2];
} MovePoint;
MovePoint movePoints[MAX_MOVE_POINTS];
int nMovePoints = 0;

#define TESSELLATION	64
#define INF		100000000000.f

void 
init(void)
{
    /*
     * tack corners in place 
     */

    movePoints[0].old[0] = movePoints[0].new[0] = 0;
    movePoints[0].old[1] = movePoints[0].new[1] = 0;
    movePoints[0].vec[0] = movePoints[0].vec[1] = 0;

    movePoints[1].old[0] = movePoints[1].new[0] = w;
    movePoints[1].old[1] = movePoints[1].new[1] = 0;
    movePoints[1].vec[0] = movePoints[1].vec[1] = 0;

    movePoints[2].old[0] = movePoints[2].new[0] = w;
    movePoints[2].old[1] = movePoints[2].new[1] = h;
    movePoints[2].vec[0] = movePoints[2].vec[1] = 0;

    movePoints[3].old[0] = movePoints[3].new[0] = 0;
    movePoints[3].old[1] = movePoints[3].new[1] = h;
    movePoints[3].vec[0] = movePoints[3].vec[1] = 0;

    nMovePoints = 4;
}

GLuint *
load_img(const char *fname, int *imgW, int *imgH)
{
    GLuint *img;

    img = (GLuint *)read_texture(fname, imgW, imgH, &comp);
    if (!img) {
	fprintf(stderr, "Could not open %s\n", fname);
	exit(1);
    }
    return img;
}

GLuint *
resize_img(GLuint *img, int curW, int curH)
{
    /*
     * save & set buffer settings 
     */
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);

    glPixelZoom((float) w / (float) curW, (float) h / (float) curH);
    glRasterPos2i(0, 0);
    glDrawPixels(curW, curH, GL_RGBA, GL_UNSIGNED_BYTE, img);
    free(img);
    img = (GLuint *) malloc(w * h * sizeof(GLuint));
    if (!img) {
	fprintf(stderr, "Malloc of %d bytes failed.\n",
		curW * curH * sizeof(GLuint));
	exit(1);
    }
    glPixelZoom(1, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);

    glPopAttrib();

    return img;
}

void 
reshape(int winW, int winH)
{
    glViewport(0, 0, w, h);
    glLoadIdentity();
    glOrtho(0, winW, 0, winH, 0, 5);
}

GLfloat 
dist(GLfloat * a, GLfloat * b)
{
    GLfloat x;

    x = (a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]);
    x /= (w * w + h * h);

    return sqrt(x);
}

void 
warp(GLfloat * orig, GLfloat * warped)
{
    GLfloat weight, weightSum;
    MovePoint *pt = movePoints;
    int i;

    weightSum = 0;
    warped[0] = warped[1] = 0;
    for (i = 0; i < nMovePoints; i++) {
	weight = dist(orig, pt->old);
	if (weight) {
	    weight = 1.f / weight;
	} else {
	    weight = INF;
	}

	warped[0] += pt->vec[0] * weight;
	warped[1] += pt->vec[1] * weight;
	weightSum += weight;
	pt++;
    }

    if (weightSum) {
	warped[0] = orig[0] + warped[0] / weightSum;
	warped[1] = orig[1] + warped[1] / weightSum;
    } else {
	warped[0] = orig[0];
	warped[1] = orig[1];
    }
}

void 
draw_grid(void)
{
    static int first = 1;
    int x, y;
    GLfloat gridPt[2][2], warpedGridPt[2], texPt[2][2];

    if (first) {
	if (w0 != w || h0 != h) {
	    img = resize_img(img, w0, h0);
	}
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
			  img);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

	first = 0;
    }
    for (x = 0; x < TESSELLATION; x++) {
	texPt[0][0] = (float) x / (float) TESSELLATION;
	texPt[1][0] = (float) (x + 1) / (float) TESSELLATION;
	gridPt[0][0] = texPt[0][0] * (float) w;
	gridPt[1][0] = texPt[1][0] * (float) w;
	glBegin(GL_TRIANGLE_STRIP);
	for (y = 0; y <= TESSELLATION; y++) {
	    texPt[0][1] = texPt[1][1] = (float) y / (float) TESSELLATION;
	    gridPt[0][1] = gridPt[1][1] = texPt[0][1] * (float) h;
	    glTexCoord2fv(texPt[0]);
	    glColor3f(texPt[0][0], texPt[0][1], 0);
	    warp(gridPt[0], warpedGridPt);
	    glVertex2fv(warpedGridPt);
	    glTexCoord2fv(texPt[1]);
	    glColor3f(texPt[1][0], texPt[1][1], 0);
	    warp(gridPt[1], warpedGridPt);
	    glVertex2fv(warpedGridPt);
	}
	glEnd();
    }
}

void 
draw(void)
{
    GLenum err;

    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1, 1, 1);

    glEnable(GL_TEXTURE_2D);
    draw_grid();
    glDisable(GL_TEXTURE_2D);

    err = glGetError();
    if (err != GL_NO_ERROR)
	printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}

/*ARGSUSED1*/
void 
key(unsigned char key, int x, int y)
{
    if (key == 27)
	exit(0);
}

/*ARGSUSED*/
void 
button(int button, int state, int xpos, int ypos)
{
    MovePoint *pt = &movePoints[nMovePoints];
    GLfloat tmp[3];

    /*
     * if we're about to run out of space in the array, just start ignoring
     * * stuff.  we could handle this more elegranly if we felt like it... 
     */
    if (nMovePoints == MAX_MOVE_POINTS)
	return;

    /*
     * convert to sane GL coordinate system from X coordinate system 
     */
    ypos = h - ypos;

    if (state == GLUT_DOWN) {
	/*
	 * draw a little point where the mouse is 
	 */
	glDrawBuffer(GL_FRONT);
	glColor3f(1, 0, 0);
	glPointSize(16);
	glBegin(GL_POINTS);
	glVertex2f(xpos, ypos);
	glEnd();
	glDrawBuffer(GL_BACK);
	glFlush();

	/*
	 * figure out where the mouse is... 
	 */
	draw_grid();
	glReadPixels(xpos, ypos, 1, 1, GL_RGB, GL_FLOAT, tmp);
	pt->old[0] = tmp[0] * (float) w;
	pt->old[1] = tmp[1] * (float) h;
    } else {
	pt->new[0] = xpos;
	pt->new[1] = ypos;
	pt->vec[0] = pt->new[0] - pt->old[0];
	pt->vec[1] = pt->new[1] - pt->old[1];

	nMovePoints++;
	draw();
    }
}

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    if (argc > 1) {
	img = load_img(argv[1], &w0, &h0);
    } else {
	img = load_img(defaultFile, &w0, &h0);
    }
    w = w0;
    h = h0;
    if (w & (w - 1)) {
	w = 1;
	while (w < w0)
	    w <<= 1;
    }
    if (h & (h - 1)) {
	h = 1;
	while (h < h0)
	    h <<= 1;
    }
    glutInitWindowSize(w, h);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(button);
    init();

    glutMainLoop();
    return 0;
}
