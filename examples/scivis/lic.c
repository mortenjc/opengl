#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#ifdef _WIN32
#define sqrtf(x)    ((float)sqrt(x))
#endif

#define CHECK_ERROR(string)                                              \
{                                                                        \
    GLenum error_value;                                                  \
    while((error_value = glGetError()) != GL_NO_ERROR)                   \
	printf("Error Encountered: %s (%s)\n", string,                   \
	       gluErrorString(error_value));                             \
}

enum {X, Y, Z}; /* to make coordinates clearer */

GLfloat *dataArray = 0; /* array of vertex values in grid */
int dataWid = 0; /* width of grid */ 
int dataHt = 0; /* height of grid */
GLuint *indexArray = 0;
float scale = .03f;

static int winWidth, winHeight;

/*#define READ_FIELD*/
#ifdef READ_FIELD
#define SIZE	256
#else
#define SIZE	256
#endif

/* create a luminance noise texture with texture id */
void maketexture(int wid, int ht, GLint id)
{
    GLubyte *tex, *tptr;
    int i;
    tex = (GLubyte *)malloc(wid * ht * sizeof(GLubyte) * 1);
#ifdef GL_VERSION_1_1
    glBindTexture(GL_TEXTURE_2D, id);
#else
    glBindTextureEXT(GL_TEXTURE_2D, id);
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    for(tptr = tex, i = 0; i < wid * ht; i++)
#ifdef _WIN32
	*tptr++ = ((float)rand()/(float)RAND_MAX) * 256.f; /* random 0 - 255 */
#else
	*tptr++ = drand48() * 256.f; /* random 0 - 255 */
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, 1,
		 wid, ht, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tex);
    free(tex);
    CHECK_ERROR("maketexture");

}


/* use bilinear interpolation to find a vertex */
void
bilinear(float s, float t, GLfloat v0[3], GLfloat v3[3], GLfloat edge01[3],
	  GLfloat edge32[3], GLfloat *vertex)
{
    GLfloat minS[3], maxS[3], vecT[3]; /* bilinear interpolation */

    /* bottom edge */
    minS[X] = v0[X] + s * edge01[X];
    minS[Y] = v0[Y] + s * edge01[Y];
    minS[Z] = v0[Z] + s * edge01[Z];

    /* top edge */
    maxS[X] = v3[X] + s * edge32[X];
    maxS[Y] = v3[Y] + s * edge32[Y];
    maxS[Z] = v3[Z] + s * edge32[Z];

    /* vector T along s */
    vecT[X] = maxS[X] - minS[X];
    vecT[Y] = maxS[Y] - minS[Y];
    vecT[Z] = maxS[Z] - minS[Z];

    /* find point along vector T */
    vertex[X] = minS[X] + t * vecT[X];
    vertex[Y] = minS[Y] + t * vecT[Y];
    vertex[Z] = minS[Z] + t * vecT[Z];
}



/* tessellate a quad into n by m  pieces and generate opengl calls to draw it */
/* call this function during redraw */
/*
 * (j)
 * m    V3---(edge32)--->V2
 * .    /\              /\
 * .    |                |
 * .    |                |
 * 3 (edge03)         (edge12)
 * 2    |                |
 * 1    |                |
 * 0    V0---(edge01)--> V1
 *      0  1  2  3 . . . n  (i)
 */
/* n is number of squares in s, m is number of squares in t */
void tess(GLfloat v0[3], GLfloat v1[3], GLfloat v2[3], GLfloat v3[3], 
	  int sSteps, int tSteps)
{
    float s, t; /* parameter values */
    int i, j; /* subsquare indicies */
    GLuint index;
    GLfloat *dataElement; /* current location of data array to update */
    GLuint *indexElement; /* current location of index array to update */
    GLfloat edge01[3], /*edge03[3], edge12[3],*/ edge32[3]; /* edge vectors */

    /* assume s direction is from v0 to v1 and v3 to v2 */
    /* assume t direction is from v0 to v3 and from v1 to v2 */


    /* v0 -> v1 edge vector */
    edge01[X] = v1[X] - v0[X];
    edge01[Y] = v1[Y] - v0[Y];
    edge01[Z] = v1[Z] - v0[Z];

#if 0
    /* v0 -> v3 edge vector */
    edge03[X] = v3[X] - v0[X];
    edge03[Y] = v3[Y] - v0[Y];
    edge03[Z] = v3[Z] - v0[Z];

    /* v1 -> v2 edge vector */
    edge12[X] = v2[X] - v1[X];
    edge12[Y] = v2[Y] - v1[Y];
    edge12[Z] = v2[Z] - v1[Z];
#endif

    /* v3 -> v2 edge vector */
    edge32[X] = v2[X] - v3[X];
    edge32[Y] = v2[Y] - v3[Y];
    edge32[Z] = v2[Z] - v3[Z];


    /* calculate the number of verticies needed, initialize data array */

    if(sSteps != dataWid || tSteps != dataHt) {
	dataArray = (GLfloat *)realloc((void *)dataArray, 
				       sSteps * 
				       tSteps *
				       sizeof(GLfloat) * 5);
#ifdef GL_T2F_V3F
	glInterleavedArrays(GL_T2F_V3F, 0, dataArray);
#endif

	/* assuming GL_QUADS: TODO quad strips */
	indexArray = (GLuint *)realloc((void *)indexArray, 
				       (sSteps - 1)  * 
				       (tSteps - 1)  * 
				       sizeof(GLuint) * 4);

	dataWid = sSteps;
	dataHt  = tSteps;
    }
    
    /* define grid of data values */
    dataElement = dataArray;
    for(j = 0; j < tSteps; j++)
	for(i = 0; i < sSteps; i++) {
	    s = i/(sSteps - 1.f);
	    t = j/(tSteps - 1.f);
	    *dataElement++ = s;
	    *dataElement++ = t;
	    bilinear(s, t, v0, v3, edge01, edge32, dataElement);
	    dataElement += 3;
	}

    /* define array of index values to draw array as quads */
    indexElement = indexArray;
    for(j = 0; j < tSteps - 1; j++)
	for(i = 0; i < sSteps - 1; i++) {
	    index = j * sSteps + i;
	    *indexElement++ = index;

	    index++;
	    *indexElement++ = index;

	    index += sSteps;
	    *indexElement++ = index;

	    index--;
	    *indexElement++ = index;
       }
}

#if 0
/* some function to visualize */
void func(GLfloat param, int i, int j, GLfloat *s, GLfloat *t)
{
    *s = *s + param * .0001;
    *t = *t + param * .0001;
}
#endif

struct { float u, v; } *_f;

void func(GLfloat param, int i, int j, GLfloat *s, GLfloat *t)
{
#if 1
    *s += _f[j*SIZE+i].u*param;
    *t += _f[j*SIZE+i].v*param;
#else
    *s = (i-1.f)/dataWid + _f[j*SIZE+i].u*param;
    *t = (j-1.f)/dataHt + _f[j*SIZE+i].v*param;
#endif
}

/* TODO: texture array parameter instead of global */
void vecFunc(float param)
{
    int i, j;
    GLfloat *texdata;

    param *= scale;
    for(j = 0, texdata = dataArray; j < dataHt; j++)
	for(i = 0; i < dataWid; i++) {
            /* TODO: make func pointer */
	    func(param, i, j, texdata, (texdata + 1)); 
	    /* TODO: make texture coords separate array */
	    texdata += 5; /* skip to next S, T pair */ 
	}
}

/* Called when window needs to be redrawn */
void redraw(void)
{
    int i;
    int max = 30;
    int s, t;
    float sum = 0;
    glClear(GL_COLOR_BUFFER_BIT|GL_ACCUM_BUFFER_BIT); 

    for(t = 0; t < dataHt; t++) {
	for(s = 0; s < dataWid; s++) {
	    dataArray[5*dataWid*t+s*5] = (s-1.f)/dataWid;
	    dataArray[5*dataWid*t+s*5+1] = (t-1.f)/dataHt;
	}
    }
    
    vecFunc((GLfloat)-1);
    /* TODO: change to quad strips */
    for(i = -max; i <= max; i++) {
	vecFunc((GLfloat)1.f/(GLfloat)max);
#ifdef GL_T2F_V3F
	glDrawElements(GL_QUADS, 
		       (dataWid - 1) * (dataHt - 1) * 4,
		       GL_UNSIGNED_INT, indexArray);
#else
	for(t = 0; t < dataHt-1; t++) {
	    float *d = &dataArray[5*dataWid*t];
	    float *d2 = &dataArray[5*dataWid*(t+1)];
	    glBegin(GL_TRIANGLE_STRIP);
	    for(s = 0; s < dataWid; s++) {
		glTexCoord2fv(d);
		glVertex3fv(d+2);
		glTexCoord2fv(d2);
		glVertex3fv(d2+2);
		d += 5;
		d2 += 5;
	    }
	    glEnd();
	}

#endif
#if 0
	glAccum(GL_ACCUM, .5f/(GLfloat)max);
#else
	/*printf("%f\n", (max-fabs(i))/(GLfloat)max/(GLfloat)max);*/
	glAccum(GL_ACCUM, (max-fabs(i))/(GLfloat)max/(GLfloat)max);
	sum += (max-fabs(i))/(GLfloat)max/(GLfloat)max;
#endif
    }
    glAccum(GL_RETURN, 1.f);
    glFlush();
    CHECK_ERROR("redraw");
/*printf("sum = %f\n", sum);*/
}

void readfield(void) {
    FILE *fd;
    int i, j;
    _f = (void *)malloc(sizeof _f[0]*SIZE*SIZE);
#if 0
    if (!(fd = fopen("u.txt", "r"))) {
	perror("u.txt");
	exit(1);
    }
    for(j = 0; j < SIZE; j++) {
	for(i = 0; i < SIZE; i++) {
	    fscanf(fd, "%f\n", &_f[j*SIZE+i].u);
	}
    }
    fclose(fd);
    if (!(fd = fopen("v.txt", "r"))) {
	perror("u.txt");
	exit(1);
    }
    for(j = 0; j < SIZE; j++) {
	for(i = 0; i < SIZE; i++) {
	    float l;
	    fscanf(fd, "%f\n", &_f[j*SIZE+i].v);
	    l = sqrtf(_f[j*SIZE+i].u*_f[j*SIZE+i].u + _f[j*SIZE+i].v*_f[j*SIZE+i].v);
/*printf("%f %f\n", _f[j*SIZE+i].u, _f[j*SIZE+i].v);*/
	    if (l > 0) {
		_f[j*SIZE+i].u /= l;
		_f[j*SIZE+i].v /= l;
	    }
	}
    }
    fclose(fd);
#else
    if (!(fd = fopen("vector2.dat", "r"))) {
	perror("vector2.dat");
	exit(1);
    }
    for(j = 0; j < SIZE; j++) {
	for(i = 0; i < SIZE; i++) {
	    float l;
	    fread(&l, sizeof l, 1, fd);
	    _f[j*SIZE+i].u = l;
	    fread(&l, sizeof l, 1, fd);
	    _f[j*SIZE+i].v = l;
	    l = sqrtf(_f[j*SIZE+i].u*_f[j*SIZE+i].u + _f[j*SIZE+i].v*_f[j*SIZE+i].v);
/*printf("%f %f\n", _f[j*SIZE+i].u, _f[j*SIZE+i].v);*/
	    if (l > 0) {
		_f[j*SIZE+i].u /= l;
		_f[j*SIZE+i].v /= l;
	    }
	}
    }
    fclose(fd);
#endif
}

void
makefield(void) {
    float x, y;
    int i, j;

    _f = (void *)malloc(sizeof _f[0]*SIZE*SIZE);
    for(i = 0; i < SIZE; i++) {
	x = -1.f + 2.f*(i-1)/(SIZE-1.f);
	for(j = 0; j < SIZE; j++) {
	    y = -1.f + 2.f*(j-1)/(SIZE-1.f);
	    _f[j*SIZE+i].u = 10.f*y + 5.f*y / (x*x + y*y);
	}
    }
    /* compute forward difference */
    for(i = 0; i < SIZE-1; i++) {
	for(j = 0; j < SIZE-1; j++) {
	     float l;
	     _f[j*SIZE+i].v = _f[(j+1)*SIZE+i].u - _f[j*SIZE+i].u;
	     _f[j*SIZE+i].u = _f[j*SIZE+i+1].u - _f[j*SIZE+i].u;
	    l = sqrtf(_f[j*SIZE+i].u*_f[j*SIZE+i].u + _f[j*SIZE+i].v*_f[j*SIZE+i].v);
	    if (l > 0) {
		_f[j*SIZE+i].u /= l;
		_f[j*SIZE+i].v /= l;
	    }
	}
    }
}

void
help(void) {
    printf("S    - increase shift scale\n");
    printf("s    - decrease shift scale\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033': exit(EXIT_SUCCESS); break;
    case 'S': scale += 0.01f; printf("scale = %f\n", scale); break;
    case 's': scale -= 0.01f; printf("scale = %f\n", scale); break;
    default: help(); return;
    }
    glutPostRedisplay();
}

void
menu(int which) {
    key((char)which, 0, 0);
}

void
create_menu(void) {
    glutCreateMenu(menu);
    glutAddMenuEntry("increase shift scale", 'S');
    glutAddMenuEntry("decrease shift scale", 's');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void
reshape(int wid, int ht)
{
    float a;
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    a = ((float)winWidth)/winHeight;
    if (a > 1)
	glOrtho(-1.f*a, 1.f*a, -1.f, 1.f, -1.f, 1.f);
    else
	glOrtho(-1.f, 1.f, -1.f*a, 1.f*a, -1.f, 1.f);
    glMatrixMode(GL_MODELVIEW);
}

/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    GLfloat v0[3], v1[3], v2[3], v3[3];
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_RGBA|GLUT_ACCUM);
    (void)glutCreateWindow("line integral convolution");
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    //create_menu();

    /* 4 corners of tesselated quad */
    v0[X] = -1.f; v0[Y] = -1.f; v0[Z] = 0.f;
    v1[X] =  1.f; v1[Y] = -1.f; v1[Z] = 0.f;
    v2[X] =  1.f; v2[Y] =  1.f; v2[Z] = 0.f;
    v3[X] = -1.f; v3[Y] =  1.f; v3[Z] = 0.f;

#ifdef READ_FIELD
    printf("read u, v\n");
    readfield();
#else
    makefield();
#endif

    /* comment out to show surfaces */
/*    glPolygonMode(GL_FRONT, GL_LINE);*/
    glEnable(GL_TEXTURE_2D);
    maketexture(256, 256, 0);
    tess(v0, v1, v2, v3, SIZE, SIZE); /* tessellate */

    glutDisplayFunc(redraw);
    CHECK_ERROR("main");
    glutMainLoop();
    return 0;
}
