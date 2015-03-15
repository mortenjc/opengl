#ifdef MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glu.h>
#endif

GLfloat yellow[]= {1.00, 1.00, 0.00};
GLfloat orange[]= {0.80, 0.80, 0.20};
GLfloat red[]   = {1.00, 0.00, 0.00};
GLfloat blue[]  = {0.53, 0.68, 0.91};
GLfloat white[] = {1.00, 1.00, 1.00};

struct orbit_param_t {
	double a;	// semi major axis (AU)
	double e;	// eccentricity
	double i;	// inclination
	double o;	// longitude of ascending node
	double w;	// longitude of perihelion
	double L;	// mean longitude
	double Lcy;	// longitude variation per century (since Y2000)
};

struct plparm_t {
	char * name;
	GLfloat * color;
	GLuint  textureID;
	double planet_radius;
	struct orbit_param_t op;
};


#define ITEX 0   // Initial texture ID
plparm_t solar_system[] = { 
	{"Sun",     white,  ITEX, 2.5, { 0.0       , 0.0,         0.0    ,   0       ,  0       , 0        , 0                 }},
	{"Mercury", white,  ITEX, 0.5, { 0.38709893, 0.20563069,  7.00487,   48.33167,  77.45645, 252.25084, 538101628.29/3600}},
	{"Venus",   white,  ITEX, 1.0, { 0.72333199, 0.00677323,  3.39471,   76.68069, 131.53298, 181.97973, 210664136.06/3600}},
	{"Earth",   white,  ITEX, 1.0, { 1.00000011, 0.01671022,  0.00005,  -11.26064, 102.94719, 100.46435, 129597740.63/3600}},
	{"Mars",    white,  ITEX, 0.8, { 1.52366231, 0.09341233,  1.85061,   49.57854, 336.04084, 355.45332,  68905103.78/3600}},
	{"Jupiter", white,  ITEX, 5.0, { 5.20336301, 0.04839266,  1.30530,  199.55615,  14.75385,  34.40438,  10925078.35/3600}},
	{"Pluto",   white,  ITEX, 5.2, { 9.48168677, 0.24880766, 17.14175,  110.30347, 224.06676, 238.92881,    522747.90/3600}},
	{"Pluto",   white,  ITEX, 0.2, {39.48168677, 0.24880766, 17.14175,  110.30347, 224.06676, 238.92881,    522747.90/3600}},
	{ NULL,     0,      ITEX, 0.0, {00.00000000, 0.00000000, 00.00000,  000.00000, 000.00000, 000.00000, 000000000.0000000}}
};

void debug_list()
{
	int i=0;
	plparm_t * pt;
	while (solar_system[i].name != NULL)
	{
		pt = &solar_system[i];
		printf("%s orbit radius: %f\n", pt->name, pt->op.a);
		i++;
	}
}
