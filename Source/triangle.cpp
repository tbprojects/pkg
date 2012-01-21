// Triangle.cpp
// Our first OpenGL program that will just draw a triangle on the screen.

#include <GLTools.h>            // OpenGL toolkit


#ifdef __APPLE__
#include <glut/glut.h>          // OS X version of GLUT
#else
#define FREEGLUT_STATIC
#include <glut.h>            // Windows FreeGlut equivalent
#endif


GLuint shader;

GLint MVPMatrixLocation;

void RenderPyramid()
{

	float side = 1.f;
	// render base
	glFrontFace(GL_CW);
	glBegin(GL_QUADS);
		glColor3f(0.5f, 0.5f, 0.5f);
		
		glVertex3f(-side, side, 0);
		glVertex3f(side, side, 0);
		glVertex3f(side, -side, 0);
		glVertex3f(-side, -side, 0);
	glEnd();

	// render walls
	for (int i = 0; i < 4; i++)
	{
		if (i == 0)
			glColor3f(1.0f, 0.0f, 0.0f);
		if (i == 1)
			glColor3f(0.0f, 1.0f, 0.0f);
		if (i == 2)
			glColor3f(0.0f, 0.0f, .5f);
		if (i == 3)
			glColor3f(1.0f, 1.0f, 0.0f);

		glRotatef(90.0f, 0.0f, 0.0f, 1.0f);		

		glBegin(GL_TRIANGLES);
			glVertex3f(-1.0f, -1.0f, 0.0f);
			glVertex3f(0.0f, 0.0f, 2.0f);
			glVertex3f(1.0f, -1.0f, 0.0f);
		glEnd();
	}
}



///////////////////////////////////////////////////////////////////////////////
// Window has changed size, or has just been created. In either case, we need
// to use the window dimensions to set the viewport and the projection matrix.

void ChangeSize(int w, int h) {
    glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (50.0, (float)w/(float)h, 0.1f, 1000);
}


///////////////////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering context.
// This is the first opportunity to do any OpenGL related tasks.

void SetupRC() {
    // Blue background
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

    shader = gltLoadShaderPairWithAttributes("shader.vp", "shader.fp",
            2, GLT_ATTRIBUTE_VERTEX, "vVertex", GLT_ATTRIBUTE_COLOR, "vColor");
    fprintf(stdout, "GLT_ATTRIBUTE_VERTEX : %d\nGLT_ATTRIBUTE_COLOR : %d \n",
            GLT_ATTRIBUTE_VERTEX, GLT_ATTRIBUTE_COLOR);
	MVPMatrixLocation=glGetUniformLocation(shader,"MVPMatrix");
	if(MVPMatrixLocation==-1)
	{
		fprintf(stderr,"uniform MVPMatrix could not be found\n");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw scene
void RenderScene(void) {
    // Clear the window with current clearing color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(5, 5, 5,
			  0, 0, 0, 
			  0, 0, 1); 

	
    glUseProgram(shader);

	RenderPyramid();

    GLfloat matrix[16];
    glGetFloatv (GL_MODELVIEW_MATRIX, matrix);

	glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, matrix);

	

    /*glBegin(GL_TRIANGLES);
		glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 1.0, 0.0, 0.0);
		glVertex3f(-0.5f, 0.0f, 2.0f);
		glVertex3f(0.5f, 0.0f, 1.0);
		glVertex3f(0.0f, 0.5f, -4.0f);
    glEnd();*/

    // Perform the buffer swap to display back buffer
    glutSwapBuffers();
}


///////////////////////////////////////////////////////////////////////////////
// Main entry point for GLUT based programs

int main(int argc, char* argv[]) {
  

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Triangle");
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }

    SetupRC();

    glutMainLoop();
    return 0;
}
