#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>

static GLfloat pos[4]   = { 0.0, 0.0, 20.0, 0.0 };
static GLfloat red[4]   = { 0.8, 0.1, 0.0, 1.0 };
static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
static GLfloat blue[4]  = { 0.2, 0.2, 1.0, 1.0 };
static GLfloat light_amb[4]  = { 0, 0, 0, 1.0 };

static GLint surf;

//#define BENCHMARK

#ifdef BENCHMARK
#include <sys/time.h>
#include <unistd.h>

static int current_time(void) /* return current time (in seconds) */
{
   struct timeval tv;
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
   return (int) tv.tv_sec;
}
#else /*BENCHMARK*/

/* dummy */
static int
current_time(void)
{
   return 0;
}
#endif /*BENCHMARK*/

#ifndef M_PI
#define M_PI 3.14159265
#endif

static GLfloat view_rotx = 0.0, view_roty = 90.0, view_rotz = 0.0;
static GLfloat angle = 0.0;

float func(float u, float v)
{
    return (u*u - v*v);
}


static void coords()
{
    glBegin(GL_LINES);
       glVertex3f(0.0, -2.0, 0.0);
       glVertex3f(0.0,  2.0, 0.0);
    glEnd();
    glBegin(GL_LINES);
       glVertex3f(-2.0, 0.0, 0.0);
       glVertex3f( 2.0, 0.0, 0.0);
    glEnd();
    glBegin(GL_LINES);
       glVertex3f(0.0, 0.0, -2.0);
       glVertex3f(0.0, 0.0,  2.0);
    glEnd();
}


static void surface(float u0, float u1, int un, float v0, float v1, int vn)
{
    int iu, iv;
    GLfloat fu,fv, fu2,fv2;
    GLfloat du,dv,dz;

    for (iv=0;iv<vn;iv++)
    {
       fv  = ((v1-v0)*1.0*iv)/vn + v0;
       fv2 = ((v1-v0)*1.0*(iv+1))/vn + v0;

       for (iu=0;iu<un;iu++)
       {
          fu  = ((u1-u0)*1.0*iu)/un + u0;
          fu2 = ((u1-u0)*1.0*(iu+1))/un + u0;
          //printf("u0: %f, u1: %f, v0:%f, v1:%f\n", u0,u1,v0,v1); 
          //printf("fv: %f  fu:%f\n", fv,fu);
          glBegin(GL_QUADS);
             du=fu2-fu;
             dv=fv2-fv;
             dz=func(fv2,fu2)-func(fv,fu);
             glNormal3f(-dz*dv, -dz*du, du*dv);
             glVertex3f(fv, fu, func(fv,fu));
             glVertex3f(fv, fu2, func(fv,fu2));
             glVertex3f(fv2, fu2, func(fv2,fu2));
             glVertex3f(fv2, fu, func(fv2,fu));
          glEnd();
       }
     }
}


static void draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
      glRotatef(view_rotx, 1.0, 0.0, 0.0);
      glRotatef(view_roty, 0.0, 1.0, 0.0);
      glRotatef(view_rotz, 0.0, 0.0, 1.0);

      glPushMatrix();
         glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
         glCallList(surf);
         glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
         coords();
      glPopMatrix();
   glPopMatrix();
}


/* new window size or exposure */
static void reshape(int width, int height)
{
   GLfloat h = (GLfloat) height / (GLfloat) width;

   glViewport(0, 0, (GLint) width, (GLint) height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -10.0);
}


static void init(void)
{

   glLightfv(GL_LIGHT0, GL_SPECULAR, pos);
   glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb);
   //glEnable(GL_CULL_FACE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
   surf = glGenLists(1);
   glNewList(surf, GL_COMPILE);
      surface(-1.0, 1.0, 40, -1.0, 1.0, 40);
   glEndList();
   coords();
   glEnable(GL_NORMALIZE);
}


/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
static void make_window( Display *dpy, const char *name,
             int x, int y, int width, int height,
             Window *winRet, GLXContext *ctxRet)
{
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    GLX_DEPTH_SIZE, 1,
		    None };

   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   GLXContext ctx;
   XVisualInfo *visinfo;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );

   visinfo = glXChooseVisual( dpy, scrnum, attrib );
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   ctx = glXCreateContext( dpy, visinfo, NULL, True );
   if (!ctx) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   XFree(visinfo);

   *winRet = win;
   *ctxRet = ctx;
}


static void event_loop(Display *dpy, Window win)
{
   while (1) {
      while (XPending(dpy) > 0) {
         XEvent event;
         XNextEvent(dpy, &event);
         switch (event.type) {
	 case Expose:
            /* we'll redraw below */
	    break;
	 case ConfigureNotify:
	    reshape(event.xconfigure.width, event.xconfigure.height);
	    break;
         case KeyPress:
            {
               char buffer[10];
               int r, code;
               code = XLookupKeysym(&event.xkey, 0);
               if (code == XK_Left) {
                  view_roty += 5.0;
               }
               else if (code == XK_Right) {
                  view_roty -= 5.0;
               }
               else if (code == XK_Up) {
                  view_rotx += 5.0;
               }
               else if (code == XK_Down) {
                  view_rotx -= 5.0;
               }
               else {
                  r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                                    NULL, NULL);
                  if (buffer[0] == 27) {
                     /* escape */
                     return;
                  }
               }
            }
         }
      }

      /* next frame */
      angle += 2.0;

      draw();
      glXSwapBuffers(dpy, win);

      /* calc framerate */
      {
         static int t0 = -1;
         static int frames = 0;
         int t = current_time();

         if (t0 < 0)
            t0 = t;

         frames++;

         if (t - t0 >= 5.0) {
            GLfloat seconds = t - t0;
            GLfloat fps = frames / seconds;
            printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds,
                   fps);
            t0 = t;
            frames = 0;
         }
      }
   }
}


int main(int argc, char *argv[])
{
   Display *dpy;
   Window win;
   GLXContext ctx;
   char *dpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
   }

   dpy = XOpenDisplay(dpyName);
   if (!dpy) {
      printf("Error: couldn't open display %s\n", dpyName);
      return -1;
   }

   make_window(dpy, "surface", 0, 0, 300, 300, &win, &ctx);
   XMapWindow(dpy, win);
   glXMakeCurrent(dpy, win, ctx);
   reshape(300, 300);

   if (printInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }

   init();

   event_loop(dpy, win);

   glXDestroyContext(dpy, ctx);
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);

   return 0;
}
