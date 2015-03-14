/*
 *  displace.c
 *  Brad Grantham, 1999
 *
 *  Demonstrates how to create displaced geometry from a texture map
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

/* Win32 math.h doesn't define M_PI. */
#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

char defaultFile[] = "../data/rubbermaid.rgb";

int 	winWidth, winHeight;
float (*verts)[3];
float (*verts2)[3];
float (*texcoords)[2];
float (*normals)[3];
float (*normals2)[3];
int vertsU, vertsV;
unsigned int *heightMap;

int inWireFrame;
int noLighting;
int drawNormals;

typedef struct {
    float	translation[3];
    float	rotation[4];
    float	scale[3];
} transformation;

transformation *curXform;

transformation globalXform = {
    {0.0f, 0.0f, 0.0f},
    {-0.86f, 0.5f, -.4f, .76f},
    {1.0f, 1.0f, 1.0f}
};
transformation meshXform = {
    {0.0f, 0.0f, 0.0f},
    {0.9981f, 0.0074f, -0.0609f, 0.073f},
    {1.0f, 1.0f, 1.0f}
};
transformation lightXform = {
    {3.5f, 5.7f, 5.675f},
    {1.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f}
};

void applyXform(transformation *xform)
{
    glTranslatef(xform->translation[0], xform->translation[1],
        xform->translation[2]);
    glRotatef(xform->rotation[3] / M_PI * 180, xform->rotation[0],
        xform->rotation[1], xform->rotation[2]);
    glScalef(xform->scale[0], xform->scale[1], xform->scale[2]);
}

void axisamountToMat(float aa[], float mat[])
{
    float c, s, t;

    c = (float)cos(aa[3]);
    s = (float)sin(aa[3]);
    t = 1.0f - c;

    mat[0] = t * aa[0] * aa[0] + c;
    mat[1] = t * aa[0] * aa[1] + s * aa[2];
    mat[2] = t * aa[0] * aa[2] - s * aa[1];
    mat[3] = t * aa[0] * aa[1] - s * aa[2];
    mat[4] = t * aa[1] * aa[1] + c;
    mat[5] = t * aa[1] * aa[2] + s * aa[0];
    mat[6] = t * aa[0] * aa[2] + s * aa[1];
    mat[7] = t * aa[1] * aa[2] - s * aa[0];
    mat[8] = t * aa[2] * aa[2] + c;
}

void matToAxisamount(float mat[], float aa[])
{
    float c;
    float s;

    c = (mat[0] + mat[4] + mat[8] - 1.0f) / 2.0f;
    aa[3] = (float)acos(c);
    s = (float)sin(aa[3]);
    if(fabs(s / M_PI - (int)(s / M_PI)) < .0000001) {
        aa[0] = 0.0f;
        aa[1] = 1.0f;
        aa[2] = 0.0f;
    } else {
	aa[0] = (mat[5] - mat[7]) / (2.0f * s);
	aa[1] = (mat[6] - mat[2]) / (2.0f * s);
	aa[2] = (mat[1] - mat[3]) / (2.0f * s);
    }
}

void multMat(float m1[], float m2[], float r[])
{
    float t[9];
    int i;

    t[0] = m1[0] * m2[0] + m1[1] * m2[3] + m1[2] * m2[6];
    t[1] = m1[0] * m2[1] + m1[1] * m2[4] + m1[2] * m2[7];
    t[2] = m1[0] * m2[2] + m1[1] * m2[5] + m1[2] * m2[8];
    t[3] = m1[3] * m2[0] + m1[4] * m2[3] + m1[5] * m2[6];
    t[4] = m1[3] * m2[1] + m1[4] * m2[4] + m1[5] * m2[7];
    t[5] = m1[3] * m2[2] + m1[4] * m2[5] + m1[5] * m2[8];
    t[6] = m1[6] * m2[0] + m1[7] * m2[3] + m1[8] * m2[6];
    t[7] = m1[6] * m2[1] + m1[7] * m2[4] + m1[8] * m2[7];
    t[8] = m1[6] * m2[2] + m1[7] * m2[5] + m1[8] * m2[8];
    for(i = 0; i < 9; i++)
    {
        r[i] = t[i];
    }
}


void rotateTrackball(int dx, int dy, float rotation[4])
{
    float dist;
    float oldMat[9];
    float rotMat[9];
    float newRot[4];

    dist = (float)sqrt((double)(dx * dx + dy * dy));
    if(fabs(dist) < 0.99)
        return;

    newRot[0] = (float) dy / dist;
    newRot[1] = (float) dx / dist;
    newRot[2] = 0.0f;
    newRot[3] = (float)M_PI * dist / winWidth;

    axisamountToMat(rotation, oldMat);
    axisamountToMat(newRot, rotMat);
    multMat(oldMat, rotMat, oldMat);
    matToAxisamount(oldMat, rotation);

    dist = (float)sqrt(rotation[0] * rotation[0] + rotation[1] * rotation[1] +
        rotation[2] * rotation[2]);

    rotation[0] /= dist;
    rotation[1] /= dist;
    rotation[2] /= dist;
}

/*
 * Probably want to do the following before using as a floor.
 * glScalef(floorSize, 1.0f, floorSize);
 * glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
 * glTranslatef(-0.5f, -0.5f, 0.0f);
 */
void makeFlatQuadMesh(float (*verts)[3], float (*normals)[3],
    float (*texcoords)[2], int vertsU, int vertsV)
{
    int i, j;
    float dx, dy;
    float x, y;

    dx = 1.0 / (vertsU - 1.0);
    dy = 1.0 / (vertsV - 1.0);
    y = 0; 
    for(j = 0; j < vertsV; j++) {
	x = 0;
	for(i = 0; i < vertsU; i++) {
	    normals[i + j * vertsU][0] = 0.0f;
	    normals[i + j * vertsU][1] = 0.0f;
	    normals[i + j * vertsU][2] = 1.0f;
	    texcoords[i + j * vertsU][0] = x;
	    texcoords[i + j * vertsU][1] = y;
	    verts[i + j * vertsU][0] = x;
	    verts[i + j * vertsU][1] = y;
	    verts[i + j * vertsU][2] = 0;
	    x += dx;
	}
	y += dy;
    }
}

enum {
    ORIGINAL,
    NORMALS,
    VERTICES_NORMALS
} meshMode = NORMALS;

void drawQuadMesh(float (*verts)[3], float (*normals)[3],
    float (*texcoords)[2], int vertsU, int vertsV)
{
    int i, j;

    if(drawNormals) {
	glDisable(GL_LIGHTING);
	glColor3f(1, 0, 0);
	glLineWidth(2.0f);
	for(j = 0; j < vertsV; j++) {
	    glBegin(GL_LINES);
	    for(i = 0; i < vertsU; i++) {
		if(normals[i + j * vertsU][0] == 0.0f &&
		    normals[i + j * vertsU][1] == 0.0f &&
		    !(meshMode == ORIGINAL))
		    continue;
		glVertex3fv(verts[i + j * vertsU]);
		glVertex3f(verts[i + j * vertsU][0] + .1 * normals[i + j * vertsU][0],
		    verts[i + j * vertsU][1] + .1 * normals[i + j * vertsU][1],
		    verts[i + j * vertsU][2] + .1 * normals[i + j * vertsU][2]);
	    }
	    glEnd();
	}
	glLineWidth(1.0f);
	glColor3f(1, 1, 1);
	if(!noLighting)
	    glEnable(GL_LIGHTING);
    }
    for(j = 0; j < (vertsV - 1); j++) {
	glBegin(GL_TRIANGLE_STRIP);
	if(drawNormals)
	    glColor3f(.5f, .5f, .5f);
	else
	    glColor3f(1.0f, 1.0f, 1.0f);
	for(i = 0; i < vertsU; i++) {
	    if(normals != NULL) glNormal3fv(normals[i + (j + 1) * vertsU]);
	    if(texcoords != NULL) glTexCoord2fv(texcoords[i + (j + 1) *
		vertsU]);
	    glVertex3fv(verts[i + (j + 1) * vertsU]);
	    if(normals != NULL) glNormal3fv(normals[i + j * vertsU]);
	    if(texcoords != NULL) glTexCoord2fv(texcoords[i + j * vertsU]);
	    glVertex3fv(verts[i + j * vertsU]);
	}
	glEnd();
    }
}

float scale = 1;

void normalizeVec3f(float v[3], float n[3])
{
    float length;
    length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    n[0] = v[0] / length;
    n[1] = v[1] / length;
    n[2] = v[2] / length;
}

#define X 0
#define Y 1
#define Z 2

void crossVec3f(float v0[3], float v1[3], float result[3])
{
    result[X] = v0[Y] * v1[Z] - v0[Z] * v1[Y];
    result[Y] = v0[Z] * v1[X] - v0[X] * v1[Z];
    result[Z] = v0[X] * v1[Y] - v0[Y] * v1[X];
}


/*ARGSUSED*/
void displaceQuadMesh(float (*verts)[3], float (*normals)[3],
    float (*texcoords)[2], int vertsU, int vertsV, unsigned int *map,
    float (*verts2)[3], float (*normals2)[3])
{
    int i, j;
    unsigned char *bumpmap = (unsigned char *)map;
    float dnx, dny;

    for(j = 0; j < vertsV; j++) {
	for(i = 0; i < vertsU; i++) {
	    /* displace vertices */
	    verts2[i + j * vertsU][0] = verts[i + j * vertsU][0] + scale *
	        bumpmap[(i + j * vertsU) * 4] / 256.0 * normals[i + j * vertsU][0] * (1.0 / (vertsU));
	    verts2[i + j * vertsU][1] = verts[i + j * vertsU][1] + scale *
	        bumpmap[(i + j * vertsU) * 4] / 256.0 * normals[i + j * vertsU][1] * (1.0 / (vertsU));
	    verts2[i + j * vertsU][2] = verts[i + j * vertsU][2] + scale *
	        bumpmap[(i + j * vertsU) * 4] / 256.0 * normals[i + j * vertsU][2] * (1.0 / (vertsU));

	    /* approximate derivatives of bumpmap in u and then v dir */
	    if(i != vertsU - 1)
	        dnx = scale * (bumpmap[((i + 1) + j * vertsU) * 4] - bumpmap[(i + j * vertsU) * 4]) / 256.0;
	    else
	        dnx = scale * (bumpmap[(i + j * vertsU) * 4] - bumpmap[((i - 1) + j * vertsU) * 4]) / 256.0;
	    if(j != vertsV - 1)
	        dny = scale * (bumpmap[(i + (j + 1) * vertsU) * 4] - bumpmap[(i + j * vertsU) * 4]) / 256.0;
	    else
	        dny = scale * (bumpmap[(i + j * vertsU) * 4] - bumpmap[(i + (j - 1) * vertsU) * 4]) / 256.0;

	    /* displace normal by du, dv of bumpmap */
	    normals2[i + j * vertsU][0] = normals[i + j * vertsU][0] - dnx;
	    normals2[i + j * vertsU][1] = normals[i + j * vertsU][1] - dny;
	    normals2[i + j * vertsU][2] = normals[i + j * vertsU][2];
	}
    }
}

void init(const char *heightMapName)
{
    int width, height, comps;
    GLfloat specularMtl[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat diffuseMtl[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularMtl);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseMtl);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DITHER);
    glEnable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glFrustum(-.33, .33, -.33, .33, .5, 40);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_NORMALIZE);

    heightMap = read_texture(heightMapName, &width, &height, &comps);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 1, width, height, GL_RGBA,
        GL_UNSIGNED_BYTE, heightMap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR);
    /* should be GL_ALPHA texture */

    vertsU = width;
    vertsV = height;

    verts = malloc(sizeof(verts[0]) * vertsU * vertsV);
    verts2 = malloc(sizeof(verts2[0]) * vertsU * vertsV);
    normals = malloc(sizeof(normals[0]) * vertsU * vertsV);
    normals2 = malloc(sizeof(normals2[0]) * vertsU * vertsV);
    texcoords = malloc(sizeof(texcoords[0]) * vertsU * vertsV);

    makeFlatQuadMesh(verts, normals, texcoords, vertsU, vertsV);
    displaceQuadMesh(verts, normals, texcoords, vertsU, vertsV, heightMap,
        verts2, normals2);

    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);

    curXform = &globalXform;
}


static GLfloat 	zeroVec[] = {0.0f, 0.0f, 0.0f, 1.0f};

void setupLight(void)
{
    /* static GLfloat 	lightpos[] = {0.0f, 5.0f, -3.0f, 1.0f}; */
    static GLfloat 	lightpos[] = {0.0f, 0.0f, 0.0f, 1.0f};
    static GLfloat	specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glPushMatrix();
    glTranslatef(lightpos[0], lightpos[1], lightpos[2]);
    glLightfv(GL_LIGHT0, GL_POSITION, zeroVec);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glDisable(GL_LIGHTING);
    glutSolidSphere(.2, 5, 5);
    if(!noLighting)
	glEnable(GL_LIGHTING);
    glPopMatrix();
}

void redraw(void)
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    applyXform(&globalXform);
    
    glPushMatrix();

    applyXform(&lightXform);
    
    setupLight();

    glPopMatrix();

    glPushMatrix();

    applyXform(&meshXform);
    
    /* glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, black); */

    glScalef(10.0f, 10.0f, 10.0f);
    glTranslatef(-0.5f, -0.5f, 0.0f);
    if(meshMode == ORIGINAL)
	drawQuadMesh(verts, normals, texcoords, vertsU, vertsV);
    else if(meshMode == NORMALS)
	drawQuadMesh(verts, normals2, texcoords,
	    vertsU, vertsV);
    else if(meshMode == VERTICES_NORMALS)
	drawQuadMesh(verts2, normals2, texcoords,
	    vertsU, vertsV);
    else
	fprintf(stderr, "unknown mesh mode, line %d\n", __LINE__);


    glPopMatrix();

    glPopMatrix();

    glutSwapBuffers();
}

void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    winWidth = width;
    winHeight = height;
    glutPostRedisplay();
}

static int ox, oy;


/*ARGSUSED*/
void button(int b, int state, int x, int y)
{
    ox = x;
    oy = y;
}


enum trackballModeEnum {
    ROTATE,
    TRANSLATEXY,
    TRANSLATEZ,
    SCALEX,
    SCALEY,
    SCALEZ
} trackballMode = ROTATE;

void motion(int x, int y)
{
    int dx, dy;

    dx = x - ox;
    dy = y - oy;

    ox = x;
    oy = y;

    switch(trackballMode) {
	case ROTATE:
	    rotateTrackball(dx, dy, curXform->rotation);
	    break;

	case SCALEX:
	    curXform->scale[0] += (dx + dy) / 40.0f;
	    if(curXform->scale[0] < 1/40.0f)
	        curXform->scale[0] = 1/40.0f;
	    break;

	case SCALEY:
	    curXform->scale[1] += (dx + dy) / 40.0f;
	    if(curXform->scale[1] < 1/40.0f)
	        curXform->scale[1] = 1/40.0f;
	    break;

	case SCALEZ:
	    curXform->scale[2] += (dx + dy) / 40.0f;
	    if(curXform->scale[2] < 1/40.0f)
	        curXform->scale[2] = 1/40.0f;
	    break;

	case TRANSLATEXY:
	    curXform->translation[0] += dx / 40.0f;
	    curXform->translation[1] -= dy / 40.0f;
	    break;

	case TRANSLATEZ:
	    curXform->translation[2] += (dx + dy) / 40.0f;
	    break;
    }
    glutPostRedisplay();
}

/*ARGSUSED1*/
void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 'r':
	    trackballMode = ROTATE;
	    break;

	case 't':
	    trackballMode = TRANSLATEXY;
	    break;

	case 'T':
	    trackballMode = TRANSLATEZ;
	    break;

	case 'x':
	    trackballMode = SCALEX;
	    break;

	case 'y':
	    trackballMode = SCALEY;
	    break;

	case 'z':
	    trackballMode = SCALEZ;
	    break;

	case '1':
	    curXform = &meshXform;
	    break;

	case '2':
	    curXform = &lightXform;
	    break;

	case '0':
	    curXform = &globalXform;
	    break;

	case 's':
	    {
		int row;
		FILE *fp;
		unsigned char image[512][512][3];
		glReadBuffer(GL_FRONT);
		glReadPixels(0, 0, 512, 512, GL_RGB, GL_UNSIGNED_BYTE, image);
		glReadBuffer(GL_BACK);
		fp = fopen("snapshot.ppm", "w");
		fprintf(fp, "P6 512 512 255\n");
		for(row = 511; row >= 0; row--)
		    fwrite(image[row], 512, 3, fp);
		fclose(fp);
	    }
	    break;

	case 'q': case 'Q': case '\033':
	    exit(0);
	    break;

	case ',':
	    scale /= 1.5;
	    displaceQuadMesh(verts, normals, texcoords, vertsU, vertsV,
	        heightMap, verts2, normals2);
	    glutPostRedisplay();
	    break;

	case '.':
	    scale *= 1.5;
	    displaceQuadMesh(verts, normals, texcoords, vertsU, vertsV,
	        heightMap, verts2, normals2);
	    glutPostRedisplay();
	    break;

	default:
	    fprintf(stderr, "Push right mouse button for menu\n");
	    break;

    }
}

int mainMenu;
int displayMenu;

void displayMenuFunc(int entry)
{
    glutSetMenu(displayMenu);

    if(entry == 1) {
	meshMode = ORIGINAL;
    } else if(entry == 2) {
	meshMode = NORMALS;
    } else if(entry == 3) {
	meshMode = VERTICES_NORMALS;
    }
    glutPostRedisplay();
}

void mainMenuFunc(int entry)
{
    glutSetMenu(mainMenu);

    if(entry == 1) {
	if(inWireFrame)
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	else
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	inWireFrame = !inWireFrame;
	glutPostRedisplay();
    } else if(entry == 2) {
	if(noLighting)
	    glEnable(GL_LIGHTING);
	else
	    glDisable(GL_LIGHTING);
	noLighting = !noLighting;
	glutPostRedisplay();
    } else if(entry == 3) {
	drawNormals = !drawNormals;
	glutPostRedisplay();
    } else if(entry == 999){
	exit(0);
    }
}

int main(int argc, char **argv)
{
    glutInitWindowSize(winWidth = 1024, winHeight = 1024);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
    (void)glutCreateWindow(argv[0]);
    glutDisplayFunc(redraw);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(motion);
    glutMouseFunc(button);
    glutReshapeFunc(reshape);

    displayMenu = glutCreateMenu(displayMenuFunc);
    glutAddMenuEntry("Just displaced normals", 2);
    glutAddMenuEntry("Displaced normals and vertices", 3);
    glutAddMenuEntry("Original surface", 1);

    mainMenu = glutCreateMenu(mainMenuFunc);
    glutAddMenuEntry("Toggle wireframe", 1);
    glutAddMenuEntry("Toggle lighting", 2);
    glutAddMenuEntry("Toggle display of normals", 3);
    glutAddSubMenu("Mesh display", displayMenu);
    glutAddMenuEntry("Quit", 999);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

	init((argc > 1) ? argv[1] : defaultFile);

    glutMainLoop();

    return 0;
}
