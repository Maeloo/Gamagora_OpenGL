#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <list>

#include "Mesh.h";
#include "Texture.h";
#include "Global.h"

#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/GL.h>

#include <glm\glm\vec3.hpp>
#include <glm\glm\mat4x4.hpp>
#include <glm\glm\gtx\transform.hpp>
#include <glm\glm\gtc\matrix_transform.hpp>
#include <glm\glm\gtc\type_ptr.hpp>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

int WIDTH, HEIGHT;

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
	window = glfwCreateWindow ( 800, 800, "OpenGL PORTAL", NULL, NULL );
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
	GLuint shadowmap_program;
	GLuint texture_program;

	GLuint fbo;
	GLuint depthTexture;

	//GLuint vao_quad;
	GLuint vertexBuffer_texture;
	GLuint uvBuffer_texture;

	GLuint vao; // a vertex array object
	GLuint vertexBuffer;
	GLuint normalBuffer;

	GLuint vao_ground; // a vertex array object
	GLuint vertexBuffer_ground;
	GLuint normalBuffer_ground;
} gs;

GLuint mesh_size;
GLuint ground_size;
Vector3 light_pos;

glm::mat4 model;
glm::mat4 projection;

void init ( ) {
	// Build our program and an empty VAO
	gs.program = buildProgram ( "basic.vsl", "basic.fsl" );
	gs.shadowmap_program = buildProgram ( "shadowmap.vsl", "shadowmap.fsl" );
	gs.texture_program = buildProgram ( "texture.vsl", "texture.fsl" );

	//Mesh mesh = Mesh::loadOFF ( "buddha.off", false );
	Mesh mesh	= Mesh::loadOBJ ( "suzanne.obj", false );
	Mesh ground = Mesh::loadOBJ ( "cube.obj", false );

	//mesh.rotate ( M_PI / 2, Vector3 ( 1.0f, .0f, .0f ) ); // for girl.obj
	mesh.scale ( Vector3 ( 3.0f, 3.0f, 3.0f ) );

	ground.scale ( Vector3 ( 10.0f, .25f, 10.0f ) );
	ground.translate ( Vector3 ( .0f, -3.0f, .0f ) );

	mesh.indexData ( );
	ground.indexData ( );
	
	mesh_size	= mesh._indexVertexCount;
	ground_size = ground._indexVertexCount;

	/**** Init Mesh buffers ****/
	{ 
		glCreateVertexArrays ( 1, &gs.vao );
		glBindVertexArray ( gs.vao );

		// init vertex buffer
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

	/**** Init Ground buffers ****/
	{
		glCreateVertexArrays ( 1, &gs.vao_ground );
		glBindVertexArray ( gs.vao_ground );

		// init vertex buffer
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


	/**** Init Framebuffer ****/
	if (true)
	{
		glGenTextures ( 1, &gs.depthTexture );
		glBindTexture ( GL_TEXTURE_2D, gs.depthTexture );
		glTexStorage2D ( GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 800, 800 );

		glGenFramebuffers ( 1, &gs.fbo );
		glBindFramebuffer ( GL_FRAMEBUFFER, gs.fbo );
		glFramebufferTexture2D ( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gs.depthTexture, 0 );		

		GLenum Status = glCheckFramebufferStatus ( GL_FRAMEBUFFER );

		if ( Status != GL_FRAMEBUFFER_COMPLETE ) {
			printf ( "FB error, status: 0x%x\n", Status );
			exit(-1);
		}

		glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
	}



	/**** [DEBUG] Init texture ****/
	if (false)
	{
		gs.depthTexture = loadBMP_custom ( "uvtemplate.bmp" );

		glGenBuffers ( 1, &gs.vertexBuffer_texture );
		glBindBuffer ( GL_ARRAY_BUFFER, gs.vertexBuffer_texture );
		glBufferData ( GL_ARRAY_BUFFER, sizeof ( g_vertex_buffer_data2 ), g_vertex_buffer_data2, GL_STATIC_DRAW );

		glGenBuffers ( 1, &gs.uvBuffer_texture );
		glBindBuffer ( GL_ARRAY_BUFFER, gs.uvBuffer_texture );
		glBufferData ( GL_ARRAY_BUFFER, sizeof ( g_uv_buffer_data2 ), g_uv_buffer_data2, GL_STATIC_DRAW );

		glBindBuffer ( GL_ARRAY_BUFFER, 0 );

	}

	/**** Init matrix ****/
	{
		model = glm::mat4 ( 1.0f );
		projection = glm::perspective ( 45.0f, ( GLfloat ) 800 / ( GLfloat ) 800, 0.1f, 100.0f );
	}
	
	glEnable ( GL_DEPTH_TEST );
	glDepthFunc ( GL_LESS );

	light_pos = Vector3 ( 1.0f, 2.0f, 4.0f );
}

void render ( GLFWwindow* window ) {	
	glfwGetFramebufferSize ( window, &WIDTH, &HEIGHT );

	/**************************** ShadowMap Pass ****************************/
	if (false)
	{
		glViewport ( 0, 0, WIDTH, HEIGHT );

		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glUseProgram ( gs.shadowmap_program );

		/*glm::mat4 view = glm::lookAt ( 
			-light_pos, 
			glm::vec3 ( 0, 0, 0 ), 
			glm::vec3 ( 0, 1, 0 ) );*/

		GLfloat radius = 20.0f;
		GLfloat camX = sin ( glfwGetTime ( ) ) * radius;
		GLfloat camZ = cos ( glfwGetTime ( ) ) * radius;
		glm::mat4 view = glm::lookAt (
			glm::vec3 ( camX, 0.0f, camZ ),
			glm::vec3 ( 0.0f, 0.0f, 0.0f ),
			glm::vec3 ( 0.0f, 1.0f, 0.0f ) );

		glm::mat4 depthMVP = projection * view * model;

		GLuint depthMatrixLoc = glGetUniformLocation ( gs.shadowmap_program, "depthMVP" );

		glUniformMatrix4fv ( depthMatrixLoc, 1, GL_FALSE, &depthMVP[0][0] );

		glBindVertexArray ( gs.vao );
		{
			glDrawArrays ( GL_TRIANGLES, 0, mesh_size );
		}
		glBindVertexArray ( 0 );

		glBindVertexArray ( gs.vao_ground );
		{
			glDrawArrays ( GL_TRIANGLES, 0, ground_size );
		}
		glBindVertexArray ( 0 );

		glUseProgram ( 0 );
	}
	/**********************************************************************/



	/**************************** [DEBUG] Rendu Depth Texture *******************/
	if (false)
	{
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glUseProgram ( gs.texture_program );

		GLuint matrixLoc = glGetUniformLocation ( gs.texture_program, "MVP" );

		glm::mat4 view = glm::lookAt (
			glm::vec3 ( 0, 0, 2 ), 
			glm::vec3 ( 0, 0, 0 ), 
			glm::vec3 ( 0, 1, 0 ) );
		glm::mat4 MVP = projection * view * model;

		glUniformMatrix4fv ( matrixLoc, 1, GL_FALSE, &MVP[0][0] );

		glActiveTexture ( GL_TEXTURE0 );
		glBindTexture ( GL_TEXTURE_2D, gs.depthTexture );

		GLuint textureLoc = glGetUniformLocation ( gs.texture_program, "texture_sampler" );

		glUniform1i ( textureLoc, 0 );

		glEnableVertexAttribArray ( 0 );
		glBindBuffer ( GL_ARRAY_BUFFER, gs.vertexBuffer_texture );
		glVertexAttribPointer (	0, 3, GL_FLOAT, GL_FALSE, 0, ( void* ) 0 );

		glEnableVertexAttribArray ( 1 );
		glBindBuffer ( GL_ARRAY_BUFFER, gs.uvBuffer_texture );
		glVertexAttribPointer ( 1, 2, GL_FLOAT, GL_FALSE, 0, ( void* ) 0 );

		glDrawArrays ( GL_TRIANGLES, 0, 6 );

		glDisableVertexAttribArray ( 0 );
		glDisableVertexAttribArray ( 1 );
		glBindBuffer ( GL_ARRAY_BUFFER, 0 );
		glUseProgram ( 0 );
	}
	/**********************************************************************/


	
	/**************************** Rendu scene ****************************/
	if (false)
	{
		glViewport ( 0, 0, WIDTH, HEIGHT );

		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glUseProgram ( gs.program );

		GLfloat radius = 20.0f;
		GLfloat camX = sin ( glfwGetTime ( ) ) * radius;
		GLfloat camZ = cos ( glfwGetTime ( ) ) * radius;
		glm::mat4 view = glm::lookAt ( 
			glm::vec3 ( camX, 0.0f, camZ ), 
			glm::vec3 ( 0.0f, 0.0f, 0.0f ), 
			glm::vec3 ( 0.0f, 1.0f, 0.0f ) );
	
		GLint modelLoc = glGetUniformLocation ( gs.program, "model" );
		GLint viewLoc = glGetUniformLocation ( gs.program, "view" );
		GLint projLoc = glGetUniformLocation ( gs.program, "projection" );
		
		glUniformMatrix4fv ( viewLoc, 1, GL_FALSE, glm::value_ptr ( view ) );
		glUniformMatrix4fv ( projLoc, 1, GL_FALSE, glm::value_ptr ( projection ) );
		glUniformMatrix4fv ( modelLoc, 1, GL_FALSE, glm::value_ptr ( model ) );

		glProgramUniform3f ( gs.program, 3, .235f, .709f, .313f );
		glProgramUniform3f ( gs.program, 4, light_pos.x, light_pos.y, light_pos.z );

		glBindVertexArray ( gs.vao );
		{		
			glDrawArrays ( GL_TRIANGLES, 0, mesh_size );
		}
		glBindVertexArray ( 0 );

		glProgramUniform3f ( gs.program, 3, 1, 1, 1 );

		glBindVertexArray ( gs.vao_ground );
		{
			glDrawArrays ( GL_TRIANGLES, 0, ground_size );
		}
		glBindVertexArray ( 0 );

		glUseProgram ( 0 );
	}
	/**********************************************************************/
}
