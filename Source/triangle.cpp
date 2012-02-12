#include <GLTools.h>            // OpenGL toolkit
#include <GLFrame.h>
#include <GLFrustum.h>
#include <StopWatch.h>
#include <GLMatrixStack.h>

#ifdef __APPLE__
#include <glut/glut.h>          // OS X version of GLUT
#else
#define FREEGLUT_STATIC
#include <glut.h>            // Windows FreeGlut equivalent
#endif


// length of the pyramids base
const float pyramidSideLength = 2;

// screen dimensions
GLint screenWidth = 800;
GLint screenHeight = 600;

// shader related variables
GLuint shader;
GLint MVPMatrixLocation;

// view related objects
GLFrame cameraFrame;
GLFrustum viewFrustum;

// helper matrices holding partial transformation results
M3DMatrix44f modelViewProjectionMatrix;
M3DMatrix44f viewProjectionMatrix;
M3DMatrix44f cameraMatrix;

// camera vars
M3DVector3f cameraPosition = { 50.0, 1.0, 10.0 };
M3DVector3f targetPosition =  { 0.0, 0.0, 0.0 };
M3DVector3f cameraUp = { 0.0, 0.0, 1.0 };

// camera animation
float cameraAngle = 0.0f;
CStopWatch timer; 

float gridStartX = -10;
float gridEndX = 10;
float gridStartY = -10;
float gridEndY = 10;
float gridSpace = 1;


void RenderGround()
{
	glFrontFace(GL_CCW);

	// ground
	float groundSize = 3;
	glBegin(GL_QUADS);
		glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 0.0f, 0.1f, 0.25f, 1.0f);
		
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


void CopyViewProjectionMatrixToModelViewProjectionMatrix()
{
	for (int i = 0; i < 16; i++)
	{
		modelViewProjectionMatrix[i] = viewProjectionMatrix[i];
	}
}


void RenderSingleTransformedPyramid(M3DVector3f translation, float rotationAngle, M3DVector3f rotationAxis, M3DVector3f scale)
{
	CopyViewProjectionMatrixToModelViewProjectionMatrix();
	M3DMatrix44f modelMatrix;

	if (translation != NULL)
	{
		m3dTranslationMatrix44(modelMatrix, translation[0], translation[1], translation[2]);
		m3dMatrixMultiply44(modelViewProjectionMatrix, modelViewProjectionMatrix, modelMatrix);
	}

	if (rotationAxis != NULL)
	{
		m3dRotationMatrix44(modelMatrix, rotationAngle, rotationAxis[0], rotationAxis[1], rotationAxis[2]);
		m3dMatrixMultiply44(modelViewProjectionMatrix, modelViewProjectionMatrix, modelMatrix);
	}

	if (scale != NULL)
	{
		m3dScaleMatrix44(modelMatrix, scale[0], scale[1], scale[2]);
		m3dMatrixMultiply44(modelViewProjectionMatrix, modelViewProjectionMatrix, modelMatrix);
	}

	if (modelViewProjectionMatrix != NULL)
	{
		glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, modelViewProjectionMatrix);
	}

	RenderPyramid();
}

void RenderSingleTransformedPyramidWithStack(M3DVector3f translation, float rotationAngle, M3DVector3f rotationAxis, M3DVector3f scale)
{
	GLMatrixStack matrixStack;
	matrixStack.PushMatrix(viewProjectionMatrix);

	if (translation != NULL)
	{
		matrixStack.Translate(translation[0],translation[1],translation[2]);
	}

	if (rotationAxis != NULL)
	{
		matrixStack.Rotate(rotationAngle,rotationAxis[0],rotationAxis[1],rotationAxis[2]);
	}

	if (scale != NULL)
	{
		matrixStack.Scale(scale[0],scale[1],scale[2]);
	}

	if (modelViewProjectionMatrix != NULL)
	{
		glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, matrixStack.GetMatrix());
	}

	RenderPyramid();
}

void RenderPyramidsTransformed()
{
	// translated
	M3DVector3f translationVector = { 5, 0, 0 };
	RenderSingleTransformedPyramid( translationVector, 0, NULL, NULL );

	// scaled
	M3DVector3f scaleVector = { 1, 1, 3 };
	RenderSingleTransformedPyramid( NULL, 0, NULL, scaleVector );

	// rotated
	translationVector[0] = -5;
	M3DVector3f rotationAxisVector = { 0, 0, 1 };
	RenderSingleTransformedPyramid( translationVector, 45, rotationAxisVector, NULL );
}

void RenderPyramidsTransformedWithStack()
{
	// translated
	M3DVector3f translationVector = { 5, 0, 0 };
	RenderSingleTransformedPyramidWithStack( translationVector, 0, NULL, NULL );

	// scaled
	M3DVector3f scaleVector = { 1, 1, 3 };
	RenderSingleTransformedPyramidWithStack( NULL, 0, NULL, scaleVector );

	// rotated
	translationVector[0] = -5;
	M3DVector3f rotationAxisVector = { 0, 0, 1 };
	RenderSingleTransformedPyramidWithStack( translationVector, 45, rotationAxisVector, NULL );
}


void RenderGrid()
{
	glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 0.9f, 0.9f, 0.9f, 1.0f);	

	for (int x = gridStartX; x <= gridEndX; x += gridSpace)
	{
		glBegin(GL_LINES);
			glVertex3f(x, gridStartY, 0.0f);
			glVertex3f(x, gridEndY, 0.0f);
		glEnd();
	}	


	for (int y = gridStartY; y <= gridEndY; y += gridSpace)
	{
		glBegin(GL_LINES);
			glVertex3f(gridStartX, y, 0);
			glVertex3f(gridEndX, y, 0);
		glEnd();
	}
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

	// read view matrix to global variable for further use
	cameraFrame.GetCameraMatrix(cameraMatrix, false);
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


void UpdateViewProjectionMatrix() {	
	m3dMatrixMultiply44(viewProjectionMatrix, viewFrustum.GetProjectionMatrix(), cameraMatrix);		
}


void RenderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
    glUseProgram(shader);

	AnimateCamera();

	// calculate viewProjectionMatrix based on current camera state and frustum
	UpdateViewProjectionMatrix();
	
	// send transformed viewProjectionMatrix to shader
	glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, viewProjectionMatrix);


	glCullFace(GL_BACK);

// ======= ACTUAL RENDERING =========	
	RenderGrid();

	//RenderPyramid();
	//RenderPyramidsTransformed();	

	//RenderPyramidWithStack();
	RenderPyramidsTransformedWithStack();

// ==== COMMIT RENDER TO SCREEN =====
    glutSwapBuffers();

	// force refresh screen for camera animation to be visible
	glutPostRedisplay();
}


int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(screenWidth, screenHeight);

	SetupViewFrustum(screenWidth, screenHeight);

    glutCreateWindow("GPU Programming");
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
