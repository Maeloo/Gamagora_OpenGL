#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <list>

#include <GL/glew.h>

#include <GL/glfw3.h>
#include <GL/gl.h>

#include <glm\glm\vec3.hpp>
#include <glm\glm\mat4x4.hpp>
#include <glm\glm\gtx\transform.hpp>
#include <glm\glm\gtc\matrix_transform.hpp>
#include <glm\glm\gtc\type_ptr.hpp>

#include "Mesh.h";

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

void render ( GLFWwindow* );
void init ( );

#define glInfo(a) std::cout << #a << ": " << glGetString(a) << std::endl

// This function is called on any openGL API error
void debug ( GLenum, // source
			 GLenum, // type
			 GLuint, // id
			 GLenum, // severity
			 GLsizei, // length
			 const GLchar *message,
			 const void * ) // userParam
{
	std::cout << "DEBUG: " << message << std::endl;
}

int main ( void ) {
	GLFWwindow* window;

	/* Initialize the library */
	if ( !glfwInit ( ) ) {
		std::cerr << "Could not init glfw" << std::endl;
		return -1;
	}

	// This is a debug context, this is slow, but debugs, which is interesting
	glfwWindowHint ( GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE );

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow ( 640, 480, "OpenGL PORTAL", NULL, NULL );
	if ( !window ) {
		std::cerr << "Could not init window" << std::endl;
		glfwTerminate ( );
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent ( window );

	GLenum err = glewInit ( );
	if ( err != GLEW_OK ) {
		std::cerr << "Could not init GLEW" << std::endl;
		std::cerr << glewGetErrorString ( err ) << std::endl;
		glfwTerminate ( );
		return -1;
	}

	// Now that the context is initialised, print some informations
	glInfo ( GL_VENDOR );
	glInfo ( GL_RENDERER );
	glInfo ( GL_VERSION );
	glInfo ( GL_SHADING_LANGUAGE_VERSION );

	// And enable debug
	glEnable ( GL_DEBUG_OUTPUT );
	glEnable ( GL_DEBUG_OUTPUT_SYNCHRONOUS );

	glDebugMessageCallback ( GLDEBUGPROC ( debug ), nullptr );

	// This is our openGL init function which creates ressources
	init ( );

	/* Loop until the user closes the window */
	while ( !glfwWindowShouldClose ( window ) ) {
		/* Render here */
		render ( window );

		/* Swap front and back buffers */
		glfwSwapBuffers ( window );

		/* Poll for and process events */
		glfwPollEvents ( );
	}

	glfwTerminate ( );
	return 0;
}

// Build a shader from a string
GLuint buildShader ( GLenum const shaderType, std::string const src ) {
	GLuint shader = glCreateShader ( shaderType );

	const char* ptr = src.c_str ( );
	GLint length = src.length ( );

	glShaderSource ( shader, 1, &ptr, &length );

	glCompileShader ( shader );

	GLint res;
	glGetShaderiv ( shader, GL_COMPILE_STATUS, &res );
	if ( !res ) {
		std::cerr << "shader compilation error" << std::endl;

		char message[1000];

		GLsizei readSize;
		glGetShaderInfoLog ( shader, 1000, &readSize, message );
		message[999] = '\0';

		std::cerr << message << std::endl;

		glfwTerminate ( );
		exit ( -1 );
	}

	return shader;
}

// read a file content into a string
std::string fileGetContents ( const std::string path ) {
	std::ifstream t ( path );
	std::stringstream buffer;
	buffer << t.rdbuf ( );

	return buffer.str ( );
}

// build a program with a vertex shader and a fragment shader
GLuint buildProgram ( const std::string vertexFile, const std::string fragmentFile ) {
	auto vshader = buildShader ( GL_VERTEX_SHADER, fileGetContents ( vertexFile ) );
	auto fshader = buildShader ( GL_FRAGMENT_SHADER, fileGetContents ( fragmentFile ) );

	GLuint program = glCreateProgram ( );

	glAttachShader ( program, vshader );
	glAttachShader ( program, fshader );

	glLinkProgram ( program );

	GLint res;
	glGetProgramiv ( program, GL_LINK_STATUS, &res );
	if ( !res ) {
		std::cerr << "program link error" << std::endl;

		char message[1000];

		GLsizei readSize;
		glGetProgramInfoLog ( program, 1000, &readSize, message );
		message[999] = '\0';

		std::cerr << message << std::endl;

		glfwTerminate ( );
		exit ( -1 );
	}

	return program;
}

/****************************************************************
******* INTERESTING STUFFS HERE ********************************
***************************************************************/

// Store the global state of your program
struct {
	GLuint program; // a shader

	GLuint vao; // a vertex array object
	GLuint vertexBuffer;
	GLuint normalBuffer;

	GLuint vao_ground; // a vertex array object
	GLuint vertexBuffer_ground;
	GLuint normalBuffer_ground;
} gs;

GLuint _meshSize;
GLuint _groundSize;
Vector3 _lightpos;
void init ( ) {
	// Build our program and an empty VAO
	gs.program = buildProgram ( "basic.vsl", "basic.fsl" );

	//Mesh mesh = Mesh::loadOFF ( "buddha.off", false );
	Mesh mesh	= Mesh::loadOBJ ( "suzanne.obj", false );
	Mesh ground = Mesh::loadOBJ ( "cube.obj", false );

	//mesh.rotate ( M_PI / 2, Vector3 ( 1.0f, .0f, .0f ) ); // for girl.obj
	mesh.scale ( Vector3 ( 3.0f, 3.0f, 3.0f ) );

	ground.scale ( Vector3 ( 5.0f, .5f, 5.0f ) );
	ground.translate ( Vector3 ( .0f, -3.0f, .0f ) );

	mesh.indexData ( );
	ground.indexData ( );
	
	_meshSize	= mesh._indexVertexCount;
	_groundSize = ground._indexVertexCount;

	/*** Init Mesh buffers ****/
	{ 
		glCreateVertexArrays ( 1, &gs.vao );
		glBindVertexArray ( gs.vao );

		// Init vertex buffer
		glGenBuffers ( 1, &gs.vertexBuffer );
		glBindBuffer ( GL_ARRAY_BUFFER, gs.vertexBuffer );
		glBufferData ( GL_ARRAY_BUFFER, mesh._indexVertexCount * sizeof ( Vector3 ), &mesh._indexVertices[0], GL_STATIC_DRAW );

		glBindBuffer ( GL_ARRAY_BUFFER, gs.vertexBuffer );
		glEnableVertexArrayAttrib ( gs.vao, 1 );
		glVertexAttribPointer ( 1, 3, GL_FLOAT, GL_FALSE, 0, 0 );

		glBindBuffer ( GL_ARRAY_BUFFER, 0 );

		// init normal buffer
		glGenBuffers ( 1, &gs.normalBuffer );
		glBindBuffer ( GL_ARRAY_BUFFER, gs.normalBuffer );
		glBufferData ( GL_ARRAY_BUFFER, mesh._indexVertexCount * sizeof ( Vector3 ), &mesh._indexNormals[0], GL_STATIC_DRAW );
		glBindBuffer ( GL_ARRAY_BUFFER, 0 );

		glBindBuffer ( GL_ARRAY_BUFFER, gs.normalBuffer );
		glEnableVertexArrayAttrib ( gs.vao, 2 );
		glVertexAttribPointer ( 2, 3, GL_FLOAT, GL_FALSE, 0, ( void* ) 0 );
		
		glBindBuffer ( GL_ARRAY_BUFFER, 0 );

		glBindVertexArray ( 0 );
	}

	/*** Init Ground buffers ****/
	{
		glCreateVertexArrays ( 1, &gs.vao_ground );
		glBindVertexArray ( gs.vao_ground );

		// Init vertex buffer
		glGenBuffers ( 1, &gs.vertexBuffer_ground );
		glBindBuffer ( GL_ARRAY_BUFFER, gs.vertexBuffer_ground );
		glBufferData ( GL_ARRAY_BUFFER, ground._indexVertexCount * sizeof ( Vector3 ), &ground._indexVertices[0], GL_STATIC_DRAW );

		glBindBuffer ( GL_ARRAY_BUFFER, gs.vertexBuffer_ground );
		glEnableVertexArrayAttrib ( gs.vao_ground, 1 );
		glVertexAttribPointer ( 1, 3, GL_FLOAT, GL_FALSE, 0, 0 );

		glBindBuffer ( GL_ARRAY_BUFFER, 0 );

		// init normal buffer
		glGenBuffers ( 1, &gs.normalBuffer_ground );
		glBindBuffer ( GL_ARRAY_BUFFER, gs.normalBuffer_ground );
		glBufferData ( GL_ARRAY_BUFFER, ground._indexVertexCount * sizeof ( Vector3 ), &ground._indexNormals[0], GL_STATIC_DRAW );
		glBindBuffer ( GL_ARRAY_BUFFER, 0 );

		glBindBuffer ( GL_ARRAY_BUFFER, gs.normalBuffer_ground );
		glEnableVertexArrayAttrib ( gs.vao_ground, 2 );
		glVertexAttribPointer ( 2, 3, GL_FLOAT, GL_FALSE, 0, ( void* ) 0 );

		glBindBuffer ( GL_ARRAY_BUFFER, 0 );

		glBindVertexArray ( 0 );
	}
	
	// Enable depth test
	glEnable ( GL_DEPTH_TEST );
	glDepthFunc ( GL_LESS );

	_lightpos = Vector3 ( .0f, .0f, 4.0f );
}

void render ( GLFWwindow* window ) {
	int width, height;
	
	glfwGetFramebufferSize ( window, &width, &height );
	glViewport ( 0, 0, width, height );

	glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glUseProgram ( gs.program);

	// Camera/View transformation
	glm::mat4 view;
	GLfloat radius = 10.0f;
	GLfloat camX = sin ( glfwGetTime ( ) ) * radius;
	GLfloat camZ = cos ( glfwGetTime ( ) ) * radius;
	view = glm::lookAt ( glm::vec3 ( camX, 0.0f, camZ ), glm::vec3 ( 0.0f, 0.0f, 0.0f ), glm::vec3 ( 0.0f, 1.0f, 0.0f ) );
	
	// Projection 
	glm::mat4 projection;
	projection = glm::perspective ( 45.0f, ( GLfloat ) width / ( GLfloat ) height, 0.1f, 100.0f );
	
	// Get the uniform locations
	GLint modelLoc = glGetUniformLocation ( gs.program, "model" );
	GLint viewLoc = glGetUniformLocation ( gs.program, "view" );
	GLint projLoc = glGetUniformLocation ( gs.program, "projection" );
	
	// Pass the matrices to the shader
	glUniformMatrix4fv ( viewLoc, 1, GL_FALSE, glm::value_ptr ( view ) );
	glUniformMatrix4fv ( projLoc, 1, GL_FALSE, glm::value_ptr ( projection ) );

	glm::mat4 model = glm::mat4 ( );
	glUniformMatrix4fv ( modelLoc, 1, GL_FALSE, glm::value_ptr ( model ) );

	glProgramUniform3f ( gs.program, 3, .235f, .709f, .313f );
	glProgramUniform3f ( gs.program, 4, _lightpos.x, _lightpos.y, _lightpos.z );

	glBindVertexArray ( gs.vao );
	{		
		glDrawArrays ( GL_TRIANGLES, 0, _meshSize );
	}
	glBindVertexArray ( 0 );

	glProgramUniform3f ( gs.program, 3, 1, 1, 1 );

	glBindVertexArray ( gs.vao_ground );
	{
		glDrawArrays ( GL_TRIANGLES, 0, _groundSize );
	}
	glBindVertexArray ( 0 );

	glUseProgram ( 0 );
}
