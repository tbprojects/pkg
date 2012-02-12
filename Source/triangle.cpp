#include <GLTools.h>            // OpenGL toolkit
#include <GLFrame.h>
#include <GLFrustum.h>
#include <StopWatch.h>

#ifdef __APPLE__
#include <glut/glut.h>          // OS X version of GLUT
#else
#define FREEGLUT_STATIC
#include <glut.h>            // Windows FreeGlut equivalent
#endif


const float pyramidSideLength = 2;

GLint screenWidth = 800;
GLint screenHeight = 600;

GLuint shader;
GLint MVPMatrixLocation;

GLFrame cameraFrame;
GLFrustum viewFrustum;

// camera vars
M3DVector3f cameraPosition = { 50.0, 1.0, 10.0 };
M3DVector3f targetPosition =  { 0.0, 0.0, 0.0 };
M3DVector3f cameraUp = { 0.0, 0.0, 1.0 };
float cameraAngle = 0.0f;
CStopWatch timer; 

double g_applicationRunningTime = 0.0f;
float g_rotationSpeed = 1.0f;
float g_pyramidRotationAngle = 0;


void RenderGround()
{
	glFrontFace(GL_CCW);

	// ground
	float groundSize = 3;
	glBegin(GL_QUADS);
		glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 0.0, 0.1, 0.25, 1.0);
		
		glVertex3f(-groundSize, groundSize, 0.0f); 
		glVertex3f(-groundSize, -groundSize, 0.0f);
		glVertex3f(groundSize, -groundSize, 0.0f);
		glVertex3f(groundSize, groundSize, 0.0f);
	glEnd();
}


void RenderPyramid() 
{
	glFrontFace(GL_CCW);

	// base
	glBegin(GL_QUADS);
		glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 0.75, 0.75, 0.75, 1.0);

		glVertex3f(-pyramidSideLength, pyramidSideLength, 0.0f); 
		glVertex3f(-pyramidSideLength, -pyramidSideLength, 0.0f);
		glVertex3f(pyramidSideLength, -pyramidSideLength, 0.0f);
		glVertex3f(pyramidSideLength, pyramidSideLength, 0.0f);
	glEnd();
		
	// blue
	glBegin(GL_TRIANGLES);
		glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 0.0, 0.0, 1.0, 0.0);	
		
		glVertex3f(0.0f, 0.0f, pyramidSideLength);
		glVertex3f(-pyramidSideLength, pyramidSideLength, 0.0f);
		glVertex3f(-pyramidSideLength, -pyramidSideLength, 0.0f);		
	glEnd();

	// green
	glBegin(GL_TRIANGLES);
		glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 0.0, 1.0, 0.0, 1.0);	
		
		glVertex3f(0.0f, 0.0f, pyramidSideLength);
		glVertex3f(-pyramidSideLength, -pyramidSideLength, 0.0f);
		glVertex3f(pyramidSideLength, -pyramidSideLength, 0.0f);		
	glEnd();
	
	// red
	glBegin(GL_TRIANGLES);
		glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 1.0, 0.0, 0.0, 1.0);	
		
		glVertex3f(0.0f, 0.0f, pyramidSideLength);
		glVertex3f(pyramidSideLength, -pyramidSideLength, 0.0f);
		glVertex3f(pyramidSideLength, pyramidSideLength, 0.0f);		
	glEnd();
	
	// yellow
	glBegin(GL_TRIANGLES);
		glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 1.0, 1.0, 0.0, 1.0);	
		
		glVertex3f(0.0f, 0.0f, pyramidSideLength);
		glVertex3f(pyramidSideLength, pyramidSideLength, 0.0f);
		glVertex3f(-pyramidSideLength, pyramidSideLength, 0.0f);		
	glEnd();
}



void SetUpFrame(GLFrame &frame,
				const M3DVector3f origin,
				const M3DVector3f forward,
				const M3DVector3f up) {
	
	frame.SetOrigin(origin);
	frame.SetForwardVector(forward);
	
	M3DVector3f side, oUp;
	m3dCrossProduct3(side, forward, up);
	m3dCrossProduct3(oUp, side, forward);
	frame.SetUpVector(oUp);
	
	frame.Normalize();
};


void LookAt(GLFrame &frame, 
		const M3DVector3f eye,
        const M3DVector3f at,
        const M3DVector3f up) {

    M3DVector3f forward;
    m3dSubtractVectors3(forward, at, eye);
    SetUpFrame(frame, eye, forward, up);
}


void UpdateCamera() {
	LookAt(cameraFrame,
		   cameraPosition,
		   targetPosition,
		   cameraUp);
}


void AnimateCamera() 
{	
	//cameraAngle = timer.GetElapsedSeconds()*M_PI;
	cameraAngle = timer.GetElapsedSeconds()*0.2;

	cameraPosition[0]=6.8f*cos(cameraAngle);
	cameraPosition[1]=6.8f*sin(cameraAngle);
	cameraPosition[2]=5.0f; 
	LookAt(cameraFrame, cameraPosition, targetPosition, cameraUp);
}


void SetupViewFrustum(int width, int height) {
	
	viewFrustum.SetPerspective(50, ((float)width)/((float)height), 0.1f, 100);	
}


void ChangeSize(int w, int h) {
	SetupViewFrustum(w, h);
    glViewport(0, 0, w, h);	
}


void SetupRC() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	shader = gltLoadShaderPairWithAttributes(
			"pass_thru_shader.vp", 
			"pass_thru_shader.fp",
            2, 
			GLT_ATTRIBUTE_VERTEX, 
			"vVertex", 
			GLT_ATTRIBUTE_COLOR, 
			"vColor");

    fprintf(stdout, 
			"GLT_ATTRIBUTE_VERTEX : %d\nGLT_ATTRIBUTE_COLOR : %d \n",
            GLT_ATTRIBUTE_VERTEX, 
			GLT_ATTRIBUTE_COLOR);


	MVPMatrixLocation = glGetUniformLocation(shader, "mvpMatrix");
	if(MVPMatrixLocation == -1)
	{
		fprintf(stderr, "uniform mvpMatrix could not be found\n");
	}

	UpdateCamera();
}


void UpdateMvp() {
	// wczytujemy macierz kamery
	M3DMatrix44f cameraMatrix;
	cameraFrame.GetCameraMatrix(cameraMatrix, false);
	
	// obliczamy macierz Model-Widok-Rzutowanie
	M3DMatrix44f mvpMatrix;
	//m3dMatrixMultiply44(mvpMatrix, cameraMatrix, projectionMatrix);
	m3dMatrixMultiply44(mvpMatrix, viewFrustum.GetProjectionMatrix(), cameraMatrix);	
	
	// przekazujemy macierz MVP do szadera
	glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, mvpMatrix);
}


void RenderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
    glUseProgram(shader);

	AnimateCamera();
	UpdateMvp();

	RenderGround();
	RenderPyramid();

    glutSwapBuffers();

	// required for camera animation
	glutPostRedisplay();
}


int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(screenWidth, screenHeight);

	SetupViewFrustum(screenWidth, screenHeight);

    glutCreateWindow("Triangle");
	//glutIdleFunc(Idle);
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
