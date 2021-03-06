/*
 * Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */
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

// Includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// CUDA standard includes
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

// CUDA FFT Libraries
#include <cufft.h>

// CUDA helper functions
#include <helper_functions.h>
#include <rendercheck_gl.h>
#include <helper_cuda.h>
#include <helper_cuda_gl.h>

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

#include "defines.h"
#include "fluidsGL_kernels.h"

#ifdef BROADCAST
#include "half.hpp"
#include "UdpBroadcastServer.h"
#include <memory>
#include <pthread.h>
#include <unistd.h>
#endif

#define MAX_EPSILON_ERROR 1.0f

const char *sSDKname = "fluidsGL";
// CUDA example code that implements the frequency space version of
// Jos Stam's paper 'Stable Fluids' in 2D. This application uses the
// CUDA FFT library (CUFFT) to perform velocity diffusion and to
// force non-divergence in the velocity field at each time step. It uses
// CUDA-OpenGL interoperability to update the particle field directly
// instead of doing a copy to system memory before drawing. Texture is
// used for automatic bilinear interpolation at the velocity advection step.

void cleanup(void);
void reshape(int x, int y);

// CUFFT plan handle
cufftHandle planr2c;
cufftHandle planc2r;
static cData *vxfield = NULL;
static cData *vyfield = NULL;

cData *hvfield = NULL;
cData *dvfield = NULL;
static int wWidth  = MAX(512, DIM);
static int wHeight = MAX(512, DIM);

static int clicked = 0, fullscreen = 0;
static int fpsCount = 0;
static int fpsLimit = 1;
StopWatchInterface *timer = NULL;

// Particle data
GLuint vbo = 0;                 // OpenGL vertex buffer object
struct cudaGraphicsResource *cuda_vbo_resource; // handles OpenGL-CUDA exchange
#if !defined(OPTIMUS) && !defined(BROADCAST)
static cData *particles = NULL; // particle positions in host memory
#else
cData *particles = NULL; // particle positions in host memory
#if defined(BROADCAST)
char *packets = NULL; // particle packets for client broadcast
#endif
cData *particles_gpu = NULL; // particle positions in device memory
#endif
static int lastx = 0, lasty = 0;

// Texture pitch
size_t tPitch = 0; // Now this is compatible with gcc in 64-bit

char *ref_file         = NULL;
bool g_bQAAddTestForce = true;
int  g_iFrameToCompare = 100;
int  g_TotalErrors     = 0;

bool g_bExitESC = false;

// CheckFBO/BackBuffer class objects
CheckRender       *g_CheckRender = NULL;

// Visualization broadcasting server
#ifdef BROADCAST
std::auto_ptr<UdpBroadcastServer> server;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
static int wstep, hstep;
#endif

void autoTest();

extern "C" void addForces(cData *v, int dx, int dy, int spx, int spy, float fx, float fy, int r);
extern "C" void advectVelocity(cData *v, float *vx, float *vy, int dx, int pdx, int dy, float dt);
extern "C" void diffuseProject(cData *vx, cData *vy, int dx, int dy, float dt, float visc);
extern "C" void updateVelocity(cData *v, float *vx, float *vy, int dx, int pdx, int dy);
extern "C" void advectParticles(GLuint vbo, cData *v, int dx, int dy, float dt);


void simulateFluids(void)
{
    // simulate fluid
    advectVelocity(dvfield, (float *)vxfield, (float *)vyfield, DIM, RPADW, DIM, DT);
    diffuseProject(vxfield, vyfield, CPADW, DIM, DT, VIS);
    updateVelocity(dvfield, (float *)vxfield, (float *)vyfield, DIM, RPADW, DIM);
    advectParticles(vbo, dvfield, DIM, DIM, DT);
}

void display(void)
{
	pthread_mutex_lock(&display_mutex);

    if (!ref_file)
    {
        sdkStartTimer(&timer);
        simulateFluids();
    }

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
#ifdef OPTIMUS
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(cData) * DS,
                    particles, GL_DYNAMIC_DRAW_ARB);
#endif
    glVertexPointer(2, GL_FLOAT, 0, NULL);
    glDrawArrays(GL_POINTS, 0, DS);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);

    if (ref_file)
    {
        return;
    }

    // Finish timing before swap buffers to avoid refresh sync
    sdkStopTimer(&timer);
    glutSwapBuffers();

    fpsCount++;

    if (fpsCount == fpsLimit)
    {
        char fps[256];
        float ifps = 1.f / (sdkGetAverageTimerValue(&timer) / 1000.f);
        sprintf(fps, "Caffe Macchiato / Stable Fluids (%d x %d): %3.1f fps", DIM, DIM, ifps);
        glutSetWindowTitle(fps);
        fpsCount = 0;
        fpsLimit = (int)MAX(ifps, 1.f);
        sdkResetTimer(&timer);
    }

    glutPostRedisplay();

	pthread_mutex_unlock(&display_mutex);
}

void autoTest(char **argv)
{
    CFrameBufferObject *fbo = new CFrameBufferObject(wWidth, wHeight, 4, false, GL_TEXTURE_2D);
    g_CheckRender = new CheckFBO(wWidth, wHeight, 4, fbo);
    g_CheckRender->setPixelFormat(GL_RGBA);
    g_CheckRender->setExecPath(argv[0]);
    g_CheckRender->EnableQAReadback(true);

    fbo->bindRenderPath();

    reshape(wWidth, wHeight);

    for (int count=0; count<g_iFrameToCompare; count++)
    {
        simulateFluids();

        // add in a little force so the automated testing is interesing.
        if (ref_file)
        {
            int x = wWidth/(count+1);
            int y = wHeight/(count+1);
            float fx = (x / (float)wWidth);
            float fy = (y / (float)wHeight);
            int nx = (int)(fx * DIM);
            int ny = (int)(fy * DIM);

            int ddx = 35;
            int ddy = 35;
            fx = ddx / (float)wWidth;
            fy = ddy / (float)wHeight;
            int spy = ny-FR;
            int spx = nx-FR;

            addForces(dvfield, DIM, DIM, spx, spy, FORCE * DT * fx, FORCE * DT * fy, FR);
            lastx = x;
            lasty = y;
        }
    }

    display();

    fbo->unbindRenderPath();

    // compare to offical reference image, printing PASS or FAIL.
    printf("> (Frame %d) Readback BackBuffer\n", 100);
    g_CheckRender->readback(wWidth, wHeight);
    g_CheckRender->savePPM("fluidsGL.ppm", true, NULL);

    if (!g_CheckRender->PPMvsPPM("fluidsGL.ppm", ref_file, MAX_EPSILON_ERROR, 0.25f))
    {
        g_TotalErrors++;
    }
}

// very simple von neumann middle-square prng.  can't use rand() in -qatest
// mode because its implementation varies across platforms which makes testing
// for consistency in the important parts of this program difficult.
float myrand(void)
{
    static int seed = 72191;
    char sq[22];

    if (ref_file)
    {
        seed *= seed;
        sprintf(sq, "%010d", seed);
        // pull the middle 5 digits out of sq
        sq[8] = 0;
        seed = atoi(&sq[3]);

        return seed/99999.f;
    }
    else
    {
        return rand()/(float)RAND_MAX;
    }
}

void initParticles(cData *p, int dx, int dy)
{
    for (int i = 0; i < dy; i++)
    {
        for (int j = 0; j < dx; j++)
        {
            p[i*dx+j].x = (j+0.5f+(myrand() - 0.5f))/dx;
            p[i*dx+j].y = (i+0.5f+(myrand() - 0.5f))/dy;
        }
    }
    
    unsigned char* data;
    unsigned int width, height;
    bool loaded = sdkLoadPPM4ub("data/usi.ppm", &data, &width, &height);
    if (!loaded) return;
    
    const char* no_label = getenv("NO_LABEL");
    if (!no_label)
    {
        const int nchannels = 4;
        for (int j = 0; j < height; j++)
    	    for (int i = 0; i < width; i++)
            {
                unsigned char* pixel = &data[nchannels * (width * j + i)];
                if ((pixel[0] == 0) && (pixel[1] == 0) && (pixel[2] == 0))
                {
                    p[j*dx+i].x = 0;
                    p[j*dx+i].y = 0;
                }
            }
    }
#ifdef BROADCAST
    // Randomly reorder particles to eliminate initial jittering
    // in UDP client.
    for (int i = 0; i < dy; i++)
    {
        for (int j = 0; j < dx; j++)
        {
        	int i_rand = myrand() * (dx - 1);
        	int j_rand = myrand() * (dy - 1);

        	cData* p0 = &p[j*dx+i];        	
        	cData* p1 = &p[j_rand*dx+i_rand];
        	
        	cData p2 = *p0;
        	*p0 = *p1;
        	*p1 = p2;
        }
    }
#endif
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
			pthread_mutex_lock(&display_mutex);
            memset(hvfield, 0, sizeof(cData) * DS);
            cudaMemcpy(dvfield, hvfield, sizeof(cData) * DS,
                       cudaMemcpyHostToDevice);

            initParticles(particles, DIM, DIM);

#ifndef OPTIMUS
            cudaGraphicsUnregisterResource(cuda_vbo_resource);
            getLastCudaError("cudaGraphicsUnregisterBuffer failed");
#endif

#if defined(OPTIMUS) || defined(BROADCAST)
            cudaMemcpy(particles_gpu, particles, sizeof(cData) * DS, cudaMemcpyHostToDevice);
#endif

            glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
            glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(cData) * DS,
                            particles, GL_DYNAMIC_DRAW_ARB);
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

#ifndef OPTIMUS
            cudaGraphicsGLRegisterBuffer(&cuda_vbo_resource, vbo, cudaGraphicsMapFlagsNone);
            getLastCudaError("cudaGraphicsGLRegisterBuffer failed");
#endif
			pthread_mutex_unlock(&display_mutex);
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
    int nx = (int)(fx * DIM);
    int ny = (int)(fy * DIM);

    if (clicked && nx < DIM-FR && nx > FR-1 && ny < DIM-FR && ny > FR-1)
    {
        int ddx = x - lastx;
        int ddy = y - lasty;
        fx = ddx / (float)wWidth;
        fy = ddy / (float)wHeight;
        int spy = ny-FR;
        int spx = nx-FR;
        addForces(dvfield, DIM, DIM, spx, spy, FORCE * DT * fx, FORCE * DT * fy, FR);
        lastx = x;
        lasty = y;
    }

    glutPostRedisplay();
}

void motion(int x, int y, int nx, int ny, float fx, float fy, int spy, int spx)
{
    if (clicked && nx < DIM-FR && nx > FR-1 && ny < DIM-FR && ny > FR-1)
    {
        addForces(dvfield, DIM, DIM, spx, spy, FORCE * DT * fx, FORCE * DT * fy, FR);
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
    cudaGraphicsUnregisterResource(cuda_vbo_resource);

    unbindTexture();
    deleteTexture();

    // Free all host and device resources
    free(hvfield);
    free(particles);
#ifdef BROADCAST
	free(packets);
#endif
    cudaFree(dvfield);
    cudaFree(vxfield);
    cudaFree(vyfield);
    cufftDestroy(planr2c);
    cufftDestroy(planc2r);

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glDeleteBuffersARB(1, &vbo);

    sdkDeleteTimer(&timer);
}

int initGL(int *argc, char **argv)
{
    if (IsOpenGLAvailable(sSDKname))
    {
        fprintf(stderr, "   OpenGL device is Available\n");
    }
    else
    {
        fprintf(stderr, "   OpenGL device is NOT Available, [%s] exiting...\n", sSDKname);
        exit(EXIT_SUCCESS);
    }

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

    if (! glewIsSupported(
            "GL_ARB_vertex_buffer_object"
        ))
    {
        fprintf(stderr, "ERROR: Support for necessary OpenGL extensions missing.");
        fflush(stderr);
        return false;
    }

    return true;
}

#ifdef BROADCAST
void* feedback_listener(void* args)
{
	for (;;)
	{
		FeedbackConfig config;
	
		// Listen to the feedback from clients.
		server->feedback(config);
		
		if (config.reset)
		{
			memset(particles, 0, sizeof(cData) * DS);
			keyboard('r', 0, 0);
		}

		if (config.click)
		{
			lastx = config.lastx * wstep;
			lasty = config.lasty * hstep;
			clicked = config.clicked;
		}
		
		if (config.motion)
		{
			pthread_mutex_lock(&display_mutex);
			lastx = config.lastx * wstep;
			lasty = config.lasty * hstep;
			motion(config.lastx * wstep, config.lasty * hstep, config.nx, config.ny,
				config.fx, config.fy, config.spy, config.spx);
			pthread_mutex_unlock(&display_mutex);
		}
	}
}

void* broadcaster(void* args)
{
	int step = *(int*)args;
	int wstep = step, hstep = step;
	int npackets = sizeof(float) * (DIM / wstep) * (DIM / hstep) / UdpBroadcastServer::PacketSize;
	if (sizeof(float) * (DIM / wstep) * (DIM / hstep) % UdpBroadcastServer::PacketSize)
		npackets++;

	for (;;)
	{	
		// Split broadcast message into smaller packets,
		// each assigned with an index for ordered collection.
		// Compress float to half-float to save bandwidth.
		for (int i = 0; i < npackets; i++)
		{
			char* packet = packets + i * (UdpBroadcastServer::PacketSize + sizeof(unsigned int));
			unsigned int* index = (unsigned int*)packet;
			*index = i + 1;
			float* dst = (float*)(index + 1);
			cData* src = (cData*)((char*)particles + 2 * i * UdpBroadcastServer::PacketSize);
			for (int j = 0, e = UdpBroadcastServer::PacketSize / sizeof(float); j != e; j++)
			{
				half_float::half* halfs = (half_float::half*)&dst[j];
				halfs[0] = half_float::half_cast<half_float::half,std::round_to_nearest>(src[j * wstep].x);
				halfs[1] = half_float::half_cast<half_float::half,std::round_to_nearest>(src[j * wstep].y);
			}
		}

		// Broadcast the current particles array to listeners.
		server->broadcast(packets, DIM, DIM, sizeof(cData), wstep, hstep);

		// Do not send data faster than 30 FPS - does not make sense anyways,
		// and also does not overheat the network router.
		usleep(1e6 / 30);
   	}
	
	return NULL;
}
#endif

int main(int argc, char **argv)
{
    int devID;
    cudaDeviceProp deviceProps;
    printf("%s Starting...\n\n", sSDKname);
    printf("[%s] - [OpenGL/CUDA simulation] starting...\n", sSDKname);

    // First initialize OpenGL context, so we can properly set the GL for CUDA.
    // This is necessary in order to achieve optimal performance with OpenGL/CUDA interop.
    if (false == initGL(&argc, argv))
    {
        exit(EXIT_SUCCESS);
    }

    // use command-line specified CUDA device, otherwise use device with highest Gflops/s
#ifndef OPTIMUS
    devID = findCudaGLDevice(argc, (const char **)argv);
#else
    devID = gpuGetMaxGflopsDeviceId();
#endif

    // get number of SMs on this GPU
    checkCudaErrors(cudaGetDeviceProperties(&deviceProps, devID));
    printf("CUDA device [%s] has %d Multi-Processors\n",
           deviceProps.name, deviceProps.multiProcessorCount);

    // automated build testing harness
    if (checkCmdLineFlag(argc, (const char **)argv, "file"))
    {
        getCmdLineArgumentString(argc, (const char **)argv, "file", &ref_file);
    }

    // Allocate and initialize host data
    GLint bsize;

    sdkCreateTimer(&timer);
    sdkResetTimer(&timer);

    hvfield = (cData *)malloc(sizeof(cData) * DS);
    memset(hvfield, 0, sizeof(cData) * DS);

    // Allocate and initialize device data
    cudaMallocPitch((void **)&dvfield, &tPitch, sizeof(cData)*DIM, DIM);

    cudaMemcpy(dvfield, hvfield, sizeof(cData) * DS,
               cudaMemcpyHostToDevice);
    // Temporary complex velocity field data
    cudaMalloc((void **)&vxfield, sizeof(cData) * PDS);
    cudaMalloc((void **)&vyfield, sizeof(cData) * PDS);

    setupTexture(DIM, DIM);
    bindTexture();

    // Create particle array in host memory
    particles = (cData *)malloc(sizeof(cData) * DS);
    memset(particles, 0, sizeof(cData) * DS);

#ifdef BROADCAST
	int step = 1;

	// Broadcasted visualization stepping.
	if (argc > 3)
		step = atoi(argv[3]);

	// Create additional space to store particle packets
	// for broadcasting.
	wstep = step; hstep = step;
	int npackets = sizeof(float) * (DIM / wstep) * (DIM / hstep) / UdpBroadcastServer::PacketSize;
	if (sizeof(float) * (DIM / wstep) * (DIM / hstep) % UdpBroadcastServer::PacketSize)
		npackets++;
	packets = (char*)malloc(npackets *
		(UdpBroadcastServer::PacketSize + sizeof(unsigned int)));
#endif

    initParticles(particles, DIM, DIM);

#if defined(OPTIMUS) || defined(BROADCAST)
    // Create particle array in device memory
    cudaMalloc((void **)&particles_gpu, sizeof(cData) * DS);
    cudaMemcpy(particles_gpu, particles, sizeof(cData) * DS, cudaMemcpyHostToDevice);
#endif

    // Create CUFFT transform plan configuration
    cufftPlan2d(&planr2c, DIM, DIM, CUFFT_R2C);
    cufftPlan2d(&planc2r, DIM, DIM, CUFFT_C2R);
    // TODO: update kernels to use the new unpadded memory layout for perf
    // rather than the old FFTW-compatible layout
    cufftSetCompatibilityMode(planr2c, CUFFT_COMPATIBILITY_FFTW_PADDING);
    cufftSetCompatibilityMode(planc2r, CUFFT_COMPATIBILITY_FFTW_PADDING);

    glGenBuffersARB(1, &vbo);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(cData) * DS,
                    particles, GL_DYNAMIC_DRAW_ARB);

    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bsize);

    if (bsize != (sizeof(cData) * DS))
        goto EXTERR;

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

#ifndef OPTIMUS
    checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_vbo_resource, vbo, cudaGraphicsMapFlagsNone));
    getLastCudaError("cudaGraphicsGLRegisterBuffer failed");
#endif

    if (ref_file)
    {
        autoTest(argv);
        cleanup();

        // cudaDeviceReset causes the driver to clean up all state. While
        // not mandatory in normal operation, it is good practice.  It is also
        // needed to ensure correct operation when the application is being
        // profiled. Calling cudaDeviceReset causes all profile data to be
        // flushed before the application exits
        cudaDeviceReset();
        printf("[fluidsGL] - Test Results: %d Failures\n", g_TotalErrors);
        exit(g_TotalErrors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);

    }
    else
    {
#ifdef BROADCAST
		const char *sv_addr = "127.0.0:9097";
		const char *bc_addr = "127.255.255.2:9097";

		// Server address
		if (argc > 2)
			sv_addr = argv[2];

		// Broadcast address
		if (argc > 1)
			bc_addr = argv[1];

		server.reset(new UdpBroadcastServer(sv_addr, bc_addr));

		// Listen to clients' feedbacks in a separate thread.
		{
			pthread_t tid;
			pthread_create(&tid, NULL, &feedback_listener, &step);
		}

		// Broadcast the particles state in a separate thread.
		{
			pthread_t tid;
			pthread_create(&tid, NULL, &broadcaster, &step);
		}
#endif
#if defined (__APPLE__) || defined(MACOSX)
        atexit(cleanup);
#else
        glutCloseFunc(cleanup);
#endif
        glutMainLoop();
    }

    // cudaDeviceReset causes the driver to clean up all state. While
    // not mandatory in normal operation, it is good practice.  It is also
    // needed to ensure correct operation when the application is being
    // profiled. Calling cudaDeviceReset causes all profile data to be
    // flushed before the application exits
    cudaDeviceReset();

    if (!ref_file)
    {
        exit(EXIT_SUCCESS);
    }

    return 0;

EXTERR:
    printf("Failed to initialize GL extensions.\n");

    // cudaDeviceReset causes the driver to clean up all state. While
    // not mandatory in normal operation, it is good practice.  It is also
    // needed to ensure correct operation when the application is being
    // profiled. Calling cudaDeviceReset causes all profile data to be
    // flushed before the application exits
    cudaDeviceReset();
    exit(EXIT_FAILURE);
}
