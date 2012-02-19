#include <GLTools.h>
#include <GLFrame.h>
#include <GLFrustum.h>
#include <GLMatrixStack.h>
#include <GLGeometryTransform.h>
#include <StopWatch.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include "glut.h"
#endif

// zmienne szadera
GLuint shader;
GLint MVPMatrixLocation; // adres macierzy Model-View-Projection

// obiekty pomocnicze
GLMatrixStack modelViewMatrix;
GLMatrixStack projectionMatrix;
GLGeometryTransform transformPipeline;

GLFrame cameraFrame;
GLFrustum viewFrustum;

CStopWatch timer; 

// kamera
M3DVector3f cameraPosition = { 10.0f, 10.0f, 12.0f }; // po³o¿enie kamery
M3DVector3f targetPosition =  { 0.0f, 0.0f, 0.0f }; // cel, na który patrzy kamera
M3DVector3f cameraUpDirection = { 0.0f, 0.0f, 1.0f }; // wektor "do góry" okreœlaj¹cy orientacjê kamery

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

///////////////////////////////////////////////////////////////////////////////
// Window has changed size, or has just been created. In either case, we need
// to use the window dimensions to set the viewport and the projection matrix.
void ChangeSize(int w, int h) 
{
    glViewport(0, 0, w, h);
	viewFrustum.SetPerspective( 50, float(w)/float(h), 0.1f, 100.0f );
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix()); // przechowaj macierz rzutowania
	transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix); // niech potok transformacji korzysta z przygotowanych macierzy Model-View i rzutowania
}



///////////////////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering context.
// This is the first opportunity to do any OpenGL related tasks.
void SetupRC() 
{
	glEnable(GL_CULL_FACE);

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

    shader = gltLoadShaderPairWithAttributes("pass_thru_shader.vp", "pass_thru_shader.fp",
        2, GLT_ATTRIBUTE_VERTEX, "vVertex", GLT_ATTRIBUTE_COLOR, "vColor");
    
    fprintf(stdout, "GLT_ATTRIBUTE_VERTEX : %d\nGLT_ATTRIBUTE_COLOR : %d \n",
        GLT_ATTRIBUTE_VERTEX, GLT_ATTRIBUTE_COLOR);

	MVPMatrixLocation = glGetUniformLocation(shader, "mvpMatrix");
	if(MVPMatrixLocation == -1)
	{
		fprintf(stderr, "uniform MVPMatrix could not be found\n");
	}

	CreateVerticesBuffer();
	CreateFacesBuffer();
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
	glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());
}


void GetViewProjectionMatrix(float *viewProjectionMatrix)
{	
	M3DMatrix44f viewMatrix;
	cameraFrame.GetCameraMatrix(viewMatrix);

	m3dMatrixMultiply44(viewProjectionMatrix, viewFrustum.GetProjectionMatrix(), viewMatrix);
}


void RenderGround()
{
	LoadModelViewProjectionMatrixToShader();

	glFrontFace(GL_CCW);

	float size = 10;

	// rysuj podstawê piramidy
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





///////////////////////////////////////////////////////////////////////////////
// Called to draw scene
void RenderScene(void) 
{
    // Clear the window with current clearing color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glUseProgram(shader);

	// wykorzystanie przekszta³cenia kamery	
	M3DMatrix44f cameraMatrix;
	cameraFrame.GetCameraMatrix(cameraMatrix);	
	modelViewMatrix.PushMatrix(cameraMatrix);

	// rysuj ziemiê
	RenderGround();	

	// rysuj figurê
	RenderFigure();

	// przywróæ stos MV do macierzy kamery
	modelViewMatrix.PopMatrix();

    // Perform the buffer swap to display back buffer
    glutSwapBuffers();
	glutPostRedisplay();
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