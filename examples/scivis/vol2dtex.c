/*
 * vol2dtex.c - volume rendering using a stack of 2D textures
 *            - this version keeps a single stack rather than 3 stacks
 *              to keep things simple, but will show severe artifacts
 *              when the slices are viewed near edge-on.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include <math.h>
#include "texture.h"

#include "trackball.c"

static void cleanup(void);

/* nonzero if not power of 2 */ 
#define NOTPOW2(num) ((num) & (num - 1))

int
makepow2(int val)
{
    int power = 0;
    if(!val)
	return 0;

    while(val >>= 1)
	power++;

    return(1 << power);
}

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

enum {X, Y, Z, W};
enum {R, G, B, A};
enum {OVER, ATTENUATE, NONE, LASTOP};
enum {OBJ_ANGLE, SLICES, CUTTING};

/* window dimensions */
int winWidth = 1024;
int winHeight = 1024;
int active;
int operator = OVER;
GLboolean texture = GL_TRUE;
GLboolean dblbuf = GL_TRUE;
GLboolean cut = GL_FALSE;
GLint cutbias = 50;
int ext_blend_color = 0;

GLfloat lightangle[2] = {0.f, 0.f};
GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

/* 3d texture data that's read in */
/* XXX TODO; make command line arguments */
/* To avoid differences in brightness caused by loss of accuracy during
/*   blending, really should have same number of slices in all three
/*   dimensions. */
GLubyte *tex3ddata;
int Texwid = 128; /* dimensions of each 2D texture */
int Texht = 128;
int Texdepth = 64; /* number of 2D textures */

/* 512, just in case we decide to change the data set */
int textures_X[512];
int textures_Y[512];
int textures_Z[512];
GLubyte textmp[1024 * 1024];
int texwid, texht, texdepth;
int slices_X;
int slices_Y;
int slices_Z;

Transform rootXform;


GLfloat *lighttex = 0;
GLfloat lightpos[4] = {0.f, 0.f, 1.f, 0.f};
GLboolean lightchanged[2] = {GL_TRUE, GL_TRUE};


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
    /* cube, 300 on a side */

#if 1
    /*
     * Doctors prefer the orthogonal view because it matches what they
     * are used to seeing, but perspective view is kind of fun...
     */
    if (a > 1)
	glOrtho(-150.*a, 150.*a, -150., 150., -150., 500.);
    else
	glOrtho(-150., 150., -150.*a, 150.*a, -150., 500.);
#else
    if (a > 1)
	glFrustum(-6.6 * a, 6.6 * a, -6.6, 6.6, 10, 500);
    else
	glFrustum(-6.6, 6.6, -6.6 * a, 6.6 * a, 10, 500);
#endif

    glMatrixMode(GL_MODELVIEW);
}

int ox, oy;


void
motion(int x, int y)
{
    int dx, dy;

    dx = x - ox;
    dy = y - oy;
    ox = x;
    oy = y;

    switch(active) {
    case OBJ_ANGLE:
        if(winWidth < winHeight)
            xformMotion(&rootXform, dx / (float) winWidth,
	        dy / (float)winWidth);
	else
            xformMotion(&rootXform, dx / (float) winHeight,
	        dy / (float)winHeight);
	glutPostRedisplay();
	break;
    case CUTTING:
	cutbias = (x - winWidth/2) * 300/winWidth;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
	switch(button) {
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = OBJ_ANGLE;
	    ox = x;
	    oy = y;
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = CUTTING;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    break;
	}
}

GLdouble clipplane0[] = {-1.,  0.,  0., 100.};
GLdouble clipplane1[] = { 1.,  0.,  0., 100.};
GLdouble clipplane2[] = { 0., -1.,  0., 100.};
GLdouble clipplane3[] = { 0.,  1.,  0., 100.};
GLdouble clipplane4[] = { 0.,  0., -1., 100.};
GLdouble clipplane5[] = { 0.,  0.,  1., 100.};

/* define a cutting plane */
GLdouble cutplane[] = {0.f, -.5f, -2.f, 50.f};

float origin[4] = {0, 0, 0, 1};
float Xaxis[4] = {1, 0, 0, 1};
float Yaxis[4] = {0, 1, 0, 1};
float Zaxis[4] = {0, 0, 1, 1};

void vec4fMult(float m[4][4], float v[4], float d[4])
{
    int i;
    for(i = 0; i < 4; i++)
        d[i] =
            v[0] * m[0][i] +
            v[1] * m[1][i] +
            v[2] * m[2][i] +
            v[3] * m[3][i];
}

float vec4fDot(float v1[4], float v2[4])
{
    return(v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] + v1[3] * v2[3]);
}

int findMajorGLAxis(int *sign)
{
    float matrix[4][4];
    float Oeye[4];
    float Xeye[4], Xsize;
    float Yeye[4], Ysize;
    float Zeye[4], Zsize;

    glGetFloatv(GL_PROJECTION_MATRIX, (float *)matrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, (float *)matrix);
    vec4fMult(matrix, origin, Oeye);
    vec4fMult(matrix, Xaxis, Xeye);
    vec4fMult(matrix, Yaxis, Yeye);
    vec4fMult(matrix, Zaxis, Zeye);
    Xeye[0] -= Oeye[0]; Xeye[1] -= Oeye[1]; Xeye[2] -= Oeye[2]; Xeye[3] = 0;
    Yeye[0] -= Oeye[0]; Yeye[1] -= Oeye[1]; Yeye[2] -= Oeye[2]; Yeye[3] = 0;
    Zeye[0] -= Oeye[0]; Zeye[1] -= Oeye[1]; Zeye[2] -= Oeye[2]; Zeye[3] = 0;
    Xsize = vec4fDot(Xeye, Zaxis);
    Ysize = vec4fDot(Yeye, Zaxis);
    Zsize = vec4fDot(Zeye, Zaxis);
    if(fabs(Xsize) > fabs(Zsize) && fabs(Xsize) > fabs(Ysize)) {
	*sign = (Xsize < 0) ? -1 : 1;
        return 0;
    }
    if(fabs(Ysize) > fabs(Zsize)) {
	*sign = (Ysize < 0) ? -1 : 1;
        return 1;
    }
    *sign = (Zsize < 0) ? -1 : 1;
        return 2;
}

/* draw the object unlit without surface texture */
void redraw(void)
{
    int axis, sign;
    int i;
    GLfloat offS, offT, offR; /* mapping texture to planes */

    /* want (texwid - 1) * offS == 1.0 */
    offS = 2.0f / (texwid - 1);
    offT = 2.0f / (texht - 1);
    offR = 2.0f / (texdepth - 1);
    
    clipplane0[W] = 100.f - offS;
    clipplane1[W] = 100.f - offS;
    clipplane2[W] = 100.f - offT;
    clipplane3[W] = 100.f - offT;
    clipplane4[W] = 100.f - offR;
    clipplane5[W] = 100.f - offR;

    glClear(GL_COLOR_BUFFER_BIT); /* |GL_DEPTH_BUFFER_BIT); */

    /* GL_MODELVIEW */
    if(cut) {
	cutplane[W] = cutbias;
	glClipPlane(GL_CLIP_PLANE5, cutplane);
    }

    glPushMatrix(); /* identity */

    glMultMatrixf((float *)rootXform.matrix);
    
    if(!cut)
	glClipPlane(GL_CLIP_PLANE5, clipplane5);

    switch(operator) {
    case OVER:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	break;
    case ATTENUATE:
#ifdef GL_CONSTANT_ALPHA_EXT
	if (ext_blend_color) {
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE);
	    glBlendColorEXT(1.f, 1.f, 1.f, 1.f/slices_Z);
	    break;
	}
#endif
	printf("EXT_blend_color not supported\n");
	break;
    case NONE:
	/* don't blend */
	break;
    }

    if(texture)
       glEnable(GL_TEXTURE_2D);
    else {
       glDisable(GL_TEXTURE_2D);
       glEnable(GL_LIGHTING);
       glEnable(GL_LIGHT0);
    }

    /*
     * Find the axis pointing most directly towards the viewer and its
     * sign; use the axis to choose which texture data set to use and use
     * the sign to determine whether to draw back-to-front (sign == 1) or
     * front-to-back (sign == -1) in order to draw back-to-front from the
     * viewer's perspective
     */
    axis = findMajorGLAxis(&sign);

    glScalef(100.0f, 100.0f, 100.0f);

    if(axis == 0) {
	for(i = 0; i < slices_X; i++) {
	    int slice = (sign == 1) ? (slices_X - 1 - i) : i;
	    glBindTexture(GL_TEXTURE_2D, textures_X[slice]);
	    glBegin(GL_QUADS);
	    glTexCoord2i(0, 0);
	    glVertex3f( 1.f - slice * offS, -1.f, 1.f);
	    glTexCoord2i(1, 0);
	    glVertex3f( 1.f - slice * offS, -1.f, -1.f);
	    glTexCoord2i(1, 1);
	    glVertex3f( 1.f - slice * offS, 1.f, -1.f);
	    glTexCoord2i(0, 1);
	    glVertex3f( 1.f - slice * offS, 1.f, 1.f);
	    glEnd();
	}
    } else if(axis == 1) {
	for(i = 0; i < slices_Y; i++) {
	    int slice = (sign == 1) ? (slices_Y - 1 - i) : i;
	    glBindTexture(GL_TEXTURE_2D, textures_Y[slice]);
	    glBegin(GL_QUADS);
	    glTexCoord2i(0, 0);
	    glVertex3f( -1.f, 1.f - slice * offT, 1.f);
	    glTexCoord2i(1, 0);
	    glVertex3f( 1.f, 1.f - slice * offT, 1.f);
	    glTexCoord2i(1, 1);
	    glVertex3f( 1.f, 1.f - slice * offT, -1.f);
	    glTexCoord2i(0, 1);
	    glVertex3f( -1.f, 1.f - slice * offT, -1.f);
	    glEnd();
	}
    } else {
	for(i = 0; i < slices_Z; i++) {
	    int slice = (sign == 1) ? (slices_Z - 1 - i) : i;
	    glBindTexture(GL_TEXTURE_2D, textures_Z[slice]);
	    glBegin(GL_QUADS);
	    glTexCoord2i(0, 0);
	    glVertex3f( -1.f, -1.f, 1.f - slice * offR);
	    glTexCoord2i(1, 0);
	    glVertex3f( 1.f, -1.f, 1.f - slice * offR);
	    glTexCoord2i(1, 1);
	    glVertex3f( 1.f, 1.f, 1.f - slice * offR);
	    glTexCoord2i(0, 1);
	    glVertex3f( -1.f, 1.f, 1.f - slice * offR);
	    glEnd();
	}
    }
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    if(!texture) {
       glDisable(GL_LIGHTING);
    }
    glDisable(GL_BLEND);

    if(operator == ATTENUATE) {
	glPixelTransferf(GL_RED_SCALE, 3.f); /* brighten image */
	glPixelTransferf(GL_GREEN_SCALE, 3.f);
	glPixelTransferf(GL_BLUE_SCALE, 3.f);
	glCopyPixels(0, 0, winWidth, winHeight, GL_COLOR);
    }
    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");
}

void
help(void) {
    printf("o    - toggle operator (over, attenuate, none\n");
    printf("l    - toggle texture\n");
    printf("c    - toggle cutting plane\n");
    printf("left mouse    - rotate object\n");
    printf("middle mouse  - move cutting plane\n");
    printf("right mouse   - change # slices\n");
}

/*ARGSUSED1*/
void key(unsigned char key, int x, int y)
{
    switch(key) {
    case 'o':
	operator++;
	if(operator == LASTOP)
	    operator = OVER;
	glutPostRedisplay();
	break;
    case 'l':
	if(texture)
	    texture = GL_FALSE;
	else
	    texture = GL_TRUE;
	glutPostRedisplay();
	break;
    case 'c':
	if(cut)
	    cut = GL_FALSE;
	else
	    cut = GL_TRUE;
	glutPostRedisplay();
	break;
    case '\033': case 'q':
	cleanup();
	exit(0);
	break;
    default: help(); break;
    }

}

/*
** Create a single component 3d texture map
** GL_LUMINANCE_ALPHA 3D Checkerboard
*/
GLfloat *make_texture3dcheck(int maxs, int maxt, int maxr)
{
    int s, t, r;
    GLfloat value;
    static GLfloat *texture;

    texture = (GLfloat *)malloc(2 * maxs * maxt * maxr * sizeof(GLfloat));
    for(r = 0; r < maxr; r++)
	for(t = 0; t < maxt; t++)
	    for(s = 0; s < maxs; s++) {
		value = (((s >> 2) & 0x3)) ^ 
		        (((t >> 2) & 0x3)) ^ 
		        (((r >> 2) & 0x1));
		texture[    (s + (t + r * maxt) * maxs) * 2] = value;
		texture[1 + (s + (t + r * maxt) * maxs) * 2] = value;
	    }    
    return texture;
}

#undef HALF_SIZE

GLubyte *
loadtex3d(int *texwid, int *texht, int *texdepth, int *texcomps)
{
    char *filename;
    GLubyte *tex3ddata;
    GLuint *texslice; /* 2D slice of 3D texture */
    GLint max3dtexdims; /* maximum allowed 3d texture dimension */
    GLint newval;
    int i;

    /* load 3D texture data */
    filename = (char*)malloc(sizeof(char) * strlen("../data/skull/skullXX.la"));

    tex3ddata = (GLubyte *)malloc(Texwid * Texht * Texdepth * 
				  4 * sizeof(GLubyte));
    for(i = 0; i < Texdepth; i++) {
	sprintf(filename, "../data/skull/skull%d.la", i);
	/* read_texture reads as RGBA */
	texslice = read_texture(filename, texwid, texht, texcomps);
#if defined(HALF_SIZE)
	gluScaleImage(GL_RGBA, Texwid, Texht, GL_UNSIGNED_BYTE, texslice,
                       Texwid/2, Texht/2, GL_UNSIGNED_BYTE, &tex3ddata[i * Texwid/2 * Texht/2 * 4]);
#else
	memcpy(&tex3ddata[i * Texwid * Texht * 4],  /* copy in a slice */
	       texslice, 
	       Texwid * Texht * 4 * sizeof(GLubyte));
#endif
	free(texslice);
    }
#if defined(HALF_SIZE)
    Texwid /= 2;
    Texht /= 2;
    *texwid /= 2;
    *texht /= 2;
#endif
    free(filename);

    *texdepth = Texdepth;
    max3dtexdims = 256;

    /* adjust width */
    newval = *texwid;
    if(*texwid > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texwid))
        newval = makepow2(*texwid);
    if(newval != *texwid) {
	/* glPixelStorei(GL_UNPACK_ROW_LENGTH, *texwid); */
	/* glPixelStorei(GL_UNPACK_SKIP_PIXELS, (*texwid - newval)/2); */
	*texwid = newval;
    }

    /* adjust height */
    newval = *texht;
    if(*texht > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texht))
        newval = makepow2(*texht);
    if(*texht > newval) {
	/* glPixelStorei(GL_UNPACK_SKIP_ROWS, (*texht - newval)/2); */
	*texht = newval;
    }

    /* adjust depth */
    newval = *texdepth;
    if(*texdepth > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texdepth))
        newval = makepow2(*texdepth);
    if(*texdepth > newval) {
	*texdepth = newval;
    }
    return tex3ddata;
}


static void
cleanup(void) {
}

int
main(int argc, char *argv[])
{
    FILE *fp;
    char fpstr[512];
    GLubyte *dest, *src;
    float box[6];
    int x, y, z;
    int i, j;
    int texcomps;
    static GLfloat lightpos[4] = {150., 150., 150., 1.f};


    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1) {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
	while(!done) {
	    switch(*args) {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = GL_FALSE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = GL_TRUE;
		break;
	    }
	    args++;
	}
    }
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA /* | GLUT_DEPTH */ | GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA /* | GLUT_DEPTH */);

    (void)glutCreateWindow("volume rendering demo");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    ext_blend_color = glutExtensionSupported("GL_EXT_blend_color");

    /* Initialize OpenGL State */

    glEnable(GL_TEXTURE_2D);

    glEnable(GL_CLIP_PLANE5);

    glDisable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    tex3ddata = loadtex3d(&texwid, &texht, &texdepth, &texcomps);

    slices_X = texwid;
    slices_Y = texht;
    slices_Z = texdepth;
    glGenTextures(slices_X, textures_X);
    glGenTextures(slices_Y, textures_Y);
    glGenTextures(slices_Z, textures_Z);

#if 0
    /* rescaling the texture data has the effect of changing the
       translucency and colors of the data */
    for(x = 0; x < slices_X; x++)
	for(y = 0; y < slices_Y; y++)
	    for(z = 0; z < slices_Z; z++) {
		src = tex3ddata + (z * (texwid * texht) + y * texwid + x) * 4;
		src[0] = pow(src[0] / 255.0, .3) * 255.0;
		src[1] = pow(src[1] / 255.0, .3) * 255.0;
		src[2] = pow(src[2] / 255.0, .3) * 255.0;
		src[3] = pow(src[3] / 255.0, .3) * 255.0;
	    }
#endif

    for(x = 0; x < slices_X; x++) {
	for(j = 0; j < texht; j++)
	    for(i = 0; i < texdepth; i++) {
		dest = textmp + (i + texdepth * j) * 4;
		src = tex3ddata + (i * (texwid * texht) +
		    j * texwid + texwid - x - 1) * 4;
		memcpy(dest, src, sizeof(GLubyte) * 4);
	    }

	glBindTexture(GL_TEXTURE_2D, textures_X[x]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		    texdepth, texht, 0,
		    GL_RGBA, GL_UNSIGNED_BYTE,
		    textmp);
    }

    for(y = 0; y < slices_Y; y++) {
	for(j = 0; j < texdepth; j++)
	    for(i = 0; i < texwid; i++) {
		dest = textmp + (i + texwid * j) * 4;
		src = tex3ddata + (j * (texwid * texht) +
		    (texht - y - 1) * texwid + i) * 4;
		memcpy(dest, src, sizeof(GLubyte) * 4);
	    }
	glBindTexture(GL_TEXTURE_2D, textures_Y[y]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		    texwid, texdepth, 0,
		    GL_RGBA, GL_UNSIGNED_BYTE,
		    textmp);
    }

    for(z = 0; z < slices_Z; z++) {
	glBindTexture(GL_TEXTURE_2D, textures_Z[z]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		    texwid, texht, 0,
		    GL_RGBA, GL_UNSIGNED_BYTE,
		    tex3ddata+texwid*texht*z*sizeof(GLubyte)*4);
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    free(tex3ddata);

    box[0] = -100.0f;
    box[1] = 100.0f;
    box[2] = -100.0f;
    box[3] = 100.0f;
    box[4] = -100.0f;
    box[5] = 100.0f;
    xformInitializeFromBox(&rootXform, box, .57595f);

    CHECK_ERROR("end of main");

    glutMainLoop();
    return 0;
}
