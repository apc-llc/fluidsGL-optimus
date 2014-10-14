package com.example.tenyears;

import javax.microedition.khronos.opengles.GL10;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView.Renderer;

public class RendererWrapper implements Renderer  {
	@Override
    public void onSurfaceCreated(GL10 gl, javax.microedition.khronos.egl.EGLConfig config) {
        //GLES20.glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
		
		
		GameLibJNIWrapper.on_surface_created();
    }
 
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        // No-op
    }
 
    @Override
    public void onDrawFrame(GL10 gl) {
    	GameLibJNIWrapper.on_draw_frame();
    }

	

}
