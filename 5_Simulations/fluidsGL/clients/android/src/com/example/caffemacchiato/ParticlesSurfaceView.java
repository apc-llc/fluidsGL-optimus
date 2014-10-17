package com.example.caffemacchiato;

import com.example.caffemacchiato.util.SystemUiHider;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

class ParticlesSurfaceView extends GLSurfaceView {
    private boolean isProbablyEmulator() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1
                && (Build.FINGERPRINT.startsWith("generic")
                        || Build.FINGERPRINT.startsWith("unknown")
                        || Build.MODEL.contains("google_sdk")
                        || Build.MODEL.contains("Emulator")
                        || Build.MODEL.contains("Android SDK built for x86"));
    }

    public ParticlesSurfaceView(Context context) {
        super(context);
        if (isProbablyEmulator()) {
            // Avoids crashes on startup with some emulator images.
            setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        }
        setEGLContextClientVersion(2);
        setRenderer(new ParticlesRenderer());
    }

    public ParticlesSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        if (isProbablyEmulator()) {
            // Avoids crashes on startup with some emulator images.
            setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        }
        setEGLContextClientVersion(2);
        setRenderer(new ParticlesRenderer());
    }

    @Override
    protected void onAttachedToWindow() {
        Log.v("AA", "ParticlesSurfaceView.onAttachedToWindow");
        super.onAttachedToWindow();
    }
}

