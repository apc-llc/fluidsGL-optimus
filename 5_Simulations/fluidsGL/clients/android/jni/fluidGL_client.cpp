#include "fluidGL_client.h"

#include <android/log.h>
#include <cstdio>
#include <cstdlib>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <memory>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <vector>

#define DEBUG 1

#define LOG_TAG "GLES3JNI"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#if DEBUG
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#define ALOGV(...)
#endif

static bool checkGlError(const char* funcName) {
	GLint err = glGetError();
	if (err != GL_NO_ERROR) {
		ALOGE("GL error after %s(): 0x%08x\n", funcName, err);
		return true;
	}
	return false;
}

static GLuint createShader(GLenum shaderType, const char* src) {
	GLuint shader = glCreateShader(shaderType);
	if (!shader) {
		checkGlError("glCreateShader");
		return 0;
	}
	glShaderSource(shader, 1, &src, NULL);

	GLint compiled = GL_FALSE;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLogLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
		if (infoLogLen > 0) {
			GLchar* infoLog = (GLchar*)malloc(infoLogLen);
			if (infoLog) {
				glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
				ALOGE("Could not compile %s shader:\n%s\n",
						shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
						infoLog);
				free(infoLog);
			}
		}
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint createProgram(const char* vtxSrc, const char* fragSrc) {
	GLuint vtxShader = 0;
	GLuint fragShader = 0;
	GLuint program = 0;
	GLint linked = GL_FALSE;

	vtxShader = createShader(GL_VERTEX_SHADER, vtxSrc);
	if (!vtxShader)
		goto exit;

	fragShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
	if (!fragShader)
		goto exit;

	program = glCreateProgram();
	if (!program) {
		checkGlError("glCreateProgram");
		goto exit;
	}
	glAttachShader(program, vtxShader);
	glAttachShader(program, fragShader);

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		ALOGE("Could not link program");
		GLint infoLogLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
		if (infoLogLen) {
			GLchar* infoLog = (GLchar*)malloc(infoLogLen);
			if (infoLog) {
				glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
				ALOGE("Could not link program:\n%s\n", infoLog);
				free(infoLog);
			}
		}
		glDeleteProgram(program);
		program = 0;
	}

exit:
	glDeleteShader(vtxShader);
	glDeleteShader(fragShader);
	return program;
}

using namespace std;

string vertex_shader_format =
	"#version 300 es\n"
    "layout(location = 0) in vec2 pos;\n"
	"out vec4 vColor;\n"
	"void main() {\n"
    "    gl_Position = vec4(pos.x * 2.0f - 1.0f, -(pos.y * 2.0f - 1.0f), 1.0f, 1.0f);\n"
    "    gl_PointSize = %ff;\n"
    "    vColor = vec4(1.f, 1.f, 1.f, 0.5f);\n"
	"}\n";

string fragment_shader_format =
	"#version 300 es\n"
	"precision mediump float;\n"
	"out vec4 outColor;\n"
	"void main() {\n"
	"	outColor = vec4(1.f, 1.f, 1.f, 0.5f);\n"
	"}\n";

static int width, height;
static int DS, cData;

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static int wWidth = 0;
static int wHeight = 0;

static int clicked = 0, fullscreen = 0;

GLuint vbo = 0;                 // OpenGL vertex buffer object
static char *particles = NULL;  // particle positions in host memory
static int lastx = 0, lasty = 0;

GLuint mProgram;
GLuint mVBState;

float radius = 1.0f;            // Particle radius

#include "UdpBroadcastClient.h"

std::auto_ptr<UdpBroadcastClient> client;

void on_surface_created()
{
    ALOGV("on_surface_created");
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void on_surface_changed(int wWidth_, int wHeight_)
{
    ALOGV("on_surface_changed");

    wWidth = wWidth_;
    wHeight = wHeight_;

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glGenVertexArrays(1, &mVBState);
    glBindVertexArray(mVBState);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
}

void on_draw_frame()
{
    ALOGV("on_draw_frame()");

	if (!mProgram)
	{
		// Substitute radius into vertex shader code.
		vector<char> vvertex_shader;
		vvertex_shader.resize(snprintf(NULL, 0, vertex_shader_format.c_str(), radius) + 1);
		char* vertex_shader = &vvertex_shader[0];
		sprintf(vertex_shader, vertex_shader_format.c_str(), radius);

		// Substitute radius into fragment shader code.
		vector<char> vfragment_shader;
		vfragment_shader.resize(snprintf(NULL, 0, "%s", fragment_shader_format.c_str()) + 1);
		char* fragment_shader = &vfragment_shader[0];
		sprintf(fragment_shader, "%s", fragment_shader_format.c_str());

		// Compile vertex and fragment shaders.
		mProgram = createProgram(vertex_shader, fragment_shader);
		if (!mProgram)
			exit(1);
	}

	if (particles)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, cData * DS, particles, GL_DYNAMIC_DRAW);
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(1.f/256*172, 1.f/256*101, 1.f/256*4, 0.f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	if (mProgram)
		glUseProgram(mProgram);
	if (particles)
	{
		glBindVertexArray(mVBState);
		glDrawArrays(GL_POINTS, 0, DS);
	}
	glDisable(GL_TEXTURE_2D);

	// Do not draw faster than 30 FPS - does not make sense anyways.
	usleep(1e6 / 30);
}

void* broadcast_listener(void* args)
{
	ALOGV("Started broadcast listener thread");

	// Listen to the broadcast and update the corresponding data
	// with the received packets.
	// TODO send connection rate to be depicted in the activity title.
	client->listen(particles, NULL);

	return NULL;
}

void on_connect(const char* bc_addr)
{
    ALOGV("on_connect(%s)", bc_addr);

	client.reset(new UdpBroadcastClient(bc_addr));

	printf("Waiting for the server...\n");

	DisplayConfig config;
	client->configure(config);

	width = config.width;
	height = config.height;
	DS = width * height;
	cData = config.szpoint;
	ALOGV("Display config: %d x %d x %d\n", width, height, cData);

	ALOGV("Allocating particles array");

    // Create particle array in host memory
    // Extra packet size - to pad broadcast messages.
    if (particles)
    {
        // First allocate new array, then delete old
        // (should avoid races with frame drawer).
        char* particlesOld = particles;
	    particles = (char*)malloc(cData * DS + UdpBroadcastServer::PacketSize);
	    free(particlesOld);
    }
    else
    {
        particles = (char*)malloc(cData * DS + UdpBroadcastServer::PacketSize);
    }
    memset(particles, 0, cData * DS);

	ALOGV("Compiling shaders");

	// Calculate the point size based on problem size
	// relative to screen size and force shaders recompilation.
	radius = 0.9f * MIN((float)wWidth / width, (float)wHeight / height);
	mProgram = 0;

	ALOGV("Starting broadcast listener thread");

    // Listen to the particles state broadcast in the separate thread.
    // TODO Cancel this thread, if already exits.
    pthread_t tid;
    pthread_create(&tid, NULL, &broadcast_listener, NULL);
}

