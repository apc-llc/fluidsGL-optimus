#include "canvas.h"

#include <android/log.h>
#include <cstdio>
#include <cstdlib>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>
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
    "    gl_Position = vec4(pos.x * 2.0f - 1.0f, pos.y * 2.0f - 1.0f, 1.0f, 1.0f);\n"
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

GLuint vbo;
GLuint mProgram;
GLuint mVBState;

const int nparticles = 8192;
float particles[nparticles * 2];

void on_surface_created()
{
	vector<char> vvertex_shader;
	vvertex_shader.resize(snprintf(NULL, 0, vertex_shader_format.c_str(), 4.0f) + 1);
	char* vertex_shader = &vvertex_shader[0];
	sprintf(vertex_shader, vertex_shader_format.c_str(), 4.0f);

	vector<char> vfragment_shader;
	vfragment_shader.resize(snprintf(NULL, 0, "%s", fragment_shader_format.c_str()) + 1);
	char* fragment_shader = &vfragment_shader[0];
	sprintf(fragment_shader, "%s", fragment_shader_format.c_str());

	mProgram = createProgram(vertex_shader, fragment_shader);
	if (!mProgram)
		exit(1);

	float finvrandmax = 1.0f / RAND_MAX;
	for (int i = 0; i < nparticles * 2; i++)
		particles[i] = rand() * finvrandmax;

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * nparticles, particles, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &mVBState);
    glBindVertexArray(mVBState);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
}

void on_surface_changed() {
	// No-op
}

void on_draw_frame()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(1.f/256*172, 1.f/256*101, 1.f/256*4, 0.f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glUseProgram(mProgram);
	glBindVertexArray(mVBState);
	glDrawArrays(GL_POINTS, 0, nparticles);
	glDisable(GL_TEXTURE_2D);
}

