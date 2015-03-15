 //light properties
    float[] ambient = {0.0f, 0.0f, 0.0f, 1.0f};
    float[] diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
    float[] specular = {1.0f, 1.0f, 1.0f, 1.0f};
    float[] position = {1.0f, 1.0f, 0.3f, 0.0f};
    
    gl.glLightfv(GL.GL_LIGHT0, GL.GL_AMBIENT, ambient);
    gl.glLightfv(GL.GL_LIGHT0, GL.GL_DIFFUSE, diffuse);
    gl.glLightfv(GL.GL_LIGHT0, GL.GL_POSITION, position);
    
    //light model properties
    float[] model_ambient = {0.4f, 0.4f, 0.4f, 1.0f};
    int model_two_side = 1;                                //0=2sided, 1=1sided
    int viewpoint = 0;                                     //0=infiniteViewpoint, 1=localViewpoint
    
    /******************************
     * NEW * Global ambient light *
     **********************************************************************************
     * We have seen in the previous tutorial that each lights are added between them. *
     *                                                                                *
     * We can add an ambient light for all the scene with GL_LIGHT_MODEL_AMBIENT. The *
     * particularity of this ambient light is that it come from any source.           *
     * In addition, this light is activated by GL_LIGHTING so you don't have to       *
     * enable any GL_LIGHTi to use it.                                                *
     **********************************************************************************/
    gl.glLightModelfv(GL.GL_LIGHT_MODEL_AMBIENT, model_ambient);     //small white ambient light
    
    /******************************************
     * NEW *   Local and infinite viewpoint   *
     *********************************************************************************
     * The GL_LIGHT_MODEL_LOCAL_VIEWER propertie determine the calculation of the    *
     * specular highlight (the reflection).                                          *
     * The reflection depends on the direction of this two vectors :                 *
     *   -> vector from a vertex to the viewpoint                                    *
     *   -> vector from the vertex to the light source                               *
     * Due to this reason, the reflection is dependant to the eye posiion. You can   *
     * remarks that the reflection moves when the object is deplaced.                *
     *                                                                               *
     * Two different calculation :                                                   *
     *   - with an infinite viewpoint : the direction between a vertex and the       *
     * viewpoint is always the same.                                                 *
     *   - with a local viewpoint : the directions don't remains the same. Direction *
     * must be calculated so little slower. It is more realistic but infinite        *
     * viewpoint is a good approximation in many cases (if object are fixed)         *
     *********************************************************************************/
    gl.glLightModeli(GL.GL_LIGHT_MODEL_LOCAL_VIEWER, viewpoint);
    
    /*********************************
     * NEW *   One/Two sided light   *
     *********************************************************************************
     * We can show if we would that light affects the two side of our shapes or only *
     * one face (outside face defines by the normal vector).                         *
     * We can define this with GL_LIGHT_MODEL_TWO_SIDE.                              *
     *********************************************************************************/
    //Only outside face because we don't see the inside of the spheres
    gl.glLightModeli(GL.GL_LIGHT_MODEL_TWO_SIDE, 1);
    
    gl.glEnable(GL.GL_LIGHT0);
    gl.glEnable(GL.GL_LIGHTING); 





float[] no_mat = {0.0f, 0.0f, 0.0f, 1.0f};
    float[] mat_ambient = {0.7f, 0.7f, 0.7f, 1.0f};
    float[] mat_ambient_color = {0.8f, 0.8f, 0.2f, 1.0f};
    float[] mat_diffuse = {0.1f, 0.5f, 0.8f, 1.0f};
    float[] mat_specular = {1.0f, 1.0f, 1.0f, 1.0f};
    float no_shininess = 0.0f;
    float low_shininess = 5.0f;
    float high_shininess = 100.0f;
    float[] mat_emission = {0.3f, 0.2f, 0.2f, 0.0f};
    
    /* draw sphere in first row, first column
     * diffuse reflection only; no ambient or specular 
     */
    gl.glPushMatrix();
        gl.glTranslatef(-3.75f, 3.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, no_mat);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, no_mat);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, no_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in first row, second column
     * diffuse and specular reflection; low shininess; no ambient
     */
    gl.glPushMatrix();
        gl.glTranslatef(-1.25f, 3.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, no_mat);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, mat_specular);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, low_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in first row, third column
     * diffuse and specular reflection; high shininess; no ambient
     */
    gl.glPushMatrix();
        gl.glTranslatef(1.25f, 3.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, no_mat);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, mat_specular);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, high_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in first row, fourth column
     * diffuse reflection; emission; no ambient or specular reflection
     */
    gl.glPushMatrix();
        gl.glTranslatef(3.75f, 3.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, no_mat);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, no_mat);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, no_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, mat_emission);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in second row, first column
     * ambient and diffuse reflection; no specular 
     */
    gl.glPushMatrix();
        gl.glTranslatef(-3.75f, 0.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, mat_ambient);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, no_mat);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, no_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in second row, second column
     * ambient, diffuse and specular reflection; low shininess
     */
    gl.glPushMatrix();
        gl.glTranslatef(-1.25f, 0.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, mat_ambient);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, mat_specular);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, low_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in second row, third column
     * ambient, diffuse and specular reflection; high shininess
     */
    gl.glPushMatrix();
        gl.glTranslatef(1.25f, 0.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, mat_ambient);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, mat_specular);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, high_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in second row, fourth column
     * ambient and diffuse reflection; emission; no specular
     */
    gl.glPushMatrix();
        gl.glTranslatef(3.75f, 0.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, mat_ambient);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, no_mat);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, no_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, mat_emission);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in third row, first column
     * colored ambient and diffuse reflection; no specular 
     */
    gl.glPushMatrix();
        gl.glTranslatef(-3.75f, -3.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, mat_ambient_color);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, no_mat);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, no_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in third row, second column
     * colored ambient, diffuse and specular reflection; low shininess
     */
    gl.glPushMatrix();
        gl.glTranslatef(-1.25f, -3.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, mat_ambient_color);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, mat_specular);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, low_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in third row, third column
     * colored ambient, diffuse and specular reflection; high shininess
     */
    gl.glPushMatrix();
        gl.glTranslatef(1.25f, -3.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, mat_ambient_color);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, mat_specular);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, high_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, no_mat);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();
    
    /* draw sphere in third row, fourth column
     * colored ambient and diffuse reflection; emission; no specular
     */
    gl.glPushMatrix();
        gl.glTranslatef(3.75f, -3.0f, 0.0f);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_AMBIENT, mat_ambient_color);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_DIFFUSE, mat_diffuse);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_SPECULAR, no_mat);
        gl.glMaterialf(GL.GL_FRONT, GL.GL_SHININESS, no_shininess);
        gl.glMaterialfv(GL.GL_FRONT, GL.GL_EMISSION, mat_emission);
        glu.gluSphere(quadric, 1.0f, 16, 16);
    gl.glPopMatrix();