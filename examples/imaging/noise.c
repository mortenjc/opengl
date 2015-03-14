/*
 * can this handle resizing in some way?
 * need progressive view of laying out filters
 */
#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include <GL/glut.h>

#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)(sqrt(x)))
#define floorf(x) ((float)(floor(x)))
#ifndef M_PI
#define M_PI 3.14159265f
#define M_SQRT2 1.4142135f
#endif
#endif

int levels = 5;
int steps = -1;

enum {SCRATCH, RANDOM, FILTER, NOISE, TURBULENCE, CLOUD, FIRE, MARBLE,
   VEINS, LASTTEXTURE};
#define textureCount (LASTTEXTURE)

int displayMode = RANDOM;
int oldDisplayMode = RANDOM;
float blendFactor = 1.0;
int doBlending = 0;

GLuint textureNames[textureCount];

#define unitrand() (rand() / (double)RAND_MAX)

/* near, far name mangling because VC++ sucks */
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

int windowWidth, windowHeight;
float xt, yt;
float xs = 1, ys = 1;
int ox, oy = 0;
enum {TRANSLATE, SCALE};
int motionMode = TRANSLATE;

void
motion(int x, int y)
{
    if(motionMode == TRANSLATE) {
	xt -= ((float)x - ox) / windowWidth;
	yt += ((float)y - oy) / windowHeight;
    } else {
	ys = xs = x / (float)windowWidth * 10;
    }
    ox = x;
    oy = y;
    glutPostRedisplay();
}

/*ARGSUSED*/
void
mouse(int button, int state, int x, int y)
{
    if(button == 0)
	motionMode = TRANSLATE;
    else
	motionMode = SCALE;
    ox = x;
    oy = y;
}

#define RANDOMSIZE 32
#define FILTERSIZE 64

float randomImage[RANDOMSIZE][RANDOMSIZE];
float filterImage[FILTERSIZE][FILTERSIZE];

void
draw2DTexturedQuad(float l, float r, float b, float t)
{
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(l, b);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(r, b);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(r, t);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(l, t);
    glEnd();
}

void makeNoiseArray(void)
{
    int x, y;

    for(x = 0; x < RANDOMSIZE; x++)
	for(y = 0; y < RANDOMSIZE; y++)
	    randomImage[x][y] = unitrand();
}

void makeFilterArray(void)
{
    int x, y;
    double dx, dy;
    double d;
    double u;

    for(x = 0; x < FILTERSIZE; x++)
	for(y = 0; y < FILTERSIZE; y++)
	{
	     dx = ((x + .5) / (double)FILTERSIZE) * 2.0 - 1.0;
	     dy = ((y + .5) / (double)FILTERSIZE) * 2.0 - 1.0;
	     d = sqrt(dx * dx + dy * dy);

	    /*
	     * This is actually an evaluation of the b-spline basis
	     * functions for one point in a t(x,y) spline.  The only
	     * bad thing about this filter is that it tends to squish
	     * our values towards .5.  One way to combat this is to
	     * use a "gain" function, like that described on page 25
	     * of Texturing_and_Modeling, to expand the resulting
	     * values back closer to 0 and 1.
	     */
	     if(d < .5) {
		 u = d * 2.0;
		 filterImage[x][y] = (3.0 * u * u * u - 6.0 * u * u + 4.0)
		     / 6.0;
	     } else if (d < 1.0) {
		 u = (d - .5) * 2.0;
		 filterImage[x][y] = pow(1.0 - u, 3.0) / 6.0;
	     }
	     else
		 filterImage[x][y] = 0.0f;
	     filterImage[x][y] /= M_SQRT2; /* Because it's 2D, that's why. */
	}
}

/*
 * Function (approximately) satisfies Perlin's "Noise()" function
 * requirements.  Frequency components in x and y still exist, which
 * means frequency spectrum is not rotationally invariant.  It
 * still looks good.
 */
void loadNoiseIntoBuffer(void)
{
    int x, y;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pushOrthoView(0.0f, (float)RANDOMSIZE, 0.0f, (float)RANDOMSIZE,
        -1.0f, 1.0f);

    for(y = -1; y < RANDOMSIZE + 2; y++)
	for(x = -1; x < RANDOMSIZE + 2; x++)
	{
	    int mx = (x + RANDOMSIZE) % RANDOMSIZE;
	    int my = (y + RANDOMSIZE) % RANDOMSIZE;
	    float n, dx, dy;
#if 0
	    /* adding random jitter decreases the x-y frequency components */
	    dx = randomImage[my][mx] - .5;
	    dy = randomImage[my][RANDOMSIZE - mx - 1] - .5;
#else
	    dx = 0;
	    dy = 0;
#endif
	    n = randomImage[my][mx];
	    glColor3f(n, n, n);
	    draw2DTexturedQuad(dx + x - 1.5, dx + x + 2.5, dy + y - 1.5, dy + y + 2.5);
	}

    popView();
}

double gainExponent = .75;

void loadTurbulenceIntoBuffer(void)
{
    int i;
    float weight;
    float redTable[256], blueTable[256], greenTable[256];

    glClear(GL_COLOR_BUFFER_BIT);

    pushOrthoView(0.0f, (float)windowWidth, 0.0f, (float)windowHeight,
        -1.0f, 1.0f);

    glMatrixMode(GL_TEXTURE);

    glPushMatrix();

    glBindTexture(GL_TEXTURE_2D, textureNames[NOISE]);
    weight = .5 * ((1 << levels) / ((1 << levels) - 1.0));
    for(i = 0; i < levels; i++)
    {
	glColor4f(1, 1, 1, weight);
	draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
	glScalef(2, 2, 1);
	weight *= .5;
    }
    glPopMatrix();

    /* set up gain table */
    for(i = 0; i < 256; i++)
    {
	double t;

	t = i / 255.0;
	if(t < 0.5)
	    t = .5 - .5 * pow(1 - 2 * t, gainExponent);
	else
	    t = .5 + .5 * pow(2 * t - 1, gainExponent);
	redTable[i] = t;
	greenTable[i] = t;
	blueTable[i] = t;
    }
    
    glPixelTransferi(GL_MAP_COLOR, 1);
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 256, redTable);
    glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 256, greenTable);
    glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 256, blueTable);

    glDisable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
#if USE_COPYPIXELS
    glDisable(GL_TEXTURE_2D);
    glCopyPixels(0, 0, windowWidth, windowHeight, GL_COLOR);
    glEnable(GL_TEXTURE_2D);
    glPixelTransferi(GL_MAP_COLOR, 0);
#else
    glBindTexture(GL_TEXTURE_2D, textureNames[SCRATCH]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
        windowWidth, windowHeight, 0);
    glPixelTransferi(GL_MAP_COLOR, 0);
    draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
#endif
    glEnable(GL_BLEND);

    popView();
}

void cloudColor(float factor, float *red, float *green, float *blue)
{
    if(factor < 0) factor = 0;
    if(factor > 1) factor = 1;

    /* factor = pow(factor, .5); */

#if 1
    *red = (.3 + factor * .7);
    *green = (.3 + factor * .7);
    *blue = 1.0;
#else
    *red = factor;
    *green = factor;
    *blue = factor;
#endif
}

void loadCloudIntoBuffer(void)
{
    int i;
    float weight;
    float redTable[256], blueTable[256], greenTable[256];

    glClear(GL_COLOR_BUFFER_BIT);

    pushOrthoView(0.0f, (float)windowWidth, 0.0f, (float)windowHeight,
        -1.0f, 1.0f);

    glMatrixMode(GL_TEXTURE);

    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, textureNames[NOISE]);
    weight = .5;
    for(i = 0; i < levels; i++)
    {
	glColor4f(1, 1, 1, weight);
	draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
	glScalef(2, 2, 1);
	weight *= .5;
    }
    glPopMatrix();

    if(steps != -1 && steps == 0)
	goto done;

    /* set up cloud color table */
    for(i = 0; i < 256; i++)
    {
	double t;
	t = i / 255.0;
	if(t < 0.5)
	    t = .5 - .5 * pow(1 - 2 * t, gainExponent);
	else
	    t = .5 + .5 * pow(2 * t - 1, gainExponent);
	cloudColor(pow(t, 2), redTable + i, greenTable + i,
	    blueTable + i);
    }
    
    glPixelTransferi(GL_MAP_COLOR, 1);
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 256, redTable);
    glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 256, greenTable);
    glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 256, blueTable);
    
    glDisable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
#if USE_COPYPIXELS
    glDisable(GL_TEXTURE_2D);
    glCopyPixels(0, 0, windowWidth, windowHeight, GL_COLOR);
    glEnable(GL_TEXTURE_2D);
    glPixelTransferi(GL_MAP_COLOR, 0);
#else
    glBindTexture(GL_TEXTURE_2D, textureNames[SCRATCH]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
        windowWidth, windowHeight, 0);
    glPixelTransferi(GL_MAP_COLOR, 0);
    draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
#endif
    glEnable(GL_BLEND);

done:
    popView();
}

void loadVeinsIntoBuffer(void)
{
    int i;
    float weight;
    float redTable[256], blueTable[256], greenTable[256];

    glClear(GL_COLOR_BUFFER_BIT);

    pushOrthoView(0.0f, (float)windowWidth, 0.0f, (float)windowHeight,
        -1.0f, 1.0f);

    glMatrixMode(GL_TEXTURE);

    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, textureNames[NOISE]);
    weight = .5;
    for(i = 0; i < levels; i++)
    {
	glColor4f(1, 1, 1, weight);
	draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
	glScalef(2, 2, 1);
	weight *= .5;
    }
    glPopMatrix();

    if(steps != -1 && steps == 0)
	goto done;

    /* set up veins color table */
    for(i = 0; i < 256; i++)
    {
	double t;
	t = i / 256.0;
	t = fabs(2 * t - 1);
	redTable[i] = pow(t, .7);
	greenTable[i] = pow(t, .7);
	blueTable[i] = pow(t, .7);
    }
    
    glPixelTransferi(GL_MAP_COLOR, 1);
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 256, redTable);
    glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 256, greenTable);
    glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 256, blueTable);
    
    glDisable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
#if USE_COPYPIXELS
    glDisable(GL_TEXTURE_2D);
    glCopyPixels(0, 0, windowWidth, windowHeight, GL_COLOR);
    glEnable(GL_TEXTURE_2D);
    glPixelTransferi(GL_MAP_COLOR, 0);
#else
    glBindTexture(GL_TEXTURE_2D, textureNames[SCRATCH]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
        windowWidth, windowHeight, 0);
    glPixelTransferi(GL_MAP_COLOR, 0);
    draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
#endif
    glEnable(GL_BLEND);

done:
    popView();
}

void marbleColor(float factor, float *red, float *green, float *blue)
{
    if(factor < 0) factor = 0;
    if(factor > 1) factor = 1;
    *red = factor;
    *green = factor * factor;
    *blue = factor * factor;
}

void loadMarbleIntoBuffer(void)
{
    int i;
    float redTable[256], blueTable[256], greenTable[256];

    glClear(GL_COLOR_BUFFER_BIT);

    pushOrthoView(0.0f, (float)windowWidth, 0.0f, (float)windowHeight,
        -1.0f, 1.0f);

    glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glColor3f(0, 0, 0); glVertex2f(windowWidth / 4 * 0, 0);
    glColor3f(.5, .5, .5); glVertex2f(windowWidth / 4 * 1, 0);
    glColor3f(.5, .5, .5); glVertex2f(windowWidth / 4 * 1, windowHeight);
    glColor3f(0, 0, 0); glVertex2f(windowWidth / 4 * 0, windowHeight);

    glColor3f(0, 0, 0); glVertex2f(windowWidth / 4 * 1, 0);
    glColor3f(.5, .5, .5); glVertex2f(windowWidth / 4 * 2, 0);
    glColor3f(.5, .5, .5); glVertex2f(windowWidth / 4 * 2, windowHeight);
    glColor3f(0, 0, 0); glVertex2f(windowWidth / 4 * 1, windowHeight);

    glColor3f(0, 0, 0); glVertex2f(windowWidth / 4 * 2, 0);
    glColor3f(.5, .5, .5); glVertex2f(windowWidth / 4 * 3, 0);
    glColor3f(.5, .5, .5); glVertex2f(windowWidth / 4 * 3, windowHeight);
    glColor3f(0, 0, 0); glVertex2f(windowWidth / 4 * 2, windowHeight);

    glColor3f(0, 0, 0); glVertex2f(windowWidth / 4 * 3, 0);
    glColor3f(.5, .5, .5); glVertex2f(windowWidth / 4 * 4, 0);
    glColor3f(.5, .5, .5); glVertex2f(windowWidth / 4 * 4, windowHeight);
    glColor3f(0, 0, 0); glVertex2f(windowWidth / 4 * 3, windowHeight);
    glEnd();
    glEnable(GL_TEXTURE_2D);

    if(steps != -1 && steps == 0)
	goto done;
#if 0
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, textureNames[NOISE]);
    weight = .25;
    for(i = 0; i < levels; i++)
    {
	glColor4f(1, 1, 1, weight);
	draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
	glScalef(2, 2, 1);
	weight *= .5;
    }
    glPopMatrix();
#else
    glColor4f(1, 1, 1, .25);
    glBindTexture(GL_TEXTURE_2D, textureNames[TURBULENCE]);
    draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
#endif

    if(steps != -1 && steps == 1)
	goto done;

    /* set up marble color transfer function */
    for(i = 0; i < 256; i++)
    {
	double t, x;
	/* Scale values from [0,.5] to [0,1] */
	t = i / 256.0 * 2;
	x = pow(sin(t * M_PI * 2) * .5 + .5, .3);
	marbleColor(x, redTable + i, greenTable + i, blueTable + i);
    }
    
    glPixelTransferi(GL_MAP_COLOR, 1);
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 256, redTable);
    glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 256, greenTable);
    glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 256, blueTable);

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glColor4f(1, 1, 1, 1);
    glCopyPixels(0, 0, windowWidth, windowHeight, GL_COLOR);
    glPixelTransferi(GL_MAP_COLOR, 0);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

done:
    popView();
}

void generateTextures(void)
{
    glBindTexture(GL_TEXTURE_2D, textureNames[RANDOM]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, RANDOMSIZE, RANDOMSIZE, 0,
        GL_LUMINANCE, GL_FLOAT, randomImage);

    glBindTexture(GL_TEXTURE_2D, textureNames[FILTER]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, FILTERSIZE, FILTERSIZE, 0,
        GL_LUMINANCE, GL_FLOAT, filterImage);

    loadNoiseIntoBuffer();
    glBindTexture(GL_TEXTURE_2D, textureNames[NOISE]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    /*
     * window (fb) dimensions must be power of two here; this is
     * better handled as an offscreen drawable the dimensions of which
     * can't change; user resizing window right now would be catastrophic
     */
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 0, 0,
        windowWidth, windowHeight, 0);

    loadTurbulenceIntoBuffer();
    glBindTexture(GL_TEXTURE_2D, textureNames[TURBULENCE]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
        windowWidth, windowHeight, 0);

    loadCloudIntoBuffer();
    glBindTexture(GL_TEXTURE_2D, textureNames[CLOUD]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
        windowWidth, windowHeight, 0);

    loadMarbleIntoBuffer();
    glBindTexture(GL_TEXTURE_2D, textureNames[MARBLE]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
        windowWidth, windowHeight, 0);

    loadVeinsIntoBuffer();
    glBindTexture(GL_TEXTURE_2D, textureNames[VEINS]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
        windowWidth, windowHeight, 0);
}

void
init(void)
{

    glGenTextures(textureCount, textureNames);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glDisable(GL_DITHER);

    generateTextures();

}

int inited = 0;

void
display(void)
{
    static int frame = 0;

    glClear(GL_COLOR_BUFFER_BIT);

    pushOrthoView(0.0f, (float)windowWidth, 0.0f, (float)windowHeight, -1.0f, 1.0f);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glScalef(xs, ys, 1);
    glTranslatef(xt, yt, 0);

    if(doBlending && blendFactor < 1.0f) {
	glColor4f(1, 1, 1, 1 - blendFactor);
	glBindTexture(GL_TEXTURE_2D, textureNames[oldDisplayMode]);
	draw2DTexturedQuad(0, windowWidth, 0, windowHeight);
	blendFactor += .02f;
	glColor4f(1, 1, 1, blendFactor);
    }else{
	blendFactor = 1.0f;
	oldDisplayMode = displayMode;
	glutIdleFunc(NULL);
	glColor4f(1, 1, 1, 1);
    }

    glBindTexture(GL_TEXTURE_2D, textureNames[displayMode]);
    draw2DTexturedQuad(0, windowWidth, 0, windowHeight);

    glPopMatrix();

    popView();

    glutSwapBuffers();

    frame++;
}

void
reshape(int w, int h)
{
    if(!inited)
    {
	glutSwapBuffers(); /* ARGH! */
	init();
	inited = 1;
    }

    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
}

void
help(void)
{
    printf("press ESC or 'q' to quit\n");
    printf("press LEFT button and drag to translate\n");
    printf("press MIDDLE button and drag to scale\n");
    printf("press RIGHT button for menu\n");
    printf("press '1' to display random noise texture\n");
    printf("press '2' to display filtered noise texture\n");
    printf("press '3' to display turbulence texture\n");
    printf("press '4' to display cloud texture\n");
    printf("press '5' to display marble texture\n");
    printf("press '6' to display marble veins texture\n");
    printf("press '7' to display filter texture\n");
    printf("press 'b' to toggle blending during transitions\n");
    printf("press '>' to increase texture scale\n");
    printf("press '<' to decrease texture scale\n");
    printf("press 'r' to reset texture scale\n");
    printf("press '+' to increment step at which to stop composition\n");
    printf("press '-' to decrement step at which to stop composition\n");
    printf("    (-1 steps means draw everything, press '+' "
        "once to step = 0)\n");
    printf("press 'h' to print this help message again\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y)
{
    switch(key)
    {
	case 'b':
	    doBlending = !doBlending;
	    if(doBlending)
		glutChangeToMenuEntry(1, "(b) Turn off transition blend", 1);
	    else
		glutChangeToMenuEntry(1, "(b) Turn on transition blend", 1);
	    break;

	case 'r':
	    xs = ys = 1;
	    glutPostRedisplay();
	    break;

	case '>':
	    xs = ys = ys * .5;
	    glutPostRedisplay();
	    break;

	case '<':
	    xs = ys = ys * 2;
	    glutPostRedisplay();
	    break;

	case '\033': case 'q':
	    exit(0);
	    break;

	case '-':
	    steps--;
	    printf("Steps = %d\n", steps);
	    generateTextures(); /* Ugh.  Should only gen the current one. */
	    glutPostRedisplay();
	    break;

	case '+':
	    steps++;
	    printf("Steps = %d\n", steps);
	    generateTextures();
	    glutPostRedisplay();
	    break;

	case '1':
	    displayMode = RANDOM;
	    blendFactor = 0.0;
	    glutIdleFunc(display);
	    break;

	case '2':
	    displayMode = NOISE;
	    blendFactor = 0.0;
	    glutIdleFunc(display);
	    break;

	case '3':
	    displayMode = TURBULENCE;
	    blendFactor = 0.0;
	    glutIdleFunc(display);
	    break;

	case '4':
	    displayMode = CLOUD;
	    blendFactor = 0.0;
	    glutIdleFunc(display);
	    break;

	case '5':
	    displayMode = MARBLE;
	    blendFactor = 0.0;
	    glutIdleFunc(display);
	    break;

	case '6':
	    displayMode = VEINS;
	    blendFactor = 0.0;
	    glutIdleFunc(display);
	    break;

	case '7':
	    displayMode = FILTER;
	    blendFactor = 0.0;
	    glutIdleFunc(display);
	    break;

	case 'h':          
	    help();
	    break;
    }
}

/*ARGSUSED1*/
void
special(int key, int x, int y)
{
    switch(key)
    {
	case GLUT_KEY_UP:
	    if(steps < 9)
		steps++;
	    break;
	case GLUT_KEY_DOWN:
	    if(steps > 0)
		steps--;
	    break;
    }
}

int imageMenu;
int mainMenu;

void imageMenuFunc(int m)
{
    displayMode = m;
    blendFactor = 0.0;
    glutIdleFunc(display);
}

void mainMenuFunc(int m)
{
    switch(m)
    {
	case 1:
	    key('b', 0, 0); break;
	case 2:
	    key('>', 0, 0); break;
	case 3:
	    key('<', 0, 0); break;
	case 4:
	    key('r', 0, 0); break;
	case 5:
	    key('+', 0, 0); break;
	case 6:
	    key('-', 0, 0); break;
	case 7:
	    key('q', 0, 0); break;
    }
}

int
main(int argc, char **argv)
{
    printf("Please wait, making noise array\n");
    makeNoiseArray();
    makeFilterArray();

#ifdef _WIN32
    glutInitWindowSize(windowWidth = 256, windowHeight = 256);
#else
    glutInitWindowSize(windowWidth = 512, windowHeight = 512);
#endif
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
    glutCreateWindow("Texture Map from Procedure");
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    help();

    imageMenu = glutCreateMenu(imageMenuFunc);
    glutAddMenuEntry("Random number texture", RANDOM);
    glutAddMenuEntry("Noise texture", NOISE);
    glutAddMenuEntry("Turbulence texture", TURBULENCE);
    glutAddMenuEntry("Cloud texture", CLOUD);
    glutAddMenuEntry("Marble texture", MARBLE);
    glutAddMenuEntry("Veins texture", VEINS);
    glutAddMenuEntry("Filter texture", FILTER);

    mainMenu = glutCreateMenu(mainMenuFunc);
    glutAddMenuEntry("(b) Turn on transition blend", 1);
    glutAddMenuEntry("(>) Zoom in", 2);
    glutAddMenuEntry("(<) Zoom out", 3);
    glutAddMenuEntry("(r) Reset scale", 4);
    glutAddMenuEntry("(+) Show more compositing steps", 5);
    glutAddMenuEntry("(-) Show fewer compositing steps", 6);
    glutAddSubMenu("Show which image...", imageMenu);
    glutAddMenuEntry("(ESC) Exit", 4);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();
    return 0;
}
