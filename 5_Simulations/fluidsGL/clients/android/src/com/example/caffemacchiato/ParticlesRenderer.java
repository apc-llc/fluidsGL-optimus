package com.example.caffemacchiato;

import javax.microedition.khronos.opengles.GL10;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView.Renderer;

public class ParticlesRenderer implements Renderer  {
	@Override
    public void onSurfaceCreated(GL10 gl, javax.microedition.khronos.egl.EGLConfig config) {
		GameLibJNIWrapper.on_surface_created();
    }
 
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        gl.glViewport(0, 0, width, height);
        gl.glMatrixMode(gl.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glOrthox(0, 1, 1, 0, 0, 1);
        gl.glMatrixMode(gl.GL_MODELVIEW);
        gl.glLoadIdentity();
        GameLibJNIWrapper.on_surface_changed(width, height);
    }
 
    @Override
    public void onDrawFrame(GL10 gl) {
    	GameLibJNIWrapper.on_draw_frame();
    }
}
