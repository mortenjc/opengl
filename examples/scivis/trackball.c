#include <math.h>

enum xformMode {
    XFORM_MODE_TRACKBALL,	/* rotate in direction of mouse motion */
    XFORM_MODE_SCROLL,		/* translate in X-Y */
    XFORM_MODE_DOLLY		/* translate in Z */
};

typedef struct { 
    enum xformMode mode;
    float matrix[4][4];
    float referenceSize;	/* used to calculate translations */
    float motionScale;		/* for dynamic scaling, etc */
    float rotation[4];		/* radians, x, y, z, like OpenGL */
    float translation[3];
    float center[3];		/* ignore by setting to <0,0,0> */
} Transform;

void xformCalcMatrix(Transform *xform);
void xformMotion(Transform *xform, float dx, float dy);
void xformInitializeFromBox(Transform *xform, float box[6], float fov);

/* Win32 math.h doesn't define M_PI. */
#ifdef WIN32
#define M_PI 3.14159265
#endif

/* internal use */

void makeIdentMatrix(float matrix[4][4])
{
    int i;
    for(i = 0; i < 16; i++)
	((float*)matrix)[i] = 0.0f;
    matrix[0][0] = 1.0f;
    matrix[1][1] = 1.0f;
    matrix[2][2] = 1.0f;
    matrix[3][3] = 1.0f;
}

void makeTransMatrix(float x, float y, float z, float matrix[4][4])
{
    makeIdentMatrix(matrix);
    matrix[3][0] = x;
    matrix[3][1] = y;
    matrix[3][2] = z;
}

void makeRotMatrix(float a, float x, float y, float z, float matrix[4][4])
{
    float c, s, t;

    c = (float)cos(a);
    s = (float)sin(a);
    t = 1.0f - c;

    matrix[0][0] = t * x * x + c;
    matrix[0][1] = t * x * y + s * z;
    matrix[0][2] = t * x * z - s * y;
    matrix[0][3] = 0;

    matrix[1][0] = t * x * y - s * z;
    matrix[1][1] = t * y * y + c;
    matrix[1][2] = t * y * z + s * x;
    matrix[1][3] = 0;

    matrix[2][0] = t * x * z + s * y;
    matrix[2][1] = t * y * z - s * x;
    matrix[2][2] = t * z * z + c;
    matrix[2][3] = 0;

    matrix[3][0] = 0;
    matrix[3][1] = 0;
    matrix[3][2] = 0;
    matrix[3][3] = 1;
}

void dragToRotation(float dx, float dy, float *rotation)
{
    float dist;

    /* XXX grantham 990825 - this "dist" doesn't make me confident. */
    dist = sqrt(dx * 10000 * dx * 10000 + dy * 10000 * dy * 10000) / 10000;
    /* dist = sqrt(dx * dx + dy * dy); */
    rotation[0] = (float) M_PI * dist;
    rotation[1] = (float) dy / dist;
    rotation[2] = (float) dx / dist;
    rotation[3] = 0.0f;
}

void matrixToRotation(float matrix[4][4], float rotation[4])
{
    float cosine;
    float sine;
    float d;

    cosine = (matrix[0][0] + matrix[1][1] + matrix[2][2] - 1.0f) / 2.0f;

    /* grantham 20000418 - I know this fixes the mysterious */
    /* NAN matrices, but I can't remember what I expected the above number */
    /* to do, so I don't know why this happens */
    if(cosine > 1.0){
	/* fprintf(stderr, "XXX acos of greater than 1! (clamped)\n"); */
	cosine = 1.0;
    }
    if(cosine < -1.0){
	/* fprintf(stderr, "XXX acos of less than -1! (clamped)\n"); */
	cosine = -1.0;
    }

    rotation[0] = (float)acos(cosine);
    sine = (float)sin(rotation[0]);
    rotation[1] = (matrix[1][2] - matrix[2][1]);
    rotation[2] = (matrix[2][0] - matrix[0][2]);
    rotation[3] = (matrix[0][1] - matrix[1][0]);
    d = sqrt(rotation[1] * rotation[1] + rotation[2] * rotation[2] +
	rotation[3] * rotation[3]);
    rotation[1] /= d;
    rotation[2] /= d;
    rotation[3] /= d;
}

void multMatrix(float m1[4][4], float m2[4][4], float r[4][4])
{
    float t[4][4];
    int i, j;
    for(j = 0; j < 4; j++)
	for(i = 0; i < 4; i++)
           t[i][j] = m1[i][0] * m2[0][j] + m1[i][1] * m2[1][j] +
	       m1[i][2] * m2[2][j] + m1[i][3] * m2[3][j];
    memcpy(r, t, sizeof(t));
}

void multRotation(float rotation1[4], float rotation2[4], float result[4])
{
    float matrix1[4][4];
    float matrix2[4][4];
    float matrix3[4][4];
    float dist;

    makeRotMatrix(rotation1[0], rotation1[1], rotation1[2], rotation1[3],
        matrix1);
    makeRotMatrix(rotation2[0], rotation2[1], rotation2[2], rotation2[3],
        matrix2);
    multMatrix(matrix1, matrix2, matrix3);
    matrixToRotation(matrix3, result);

    dist = (float)sqrt(result[1] * result[1] + result[2] * result[2] +
        result[3] * result[3]);

    result[1] /= dist;
    result[2] /= dist;
    result[3] /= dist;
}

void calcViewMatrix(float viewRotation[4], float viewOffset[3],
    float sceneCenter[3], float viewMatrix[4][4])
{
    float tmp[4][4];

    /* These could be applied with OpenGL matrix functions */
    makeIdentMatrix(viewMatrix);
    makeTransMatrix(viewOffset[0], viewOffset[1], viewOffset[2], tmp);
    multMatrix(tmp, viewMatrix, viewMatrix);
    makeRotMatrix(viewRotation[0], viewRotation[1], viewRotation[2],
        viewRotation[3], tmp);
    multMatrix(tmp, viewMatrix, viewMatrix);
    makeTransMatrix(-sceneCenter[0], -sceneCenter[1], -sceneCenter[2], tmp);
    multMatrix(tmp, viewMatrix, viewMatrix);
}

/* external API */

void xformCalcMatrix(Transform *xform)
{
    calcViewMatrix(xform->rotation, xform->translation, xform->center,
        xform->matrix);
}

void xformMotion(Transform *xform, float dx, float dy)
{
    float mouseRotation[4];
    switch(xform->mode) {
	case XFORM_MODE_TRACKBALL:
	    if(dx != 0 || dy != 0) {
		dragToRotation(dx, dy, mouseRotation);
		multRotation(xform->rotation, mouseRotation, xform->rotation);
	    }
	    break;
	case XFORM_MODE_SCROLL:
	    xform->translation[0] += dx * xform->referenceSize *
	        xform->motionScale;
	    xform->translation[1] -= dy * xform->referenceSize *
	        xform->motionScale;
	    break;
	case XFORM_MODE_DOLLY:
	    /* Yes, I meant "+"; Z starts -, mouse down == closer to eye */
	    xform->translation[2] += dy * xform->referenceSize *
	        xform->motionScale;
	    break;
    }
    calcViewMatrix(xform->rotation, xform->translation, xform->center,
        xform->matrix);
}

/*
 * "box" contains min and max of X, then Y, then Z.
 * Could fill in referenceSize and center yourself...
 * "xform->referenceSize" is the linear "most appropriate" size for the
 * scene; most obvious is for it to be the radius of bounding sphere,
 *   but might be better for it to be smaller if the bounding 
 *   are short...  DIY.
 */

void xformInitializeFromBox(Transform *xform, float box[6], float fov)
{
    xform->center[0] = (box[0] + box[1]) / 2.0f;
    xform->center[1] = (box[2] + box[3]) / 2.0f;
    xform->center[2] = (box[4] + box[5]) / 2.0f;

    /* This code is from sduview.cc; how come it didn't account for box Z? */
    if(box[1] - box[0] > box[3] - box[2])
        xform->referenceSize = box[1] - box[0];
    else
        xform->referenceSize = box[3] - box[2];

    xform->motionScale = 1.0;

    xform->translation[0] = 0.0f;
    xform->translation[1] = 0.0f;
    xform->translation[2] = -xform->referenceSize / cos(fov / 2.0);

    xform->rotation[0] = 0.0;
    xform->rotation[1] = 1.0;
    xform->rotation[2] = 0.0;
    xform->rotation[3] = 0.0;

    calcViewMatrix(xform->rotation, xform->translation, xform->center,
        xform->matrix);
}
