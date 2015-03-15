#include <stdio.h>
#include "lego.h"

Lego::Lego()
{
	//printf("Error uninitialized lego object\n");
	//exit(0);
}

Lego::~Lego()
{
	//printf("Error uninitialized lego object\n");
	//exit(0);
}

Lego::Lego(int type, int x, int y, int z, int angle)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = type;
	this->angle = angle;
	this->visible = 1;
	this->color[0]=0.0;
	this->color[1]=0.0;
	this->color[2]=1.0;
	display(1);
}


void Lego::displaytype1(int current)
{
		GLUquadric * cyl;
    cyl = gluNewQuadric();
	glPushMatrix();

		if (angle==0)
		{
			glTranslatef(1.0, 0.5, 0.5);
		} else 
		{
			glTranslatef(0.5, 1.0, 0.5);
		}
		glRotatef(angle, 0, 0, 1);
		glTranslatef(-x, -y, z);		

		glScalef(2, 1, 1);
		glColor3f(color[0], color[1], color[2]);
		glutSolidCube(1.0);
		if (current)
		{
			glLineWidth(2.0);
			glColor3f(1.0, 1.0, 0);
			glutWireCube(1.1);
			glColor3f(color[0], color[1], color[2]);
		}
		
		glScalef(0.5, 1, 1);
		glTranslatef(-0.75,-0.25, 0.5);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.5,-0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.5,-0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.5,-0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

	glPopMatrix();
}

void Lego::displaytype2(int current)
{
		GLUquadric * cyl;
		cyl = gluNewQuadric();
		glPushMatrix();

		if (angle==0)
		{
			glTranslatef(0.5, 0.5, 0.5);
		} else 
		{
			glTranslatef(0.5, 1.0, 0.5);
		}
		glRotatef(angle, 0, 0, 1);
		glTranslatef(-x, -y, z);		

		glColor3f(color[0], color[1], color[2]);
		glutSolidCube(1.0);
		if (current)
		{
			glLineWidth(2.0);
			glColor3f(1.0, 1.0, 0);
			glutWireCube(1.1);
			glColor3f(color[0], color[1], color[2]);
		}
		
		glTranslatef(-0.25,-0.25, 0.5);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.5,-0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

	glPopMatrix();
}


void Lego::displaytype3(int current)
{
		GLUquadric * cyl;
		cyl = gluNewQuadric();
		glPushMatrix();

		if (angle==0)
		{
			glTranslatef(1.0, 0.0, 0.5);
		} else 
		{
			glTranslatef(0.0, 1.0, 0.5);
		}
		glRotatef(angle, 0, 0, 1);
		glTranslatef(-x, -y, z);		

		glScalef(0.5, 2, 1);
		glColor3f(color[0], color[1], color[2]);
		glutSolidCube(1.0);
		if (current)
		{
			glLineWidth(2.0);
			glColor3f(1.0, 1.0, 0);
			glutWireCube(1.1);
			glColor3f(color[0], color[1], color[2]);
		}
		
		glScalef(2, 0.5, 1);
		glTranslatef(0.0,-0.75, 0.5);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

		glTranslatef(0.0,0.5, 0.0);
		glPushMatrix();
			gluCylinder(cyl, 0.2, 0.2, 0.2, 20, 15);
			glTranslatef(0,0, 0.2);
			gluDisk(cyl, 0.0, 0.2, 20, 15);
		glPopMatrix();

	glPopMatrix();
}

void Lego::display(int current)
{
	if ( this->type == 0)
		displaytype1(current);
	else 
		if (this->type == 1)
			displaytype2(current);
		else if (this->type == 2)
			displaytype3(current);
}
