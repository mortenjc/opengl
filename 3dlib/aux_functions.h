
#define PI2 (2*3.141592)
#define PID2 (PI2/4)

void auxWireCircle(float radius, float inclination, int segments) // NOT optimised
{
	GLfloat angle;
	glPushMatrix();

	glRotatef(inclination, 0.0, 0.0, 1.0);
	
	glBegin(GL_LINE_LOOP);
	
	for (int i=0; i< segments; i++)
	{
		angle = i*PI2/segments;
		glVertex3f(radius*cos(angle), 0.0, radius*sin(angle));
	}
	glEnd();
	
	glPopMatrix();
}