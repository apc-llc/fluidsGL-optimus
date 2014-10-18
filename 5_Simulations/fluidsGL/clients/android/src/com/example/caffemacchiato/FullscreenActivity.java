package com.example.caffemacchiato;

import com.example.caffemacchiato.util.SystemUiHider;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ConfigurationInfo;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import java.lang.Character;
import java.net.NetworkInterface;
import java.util.Enumeration;
import java.net.InterfaceAddress;
import java.util.ArrayList;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 *
 * @see SystemUiHider
 */
public class FullscreenActivity extends Activity {
    /**
     * Whether or not the system UI should be auto-hidden after
     * {@link #AUTO_HIDE_DELAY_MILLIS} milliseconds.
     */
    private static final boolean AUTO_HIDE = true;

    /**
     * If {@link #AUTO_HIDE} is set, the number of milliseconds to wait after
     * user interaction before hiding the system UI.
     */
    private static final int AUTO_HIDE_DELAY_MILLIS = 3000;

    /**
     * If set, will toggle the system UI visibility upon interaction. Otherwise,
     * will show the system UI visibility upon interaction.
     */
    private static final boolean TOGGLE_ON_CLICK = true;

    /**
     * The flags to pass to {@link SystemUiHider#getInstance}.
     */
    private static final int HIDER_FLAGS = SystemUiHider.FLAG_HIDE_NAVIGATION;

    /**
     * The instance of the {@link SystemUiHider} for this activity.
     */
    private SystemUiHider mSystemUiHider;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_fullscreen);

        final View controlsView = findViewById(R.id.fullscreen_content_controls);
        final View contentView = findViewById(R.id.fullscreen_content);

        // Set up an instance of SystemUiHider to control the system UI for
        // this activity.
        mSystemUiHider = SystemUiHider.getInstance(this, contentView, HIDER_FLAGS);
        mSystemUiHider.setup();
        mSystemUiHider
                .setOnVisibilityChangeListener(new SystemUiHider.OnVisibilityChangeListener() {
                    // Cached values.
                    int mControlsHeight;
                    int mShortAnimTime;

                    @Override
                    @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR2)
                    public void onVisibilityChange(boolean visible) {
                        // If the ViewPropertyAnimator APIs aren't
                        // available, simply show or hide the in-layout UI
                        // controls.
                        controlsView.setVisibility(visible ? View.VISIBLE : View.GONE);

                        if (visible && AUTO_HIDE) {
                            // Schedule a hide().
                            delayedHide(AUTO_HIDE_DELAY_MILLIS);
                        }
                    }
                });

        // Set up the user interaction to manually show or hide the system UI.
        contentView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (TOGGLE_ON_CLICK) {
                    mSystemUiHider.toggle();
                } else {
                    mSystemUiHider.show();
                }
            }
        });

        // Upon interacting with UI controls, delay any scheduled hide()
        // operations to prevent the jarring behavior of controls going away
        // while interacting with the UI.
        findViewById(R.id.connect_button).setOnTouchListener(mDelayHideTouchListener);
        findViewById(R.id.reset_button).setOnTouchListener(mDelayHideTouchListener);

        final Button connect = (Button)findViewById(R.id.connect_button);
        connect.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
		    	// Find addresses in all available network interfaces.
				Enumeration<NetworkInterface> nwis;
				ArrayList<String> addressesList = new ArrayList<String>();
				try {
					nwis = NetworkInterface.getNetworkInterfaces();
					while (nwis.hasMoreElements()) {
					    NetworkInterface ni = nwis.nextElement();
					    for (InterfaceAddress ia : ni.getInterfaceAddresses()) {
					    	String address = ia.getAddress().toString().substring(1);
					    	boolean ipv4 = true;
					    	for (int i = 0; i < address.length(); i++)
					    		if ((address.charAt(i) != '.') && !Character.isDigit(address.charAt(i)))
					    		{
					    			ipv4 = false;
					    			break;
					    		}
					    	if (!ipv4) continue;
					    	addressesList.add(address);
					    }
					}
				} catch (Exception e) {
				    e.printStackTrace();
				}
		    	final String[] addresses = addressesList.toArray(new String[addressesList.size()]);

				// Choose network interface
				address = addresses[0];
				new AlertDialog.Builder(FullscreenActivity.this)
		            .setTitle("Select the network address to use")
		            .setSingleChoiceItems(addresses, 0, new DialogInterface.OnClickListener() {
		                public void onClick(DialogInterface dialog, int whichButton) {
							address = addresses[whichButton];
		                }
		            })
		            .setPositiveButton("OK", new DialogInterface.OnClickListener() {
		                public void onClick(DialogInterface dialog, int whichButton) {
							dialog.dismiss();

							// Wait for incoming server broadcast at the specified address.
							final String bc_addr = address + ":9097";
							final ProgressDialog ringProgressDialog = ProgressDialog.show(FullscreenActivity.this,
								"Waiting for the server ...", "Waiting for incoming connection at " + bc_addr, true);
							ringProgressDialog.setCancelable(true);
							new Thread(new Runnable() {
								@Override
								public void run() {
									GameLibJNIWrapper.on_connect(bc_addr);
									ringProgressDialog.dismiss();
								}
							}).start();
		                }
		            })
		            .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
		                public void onClick(DialogInterface dialog, int whichButton) {
		                	dialog.dismiss();
		                	address = "";
		                }
		            })
		        .create().show();
            }
        });

        final Button reset = (Button)findViewById(R.id.reset_button);
        reset.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
				GameLibJNIWrapper.on_reset();
            }
        });

		final GLSurfaceView particlesSurfaceView = (GLSurfaceView)findViewById(R.id.fullscreen_content);
		particlesSurfaceView.setOnTouchListener(new View.OnTouchListener() {
		    public boolean onTouch(View view, MotionEvent motionEvent) {
		    	if (motionEvent.getAction() == MotionEvent.ACTION_DOWN)
				{
			        Log.v("AA", "particlesSurfaceView.setOnTouchListener ACTION_DOWN");
					GameLibJNIWrapper.on_click(true, (int)motionEvent.getX(), (int)motionEvent.getY());
				}
		    	if (motionEvent.getAction() == MotionEvent.ACTION_UP)
		    	{
			        Log.v("AA", "particlesSurfaceView.setOnTouchListener ACTION_UP");
					GameLibJNIWrapper.on_click(false, (int)motionEvent.getX(), (int)motionEvent.getY());
				}
		    	if (motionEvent.getAction() == MotionEvent.ACTION_MOVE)
		    	{
			        Log.v("AA", "particlesSurfaceView.setOnTouchListener ACTION_MOVE");
		    		GameLibJNIWrapper.on_motion((int)motionEvent.getX(), (int)motionEvent.getY());
		    	}
		    	return true;
		    }
		});

        ActivityManager activityManager =
                (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        ConfigurationInfo configurationInfo =
                activityManager.getDeviceConfigurationInfo();
    }

    @Override
    protected void onPause() {
        Log.v("AA", "FullscreenActivity.onPause");
        final GLSurfaceView particlesSurfaceView = (GLSurfaceView)findViewById(R.id.fullscreen_content);
        particlesSurfaceView.onPause();
        super.onPause();
    }
     
    @Override
    protected void onResume() {
        Log.v("AA", "FullscreenActivity.onResume");
        final GLSurfaceView particlesSurfaceView = (GLSurfaceView)findViewById(R.id.fullscreen_content);
        particlesSurfaceView.onResume();
        super.onResume();
    }

    /**
     * Touch listener to use for in-layout UI controls to delay hiding the
     * system UI. This is to prevent the jarring behavior of controls going away
     * while interacting with activity UI.
     */
    View.OnTouchListener mDelayHideTouchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View view, MotionEvent motionEvent) {
            if (AUTO_HIDE) {
                delayedHide(AUTO_HIDE_DELAY_MILLIS);
            }
            return false;
        }
    };

	String address = "";

    Handler mHideHandler = new Handler();
    Runnable mHideRunnable = new Runnable() {
        @Override
        public void run() {
            mSystemUiHider.hide();
        }
    };

    /**
     * Schedules a call to hide() in [delay] milliseconds, canceling any
     * previously scheduled calls.
     */
    private void delayedHide(int delayMillis) {
        mHideHandler.removeCallbacks(mHideRunnable);
        mHideHandler.postDelayed(mHideRunnable, delayMillis);
    }
}
