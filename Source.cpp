#include <iostream>
#include <string>
#include <memory>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/polar_coordinates.hpp>

/*
Description

This program is designed to animate a mario image

Cleaned up when the info is loaded into the VBO's
Right now it is done before the render loop

*/

using std::cout;
using std::endl;

//void printGLInfo(GLFWwindow* window);
void getGLVersionInfo();
GLuint createShader(GLenum eShaderType, const std::string &strShaderFile);


const float speed = 3.0f; //3 units per second
const float ball_speed = 0.5f; //3 units per second

const float animation_speed = 1.0f; // 1 frame units per second

const float paddle_size = 0.15f;
const float ball_size = 0.05f;

enum SQUARE { SQUARE1, SQUARE2 };

unsigned char* load_bmp(std::string image_path , unsigned int& width , unsigned int& height );
glm::vec2 getPosFromControls(GLFWwindow* window , double deltaTime, SQUARE square);
glm::vec2& getBallVelocity(glm::vec2& pos, glm::vec2& velocity );




struct Rectangle {
	Rectangle(float left, float top, float  right, float bottom) :
	left(left), top(top), right(right), bottom(bottom)	{}
	float left;
	float right;
	float top;
	float bottom;
};


struct Sprite {
	Sprite(glm::vec2 size_in, glm::vec2 pos_in) : size(size_in), pos(pos_in)  {
	}
	Sprite() {
	}
	void setPos(float x , float y){
		pos.x = x;
		pos.y = y;
		update();
	}
	void adjustPos(glm::vec2 pos){
		this->pos += pos;
		update();
	}
	void setSize(float x, float y){
		size.x = x;
		size.y = y;
		update();
	}
	void setSize(glm::vec2 size){
		this->size = size;
		update();
	}
	void update(){
		world_transform = glm::translate(glm::vec3(pos, 0.0f)) * glm::scale(glm::vec3(size, 1.0f));
	}
	glm::mat4& getWorldTransform(){
		return world_transform;
	}
	void setVelocity(float x, float y){
		velocity.x = x;
		velocity.y = y;
	}


	Rectangle getBoundingBox(){
		float left, right, top, bottom;
		left = pos.x - size.x;
		right = pos.x + size.x;
		top = pos.y + size.y;
		bottom = pos.y - size.y;

		return Rectangle(left, top, right, bottom);
	}


	glm::vec2 velocity{ 0.15f, 0.15f };
	glm::vec2 size{0.15f,0.15f};
	glm::vec2 pos{ 0.0f , 0.0f };
	glm::mat4 world_transform;
};

bool collision(Sprite& sprite1, Sprite& sprite2);
glm::vec2 getCollisionVector(Sprite& sprite1, Sprite& sprite2);

int main(void){

	if (!glfwInit()){
		cout << "Error Initializing GLFW" << endl;
	}


	getGLVersionInfo();

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	GLFWwindow* window = glfwCreateWindow(1024, 768, "Moving Square", NULL, NULL);
	if (window == NULL){
		cout << "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials." << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		cout << "Failed to initialize GLEW" << endl;
		return -1;
	}

	//printGLInfo(window);


	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);



	//static const char * vs_source[] =
	static const std::string vs_source =
	{
		"#version 330 core                                                 \n"
		"                                                                  \n"
		"layout(location = 0) in vec3 vertexPosition_modelspace;           \n"
		"layout(location = 1) in vec3 vertex_color;                        \n"
		"                                                                  \n"
		"uniform mat4 world_space;                                         \n"
		"                                                                  \n"
		"smooth out vec4 color;                                            \n"
		"                                                                  \n"
		"                                                                  \n"
		"void main(){                                                      \n"
		"    gl_Position = world_space                                     \n"
		"               * vec4( vertexPosition_modelspace , 1.0f);         \n"
		"                                                                  \n"
		"    color = vec4(vertex_color,1.0);                               \n"
		"}                                                                 \n"
	};


	static const std::string fs_source = 
	{
		"#version 330 core                                                 \n"
		"                                                                  \n"
		"smooth in vec4 color;                                             \n"
		"                                                                  \n"
		"// Ouput data                                                     \n"
		"out vec4 output_color;                                            \n"
		"                                                                  \n"
		"void main(void)                                                   \n"
		"{                                                                 \n"
		"    output_color = color;                                         \n"
		"}                                                                 \n"
	};


	static const std::string vs_source_texture =
	{
		"#version 330 core                                                 \n"
		"                                                                  \n"
		"layout(location = 0) in vec3 vertexPosition_modelspace;           \n"
		"layout(location = 2) in vec2 texture_pos;                         \n"
		"                                                                  \n"
		"uniform mat4 world_space;                                         \n"
		"                                                                  \n"
		"smooth out vec4 color;                                            \n"
		"smooth out vec2 texture_coord;                                    \n"
		"                                                                  \n"
		"void main(){                                                      \n"
		"    gl_Position = world_space                                     \n"
		"               * vec4( vertexPosition_modelspace , 1.0f);         \n"
		"    texture_coord = texture_pos;                                  \n"
		"    color = vec4(1.0,1.0,1.0,1.0);                                \n"
		"}                                                                 \n"
	};


	static const std::string fs_source_texture =
	{
		"#version 330 core                                                 \n"
		"                                                                  \n"
		"smooth in vec4 color;                                             \n"
		"smooth in vec2 texture_coord;                                     \n"
		"uniform sampler2D tex;                                            \n"
		"                                                                  \n"
		"// Ouput data                                                     \n"
		"out vec4 output_color;                                            \n"
		"                                                                  \n"
		"void main(void)                                                   \n"
		"{                                                                 \n"		
		"    output_color = texture(tex, texture_coord);                   \n"
		"    //output_color = vec4(0.5 , 0.5 , 0.5 , 1.0 );                \n"
		"    //output_color = texture(tex, texture_coord) * color;         \n"
		"}                                                                 \n"
	};
	

	static const std::string vs_source_animation =
	{
		"#version 330 core                                                 \n"
		"                                                                  \n"
		"layout(location = 0) in vec3 vertexPosition_modelspace;           \n"
		"layout(location = 2) in vec2 texture_pos;                         \n"
		"                                                                  \n"
		"                                                                  \n"
		"uniform mat4 world_space;                                         \n"
		"uniform uint animation_index;                                     \n"
		"uniform uvec2 sprite_grid;                                        \n"
		"                                                                  \n"
		"smooth out vec4 color;                                            \n"
		"smooth out vec2 texture_coord;                                    \n"
		"                                                                  \n"
		"void main(){                                                      \n"
		"    gl_Position = world_space                                     \n"
		"               * vec4( vertexPosition_modelspace , 1.0f);         \n"			
		"                                                                  \n"
		"    vec2 cell_size =  vec2(1.0,1.0);                              \n"
		"    cell_size.x /= sprite_grid.x;                                 \n"
		"    cell_size.y /= sprite_grid.y;                                 \n"
		"                                                                  \n"
		"    uvec2 grid_location = uvec2(0,0);                             \n"
		"    grid_location.x = animation_index % sprite_grid.x;           \n"		
		"    grid_location.y = animation_index / sprite_grid.y;            \n"
		"                                                                  \n"
		"    texture_coord = texture_pos;                                  \n"
		"    texture_coord.x /= sprite_grid.x;                             \n"
		"    texture_coord.y /= sprite_grid.y;                             \n"
		"                                                                  \n"
		"    texture_coord.x += (grid_location.x * cell_size.x);           \n"
		"    texture_coord.y += (grid_location.y * cell_size.y);           \n"	
		"    color = vec4(0.5,0.5,0.5,1.0);                                \n"
		"}                                                                 \n"
	};


	static const std::string fs_source_animation =
	{
		"#version 330 core                                                 \n"
		"                                                                  \n"
		"smooth in vec4 color;                                             \n"
		"smooth in vec2 texture_coord;                                     \n"
		"uniform sampler2D tex;                                            \n"
		"                                                                  \n"
		"// Ouput data                                                     \n"
		"out vec4 output_color;                                            \n"
		"                                                                  \n"
		"void main(void)                                                   \n"
		"{                                                                 \n"
		"    output_color = texture(tex, texture_coord);                   \n"
		"    //int test = 1 % 2;                  \n"
		"    //output_color = vec4(0.5 , 0.5 , 0.5 , 1.0 );                \n"
		"    //output_color = texture(tex, texture_coord) * color;         \n"
		"}                                                                 \n"
	};

	

	/*

	static const std::string vs_source_animation =
	{
		"#version 330 core                                                 \n"
		"                                                                  \n"
		"layout(location = 0) in vec3 vertexPosition_modelspace;           \n"
		"layout(location = 2) in vec2 texture_pos;                         \n"
		"                                                                  \n"
		"uniform mat4 world_space;                                         \n"
		"                                                                  \n"
		"smooth out vec4 color;                                            \n"
		"smooth out vec2 texture_coord;                                    \n"
		"                                                                  \n"
		"void main(){                                                      \n"
		"    gl_Position = world_space                                     \n"
		"               * vec4( vertexPosition_modelspace , 1.0f);         \n"
		"    texture_coord = texture_pos;                                  \n"
		"    color = vec4(1.0,1.0,1.0,1.0);                                \n"
		"}                                                                 \n"
	};


	static const std::string fs_source_animation =
	{
		"#version 330 core                                                 \n"
		"                                                                  \n"
		"smooth in vec4 color;                                             \n"
		"smooth in vec2 texture_coord;                                     \n"
		"uniform sampler2D tex;                                            \n"
		"                                                                  \n"
		"// Ouput data                                                     \n"
		"out vec4 output_color;                                            \n"
		"                                                                  \n"
		"void main(void)                                                   \n"
		"{                                                                 \n"
		"    output_color = texture(tex, texture_coord);                   \n"
		"    //output_color = vec4(0.5 , 0.5 , 0.5 , 1.0 );                \n"
		"    //output_color = texture(tex, texture_coord) * color;         \n"
		"}                                                                 \n"
	};*/

	GLuint program = glCreateProgram();	
	GLuint fs = createShader(GL_FRAGMENT_SHADER, fs_source);	
	GLuint vs = createShader(GL_VERTEX_SHADER, vs_source);
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	GLuint program_texture = glCreateProgram();
	GLuint fs_tex = createShader(GL_FRAGMENT_SHADER, fs_source_texture );
	GLuint vs_tex = createShader(GL_VERTEX_SHADER, vs_source_texture );
	glAttachShader(program_texture, vs_tex);
	glAttachShader(program_texture, fs_tex);
	glLinkProgram(program_texture);

	GLuint program_animation = glCreateProgram();
	GLuint fs_ani = createShader(GL_FRAGMENT_SHADER, fs_source_animation);
	GLuint vs_ani = createShader(GL_VERTEX_SHADER, vs_source_animation);
	glAttachShader(program_animation, vs_ani);
	glAttachShader(program_animation, fs_ani);
	glLinkProgram(program_animation);

	GLint pass_fail;
	glGetProgramiv(program_animation, GL_LINK_STATUS, &pass_fail);

	if (pass_fail != GL_TRUE)
		std::cout << "WARNING DID NOT LINK" << endl;

	glDeleteShader(fs);
	glDeleteShader(vs);
	glDeleteShader(fs_tex);
	glDeleteShader(vs_tex);
	glDeleteShader(fs_ani);
	glDeleteShader(vs_ani);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	//Structure of Arrays  Triangles then Colors
	static const GLfloat g_vertex_buffer_data[] = {
		//First Triangle
		-1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,		

		//Second Triangle
		1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,

		//First Color
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,


		//Second Color
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
	};


	//Array of structures, interlace triangles, color, and texture coordinates
	//To do, use an element buffer	
	static const GLfloat g_vertex_buffer_data_texture[] = {
		//First Triangle      //Coordinates
		-1.0f, -1.0f, 0.0f,   0.0f , 0.0f,
		-1.0f, 1.0f, 0.0f,    0.0f , 1.0f,
		1.0f, -1.0f, 0.0f,    1.0f , 0.0f,

		//Second Triangle
		1.0f, 1.0f, 0.0f,    1.0f , 1.0f ,
		1.0f, -1.0f, 0.0f,   1.0f , 0.0f ,
		-1.0f, 1.0f, 0.0f,   0.0f , 1.0f , 

	};






	int texture_data_per_triangle = 5;
	//int texture_stride = texture_data_per_triangle * sizeof(g_vertex_buffer_data_texture) / sizeof(g_vertex_buffer_data_texture[0]) * sizeof(GLfloat);
	int texture_stride = texture_data_per_triangle * sizeof(GLfloat);


	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint vertexbufferTexture;
	glGenBuffers(1, &vertexbufferTexture);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbufferTexture);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data_texture), g_vertex_buffer_data_texture, GL_STATIC_DRAW);


	GLuint titleID, marioID;
	glGenTextures(1, &titleID);

	unsigned int height{0}, width{0};
	std::unique_ptr<unsigned char> data ( load_bmp("Title.bmp" , width , height ) );

	glBindTexture(GL_TEXTURE_2D, titleID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data.get() );

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	data.reset( load_bmp("Mario.bmp", width, height));
	glGenTextures(1, &marioID);
	glBindTexture(GL_TEXTURE_2D, marioID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data.get() );

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	GLuint worldTransformPos = glGetUniformLocation(program, "world_space");
	GLuint worldTransformPos1 = glGetUniformLocation(program_texture, "world_space");
	GLuint worldTransformPos2 = glGetUniformLocation(program_animation, "world_space");
	GLuint animationIndexPos = glGetUniformLocation(program_animation, "animation_index");
	GLuint spriteGridPos = glGetUniformLocation(program_animation, "sprite_grid");
	//GLuint spriteGridRowsPos = glGetUniformLocation(program_animation, "sprite_grid.columns");

	double lastTime = glfwGetTime();
	double currentTime;
	float deltaTime = 0.0f;



	Sprite paddle1, paddle2, ball;

	paddle1.setPos(-0.55f, 0.55f);
	paddle2.setPos(0.55f, 0.55f);
	ball.setPos(0.0f, 0.55f);

	paddle1.setSize(paddle_size, paddle_size);
	paddle2.setSize(paddle_size, paddle_size);
	ball.setSize(ball_size, ball_size);

	ball.setVelocity(1.0f * ball_speed, 0.0f);

	Sprite title, mario;
	title.setPos(-0.55f, -0.55f);
	title.setSize(0.15f, 0.15f);

	mario.setPos(0.55f, -0.55f);
	mario.setSize(0.15f, 0.15f);

	//colums then rows
	glm::u32vec2 sprite_grid(10,5);
	
	GLuint animationIndex = 0;
	float current_animation_time{ 0 };





	/*
	//Title
	glm::vec2 title_pos(-0.25f, -0.25f);
	glm::vec2 title_scale(0.15f, 0.15f);
	glm::mat4 title_trans = glm::translate(glm::vec3(title_pos, 0.0f)) * glm::scale(glm::vec3(title_scale, 1.0f));


	//Mario
	glm::vec2 mario_pos(-0.25f, -0.25f);
	glm::vec2 mario_scale(0.15f, 0.15f);
	glm::mat4 title_trans = glm::translate(glm::vec3(title_pos, 0.0f)) * glm::scale(glm::vec3(title_scale, 1.0f));

	*/


	//Set buffer
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)72);

	//Set second buffer
	//this is for mario title screen
	glBindBuffer(GL_ARRAY_BUFFER, vertexbufferTexture);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, texture_stride, (void*)0);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, texture_stride, (void*)(3 * sizeof(GLfloat)));
	


	do{

		currentTime = glfwGetTime();
		deltaTime =float(currentTime - lastTime);

	
		glClear(GL_COLOR_BUFFER_BIT);
		
		//Paddles
		glUseProgram(program);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

		//Paddle1
		paddle1.adjustPos(getPosFromControls(window, deltaTime, SQUARE1));
		glUniformMatrix4fv(worldTransformPos, 1, GL_FALSE, glm::value_ptr(paddle1.getWorldTransform()));		
		glDrawArrays(GL_TRIANGLES, 0, 6 ); 

		//Paddle2
		paddle2.adjustPos(getPosFromControls(window, deltaTime, SQUARE2));
		glUniformMatrix4fv(worldTransformPos, 1, GL_FALSE, glm::value_ptr(paddle2.getWorldTransform()));
		glDrawArrays(GL_TRIANGLES, 0, 6 ); 

		//Ball		
		getBallVelocity( ball.pos , ball.velocity  );
		ball.adjustPos( ball.velocity * deltaTime);

		if (collision(paddle1, ball))
			ball.velocity.x *= -1;

		if (collision(paddle2, ball))
			ball.velocity.x *= -1;
	
		glUniformMatrix4fv(worldTransformPos, 1, GL_FALSE, glm::value_ptr(ball.getWorldTransform()));	
		glDrawArrays(GL_TRIANGLES, 0, 6); // 3 indices starting at 0 -> 1 triangle		
	



		//Lets do the texture here		
		glBindTexture(GL_TEXTURE_2D, titleID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbufferTexture);
		glUseProgram(program_texture);		
		glUniformMatrix4fv(worldTransformPos1, 1, GL_FALSE, glm::value_ptr(title.getWorldTransform()));
		glDrawArrays(GL_TRIANGLES, 0, 6); // 3 indices starting at 0 -> 1 triangle	
	


		//Animation
		//glUseProgram(program_texture);
		glUseProgram(program_animation);

		glBindTexture(GL_TEXTURE_2D, marioID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbufferTexture);
	
		
		current_animation_time += deltaTime;

		if (current_animation_time > animation_speed){
			current_animation_time = 0;
			++animationIndex;
		}
		
		glUniformMatrix4fv(worldTransformPos2, 1, GL_FALSE, glm::value_ptr(mario.getWorldTransform()));
		glUniform1ui(animationIndexPos, animationIndex );
		glUniform2uiv(spriteGridPos, 1, glm::value_ptr(sprite_grid));
		
		glDrawArrays(GL_TRIANGLES, 0, 6); // 3 indices starting at 0 -> 1 triangle	

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		lastTime = currentTime;

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

//	if (data != nullptr)
//		delete [] data;

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(program);


	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}





unsigned char* load_bmp(std::string image_path, unsigned int& width, unsigned int& height){
	using namespace std;

	cout << "Reading image: " << image_path << endl;

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	//unsigned int width, height;
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = fopen(image_path.c_str(), "rb");
	if (!file)							    {
		cout << image_path << "could not be opened. Are you in the right directory ? Don't forget to read the FAQ !" << endl;
		getchar();
		return nullptr;
	}

	// Read the header, i.e. the 54 first bytes

	// If less than 54 bytes are read, problem
	if (fread(header, 1, 54, file) != 54){
		cout << "Not a correct BMP file" << endl;
		return nullptr;
	}
	// A BMP files always begins with "BM"
	if (header[0] != 'B' || header[1] != 'M'){
		cout << "Not a correct BMP file" << endl;
		return nullptr;
	}
	// Make sure this is a 24bpp file
	if (*(int*)&(header[0x1E]) != 0)         {
		cout << "Not a correct BMP file" << endl;
		return nullptr;
	}
	if (*(int*)&(header[0x1C]) != 24)         {
		cout << "Not a correct BMP file" << endl;
		return nullptr;
	}

	// Read the information about the image
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

	// Create a buffer
	data = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);

	// Everything is in memory now, the file wan be closed
	fclose(file);

	return data;
}











glm::vec2 getCollisionVector(Sprite& sprite1, Sprite& sprite2){
	float x = sprite1.pos.x - sprite1.pos.x;
	float y = sprite1.pos.y - sprite1.pos.y;

	return glm::vec2();
}

bool collision(Sprite& sprite1, Sprite& sprite2){
	Rectangle box1 = sprite1.getBoundingBox();
	Rectangle box2 = sprite2.getBoundingBox();

	if (box1.bottom > box2.top) return false;
	if (box1.top < box2.bottom) return false;
	if (box1.right < box2.left) return false;
	if (box1.left > box2.right) return false;

	return true;

}

glm::vec2& getBallVelocity(glm::vec2& pos, glm::vec2& velocity ){

	float xsize = ball_size;	
	float ysize = ball_size;

	if ((pos.x - xsize) < -1) {
		//std::cout << "Pos x: " << pos.x << " Negative Collision" << std::endl;
		//pos.x = -1 + xsize; // may want to comment out?
		velocity.x *= -1;
		
	}

	if ((pos.x + xsize) > 1) {
		//std::cout << "Pos x: " << pos.x << "Positive Collision" << std::endl;
		//pos.x = 1 - xsize; // may want to comment out?
		velocity.x *=-1;
		
	}


	if ((pos.y - ysize) < -1) {
		//std::cout << "Pos x: " << pos.x << " Negative Collision" << std::endl;
		//pos.x = -1 + xsize; // may want to comment out?
		velocity.y *= -1;

	}

	if ((pos.y + ysize) > 1) {
		//std::cout << "Pos x: " << pos.x << "Positive Collision" << std::endl;
		//pos.x = 1 - xsize; // may want to comment out?
		velocity.y *= -1;

	}

	return velocity;
}

glm::vec2 getPosFromControls(GLFWwindow* window , double deltaTime, SQUARE square){
	double xpos = 0;
	double ypos = 0;

	int upKey = GLFW_KEY_UP, downKey = GLFW_KEY_DOWN, leftKey = GLFW_KEY_LEFT, rightKey = GLFW_KEY_RIGHT;

	switch (square){
	case SQUARE1:
		upKey = GLFW_KEY_W;
		downKey = GLFW_KEY_S;
		leftKey = GLFW_KEY_A;
		rightKey = GLFW_KEY_D;
		break;
	case SQUARE2:	
		break;
	
	}

	glm::vec2 pos(0.0f, 0.0f);

	// Move forward
	if (glfwGetKey(window, upKey) == GLFW_PRESS)
		pos.y += 1;
	if (glfwGetKey(window, downKey) == GLFW_PRESS)
		pos.y -= 1;
	if (glfwGetKey(window, leftKey) == GLFW_PRESS)
		pos.x -= 1;
	if (glfwGetKey(window, rightKey) == GLFW_PRESS)
		pos.x += 1;
	
	if (glm::length(pos))
		glm::normalize(pos);
	
	pos *= (deltaTime * speed);	

	return pos;

}

GLuint createShader(GLenum eShaderType, const std::string &strShaderFile)
{
	GLuint shader = glCreateShader(eShaderType);
	const char *strFileData = strShaderFile.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	const char *strShaderType = NULL;
	switch (eShaderType)
	{
		case GL_VERTEX_SHADER: strShaderType = "Vertex"; break;
		case GL_GEOMETRY_SHADER: strShaderType = "Geometry"; break;
		case GL_FRAGMENT_SHADER: strShaderType = "Fragment"; break;
	}

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

	

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}
	else {
		fprintf(stderr, "%s Shader Compile Succesful\n" , strShaderType );
	}

	return shader;
}





void getGLVersionInfo(){

	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(96, 96, "Get Version Info", NULL, NULL);

	//window = glfwCreateWindow(200, 200, "Version", NULL, NULL);

	cout << "GL Load Succesful" << endl;
	int api = 0, major = 1, minor = 0, revision;

	api = glfwGetWindowAttrib(window, GLFW_CLIENT_API);
	major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
	minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
	revision = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);

	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	glfwMakeContextCurrent(window);

	std::string  api_string = (api == GLFW_OPENGL_API) ? "OpenGl" : "OpenGL ES";

	printf("%s context version string: \"%s\"\n",
		api_string.c_str(),
		glGetString(GL_VERSION));

	printf("%s context version parsed by GLFW: %u.%u.%u\n",
		api_string.c_str(),
		major, minor, revision);


	printf("%s context renderer string: \"%s\"\n",
		api_string.c_str(),
		glGetString(GL_RENDERER));
	printf("%s context vendor string: \"%s\"\n",
		api_string.c_str(),
		glGetString(GL_VENDOR));

	if (major > 1)
	{
		printf("%s context shading language version: \"%s\"\n",
			api_string.c_str(),
			glGetString(GL_SHADING_LANGUAGE_VERSION));
	}


	glfwSetWindowShouldClose(window, true);
	glfwDestroyWindow(window);
	glfwWindowHint(GLFW_VISIBLE, GL_TRUE);

}



