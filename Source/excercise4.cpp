#include <GLTools.h>  
#include <GLFrame.h>
#include <GLFrustum.h>
#include <GLMatrixStack.h>
#include <GLGeometryTransform.h>

#include <StopWatch.h>

#define FREEGLUT_STATIC
#include "glut.h"


GLuint shader;
GLuint modelViewProjectionMatrixLocation;

GLMatrixStack modelViewMatrix;
GLMatrixStack projectionMatrix;
GLGeometryTransform transformPipeline;

GLFrame cameraFrame;
GLFrustum viewFrustum;

CStopWatch timer; 

M3DVector3f cameraPosition = { 8.0f, 4.0f, 6.0f };
M3DVector3f targetPosition =  { 0.0f, 0.0f, 0.0f };
M3DVector3f cameraUpDirection = { 0.0f, 0.0f, 1.0f };


// Gouraud shader locations
GLint mvpMatrixLocation;
GLint mvMatrixLocation;
GLint normalMatrixLocation;

GLint shaderLightPositionLocation;
GLint shaderLightColorLocation;
GLint shaderLightAngleLocation;
GLint shaderLightAttenuation0Location;
GLint shaderLightAttenuation1Location;
GLint shaderLightAttenuation2Location;

GLint materialAmbientColorLocation;
GLint materialDiffuseColorLocation;
GLint materialSpecularColorLocation;
GLint materialSpecularExponentLocation;


// Gouraud values
M3DVector3f lightPosition = { 5.0f, 0.0f, 4.0f };
M3DVector3f lightColor = { 1.0f, 1.0f, 0.75f };
float lightAngle = 0;
float lightAttenuation0 = 0.5;
float lightAttenuation1 = 0.05;
float lightAttenuation2 = 0.005;

M3DVector3f materialAmbientColor = { 0.075f, 0.075f, 0.075f };
M3DVector3f materialDiffuseColor = { 0.0f, 0.0f, 0.9f };
M3DVector3f materialSpecularColor = { 0.8f, 0.8f, 0.8f };
float materialSpecularExponent = 10;


GLuint vertex_buffer;
GLuint faces_buffer;

int n_faces;
int n_vertices;

void CreateVerticesBuffer() 
{
   int entriesPerVertex = 7;  

   FILE *fvertices=fopen("geode_vertices.dat","r");
   if(fvertices==NULL) {
     fprintf(stderr,"cannot open vertices file for reading\n");
     exit(-1);
   }

   n_vertices = 0;
   char line[120];
   float vertices[18585];
   
   while(fgets(line,120,fvertices)!=NULL) {
	   float x,y,z;
	   double norm;
	   sscanf(line,"%f %f %f",&x,&y,&z);
  
	   norm=x*x+y*y+z*z;
	   norm=sqrt(norm);
	   
	   vertices[n_vertices*entriesPerVertex]=x;
	   vertices[n_vertices*entriesPerVertex+1] = y ;
	   vertices[n_vertices*entriesPerVertex+2] = z;
	   vertices[n_vertices*entriesPerVertex+3] = 1.0f;
	   vertices[n_vertices*entriesPerVertex+4] = x/norm;
	   vertices[n_vertices*entriesPerVertex+5] = y/norm;
	   vertices[n_vertices*entriesPerVertex+6] = z/norm;

	   n_vertices++;
   }

	glGenBuffers(1,&vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

	glBufferData(GL_ARRAY_BUFFER,n_vertices*sizeof(float)*7,&vertices[0],GL_STATIC_DRAW);
	if(glGetError()!=GL_NO_ERROR) {
		fprintf(stderr,"error copying vertices\n");
	}

	glVertexAttribPointer(GLT_ATTRIBUTE_VERTEX,4,GL_FLOAT,GL_FALSE,sizeof(float)*7,(const GLvoid *)0);
	glVertexAttribPointer(GLT_ATTRIBUTE_NORMAL,3,GL_FLOAT,GL_FALSE,sizeof(float)*7,(const GLvoid *)(4*sizeof(float)) );

	glEnableVertexAttribArray(GLT_ATTRIBUTE_VERTEX);
	glEnableVertexAttribArray(GLT_ATTRIBUTE_NORMAL);
}

void CreateFacesBuffer()
{
   int entriesPerFace = 3;
   FILE *ffaces=fopen("geode_faces.dat","r");
   if(ffaces==NULL) {
	   fprintf(stderr,"cannot open faces file for reading\n");
	   exit(-1);
   }

   n_faces = 0;
   int faces[15360];
   char line[120];
   while(fgets(line,120,ffaces)!=NULL) {
	   GLuint i,j,k;

		if(3!=sscanf(line,"%u %u %u",&i,&j,&k)){
			fprintf(stderr,"error reading faces\n"); 
			exit(-1);
		}

	   faces[n_faces*entriesPerFace] = i;
	   faces[n_faces*entriesPerFace+1] = j;
	   faces[n_faces*entriesPerFace+2] = k;
	   n_faces++;
   }

	glGenBuffers(1,&faces_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,faces_buffer);
	if(glGetError()!=GL_NO_ERROR) {
		fprintf(stderr,"faces_buffer invalid\n");
	}

	glBufferData(GL_ELEMENT_ARRAY_BUFFER,n_faces*sizeof(GLuint)*3,&faces[0],GL_STATIC_DRAW);
	if(glGetError()!=GL_NO_ERROR) {
		fprintf(stderr,"error copying faces\n");
	}
}

void SetUpFrame(GLFrame &cameraFrame,const M3DVector3f origin,
				const M3DVector3f forward,
				const M3DVector3f cameraUpDirection) {
					cameraFrame.SetOrigin(origin);
					cameraFrame.SetForwardVector(forward);
	M3DVector3f side,oUp;
	m3dCrossProduct3(side,forward,cameraUpDirection);
	m3dCrossProduct3(oUp,side,forward);
	cameraFrame.SetUpVector(oUp);
	cameraFrame.Normalize();
};


void LookAt(GLFrame &cameraFrame, const M3DVector3f cameraPosition,
        const M3DVector3f targetPosition,
        const M3DVector3f cameraUpDirection) {
    M3DVector3f forward;
    m3dSubtractVectors3(forward, targetPosition, cameraPosition);
    SetUpFrame(cameraFrame, cameraPosition, forward, cameraUpDirection);
}


void LoadModelViewProjectionMatrixToShader()
{
	glUniformMatrix4fv(modelViewProjectionMatrixLocation, 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());
}


void GetViewProjectionMatrix(float *viewProjectionMatrix)
{	
	M3DMatrix44f viewMatrix;
	cameraFrame.GetCameraMatrix(viewMatrix);

	m3dMatrixMultiply44(viewProjectionMatrix, viewFrustum.GetProjectionMatrix(), viewMatrix);
}


void RenderPlane(float size)
{
	glFrontFace(GL_CCW);

	glBegin(GL_QUADS);
		glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 0.75f, 0.75f, 0.75f, 1.0f);

		glVertex3f(-size, size, 0.0f); 
		glVertex3f(-size, -size, 0.0f);
		glVertex3f(size, -size, 0.0f);
		glVertex3f(size, size, 0.0f);
	glEnd();
}

void RenderFigure()
{
	glCullFace(GL_CW);
	LoadModelViewProjectionMatrixToShader();
	glVertexAttrib4f(GLT_ATTRIBUTE_COLOR, 0.75f, 0.f, 0.f, 1.0f);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,faces_buffer);
	glDrawElements(GL_TRIANGLES,3*n_faces,GL_UNSIGNED_INT,0);	
}


void SetupGourard()
{
	mvpMatrixLocation = glGetUniformLocation(shader, "mvpMatrix");
	if(mvpMatrixLocation == -1)
		fprintf(stderr, "uniform mvpMatrix could not be found\n");

	mvMatrixLocation = glGetUniformLocation(shader, "mvMatrix");
	if (mvMatrixLocation == -1)
		fprintf(stderr, "uniform mvMatrix could not be found\n");

	normalMatrixLocation = glGetUniformLocation(shader, "normalMatrix");
	if (normalMatrixLocation == -1)
		fprintf(stderr, "uniform normalMatrix could not be found\n");

	
	
	shaderLightPositionLocation = glGetUniformLocation(shader, "light1.position");
	if (shaderLightPositionLocation == -1)
		fprintf(stderr, "uniform light1.position could not be found\n");

	shaderLightColorLocation = glGetUniformLocation(shader, "light1.color");
	if (shaderLightColorLocation == -1)
		fprintf(stderr, "uniform light1.color could not be found\n");

	shaderLightAngleLocation = glGetUniformLocation(shader, "light1.angle");
	if (shaderLightAngleLocation == -1)
		fprintf(stderr, "uniform light1.angle could not be found\n");

	shaderLightAttenuation0Location = glGetUniformLocation(shader, "light1.attenuation0");
	if (shaderLightAttenuation0Location == -1)
		fprintf(stderr, "uniform light1.attenuation0 could not be found\n");

	shaderLightAttenuation1Location = glGetUniformLocation(shader, "light1.attenuation1");
	if (shaderLightAttenuation1Location == -1)
		fprintf(stderr, "uniform light1.attenuation1 could not be found\n");

	shaderLightAttenuation2Location = glGetUniformLocation(shader, "light1.attenuation2");
	if (shaderLightAttenuation2Location == -1)
		fprintf(stderr, "uniform light1.attenuation2 could not be found\n");

	
	
	materialAmbientColorLocation = glGetUniformLocation(shader, "material.ambientColor");
	if (materialAmbientColorLocation == -1)
		fprintf(stderr, "uniform material.ambientColor could not be found\n");

    materialDiffuseColorLocation = glGetUniformLocation(shader, "material.diffuseColor");
	if (materialDiffuseColorLocation == -1)
		fprintf(stderr, "uniform material.diffuseColor could not be found\n");

    materialSpecularColorLocation = glGetUniformLocation(shader, "material.specularColor");
	if (materialSpecularColorLocation == -1)
		fprintf(stderr, "uniform material.specularColor could not be found\n");

    materialSpecularExponentLocation = glGetUniformLocation(shader, "material.specularExponent");
	if (materialSpecularExponentLocation == -1)
		fprintf(stderr, "uniform material.specularExponent could not be found\n");
}


void PassGouraudDataToShader()
{
	glUniformMatrix4fv(mvpMatrixLocation, 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());
	glUniformMatrix4fv(mvMatrixLocation, 1, GL_FALSE, transformPipeline.GetModelViewMatrix());
	glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, transformPipeline.GetNormalMatrix());

	M3DMatrix44f viewMatrix;
	cameraFrame.GetCameraMatrix(viewMatrix);
	M3DVector3f lightPositionInEyeSpace;
	m3dTransformVector3(lightPositionInEyeSpace, lightPosition, viewMatrix);

	glUniform3fv(shaderLightPositionLocation, 1, lightPositionInEyeSpace);
	glUniform3fv(shaderLightColorLocation, 1, lightColor);
	glUniform1f(shaderLightAngleLocation, lightAngle);
	glUniform1f(shaderLightAttenuation0Location, lightAttenuation0);
	glUniform1f(shaderLightAttenuation1Location, lightAttenuation1);
	glUniform1f(shaderLightAttenuation2Location, lightAttenuation2);

	glUniform3fv(materialAmbientColorLocation, 1, materialAmbientColor);
	glUniform3fv(materialDiffuseColorLocation, 1, materialDiffuseColor);
	glUniform3fv(materialSpecularColorLocation, 1, materialSpecularColor);
	glUniform1f(materialSpecularExponentLocation, materialSpecularExponent);
}


void TriangleFace(M3DVector3f a, M3DVector3f b, M3DVector3f c) {
      M3DVector3f normal, bMa, cMa;
      m3dSubtractVectors3(bMa, b, a);
      m3dSubtractVectors3(cMa, c, a);
      m3dCrossProduct3(normal, bMa, cMa);
      m3dNormalizeVector3(normal);
      glVertexAttrib3fv(GLT_ATTRIBUTE_NORMAL, normal);
      glVertex3fv(a);
      glVertex3fv(b);
      glVertex3fv(c);
}


void drawTriangles(int n_faces, float *vertices, int *faces) {
      for (int i = 0; i < n_faces; i++) {
      glBegin(GL_TRIANGLES);
      TriangleFace(vertices + 3 * faces[3 * i], vertices + 3 * faces[3 * i + 1], vertices + 3 * faces[3 * i + 2]);
      glEnd();
      }
}
 
void drawSmoothTriangles(int n_faces, float *vertices, int *faces) {
      M3DVector3f normal;
      for (int i = 0; i < n_faces; i++) {
      glBegin(GL_TRIANGLES);
      for(int j=0;j<3;++j) {
      m3dCopyVector3(normal,vertices+3*faces[i*3+j]);
      m3dNormalizeVector3(normal);
      glVertexAttrib3fv(GLT_ATTRIBUTE_NORMAL, normal);
      glVertex3fv(vertices+3*faces[i*3+j]);
      
      }
      glEnd();
      }
}


void RenderTwentatenta()
{
	float ico_vertices[3 * 12] = {
      0., 0., -0.9510565162951536,
      0., 0., 0.9510565162951536,
      -0.85065080835204, 0., -0.42532540417601994,
      0.85065080835204, 0., 0.42532540417601994,
      0.6881909602355868, -0.5, -0.42532540417601994,
      0.6881909602355868, 0.5, -0.42532540417601994,
      -0.6881909602355868, -0.5, 0.42532540417601994,
      -0.6881909602355868, 0.5, 0.42532540417601994,
      -0.2628655560595668, -0.8090169943749475, -0.42532540417601994,
      -0.2628655560595668, 0.8090169943749475, -0.42532540417601994,
      0.2628655560595668, -0.8090169943749475, 0.42532540417601994,
      0.2628655560595668, 0.8090169943749475, 0.42532540417601994 };

	int ico_faces[3*20] = {
      1 ,			 11 ,			 7 ,
      1 ,			 7 ,			 6 ,
      1 ,			 6 ,			 10 ,
      1 ,			 10 ,			 3 ,
      1 ,			 3 ,			 11 ,
      4 ,			 8 ,			 0 ,
      5 ,			 4 ,			 0 ,
      9 ,			 5 ,			 0 ,
      2 ,			 9 ,			 0 ,
      8 ,			 2 ,			 0 ,
      11 ,			 9 ,			 7 ,
      7 ,			 2 ,			 6 ,
      6 ,			 8 ,			 10 ,
      10 ,			 4 ,			 3 ,
      3 ,			 5 ,			 11 ,
      4 ,			 10 ,			 8 ,
      5 ,			 3 ,			 4 ,
      9 ,			 11 ,			 5 ,
      2 ,			 7 ,			 9 ,
      8 ,			 6 ,			 2 };

	  drawTriangles(20, ico_vertices, ico_faces);
}


void RenderSmoothTwentatenta()
{
	float ico_vertices[3 * 12] = {
      0., 0., -0.9510565162951536,
      0., 0., 0.9510565162951536,
      -0.85065080835204, 0., -0.42532540417601994,
      0.85065080835204, 0., 0.42532540417601994,
      0.6881909602355868, -0.5, -0.42532540417601994,
      0.6881909602355868, 0.5, -0.42532540417601994,
      -0.6881909602355868, -0.5, 0.42532540417601994,
      -0.6881909602355868, 0.5, 0.42532540417601994,
      -0.2628655560595668, -0.8090169943749475, -0.42532540417601994,
      -0.2628655560595668, 0.8090169943749475, -0.42532540417601994,
      0.2628655560595668, -0.8090169943749475, 0.42532540417601994,
      0.2628655560595668, 0.8090169943749475, 0.42532540417601994 };

	int ico_faces[3*20] = {
      1 ,			 11 ,			 7 ,
      1 ,			 7 ,			 6 ,
      1 ,			 6 ,			 10 ,
      1 ,			 10 ,			 3 ,
      1 ,			 3 ,			 11 ,
      4 ,			 8 ,			 0 ,
      5 ,			 4 ,			 0 ,
      9 ,			 5 ,			 0 ,
      2 ,			 9 ,			 0 ,
      8 ,			 2 ,			 0 ,
      11 ,			 9 ,			 7 ,
      7 ,			 2 ,			 6 ,
      6 ,			 8 ,			 10 ,
      10 ,			 4 ,			 3 ,
      3 ,			 5 ,			 11 ,
      4 ,			 10 ,			 8 ,
      5 ,			 3 ,			 4 ,
      9 ,			 11 ,			 5 ,
      2 ,			 7 ,			 9 ,
      8 ,			 6 ,			 2 };

	  drawSmoothTriangles(20, ico_vertices, ico_faces);
}



void RenderScene(void) 
{
    // clear the window with current clearing lightColor
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glUseProgram(shader);
	
	// use camera transform
	M3DMatrix44f cameraMatrix;
	cameraFrame.GetCameraMatrix(cameraMatrix);	
	modelViewMatrix.PushMatrix(cameraMatrix);	

	
	modelViewMatrix.PushMatrix();
		modelViewMatrix.Translate(0.0f, 0.0f, -10.0f);
		materialDiffuseColor[0] = materialDiffuseColor[1] = materialDiffuseColor[2] = 0.9f;
		PassGouraudDataToShader();	
		glutSolidCube(20);
	modelViewMatrix.PopMatrix();

	modelViewMatrix.PushMatrix();
		modelViewMatrix.Translate(0, 2, 0);
		modelViewMatrix.Rotate(timer.GetElapsedSeconds()*5, 0.0f, 0.0f, 1.0f);
		materialDiffuseColor[0] = 0.0f;
		materialDiffuseColor[1] = 0.0f;
		materialDiffuseColor[2] = 0.9f;
		PassGouraudDataToShader();	
		RenderTwentatenta();
	modelViewMatrix.PopMatrix();

	modelViewMatrix.PushMatrix();
		modelViewMatrix.Translate(0, -2, 0);
		modelViewMatrix.Rotate(timer.GetElapsedSeconds()*10, 0.0f, 0.0f, 1.0f);
		materialDiffuseColor[0] = 0.0f;
		materialDiffuseColor[1] = 0.9f;
		materialDiffuseColor[2] = 0.0f;
		PassGouraudDataToShader();	
		RenderSmoothTwentatenta();
	modelViewMatrix.PopMatrix();


	modelViewMatrix.PushMatrix();
		//modelViewMatrix.Translate(0, 0, 0);
		modelViewMatrix.Rotate(timer.GetElapsedSeconds()*10, 0.0f, 0.0f, 1.0f);
		materialDiffuseColor[0] = 0.9f;
		materialDiffuseColor[1] = 0.0f;
		materialDiffuseColor[2] = 0.0f;
		PassGouraudDataToShader();	
		glutSolidSphere(1, 20, 20);

	modelViewMatrix.PopMatrix();

	
	modelViewMatrix.PushMatrix();
		modelViewMatrix.Translate(5.0f, 0.0f, 3.0f);		
		materialDiffuseColor[0] = materialDiffuseColor[1] = materialDiffuseColor[2] = 10.0f;
		PassGouraudDataToShader();	
		glutSolidCube(0.1);
	modelViewMatrix.PopMatrix();

	// bring camera matrix back to top of the stack
	modelViewMatrix.PopMatrix();
	


    // perform the buffer swap to display back buffer
    glutSwapBuffers();
	glutPostRedisplay();
}


void ChangeSize(int w, int h) 
{
    glViewport(0, 0, w, h);
	viewFrustum.SetPerspective( 50, float(w)/float(h), 0.1f, 100.0f );
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
	transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}


void SetupRC() 
{
	glEnable(GL_CULL_FACE);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    shader = gltLoadShaderPairWithAttributes("gouraud_phong.vp", 
											 "gouraud_phong.fp",
											 2, 
											 GLT_ATTRIBUTE_VERTEX, 
											 "vVertex", 
											 GLT_ATTRIBUTE_NORMAL, 
											 "vNormal");

	fprintf(stdout, "GLT_ATTRIBUTE_VERTEX : %d\nGLT_ATTRIBUTE_COLOR : %d \n", GLT_ATTRIBUTE_VERTEX, GLT_ATTRIBUTE_COLOR);

	CreateVerticesBuffer();
	CreateFacesBuffer();    
	SetupGourard();
}



void InitializeScene()
{
	LookAt(cameraFrame, cameraPosition, targetPosition, cameraUpDirection);
}


int main(int argc, char* argv[]) 
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Excercise 4 - Gouraud Shader");
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }

    SetupRC();
	InitializeScene();

    glutMainLoop();
    return 0;
}