#include <GL/glut.h>

class Lego
{
public:
	Lego();
	~Lego();
	Lego(int type, int x, int y, int z, int angle);
	void display(int current);
	void displaytype1(int current);
	void displaytype2(int current);
	void displaytype3(int current);

	GLfloat color[4];
	int     colorindex;
	int     visible;
	int     type;
	int     x;
	int     y;
	int     z;
	int     angle;
};
