#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <GL/glut.h>

void pushOrthoView(float left, float right, float bottom, float top,
    float v_near, float v_far)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(left, right, bottom, top, v_near, v_far);
}

void popView(void)
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/* Setup -------------------------------------------------------------------*/

/* Win32 math.h doesn't define M_PI. */
#ifdef WIN32
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

#define TRUE	1
#define FALSE	0

/* Trackball ---------------------------------------------------------------*/

typedef struct {
    float	translation[3];
    float	rotation[4];
    float	scale[3];
} transformation;

transformation *curXform;

transformation globalXform = {
    {0.0f, 0.0f, 0.0f},
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

int 		winWidth, winHeight;

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

/* Surface definition & creation -------------------------------------------*/

typedef struct {
    float v[3]; /* coordinate */
    float t[2]; /* texture coordinate */
    float n[3]; /* normal */
    float c[3]; /* color */
} vertex;

typedef struct {
    float m[2]; /* map coordinates */
    int f; /* is front-facing, if back, has been fixed up */
} mapvert;

typedef struct {
    vertex *verts;
    vertex *everts; /* transformed into eye space, lit, textured */
    vertex *rverts; /* reflected vertices */
    mapvert *mapvertsNear; /* reflection map coordinates */
    mapvert *mapvertsFar;  /* reflection map coordinates */
    int vertCount;
    int *indices; /* triangle indices into the three "verts" arrays above */
    int triCount;
    int hasTexture, hasColor;
    float ambient[4]; 
    float diffuse[4]; 
    float specular[4]; 
    float center[3]; /* used in ExplosionMap creation */
} surface;

/*
 * Given verts & indices, make a tesselated quad out of triangles.
 */
int makeTesselatedSquare(int perSide, vertex *verts, int *indices, int base,
    float left, float width, float bottom, float height)
{
    int i, j;
    vertex *vp;
    int *ip;

    vp = verts;
    for(j = 0; j < perSide + 1; j++)
	for(i = 0; i < perSide + 1; i++) {
 	    /* coordinate */
            vp->v[0] = left + i * width / (float)perSide;
            vp->v[1] = bottom + j * height / (float)perSide;
	    vp->v[2] = 0.0f;

	    /* normal */
	    vp->n[0] = 0.0f;
	    vp->n[1] = 0.0f;
	    vp->n[2] = 1.0f;

 	    /* texture coordinate, just for yucks */
            vp->t[0] = i / (float)perSide;
            vp->t[1] = j / (float)perSide;

	    vp++;
	}

    ip = indices;
    for(j = 0; j < perSide; j++)
	for(i = 0; i < perSide; i++) {
	    /* triangle 1 */
	    *ip++ = base + j * (perSide + 1) + i;		/* lower left */
	    *ip++ = base + j * (perSide + 1) + i + 1;		/* lower right */
	    *ip++ = base + (j + 1) * (perSide + 1) + i + 1;	/* upper right */

	    /* triangle 2 */
	    *ip++ = base + j * (perSide + 1) + i;		/* lower left */
	    *ip++ = base + (j + 1) * (perSide + 1) + i + 1;	/* upper right */
	    *ip++ = base + (j + 1) * (perSide + 1) + i;		/* upper left */
	}
    return perSide * perSide;
}

void allocSurfaceArrays(surface *surf, int vertCount, int triCount)
{
    surf->verts = malloc(sizeof(vertex) * vertCount);
    surf->everts = malloc(sizeof(vertex) * vertCount);
    surf->rverts = malloc(sizeof(vertex) * vertCount);
    surf->mapvertsNear = malloc(sizeof(mapvert) * vertCount);
    surf->mapvertsFar = malloc(sizeof(mapvert) * vertCount);
    surf->vertCount = vertCount;

    surf->indices = malloc(sizeof(int) * triCount * 3);
    surf->triCount = triCount;
}

surface surf1 = {
    NULL, NULL, NULL, NULL, NULL, 0, NULL, 0,
    0, 0,
    {0.8f, 0.4f, 0.4f, 1.0f},
    {0.8f, 0.4f, 0.4f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.0f}, /* {0.8f, 0.4f, 0.4f, 1.0f}, */
    {0.0f, 0.0f, 0.0f},
};
transformation surf1Xform = {
    {0.0f, 0.0f, -0.8f},
    {1.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f}
};

/*
 * This is actually one cube face tesselated and normalized as if it
 * was part of a sphere tesselation.
 * Could use triangle strips for this, but I decided it would just look
 * too complicated with strip lengths, etc...
 */
void makeCurved(int perSide)
{
    int i;
    vertex *vp;

    /*
     * perSide quads means "perSide + 1" vertices.
     * perSide quads means "per side * two" triangles
     */
    allocSurfaceArrays(&surf1, (perSide + 1) * (perSide + 1),
        perSide * perSide * 2);

    makeTesselatedSquare(perSide, surf1.verts, surf1.indices, 0,
        -.5, 1, -.5, 1);

    /* Fix up vertices */
    vp = surf1.verts;
    for(vp = surf1.verts, i = 0; i < (perSide + 1) * (perSide + 1); i++, vp++) {
	float dist;

	/* normalize coordinate so surface curves */
	vp->v[2] = 0.5f; /* fix Z output from makeTesselatedSquare */
	dist = sqrt(vp->v[0] * vp->v[0] + vp->v[1] * vp->v[1] + 
	    vp->v[2] * vp->v[2]);
	vp->v[0] /= dist;
	vp->v[1] /= dist;
	vp->v[2] /= dist;

	/* normal is conveniently the same as normalized coordinate */
	vp->n[0] = vp->v[0];
	vp->n[1] = vp->v[1];
	vp->n[2] = vp->v[2];
    }
}

surface surf2 = {
    NULL, NULL, NULL, NULL, NULL, 0, NULL, 0,
    0, 1, 
    {1.0f, 1.0f, 1.0f, 1.0f}, /* not used */
    {1.0f, 1.0f, 1.0f, 1.0f}, /* not used */
    {0.0f, 0.0f, 0.0f, 0.0f}, /* {1.0f, 1.0f, 1.0f, 1.0f}, */
    {0, 0, -1},
};
transformation surf2Xform = {
    {0.0f, -0.3f, 0.8f},
    {1.0f, 0.0f, 0.0f, -1.570795f},
    {1.0f, 1.0f, 1.0f}
};

/*
 * This is a checkerboard with "checkCount * checkCount" squares
 * alternating black and white.  Each square is also broken into
 * "perCheck * perCheck" pieces.
 */
void makeChecker(int checkCount, int perCheck)
{
    float left, bottom, width, height;
    vertex *vp;
    int *ip;
    int i, j, k;

    allocSurfaceArrays(&surf2,
        (perCheck + 1) * (perCheck + 1) * checkCount * checkCount,
	perCheck * perCheck * 2 * checkCount * checkCount);

    width = 1 / (float)checkCount;
    height = 1 / (float)checkCount;
    vp = surf2.verts;
    ip = surf2.indices;
    for(j = 0; j < checkCount; j++)
	for(i = 0; i < checkCount; i++) {
	    left = -.5 + i / (float) checkCount;
	    bottom = -.5 + j / (float) checkCount;
	    makeTesselatedSquare(perCheck, vp, ip,
	        (i + j * checkCount) * (perCheck + 1) * (perCheck + 1),
		left, width, bottom, height);
	    for(k = 0; k < (perCheck + 1) * (perCheck + 1); k++) {
		vp[k].c[0] = (float)((i + j) % 2);
		vp[k].c[1] = (float)((i + j) % 2); 
		vp[k].c[2] = (float)((i + j) % 2); 
	    }
	    vp += (perCheck + 1) * (perCheck + 1);
	    ip += perCheck * perCheck * 2 * 3;
	}
}

void drawSurf(vertex *verts, int *indices, int triCount, int sendNormal,
    int sendColor, int sendTexture)
{
    int i;
    int *ip = indices;
    
    /* Could use DrawElements here */

    glBegin(GL_TRIANGLES);

    for(ip = indices, i = 0; i < triCount * 3; ip++, i++) {
	if(sendNormal) {
	    glNormal3fv(verts[*ip].n);
	    /* printf("normal %f %f %f\n", verts[*ip].n[0], verts[*ip].n[1], */
	        /* verts[*ip].n[2]); */
	}

	if(sendColor) {
	    glColor3fv(verts[*ip].c);
	    /* printf("color %f %f %f\n", verts[*ip].c[0], verts[*ip].c[1], */
	        /* verts[*ip].c[2]); */
	}

	if(sendTexture) {
	    glTexCoord3fv(verts[*ip].t);
	    /* printf("texture %f %f %f\n", verts[*ip].t[0], verts[*ip].t[1], */
	        /* verts[*ip].t[2]); */
	}

	glVertex3fv(verts[*ip].v);
	/* printf("vertex %f %f %f\n", verts[*ip].v[0], verts[*ip].v[1], */
	    /* verts[*ip].v[2]); */
    }

    glEnd();
}

/* Explosion map reflections -----------------------------------------------*/

void matrix4fTranspose(float m[4][4])
{
    float t;
#define MSWAP(i, j)  t = m[i][j]; m[i][j] = m[j][i]; m[j][i] = t;

    MSWAP(0, 1);
    MSWAP(0, 2);
    MSWAP(1, 2);
    MSWAP(0, 3);
    MSWAP(1, 3);
    MSWAP(2, 3);
}


float matrix4fDeterminant(float mat[4][4])
{
    return((mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0]) *
        (mat[2][2] * mat[3][3] - mat[2][3] * mat[3][2]) + 
        (mat[0][2] * mat[1][0] - mat[0][0] * mat[1][2]) *
	(mat[2][1] * mat[3][3] - mat[2][3] * mat[3][1]) + 
        (mat[0][0] * mat[1][3] - mat[0][3] * mat[1][0]) *
	(mat[2][1] * mat[3][2] - mat[2][2] * mat[3][1]) + 
        (mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1]) *
	(mat[2][0] * mat[3][3] - mat[2][3] * mat[3][0]) + 
        (mat[0][3] * mat[1][1] - mat[0][1] * mat[1][3]) *
	(mat[2][0] * mat[3][2] - mat[2][2] * mat[3][0]) + 
        (mat[0][2] * mat[1][3] - mat[0][3] * mat[1][2]) *
	(mat[2][0] * mat[3][1] - mat[2][1] * mat[3][0]));
}


#define EPSILON .00001


float identMat[16] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0,
};


void matrix4fInvert(float mat[4][4], float inv[4][4])
{
    int		i, rswap;
    float	det,div,swap;
    float	hold[4][4];

    memcpy(hold, mat, sizeof(float) * 16);
    memcpy(inv, identMat, sizeof(float) * 16);
    det = matrix4fDeterminant(mat);
    if(fabs(det) < EPSILON)
	return;

    rswap = 0;
    /* this loop isn't entered unless [0][0] > EPSILON and det > EPSILON,
	 so rswap wouldn't be 0, but I initialize it to fool gcc */
    if(fabs(hold[0][0]) < EPSILON) {
        if(fabs(hold[0][1]) > EPSILON)
            rswap = 1;
        else if(fabs(hold[0][2]) > EPSILON)
	    rswap = 2;
        else if(fabs(hold[0][3]) > EPSILON)
	    rswap = 3;

        for(i = 0; i < 4; i++) {
            swap = hold[i][0];
            hold[i][0] = hold[i][rswap];
            hold[i][rswap] = swap;

            swap = inv[i][0];
            inv[i][0] = inv[i][rswap];
            inv[i][rswap] = swap;
        }
    }
        
    div = hold[0][0];
    for(i = 0; i < 4; i++) {
        hold[i][0] /= div;
        inv[i][0] /= div;
    }

    div = hold[0][1];
    for(i = 0; i < 4; i++) {
        hold[i][1] -= div * hold[i][0];
        inv[i][1] -= div * inv[i][0];
    }
    div = hold[0][2];
    for(i = 0; i < 4; i++) {
        hold[i][2] -= div * hold[i][0];
        inv[i][2] -= div * inv[i][0];
    }
    div = hold[0][3];
    for(i = 0; i < 4; i++) {
        hold[i][3] -= div * hold[i][0];
        inv[i][3] -= div * inv[i][0];
    }

    if(fabs(hold[1][1]) < EPSILON){
        if(fabs(hold[1][2]) > EPSILON)
	    rswap = 2;
        else if(fabs(hold[1][3]) > EPSILON)
	    rswap = 3;

        for(i = 0; i < 4; i++) {
            swap = hold[i][1];
            hold[i][1] = hold[i][rswap];
            hold[i][rswap] = swap;

            swap = inv[i][1];
            inv[i][1] = inv[i][rswap];
            inv[i][rswap] = swap;
        }
    }

    div = hold[1][1];
    for(i = 0; i < 4; i++) {
        hold[i][1] /= div;
        inv[i][1] /= div;
    }

    div = hold[1][0];
    for(i = 0; i < 4; i++) {
        hold[i][0] -= div * hold[i][1];
        inv[i][0] -= div * inv[i][1];
    }
    div = hold[1][2];
    for(i = 0; i < 4; i++) {
        hold[i][2] -= div * hold[i][1];
        inv[i][2] -= div * inv[i][1];
    }
    div = hold[1][3];
    for(i = 0; i < 4; i++) {
        hold[i][3] -= div * hold[i][1];
        inv[i][3] -= div * inv[i][1];
    }

    if(fabs(hold[2][2]) < EPSILON){
        for(i = 0; i < 4; i++) {
            swap = hold[i][2];
            hold[i][2] = hold[i][3];
            hold[i][3] = swap;

            swap = inv[i][2];
            inv[i][2] = inv[i][3];
            inv[i][3] = swap;
        }
    }

    div = hold[2][2];
    for(i = 0; i < 4; i++) {
        hold[i][2] /= div;
        inv[i][2] /= div;
    }

    div = hold[2][0];
    for(i = 0; i < 4; i++) {
        hold[i][0] -= div * hold[i][2];
        inv[i][0] -= div * inv[i][2];
    }
    div = hold[2][1];
    for(i = 0; i < 4; i++) {
        hold[i][1] -= div * hold[i][2];
        inv[i][1] -= div * inv[i][2];
    }
    div = hold[2][3];
    for(i = 0; i < 4; i++) {
        hold[i][3] -= div * hold[i][2];
        inv[i][3] -= div * inv[i][2];
    }

    div = hold[3][3];
    for(i = 0; i < 4; i++) {
        hold[i][3] /= div;
        inv[i][3] /= div;
    }

    div = hold[3][0];
    for(i = 0; i < 4; i++) {
        hold[i][0] -= div * hold[i][3];
        inv[i][0] -= div * inv[i][3];
    }
    div = hold[3][1];
    for(i = 0; i < 4; i++) {
        hold[i][1] -= div * hold[i][3];
        inv[i][1] -= div * inv[i][3];
    }
    div = hold[3][2];
    for(i = 0; i < 4; i++) {
        hold[i][2] -= div * hold[i][3];
        inv[i][2] -= div * inv[i][3];
    }
}

void inverseTranspose(float m[4][4], float it[4][4])
{
    matrix4fInvert(m, it);
    matrix4fTranspose(it);
}

void multVec3fByMatrix4f(float m[4][4], float in[3], float out[3])
{
    int i;

    for(i = 0; i < 3; i++)
	out[i] =
	    m[0][i] * in[0] + 
	    m[1][i] * in[1] + 
	    m[2][i] * in[2] + 
	    m[3][i]; /* 4th element is implicit 1.0f */
}

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

float dotVec3f(float v1[3], float v2[3])
{
    return(v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]);
}

/*
 * Calculate diffuse lighting here, and calculate *_LINEAR texgen here
 * Because specular lighting and SPHEREMAP depend on the viewpoint, have
 * to calculate those at refelction time.  (Urg!)
 *
 * Basically have to do all the work OpenGL normally does to figure out
 * how to shade an object, minus the perspective projection.
 *
 * In fact, hard to do SPHEREMAP texgen at all because it assumes
 * restrictions like viewer at (0, 0, inf), and suddenly that changes.
 *
 */
void transformSceneToEye(void) 
{
    float modelView[4][4];
    float modelViewIT[4][4];
    int i;

    /* luckily our light source is directional light at 0, 0, 1. */

    /* might as well use OpenGL for the transformation operations */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    applyXform(&globalXform);

    glPushMatrix();

    applyXform(&surf1Xform);
    glGetFloatv(GL_MODELVIEW_MATRIX, (float *)modelView);

    inverseTranspose(modelView, modelViewIT);

    /*
     * transform all vertices of surface 1, light them, and calc eye-dependent
     * texture coordinates (not spheremapped texgen...)
     */
    for(i = 0; i < surf1.vertCount; i++) {
	surf1.everts[i] = surf1.verts[i];
	multVec3fByMatrix4f(modelView, surf1.verts[i].v, surf1.everts[i].v);
	multVec3fByMatrix4f(modelViewIT, surf1.verts[i].n, surf1.everts[i].n);
	normalizeVec3f(surf1.everts[i].n, surf1.everts[i].n);
	/* Ignore the lighting for this sample, should calc lighting color */
	/* Ignore texgen, etc for this sample, should calc LINEAR here */
    }

    glPopMatrix();

    glPushMatrix();

    applyXform(&surf2Xform);
    glGetFloatv(GL_MODELVIEW_MATRIX, (float *)modelView);

    inverseTranspose(modelView, modelViewIT);

    /*
     * transform all vertices of surface 2 & light them & calc eye-dependent
     * texture coordinates (not spheremapped texgen...)
     */
    for(i = 0; i < surf2.vertCount; i++) {
	surf2.everts[i] = surf2.verts[i];
	multVec3fByMatrix4f(modelView, surf2.verts[i].v, surf2.everts[i].v);
	multVec3fByMatrix4f(modelViewIT, surf2.verts[i].n, surf2.everts[i].n);
	normalizeVec3f(surf2.everts[i].n, surf2.everts[i].n);
	/* Ignore the lighting for this sample, should calc lighting color */
	/* Ignore texgen, etc for this sample, should calc LINEAR here */
    }

    glPopMatrix();

    glPopMatrix();
}

unsigned char nearMap[256][256][3];
unsigned char farMap[256][256][3];
unsigned char hiddenMap[256][256][3];

#define MAXX	0
#define MINX	1
#define MAXY	2
#define MINY	3
#define MAXZ	4
#define MINZ	5

void initBox(float b[6])
{
    b[MAXX] = b[MAXY] = b[MAXZ] = -FLT_MAX;
    b[MINX] = b[MINY] = b[MINZ] = FLT_MAX;
}

void extendBoxByPoint(float b[6], float x, float y, float z)
{
    if(x > b[MAXX]) b[MAXX] = x;
    if(x < b[MINX]) b[MINX] = x;
    if(y > b[MAXY]) b[MAXY] = y;
    if(y < b[MINY]) b[MINY] = y;
    if(z > b[MAXZ]) b[MAXZ] = z;
    if(z < b[MINZ]) b[MINZ] = z;
}

void makeBoundSphere(surface *surf, float sphere[4])
{
    float box[6];
    int i;

    initBox(box);
    for(i = 0; i < surf->vertCount; i++) {
	extendBoxByPoint(box, surf->everts[i].v[0], surf->everts[i].v[1], 
	    surf->everts[i].v[2]);
    }
    sphere[0] = (box[0] + box[1]) / 2.0f;
    sphere[1] = (box[2] + box[3]) / 2.0f;
    sphere[2] = (box[4] + box[5]) / 2.0f;
    sphere[3] = 0.5f * sqrt((box[0] - box[1]) * (box[0] - box[1]) + 
        (box[2] - box[3]) * (box[2] - box[3]) + 
        (box[4] - box[5]) * (box[4] - box[5]));
}
 
void mapCoords(float d[3], float *s, float *t)
{
    float denom;
    denom = 2 * sqrt(d[0] * d[0] + d[1] * d[1] + (d[2] + 1) * (d[2] + 1));
    *s = .5 + d[0] / denom;
    *t = .5 + d[1] / denom;
}

int isectRaySphere(float origin[3], float direction[3], float center[3],
    float radius, float *distance, float point[3])
{
    float b, c, t1, t2;
    float squareRoot;
    float underSqrt;

    /*
     * We assume the "a" value in the quadratic equation is 1.0, because it
     * will be if the ray is normalized.
     */

    b = 2 * (direction[X] * (origin[X] - center[X]) +
             direction[Y] * (origin[Y] - center[Y]) +
	     direction[Z] * (origin[Z] - center[Z]));

    /*
     * You could encapsulate sphere information and calculate this
     * value only when the center or radius changed.  This function
     * doesn't take on that responsibility.
     */
    c = origin[X] * origin[X] + 
        origin[Y] * origin[Y] + 
        origin[Z] * origin[Z]
        
        +

        center[X] * center[X] + 
        center[Y] * center[Y] + 
        center[Z] * center[Z]

        - 

	radius * radius 

        -

	2 * (origin[X] * center[X] + 
             origin[Y] * center[Y] +
             origin[Z] * center[Z]);

    underSqrt = b * b - 4 * c;

    /* This means it misses the sphere */
    if(underSqrt < 0)
        return(0);

    squareRoot = sqrt(underSqrt);

    t1 = (- b - squareRoot) / 2;

    t2 = (- b + squareRoot) / 2;

    /* "t1" is the closer intersection and "t2" is the farther intersection. */

    /*
     * Return the intersection closest to ray origin that is strictly on
     * the ray, i.e. t > 0.
     */

    if(t2 < 0)
        return(0);

    if(t1 < 0)
        *distance = t2;
    else
        *distance = t1;

    point[X] = origin[X] + direction[X] * *distance;
    point[Y] = origin[Y] + direction[Y] * *distance;
    point[Z] = origin[Z] + direction[Z] * *distance;

    return(1);
}

void mapReflection(float e[3], float p[3], float n[3], float sphere[4], float *s, float *t)
{
    int isected;
    float v[3];
    float r[3];
    float u[3];
    float pt[3];
    float dot;
    float dist;

    v[0] = p[0] - e[0];
    v[1] = p[1] - e[1];
    v[2] = p[2] - e[2];

    normalizeVec3f(v, u);

    dot = dotVec3f(u, n);

    r[0] = u[0] - 2.0f * n[0] * dot;
    r[1] = u[1] - 2.0f * n[1] * dot;
    r[2] = u[2] - 2.0f * n[2] * dot;
    normalizeVec3f(r, r);

    isected = isectRaySphere(p, r, sphere, sphere[3], &dist, pt);
    if(!isected) {
	fprintf(stderr, "Internal error: ray projected from sphere center\n");
	fprintf(stderr, "  did not intersect sphere...\n");
	fprintf(stderr, "point: %f %f %f\n", p[0], p[1], p[2]);
	fprintf(stderr, "r: %f %f %f\n", r[0], r[1], r[2]);
	fprintf(stderr, "sphere: %f %f %f %f\n", sphere[0], sphere[1],
	   sphere[2], sphere[3]);
    }
    r[0] = pt[0] - sphere[0];
    r[1] = pt[1] - sphere[1];
    r[2] = pt[2] - sphere[2];
    normalizeVec3f(r, r);

    mapCoords(r, s, t);
}

int edgeMap[512][512][2];

/* better not be more than 4096 reflector triangles... */
void makeColorFromId(int id, unsigned char color[3])
{
    color[0] = (id % 16) << 4;
    color[1] = ((id % 256) / 16) << 4;
    color[2] = (id / 256) << 4;
}

int makeIdFromColor(unsigned char color[3])
{
    return
        ((color[0] & 0xf0) >> 4) + 
        (color[1] & 0xf0) + 
        ((color[2] & 0xf0) << 4);
}

void colorEdge(int a, int b, int color)
{
    if(a > b) {
	edgeMap[a][b][0]++;
	edgeMap[a][b][1] = color;
    } else {
	edgeMap[b][a][0]++;
	edgeMap[b][a][1] = color;
    }
}

void getExplosionMap(unsigned char map[256][256][3], surface *surf,
    mapvert *mv, float sphere[4])
{
    int i, j;
    float eye[3];
    unsigned char color[3];

    eye[0] = 0;
    eye[1] = 0;
    eye[2] = 3;

    glViewport(0, 0, 256, 256);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 256.0, 0.0, 256.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    memset(edgeMap, 0, sizeof(edgeMap));

    for(i = 0; i < surf->vertCount; i++) {
	float p[3];
	vertex *v = &surf->everts[i];
	mapvert *m = &mv[i];
	p[0] = eye[0] - v->v[0];
	p[1] = eye[1] - v->v[1];
	p[2] = eye[2] - v->v[2];
	if(dotVec3f(p, v->n) > 0) {
	    /* front facing */
	    mapReflection(eye, v->v, v->n, sphere, &m->m[0], &m->m[1]);
	    m->f = 1;
	    /* st from this calculation is from 0 to 1. */
	} else {
	    float cross1[3];
	    float ortho[3];
	    normalizeVec3f(p, p);
	    crossVec3f(v->n, p, cross1);
	    /* be careful that ortho is not parallel to x-y, or the */
	    /* explosion map coordinate is undefined. */
	    crossVec3f(p, cross1, ortho);
	    mapReflection(eye, v->v, ortho, sphere, &m->m[0], &m->m[1]);
	    /* printf("backfacing mapped to %f %f\n", m->m[0], m->m[1]); */
	    m->f = 0;
	}
    }
    for(i = 0; i < surf->triCount; i++) {
	mapvert *a, *b, *c;
	int ia, ib, ic;

	ia = surf->indices[i * 3 + 0];
	ib = surf->indices[i * 3 + 1];
	ic = surf->indices[i * 3 + 2];
	a = &mv[ia];
	b = &mv[ib];
	c = &mv[ic];

	makeColorFromId(i, color);
	glColor3ubv(color);


	if(a->f + b->f + c->f > 0) {
	    /* if at least one vertex is front-facing */
	    glBegin(GL_TRIANGLES);
	    glVertex2f(1.0f + 254.0f * a->m[0], 1.0f + 254.0f * a->m[1]);
	    glVertex2f(1.0f + 254.0f * b->m[0], 1.0f + 254.0f * b->m[1]);
	    glVertex2f(1.0f + 254.0f * c->m[0], 1.0f + 254.0f * c->m[1]);
	    glEnd();
	    /* mark that these edges were drawn. */
	    colorEdge(ia, ib, i);
	    colorEdge(ib, ic, i);
	    colorEdge(ic, ia, i);
	}
    }
    for(i = 0; i < surf->vertCount; i++) {
	for(j = 0; j <= i; j++) { /* yes, you want j <= i; see colorEdge. */
	    if(edgeMap[i][j][0] == 1) {
		float e0[2], e1[2]; /* extensions */
		float *m0, *m1;
		float x, y, d;

		makeColorFromId(edgeMap[i][j][1], color);
		glColor3ubv(color);

		m0 = mv[i].m;
		m1 = mv[j].m;

		x = 2.0f * m0[0] - 1.0f;
		y = 2.0f * m0[1] - 1.0f;
		d = sqrt(x * x + y * y);
		e0[0] = .5 + .6 * x / d;
		e0[1] = .5 + .6 * y / d;

		x = 2.0f * m1[0] - 1.0f;
		y = 2.0f * m1[1] - 1.0f;
		d = sqrt(x * x + y * y);
		e1[0] = .5 + .6 * x / d;
		e1[1] = .5 + .6 * y / d;

		glBegin(GL_TRIANGLES);
		glVertex2f(1.0f + 254.0f * m0[0], 1.0f + 254.0f * m0[1]);
		glVertex2f(1.0f + 254.0f * e0[0], 1.0f + 254.0f * e0[1]);
		glVertex2f(1.0f + 254.0f * m1[0], 1.0f + 254.0f * m1[1]);

		glVertex2f(1.0f + 254.0f * m1[0], 1.0f + 254.0f * m1[1]);
		glVertex2f(1.0f + 254.0f * e0[0], 1.0f + 254.0f * e0[1]);
		glVertex2f(1.0f + 254.0f * e1[0], 1.0f + 254.0f * e1[1]);
		glEnd();
	    } else if(edgeMap[i][j][0] > 2) {
		printf("bad internal state, edge marked %d times\n", edgeMap[i][j][0]);
	    }
	}
    }

    glReadPixels(0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, map);
    glEnable(GL_DITHER);
}

float
getTriangleArea(float a[2], float b[2], float c[2])
{
    float e1[2];
    float e2[2];

    e1[0] = a[0] - b[0];
    e1[1] = a[1] - b[1];

    e2[0] = c[0] - b[0];
    e2[1] = c[1] - b[1];

    return (e1[0]*e2[1] - e1[1]*e2[0])/2;
}


void
calcBarycentric(float tri0[2], float tri1[2], float tri2[2], float pt[2], float *a, float *b, float *c)
{
    float area;

    area = getTriangleArea(tri0, tri1, tri2);

    *a = getTriangleArea(tri1, tri2, pt)/area;
    *b = getTriangleArea(tri2, tri0, pt)/area;
    *c = getTriangleArea(tri0, tri1, pt)/area;
}

void reflectAboutPlane(float point[3], float plane[3], float normal[3],
    float reflected[3])
{
    float planeToPoint[3];
    float length;

    planeToPoint[0] = point[0] - plane[0];
    planeToPoint[1] = point[1] - plane[1];
    planeToPoint[2] = point[2] - plane[2];
    length = dotVec3f(planeToPoint, normal);
    reflected[0] = point[0] - 2 * normal[0] * length;
    reflected[1] = point[1] - 2 * normal[1] * length;
    reflected[2] = point[2] - 2 * normal[2] * length;
}

void mapToVirtual(float point[3], unsigned char map[256][256][3],
    surface *reflector, mapvert *mv, float sphere[4], float reflected[3])
{
    float centToPoint[3];
    float st[2];
    float ptOnPlane[3];
    float normal[3];
    int id;
    float a, b, c;
    int index0, index1, index2;

    centToPoint[0] = point[0] - sphere[0];
    centToPoint[1] = point[1] - sphere[1];
    centToPoint[2] = point[2] - sphere[2];
    normalizeVec3f(centToPoint, centToPoint);
    mapCoords(centToPoint, &st[0], &st[1]);
    /* Lawrence reminded me that image is stored row-major, or y, x: */
    id = makeIdFromColor(
        map[1 + (int)(254 * st[1])][1 + (int)(254 * st[0])]);

    /* (if no id, punt; reflected = point, return); have to fix this */
    if(id == 0xfff) {
	memcpy(reflected, point, sizeof(float) * 3);
	return;
    }

    index0 = reflector->indices[id * 3 + 0];
    index1 = reflector->indices[id * 3 + 1];
    index2 = reflector->indices[id * 3 + 2];

    /* calculate barycentric a, b, c from s, t and mapped coordinates */
    calcBarycentric(mv[index0].m, mv[index1].m, mv[index2].m, st, &a, &b, &c);

    ptOnPlane[0] = a * reflector->everts[index0].v[0] +
	b * reflector->everts[index1].v[0] +
	c * reflector->everts[index2].v[0];
    ptOnPlane[1] = a * reflector->everts[index0].v[1] +
	b * reflector->everts[index1].v[1] +
	c * reflector->everts[index2].v[1];
    ptOnPlane[2] = a * reflector->everts[index0].v[2] +
	b * reflector->everts[index1].v[2] +
	c * reflector->everts[index2].v[2];
    normal[0] = a * reflector->everts[index0].n[0] +
	b * reflector->everts[index1].n[0] +
	c * reflector->everts[index2].n[0];
    normal[1] = a * reflector->everts[index0].n[1] +
	b * reflector->everts[index1].n[1] +
	c * reflector->everts[index2].n[1];
    normal[2] = a * reflector->everts[index0].n[2] +
	b * reflector->everts[index1].n[2] +
	c * reflector->everts[index2].n[2];
    normalizeVec3f(normal, normal);
    reflectAboutPlane(point, ptOnPlane, normal, reflected);
}

/*
 * luckily for us, nearSphere and farSphere have the same center, so it's
 * easy to calculate the linear weights from near to far sphere
 */
void reflectObject(unsigned char nearMap[256][256][3], float nearSphere[4],
    unsigned char farMap[256][256][3], float farSphere[4],
    surface *surf, surface *reflector)
{
    int i;
    for(i = 0; i < surf->vertCount; i++) {
	vertex *v = &surf->everts[i];
	vertex *r = &surf->rverts[i];
	/* if hidden, map coord using hidden map */
	float dist;
	float interp;
	float p[3];
	float reflNear[3], reflFar[3];

	/*
	 * XXX may have to interpolate reflected normal vector, too,
	 * but for any eye-dependent calculation, I recommend reflecting
	 * the eye vector instead and using that in world-space lighting
	 * or sphere-mapping calculations; then you can leave normal, light
	 * position, etc where they were.
	 */
	mapToVirtual(v->v, nearMap, reflector, reflector->mapvertsNear,
	    nearSphere, reflNear);
	mapToVirtual(v->v, farMap, reflector, reflector->mapvertsFar,
	    farSphere, reflFar);

	p[0] = v->v[0] - nearSphere[0];
	p[1] = v->v[1] - nearSphere[1];
	p[2] = v->v[2] - nearSphere[2];
	dist = sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
	interp = (dist - nearSphere[3]) / (farSphere[3] - nearSphere[3]);
	r->v[0] = (1 - interp) * reflNear[0] + interp * reflFar[0];
	r->v[1] = (1 - interp) * reflNear[1] + interp * reflFar[1];
	r->v[2] = (1 - interp) * reflNear[2] + interp * reflFar[2];
	r->n[0] = v->n[0];
	r->n[1] = v->n[1];
	r->n[2] = v->n[2];
	memcpy(r->c, v->c, sizeof(float) * 3);
    }
}

/* normalized i, n */
/* doesn't normalize r */
/* r = u - 2 * n * dot(n, u) */
void reflectVec3f(float i[3], float n[3], float r[3])
{
    float dot;

    dot = dotVec3f(i, n);

    r[0] = i[0] - 2.0f * n[0] * dot;
    r[1] = i[1] - 2.0f * n[1] * dot;
    r[2] = i[2] - 2.0f * n[2] * dot;
}

unsigned char colors[][3] = {
    {255, 0, 0},
    {255, 128, 0},
    {128, 255, 128},
    {255, 0, 128},
    {0, 255, 0},
    {128, 128, 255},
    {0, 128, 255},
    {0, 0, 255},
    {128, 255, 0},
    {255, 128, 128},
    {0, 255, 128}
};

void redraw(void)
{
    float farSphere[4];
    float nearSphere[4];

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-.33, .33, -.33, .33, .5, 40);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 3, 0, 0, 0, 0, 1, 0);

    transformSceneToEye();

    makeBoundSphere(&surf1, nearSphere);

    farSphere[0] = nearSphere[0];
    farSphere[1] = nearSphere[1];
    farSphere[2] = nearSphere[2];
    farSphere[3] = 20; /* should be plenty */

    getExplosionMap(nearMap, &surf1, surf1.mapvertsNear, nearSphere);
    getExplosionMap(farMap, &surf1, surf1.mapvertsFar, farSphere);

    /* There should be a hidden map step here */

    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-.33, .33, -.33, .33, .5, 40);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 3, 0, 0, 0, 0, 1, 0);

    glViewport(0, 0, winWidth, winHeight);

    glPushMatrix();

    reflectObject(nearMap, nearSphere, farMap, farSphere, &surf2, &surf1);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, surf2.specular);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    drawSurf(surf2.everts, surf2.indices, surf2.triCount, 1, 1, 0);
    glDisable(GL_COLOR_MATERIAL);

    /* Mark the stencil buffer where our reflector passed depth test. */
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glDisable(GL_LIGHTING);
    glColorMask(0, 0, 0, 0);
    glDepthMask(0);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    drawSurf(surf1.everts, surf1.indices, surf1.triCount, 0, 0, 0);

    /* Clear the depth and color buffer where stencil is marked */
    glColorMask(1, 1, 1, 1);
    glDepthMask(1);
    glStencilFunc(GL_EQUAL, 1, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDepthFunc(GL_ALWAYS);
    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.8f, 0.8f); /* Same as ClearColor */
    glVertex3f(-14.0f, -14.0f, -36.5);
    glVertex3f(14.0f, -14.0f, -36.5);
    glVertex3f(14.0f, 14.0f, -36.5);
    glVertex3f(-14.0f, 14.0f, -36.5);
    glEnd();

    /* Draw reflected scene where stencil is marked */
    glDisable(GL_LIGHTING);
    glDepthFunc(GL_LEQUAL);
    /*
     * No lighting + no sending normals because lighting was
     * already calculated.
     */
    drawSurf(surf2.rverts, surf2.indices, surf2.triCount, 0, 1, 0);

    /* Draw the reflector back in with color modulation */
    glEnable(GL_LIGHTING);
    /*
     * This is probably bad; need original depth buffer information to 
     * do hidden surfaces right.  Fake it with CULLFACE
     */
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, surf1.ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, surf1.diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, surf1.specular);
    drawSurf(surf1.everts, surf1.indices, surf1.triCount, 1, 0, 0);

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

#if 0
    draw reflector modulated in with no stencil
        modulate base color first
        drawSurf(refl1Verts, surf1.indices, surf1.triCount, 0, 0, 0);
	add in diffuse lighting
        drawSurf(refl1Verts, surf1.indices, surf1.triCount, 1, 0, 1);
	add in specular lighting
        drawSurf(refl1Verts, surf1.indices, surf1.triCount, 1, 0, 0);
	blend based on fresnel component?
    draw normal checkerboard
#endif

    glPopMatrix();

#if 0
    pushOrthoView(0, winWidth, 0, winHeight, -1, 1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    glRasterPos3f(0, 0, 0);
    glPixelZoom(.5, .5);
    glDrawPixels(256, 256, GL_RGB, GL_UNSIGNED_BYTE, nearMap);
    glRasterPos3f(128, 0, 0);
    glDrawPixels(256, 256, GL_RGB, GL_UNSIGNED_BYTE, farMap);
    glPixelZoom(1.0, 1.0);
    popView();
#endif

    glutSwapBuffers();
}


void init(void)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_LIGHT0);

    glEnable(GL_NORMALIZE);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    curXform = &globalXform;
}


enum trackballModeEnum {
    ROTATE,
    TRANSLATEXY,
    TRANSLATEZ,
    SCALEX,
    SCALEY,
    SCALEZ
} trackballMode = ROTATE;


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
	    curXform = &surf1Xform;
	    break;

	case '2':
	    curXform = &surf2Xform;
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
    }
}


static int ox, oy;


/*ARGSUSED*/
void button(int b, int state, int x, int y)
{
    ox = x;
    oy = y;
}


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


void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    winWidth = width;
    winHeight = height;
    glutPostRedisplay();
}


int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(512,512);
//    glutInitDisplayString("samples rgb double depth stencil");
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
    glutCreateWindow("interactive curved reflections");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(motion);
    glutMouseFunc(button);
    glutReshapeFunc(reshape);

    makeCurved(8);
    makeChecker(4, 2);

    init();

	/* Display input information */
	printf("Select what you want to transform:\n");
	printf("\t0: object and reflector\n");
	printf("\t1: reflector only\n");
	printf("\t2: object only\n\n");
	printf("Then select your transformation:\n");
	printf("\tr - trackball-style rotatation\n");
	printf("\tt - translate in xy plane\n");
	printf("\tT - translate along z\n");
	printf("\tx - scale along x\n");
	printf("\ty - scale along y\n");
	printf("\tz - scale along z\n\n");
	printf("Then use the mouse to transform.\n\n");

    glutMainLoop();
    return(0);
}
