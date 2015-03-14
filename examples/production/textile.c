#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"

int maxTextureSize;
int maxTextureLevel;

int imageWidth, imageHeight;
GLubyte *imageData;

int texWidthLevel0, texHeightLevel0;
int texWidthTiles, texHeightTiles;
GLubyte **texImageLevel;

GLboolean useBorder = GL_TRUE;
GLboolean useClamp = GL_TRUE;
GLboolean useLinear = GL_TRUE;
GLboolean useMipmap = GL_TRUE;
GLboolean useTextureTiling = GL_TRUE;

int winWidth, winHeight;

#define CHECK_ERROR(string)                                              \
{                                                                        \
    GLenum error_value;                                                  \
    while((error_value = glGetError()) != GL_NO_ERROR)                   \
	printf("Error Encountered: %s (%s)\n", string,                   \
	       gluErrorString(error_value));                             \
}

/* (int)floor(log2(a)) */
static int
iflog2(unsigned int a)
{
    int x = 0;
    while (a >>= 1) ++x;
    return x;
}

static void
initialize(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.5, 0.5, -0.5, 0.5, 0.5, 1.5);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -0.90f);
    glRotatef( 45.0, 0, 1, 0);
    glTranslatef(-0.5, -0.5, 0.0);

#if 0
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
#else
    maxTextureSize = 32;
#endif
    maxTextureLevel = iflog2(maxTextureSize);

    texImageLevel = (GLubyte **) calloc(maxTextureLevel+1, sizeof(GLubyte *));
    if (texImageLevel == NULL) {
	fprintf(stderr, "texture level image allocation failed\n");
	exit(EXIT_FAILURE);
    }
}

static void
imgLoad(char *filename_in, int *w_out, int *h_out, GLubyte **img_out)
{
    int comp;

    *img_out = (GLubyte*)read_texture(filename_in, w_out, h_out, &comp);
    if (!*img_out) {
	perror(filename_in);
	exit(1);
    }
}

static void
buildMipmaps(void)
{
    int level, levelWidth, levelHeight;

    if (useTextureTiling) {
	int width2 = iflog2(imageWidth);
	int height2 = iflog2(imageHeight);

	width2 = (width2 > maxTextureLevel) ? width2 : maxTextureLevel;
	height2 = (height2 > maxTextureLevel) ? height2 : maxTextureLevel;

	texWidthLevel0 = 1 << width2;
	texHeightLevel0 = 1 << height2;
	texWidthTiles = texWidthLevel0 >> maxTextureLevel;
	texHeightTiles = texHeightLevel0 >> maxTextureLevel;
    } else {
	texWidthLevel0 = maxTextureSize;
	texHeightLevel0 = maxTextureSize;
	texWidthTiles = 1;
	texHeightTiles = 1;
    }

    texImageLevel[0] = (GLubyte *)
	calloc(1, (texWidthLevel0+2)*(texHeightLevel0+2)*4*sizeof(GLubyte));

    glPixelStorei(GL_PACK_ROW_LENGTH, texWidthLevel0+2);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 1);
    glPixelStorei(GL_PACK_SKIP_ROWS, 1);

    gluScaleImage(GL_RGBA, imageWidth, imageHeight,
		  GL_UNSIGNED_BYTE, imageData,
		  texWidthLevel0, texHeightLevel0,
		  GL_UNSIGNED_BYTE, texImageLevel[0]);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 1);

    levelWidth = texWidthLevel0;
    levelHeight = texHeightLevel0;
    for (level=0; level<maxTextureLevel; ++level) {
	int newLevelWidth = (levelWidth > 1) ? levelWidth / 2 : 1;
	int newLevelHeight = (levelHeight > 1) ? levelHeight / 2 : 1;
	
	texImageLevel[level+1] = (GLubyte *)
	    calloc(1, (newLevelWidth+2)*(newLevelHeight+2)*4*sizeof(GLubyte));

	glPixelStorei(GL_PACK_ROW_LENGTH, newLevelWidth+2);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, levelWidth+2);

	gluScaleImage(GL_RGBA, levelWidth, levelHeight,
		      GL_UNSIGNED_BYTE, texImageLevel[level],
		      newLevelWidth, newLevelHeight,
		      GL_UNSIGNED_BYTE, texImageLevel[level+1]);

	levelWidth = newLevelWidth;
	levelHeight = newLevelHeight;
    }

    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
}

static void
freeMipmaps(void)
{
    int i;

    for (i=0; i<=maxTextureLevel; ++i) {
	if (texImageLevel[i] != NULL) {
	    free(texImageLevel[i]);
	    texImageLevel[i] = NULL;
	}
    }
}

static void
loadTile(int row, int col)
{
    int border = useBorder ? 1 : 0;
    int level, levelWidth, levelHeight;

    levelWidth = texWidthLevel0;
    levelHeight = texHeightLevel0;
    for (level=0; level<=maxTextureLevel; ++level) {
	int tileWidth = levelWidth / texWidthTiles;
	int tileHeight = levelHeight / texHeightTiles;
	int skipPixels = col * tileWidth + (1 - border);
	int skipRows = row * tileHeight + (1 - border);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, levelWidth+2);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);

	glTexImage2D(GL_TEXTURE_2D, level, 4,
		     tileWidth + 2*border, tileHeight + 2*border,
		     border, GL_RGBA, GL_UNSIGNED_BYTE, texImageLevel[level]);
	
	if (levelWidth > 1) levelWidth = levelWidth / 2;
	if (levelHeight > 1) levelHeight = levelHeight / 2;
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
}

static void
redraw(void)
{
    GLenum minFilterMode, magFilterMode, wrapMode;
    char *minFilterName, *magFilterName, *wrapName;
    int i, j;

    if (useLinear) {
	if (useMipmap) {
	    minFilterMode = GL_LINEAR_MIPMAP_LINEAR;
	    minFilterName = "LINEAR_MIPMAP_LINEAR";
	} else {
	    minFilterMode = GL_LINEAR;
	    minFilterName = "LINEAR";
	}
	magFilterMode = GL_LINEAR;
	magFilterName = "LINEAR";
    } else {
	if (useMipmap) {
	    minFilterMode = GL_NEAREST_MIPMAP_LINEAR;
	    minFilterName = "NEAREST_MIPMAP_LINEAR";
	} else {
	    minFilterMode = GL_NEAREST;
	    minFilterName = "NEAREST";
	}
	magFilterMode = GL_NEAREST;
	magFilterName = "NEAREST";
    }

    if (useClamp) {
	wrapMode = GL_CLAMP;
	wrapName = "CLAMP";
    } else {
	wrapMode = GL_REPEAT;
	wrapName = "REPEAT";
    }

    fprintf(stderr, "tile(%s) ", useTextureTiling ? "yes" : "no");
    fprintf(stderr, "border(%s) ", useBorder ? "yes" : "no");
    fprintf(stderr, "filter(%s, %s) ", minFilterName, magFilterName);
    fprintf(stderr, "wrap(%s) ", wrapName);
    fprintf(stderr, "\n");

    glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, winWidth, winHeight);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    buildMipmaps();

    glEnable(GL_TEXTURE_2D);

    for (i=0; i<texHeightTiles; ++i) {
	float ySize = 1.0 / texHeightTiles;
	float y0 = i * ySize;
	float y1 = y0 + ySize;

	for (j=0; j<texWidthTiles; ++j) {
	    float xSize = 1.0 / texWidthTiles;
	    float x0 = j * xSize;
	    float x1 = x0 + xSize;

	    loadTile(i, j);

	    glBegin(GL_TRIANGLE_STRIP);
	    glTexCoord2f(0.0, 1.0); glVertex2f(x0, y1);
	    glTexCoord2f(0.0, 0.0); glVertex2f(x0, y0);
	    glTexCoord2f(1.0, 1.0); glVertex2f(x1, y1);
	    glTexCoord2f(1.0, 0.0); glVertex2f(x1, y0);
	    glEnd();
	}
    }

    glDisable(GL_TEXTURE_2D);

    freeMipmaps();
}

void
help(void) {
    printf("b    - toggle border\n");
    printf("w    - toggle clamping\n");
    printf("l    - toggle linear filter\n");
    printf("m    - toggle mipmapping\n");
    printf("ESC  - exit\n");
}

/*ARGSUSED*/
static void
usage(int argc, char *argv[])
{
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [ options ] filename\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "    Demonstrates using texture borders\n");
    fprintf(stderr, "    to tile a large texture\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -sb  single buffered\n");
    fprintf(stderr, "    -db  double buffered\n");
    fprintf(stderr, "\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    static int fill = 1;
    switch(key) {
    case '\033': exit(EXIT_SUCCESS); break;
    case 'b':
	useBorder = !useBorder;
	break;
    case 'w':
	useClamp = !useClamp;
	break;
    case 'l':
	useLinear = !useLinear;
	break;
    case 'm':
	useMipmap = !useMipmap;
	break;
    case 't':
	useTextureTiling = !useTextureTiling;
	break;
    case 'f':
        fill ^= 1;
	glPolygonMode(GL_FRONT_AND_BACK, fill ? GL_FILL : GL_LINE);
	break;
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
    glutAddMenuEntry("toggle border", 'b');
    glutAddMenuEntry("toggle tiling", 't');
    glutAddMenuEntry("toggle linear filter", 'l');
    glutAddMenuEntry("toggle mipmapping", 'm');
    glutAddMenuEntry("toggle wire frame", 'f');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.5, 0.5, -0.5, 0.5, 0.5, 1.5);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -0.90f);
    glRotatef( 45.0, 0, 1, 0);
    glTranslatef(-0.5, -0.5, 0.0);
}

int
main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);
    (void)glutCreateWindow("texture tile");
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
/*    create_menu();	*/

    if (argc > 2) {
	usage(argc, argv);
	exit(1);
    }
    imgLoad((argc > 1) ? argv[1] : "../data/mandrill.rgb", &imageWidth, &imageHeight, &imageData);

    initialize();

    glutDisplayFunc(redraw);
    create_menu();
    CHECK_ERROR("main");
    glutMainLoop();
    return 0;
}

