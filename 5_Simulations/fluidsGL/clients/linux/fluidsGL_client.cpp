#include "defines.h"
#undef cData
#undef DIM
#undef DS

// OpenGL Graphics includes
#include <GL/glew.h>
#if defined(__APPLE__) || defined(MACOSX)
  #include <GLUT/glut.h>
  #ifndef glutCloseFunc
  #define glutCloseFunc glutWMCloseFunc
  #endif
#else
#include <GL/freeglut.h>
#endif

#ifdef WIN32
bool IsOpenGLAvailable(const char *appName)
{
    return true;
}
#else
#if (defined(__APPLE__) || defined(MACOSX))
bool IsOpenGLAvailable(const char *appName)
{
    return true;
}
#else
// check if this is a linux machine
#include <X11/Xlib.h>

bool IsOpenGLAvailable(const char *appName)
{
    Display *Xdisplay = XOpenDisplay(NULL);

    if (Xdisplay == NULL)
    {
        return false;
    }
    else
    {
        XCloseDisplay(Xdisplay);
        return true;
    }
}
#endif
#endif

#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

void cleanup(void);
void reshape(int x, int y);

static int width, height;
static int DS, cData;

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static int wWidth = 0;
static int wHeight = 0;

static int clicked = 0, fullscreen = 0;

GLuint vbo = 0;                 // OpenGL vertex buffer object
static char *particles = NULL;  // particle positions in host memory
static int lastx = 0, lasty = 0;

bool g_bExitESC = false;

#include "UdpBroadcastClient.h"

#include <memory>
std::auto_ptr<UdpBroadcastClient> client;

#include <cstdio>
#include <malloc.h>
#include <string.h>
#include <pthread.h>

double connection = 0;

void display(void)
{
    // render points from vertex buffer
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(1.f/256*172, 1.f/256*101, 1.f/256*4, 0.f);
    glColor4f(1.f, 1.f, 1.f, 0.5f);
    glPointSize(1);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, cData * DS,
                    particles, GL_DYNAMIC_DRAW_ARB);
    glVertexPointer(2, GL_FLOAT, 0, NULL);
    glDrawArrays(GL_POINTS, 0, DS);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);

    glutSwapBuffers();

    char fps[256];
    sprintf(fps, "Caffe Macchiato / Stable Fluids (%d x %d): broadcast @ %f MB/sec", width, height, connection);
    glutSetWindowTitle(fps);

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27:
            g_bExitESC = true;
            exit(EXIT_SUCCESS);
            break;

        case 'f':
        	if (!fullscreen)
        	{
        		fullscreen = 1;
				glutFullScreenToggle();
			}
			else
			{
				fullscreen = 0;
				glutLeaveFullScreen();
			}
			break;

        case 'r':
            break;

        default:
            break;
    }
}

void click(int button, int updown, int x, int y)
{
    lastx = x;
    lasty = y;
    clicked = !clicked;
}

void motion(int x, int y)
{
    // Convert motion coordinates to domain
    float fx = (lastx / (float)wWidth);
    float fy = (lasty / (float)wHeight);
    int nx = (int)(fx * width);
    int ny = (int)(fy * height);

    if (clicked && nx < width-FR && nx > FR-1 && ny < height-FR && ny > FR-1)
    {
        int ddx = x - lastx;
        int ddy = y - lasty;
        fx = ddx / (float)wWidth;
        fy = ddy / (float)wHeight;
        int spy = ny-FR;
        int spx = nx-FR;
        //addForces(dvfield, DIM, DIM, spx, spy, FORCE * DT * fx, FORCE * DT * fy, FR);
        lastx = x;
        lasty = y;
    }

    glutPostRedisplay();
}

void reshape(int x, int y)
{
    wWidth = x;
    wHeight = y;
    glViewport(0, 0, x, y);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glutPostRedisplay();
}

void cleanup(void)
{
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glDeleteBuffersARB(1, &vbo);
}

int initGL(int *argc, char **argv)
{
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(wWidth, wHeight);
    glutCreateWindow("Compute Stable Fluids");
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(click);
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);

    glewInit();

    return true;
}

void* broadcast_listener(void* args)
{
	// Listen to the broadcast and update the corresponding data
	// with the received packets.
	client->listen(particles, &connection);

	return NULL;
}

int main(int argc, char* argv[])
{
	const char *bc_addr = "127.255.255.2:9097";

	// Broadcast address
	if (argc > 1)
		bc_addr = argv[1];

	client.reset(new UdpBroadcastClient(bc_addr));

	printf("Waiting for the server...\n");

	DisplayConfig config;
	client->configure(config);

	width = config.width;
	height = config.height;
	DS = width * height;
	cData = config.szpoint;
	printf("Display config: %d x %d x %d\n", width, height, cData);

	wWidth  = MAX(512, width);
	wHeight = MAX(512, height);

    // First initialize OpenGL context.
    if (false == initGL(&argc, argv))
    {
        exit(EXIT_SUCCESS);
    }

    // Create particle array in host memory
    // Extra packet size - to pad broadcast messages.
    particles = (char*)malloc(cData * DS + UdpBroadcastServer::PacketSize);
    memset(particles, 0, cData * DS);

    glGenBuffersARB(1, &vbo);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, cData * DS,
                    particles, GL_DYNAMIC_DRAW_ARB);

    // Allocate and initialize host data
    GLint bsize;
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bsize);

    if (bsize != (cData * DS))
        goto EXTERR;

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    
    // Listen to the particles state broadcast in the separate thread.
    pthread_t tid;
    pthread_create(&tid, NULL, &broadcast_listener, NULL);

#if defined (__APPLE__) || defined(MACOSX)
    atexit(cleanup);
#else
    glutCloseFunc(cleanup);
#endif
    glutMainLoop();
	
	free(particles);

	return 0;

EXTERR:
    printf("Failed to initialize GL extensions.\n");

    exit(EXIT_FAILURE);
}

