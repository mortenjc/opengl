/*
 * Illuminated lines program 
 *
 * The option to draw the streamlines as cylinders doesn't
 * work right now, that's why there's a fair amound of code
 * commented out.  The problem lies in getting the normals
 * correct at the appropriate places.  I'll come back and fix
 * it later if I have time.  -Chris
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI    3.14159265f
#endif

/* * For the anisotropic texture */
#define texWidth  256
#define texHeight 256
GLubyte anisoTexture[texHeight][texWidth][3];

GLfloat color[4] = { 0.4f, 0.4f, 0.7f, 1.0f };
GLfloat white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

GLfloat light[4] = { 0.2357f, 0.2357f, 0.94281f, 0.0f };
GLfloat view[4] = { 0.0f, 0.0f, 1.0f, 0.0f };

int showTexture = 0, drawLines = 1, useLighting = 1, moveLight = 0;
int moving = -1, px, py, drawLight = 1, antialias = 0, currentList = 1;
int useAmbient = 1, useDiffuse = 1, useSpecular = 1;
float ltheta = M_PI / 4.f, lphi = M_PI / 6.f;
float theta = 9.f * M_PI / 8.f, phi = M_PI / 3.f, distance = 6.0f;
float specular_ns = 60.0f;
int winWidth;
int winHeight;

#define CYL_SIDES 8

void reshape (int wid, int ht)
{
  winWidth = wid;
  winHeight = ht;
  glViewport (0, 0, wid, ht);
}

/* * Load streamlines from a file */
void
loadStreamlines (char *filename)
{
  int i, j, numLines, *numVertices, *openEnded;
  float **x, **y, **z, **tx, **ty, **tz;
  FILE *fp = fopen (filename, "r");

  if (fp == NULL)
    {
      printf ("File not opened.\n");
      exit (0);
    }
  /* * Get the number of lines and allocate arrays */
  fscanf (fp, "%d", &numLines);
  numVertices = (int *) malloc (numLines * sizeof (int));
  openEnded = (int *) malloc (numLines * sizeof (int));
  x = (float **) malloc (numLines * sizeof (float *));
  y = (float **) malloc (numLines * sizeof (float *));
  z = (float **) malloc (numLines * sizeof (float *));
  tx = (float **) malloc (numLines * sizeof (float *));
  ty = (float **) malloc (numLines * sizeof (float *));
  tz = (float **) malloc (numLines * sizeof (float *));

  /*
   * Read the lines 
   */
  for (i = 0; i < numLines; i++)
    {
      /* Read the number of vertices and whether the stream is open/closed ended */
      fscanf (fp, "%d %d/n", numVertices + i, openEnded + i);

      /* Allocate the data arrays */
      x[i] = (float *) malloc (numVertices[i] * sizeof (float));
      y[i] = (float *) malloc (numVertices[i] * sizeof (float));
      z[i] = (float *) malloc (numVertices[i] * sizeof (float));
      tx[i] = (float *) malloc (numVertices[i] * sizeof (float));
      ty[i] = (float *) malloc (numVertices[i] * sizeof (float));
      tz[i] = (float *) malloc (numVertices[i] * sizeof (float));


      /* Read the streamline vertices and tangents */
      for (j = 0; j < numVertices[i]; j++)
	fscanf (fp, "%f %f %f %f %f %f/n", &x[i][j], &y[i][j], &z[i][j],
		&tx[i][j], &ty[i][j], &tz[i][j]);
    }

  /* Close the file */
  fclose (fp);

  /*
   * Display list 1 - lines 
   */
  glNewList (1, GL_COMPILE);
  for (i = 0; i < numLines; i++)
    {
      glBegin (openEnded[i] ? GL_LINE_STRIP : GL_LINE_LOOP);

      for (j = 0; j < numVertices[i]; j++)
	{
	  glTexCoord3f (tx[i][j], ty[i][j], tz[i][j]);
	  glVertex3f (x[i][j], y[i][j], z[i][j]);
	}

      glEnd ();
    }
  glEndList ();

  /* * Display list 2 - long, thin cylinders */
#if 0
  glNewList (2, GL_COMPILE);
  glMatrixMode (GL_MODELVIEW);
  for (i = 0; i < numLines; i++)
    {
      for (c = 0; c < CYL_SIDES; c++)
	{
	  glBegin (GL_TRIANGLE_STRIP);

	  for (j = 0; j < numVertices[i]; j++)
	    {
	      XXXX
		glNormal3f (cos (2 * M_PI * c / (float) CYL_SIDES),
			    sin (2 * M_PI * c / (float) CYL_SIDES), 0.0);
	      glVertex3f (x[i][j] +
			  0.01 * cos (2 * M_PI * c / (float) CYL_SIDES),
			  y[i][j] +
			  0.01 * sin (2 * M_PI * c / (float) CYL_SIDES),
			  z[i][j]);
	      XXXX
		glNormal3f (cos
			    (2 * M_PI * ((c + 1) % CYL_SIDES) /
			     (float) CYL_SIDES),
			    sin (2 * M_PI * ((c + 1) % CYL_SIDES) /
				 (float) CYL_SIDES), 0.0);
	      glVertex3f (x[i][j] +
			  0.01 * cos (2 * M_PI * ((c + 1) % CYL_SIDES) /
				      (float) CYL_SIDES),
			  y[i][j] +
			  0.01 * sin (2 * M_PI * ((c + 1) % CYL_SIDES) /
				      (float) CYL_SIDES), z[i][j]);
	    }

	  glEnd ();
	}
    }
  glEndList ();
#endif
}

GLubyte
colorBound (float input)
{
  if (input < 0.0)
    return 0;
  else if (input > 1.0)
    return 255;
  else
    return (GLubyte) (255 * input);
}

void
generateAnisoTexture (void)
{
  /*
   * Material properties 
   */
  float ka = 0.5f, kd = 0.4f, ks = 0.8f;
  float p = 2.0f;		/* compensation for unusually uniform brightness */
  int i, x, y;

  for (y = 0; y < texHeight; y++)
    {
      float LdotT = 2.0 * y / (texHeight - 1.0) - 1.0;
      float LdotN = sqrt (1.0 - LdotT * LdotT);

      for (x = 0; x < texWidth; x++)
	{
	  float VdotT = 2.0 * x / (texWidth - 1.0) - 1.0;
	  float VdotN = sqrt (1.0 - VdotT * VdotT);
	  float VdotR = LdotN * VdotN - LdotT * VdotT;

	  if (VdotR < 0)
	    VdotR = 0;

	  for (i = 0; i < 3; i++)
	    {
	      float c = (useAmbient ? ka : 0.0) +
		(useDiffuse ? color[i] * kd * pow (LdotN, p) : 0.0) +
		(useSpecular ? ks * pow (VdotR, specular_ns) : 0.0);
	      anisoTexture[y][x][i] = colorBound (c);
	    }
	}
    }

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight,
		0, GL_RGB, GL_UNSIGNED_BYTE, anisoTexture);
}

void
loadTextureMatrix ()
{
  GLfloat m[16];

  /*
   * Get current view direction 
   */
  view[0] = sin (phi) * cos (theta);
  view[1] = sin (phi) * sin (theta);
  view[2] = cos (phi);

  /* Create and load texture matrix */
  m[0] = 0.5 * -light[0];
  m[4] = 0.5 * -light[1];
  m[8] = 0.5 * -light[2];
  m[12] = 0.5;
  m[1] = 0.5 * view[0];
  m[5] = 0.5 * view[1];
  m[9] = 0.5 * view[2];
  m[13] = 0.5;
  m[2] = 0.0;
  m[6] = 0.0;
  m[10] = 0.0;
  m[14] = 0.0;
  m[3] = 0.0;
  m[7] = 0.0;
  m[11] = 0.0;
  m[15] = 1.0;
  glMatrixMode (GL_TEXTURE);
  glLoadMatrixf (m);
}


/* * Display callback */
void
cbDisplay (void)
{
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (showTexture)
    {
      glPushAttrib (GL_ENABLE_BIT);
      glEnable (GL_TEXTURE_2D);
      glDisable (GL_LIGHTING);
      glBegin (GL_QUADS);
      glTexCoord2f (0.0, 0.0);
      glVertex2f (-1.0, -1.0);
      glTexCoord2f (1.0, 0.0);
      glVertex2f (1.0, -1.0);
      glTexCoord2f (1.0, 1.0);
      glVertex2f (1.0, 1.0);
      glTexCoord2f (0.0, 1.0);
      glVertex2f (-1.0, 1.0);
      glEnd ();
      glPopAttrib ();
    }
  else
    {
      /*
       * Draw the streamlines 
       */
      glColor4f (0.4f, 0.4f, 0.7f, 1.0f);
      glMatrixMode (GL_MODELVIEW);
      glPushMatrix ();
      glRotatef (90.0f, 1.0f, 0.0f, 0.0f);
      glCallList (currentList);
      glPopMatrix ();

      /*
       * Draw the light vector 
       */
      if (useLighting && drawLight)
	{
	  glPushAttrib (GL_ENABLE_BIT);
	  glDisable (GL_TEXTURE_2D);
	  glDisable (GL_LIGHTING);
	  glColor4f (1.0, 1.0, 0.0, 1.0);
	  glBegin (GL_LINES);
	  glVertex3f (0.0, 0.0, 0.0);
	  glVertex3f (5 * light[0], 5 * light[1], 5 * light[2]);
	  glEnd ();
	  glBegin (GL_POINTS);
	  glVertex3f (5 * light[0], 5 * light[1], 5 * light[2]);
	  glEnd ();
	  glPopAttrib ();
	}
    }

  glutSwapBuffers ();
}


/* * Mouse button callback */
void
cbMouse (int button, int state, int x, int y)
{
  if (moving == -1 && state == GLUT_DOWN)
    {
      moving = button;
      px = x;
      py = y;
    }
  else if (button == moving && state == GLUT_UP)
    {
      moving = -1;
    }
}

/* * Mouse motion callback */
void
cbMotion (int x, int y)
{
  /*
   * If we're showing the texture then motion should do nothing 
   */
  if (showTexture)
    return;

  switch (moving)
    {
    case GLUT_LEFT_BUTTON:
      if (moveLight)
	{
	  ltheta -= 0.03 * (x - px);
	  lphi -= 0.03 * (y - py);
	}
      else
	{
	  theta += (phi < M_PI) ? -0.03 * (x - px) : 0.03 * (x - px);
	  phi -= 0.03 * (y - py);
	}
      break;
    case GLUT_MIDDLE_BUTTON:
      if (!moveLight)
	distance *= pow (0.98, y - py);
      break;
    case GLUT_RIGHT_BUTTON:
    default:
      return;
    }

  /* * Update */
  if (moveLight)
    {
      light[0] = cos (ltheta) * sin (lphi);
      light[1] = sin (ltheta) * sin (lphi);
      light[2] = cos (lphi);
    }
  else
    {
      if (phi < 0.0)
	phi += 2 * M_PI;
      if (phi > 2 * M_PI)
	phi -= 2 * M_PI;

      glMatrixMode (GL_MODELVIEW);
      glLoadIdentity ();
      gluLookAt (distance * sin (phi) * cos (theta),
		 distance * sin (phi) * sin (theta), distance * cos (phi),
		 0.0, 0.0, 0.0, 0.0, 0.0, ((phi < M_PI) ? 1.0 : -1.0));
    }

  px = x;
  py = y;

  glLightfv (GL_LIGHT0, GL_POSITION, light);
  loadTextureMatrix ();

  glutPostRedisplay ();
}

/* * Keyboard callback */
void
cbKeyboard (unsigned char key, int x, int y)
{
  switch (key)
    {
    case 27:
    case 'Q':
    case 'q':
      exit (0);

    case '1':
      useAmbient = !useAmbient;
      generateAnisoTexture ();
      break;
    case '2':
      useDiffuse = !useDiffuse;
      generateAnisoTexture ();
      break;
    case '3':
      useSpecular = !useSpecular;
      generateAnisoTexture ();
      break;

    case '+':
      specular_ns *= 1.5;
      generateAnisoTexture ();
      break;
    case '-':
      specular_ns /= 1.5;
      generateAnisoTexture ();
      break;
    case 'a':
    case 'A':
      antialias = !antialias;
      if (antialias)
	{
	  glEnable (GL_LINE_SMOOTH);
	  glEnable (GL_BLEND);
	}
      else
	{
	  glDisable (GL_LINE_SMOOTH);
	  glDisable (GL_BLEND);
	}
      break;
    case 'f':
    case 'F':
      {
	int row;
	FILE *fp;
	unsigned char *image = NULL;
	fprintf (stderr, "Saving Image... ");
	image = (unsigned char *) malloc (winWidth * winHeight * 3);
	if (!image)
	  {
	    fprintf (stderr, "snapshot failed; couldn't malloc buffer\n");
	    break;
	  }
	glReadBuffer (GL_FRONT);
	glReadPixels (0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE,
		      image);
	glReadBuffer (GL_BACK);
	fp = fopen ("snapshot.ppm", "w");
	fprintf (fp, "P6 %d %d 255\n", winWidth, winHeight);
	for (row = winHeight - 1; row >= 0; row--)
	  fwrite (&image[row * winWidth * 3], winWidth, 3, fp);
	fclose (fp);
	free (image);
	fprintf (stderr, "Image Saved\n");
	return;
      }
    case 'd':
    case 'D':
      drawLight = !drawLight;
      break;
#if 0
    case 'c':
    case 'C':
      drawLines = 0;
      currentList = 2;
      glutChangeToMenuEntry (2, "Render streamlines as lines ('n')", 'n');
      glDisable (GL_TEXTURE_2D);
      glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
      glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, white);
      glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, specular_ns);
      if (useLighting)
	glEnable (GL_LIGHTING);
      else
	glDisable (GL_LIGHTING);
      break;
    case 'n':
    case 'N':
      drawLines = 1;
      currentList = 1;
      glutChangeToMenuEntry (2, "Render streamlines as cylinders ('c')", 'c');
      glDisable (GL_LIGHTING);
      if (useLighting)
	glEnable (GL_TEXTURE_2D);
      else
	glDisable (GL_TEXTURE_2D);
      break;
#endif
    case 'l':
    case 'L':
      useLighting = !useLighting;
      if (drawLines)
	{
	  glDisable (GL_LIGHTING);
	  if (useLighting)
	    glEnable (GL_TEXTURE_2D);
	  else
	    glDisable (GL_TEXTURE_2D);
	}
      else
	{
	  glDisable (GL_TEXTURE_2D);
	  if (useLighting)
	    glEnable (GL_LIGHTING);
	  else
	    glDisable (GL_LIGHTING);
	}
      break;
    case 'M':
    case 'm':
      moveLight = !moveLight;
      break;
    case 'V':
    case 'v':
      showTexture = !showTexture;
      if (showTexture)
	{
	  glMatrixMode (GL_TEXTURE);
	  glLoadIdentity ();
	  glMatrixMode (GL_MODELVIEW);
	  glPushMatrix ();
	  glLoadIdentity ();
	  glMatrixMode (GL_PROJECTION);
	  glPushMatrix ();
	  glLoadIdentity ();
	  gluOrtho2D (-1.0, 1.0, -1.0, 1.0);
	}
      else
	{
	  glMatrixMode (GL_PROJECTION);
	  glPopMatrix ();
	  glMatrixMode (GL_MODELVIEW);
	  glPopMatrix ();
	  glMatrixMode (GL_TEXTURE);
	  loadTextureMatrix ();
	}
      break;

    default:
      return;
    }

  glutPostRedisplay ();
}

/* * Menu callback */
void
cbMenu (int option)
{
  cbKeyboard ((unsigned char) option, 0, 0);
}

void
init (char *filename)
{
  GLuint anisoName;

  /* * Load streamlines */
  loadStreamlines (filename);

  /* * Texture */
  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  glGenTextures (1, &anisoName);
  glBindTexture (GL_TEXTURE_2D, anisoName);
  generateAnisoTexture ();
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  if (!showTexture)
    loadTextureMatrix ();

  /* * Matrices */
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (40.0, 1.0, 0.1, 100.0);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  gluLookAt (distance * sin (phi) * cos (theta),
	     distance * sin (phi) * sin (theta), distance * cos (phi), 0.0,
	     0.0, 0.0, 0.0, 0.0, 1.0);

  /* * Lighting */
  glLightfv (GL_LIGHT0, GL_POSITION, light);
  glEnable (GL_LIGHT0);
  glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  glDisable (GL_LIGHTING);

  /* * Misc */
  glEnable (GL_DEPTH_TEST);
  glEnable (GL_TEXTURE_2D);
  glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	/* not enabled yet */
  glPointSize (4.0);
  glClearColor (0.0, 0.0, 0.0, 0.0);
}

int
main (int argc, char *argv[])
{
  int lightComponentMenu;

  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize (512, 512);
  glutInitWindowPosition (100, 100);
  glutCreateWindow ("Illuminated streamlines");

  glutDisplayFunc (cbDisplay);
  glutKeyboardFunc (cbKeyboard);
  glutMouseFunc (cbMouse);
  glutMotionFunc (cbMotion);
  glutReshapeFunc (reshape);

#if 0
  lightComponentMenu = glutCreateMenu (cbMenu);
  glutAddMenuEntry ("Toggle ambient ('1')", '1');
  glutAddMenuEntry ("Toggle diffuse ('2')", '2');
  glutAddMenuEntry ("Toggle specular ('3')", '3');

  glutCreateMenu (cbMenu);
  glutAddMenuEntry ("Toggle moving viewpoint vs. moving light ('m')", 'm');
  /* glutAddMenuEntry("Render streamlines as cylinders ('c')", 'c'); */
  glutAddMenuEntry ("Toggle lighting ('l')", 'l');
  glutAddSubMenu ("Lighting components:", lightComponentMenu);
  glutAddMenuEntry ("Toggle drawing light vector ('d')", 'd');
  glutAddMenuEntry ("Toggle line antialiasing ('a')", 'a');
  glutAddMenuEntry ("Increase specular exponent ('+')", '+');
  glutAddMenuEntry ("Decrease specular exponent ('-')", '-');
  glutAddMenuEntry ("Toggle illumline/texture view ('v')", 'v');
  glutAttachMenu (GLUT_RIGHT_BUTTON);
#endif

  init ((argc > 1) ? argv[1] : "../data/sphereflow.txt");

  glutMainLoop ();
  return 0;
}
