#include"canvas.h"
#include<stdio.h>
#include<stdlib.h>
#include <GLES2/gl2.h>

GLuint vbo = 0;                 // OpenGL vertex buffer object


void on_surface_created() {
	glGenBuffers(1, &vbo);


}

void on_surface_changed() {
    // No-op
}

void on_draw_frame() {
    glClear(GL_COLOR_BUFFER_BIT);
    float particels[128*128];
    	int i;
    	for(i=0;i<128*128;i++){
    		particels[i]=rand() / RAND_MAX;
    	}

        //glClearColor(1.0f, 0.0f, 0.0f, 0.0f);

    	glBindBuffer(GL_ARRAY_BUFFER, vbo);

    	    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 128*128,
    	    		particels, GL_STATIC_DRAW);

    	    glVertexPointer(2, GL_FLOAT, 0, &particels);
    	    glDrawArrays(GL_POINTS, 0, 128 * 128 / 2);
    	    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
