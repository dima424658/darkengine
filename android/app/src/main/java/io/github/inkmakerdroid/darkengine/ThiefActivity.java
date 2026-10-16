package io.github.inkmakerdroid.darkengine;

import android.content.Intent;
import android.content.pm.ApplicationInfo;
import java.io.File;
import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicBoolean;

import android.net.Uri;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;

import org.libsdl.app.SDLActivity;
import org.videolan.libvlc.LibVLC;
import org.videolan.libvlc.Media;
import org.videolan.libvlc.MediaPlayer;
import org.videolan.libvlc.interfaces.IVLCVout;

public final class ThiefActivity extends SDLActivity {
    private static ThiefActivity activeInstance;

    static final String EXTRA_GAME_PATH =
        "io.github.inkmakerdroid.darkengine.extra.GAME_PATH";
    static final String EXTRA_QUICKSTART =
        "io.github.inkmakerdroid.darkengine.extra.QUICKSTART";
    static final String EXTRA_QUICKSTART_POS =
        "io.github.inkmakerdroid.darkengine.extra.QUICKSTART_POS";
    static final String EXTRA_QUICKSTART_ANGLES =
        "io.github.inkmakerdroid.darkengine.extra.QUICKSTART_ANGLES";
    static final String EXTRA_AUTO_STOP =
        "io.github.inkmakerdroid.darkengine.extra.AUTO_STOP";

    static native void nativeSetTouchControllerConnected(boolean connected);
    static native void nativeSetTouchAxis(int axis, float x, float y);
    static native void nativeSetTouchButton(int button, boolean down);
    static native void nativeNotifyTouchInput();
    static native boolean nativeTouchControlsActive();
    static native boolean nativeTouchControlsVisible();

    private TouchControlsView touchControls;
    private final Object movieLock = new Object();
    private LibVLC movieLib;
    private MediaPlayer moviePlayer;
    private SurfaceView movieSurface;
    private CountDownLatch movieFinished;
    private AtomicBoolean movieResult;
    private boolean moviePlaybackStarted;
    private int movieTouchVisibility = View.VISIBLE;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        activeInstance = this;
        if (mLayout != null) {
            touchControls = new TouchControlsView(this);
            mLayout.addView(touchControls, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
            nativeSetTouchControllerConnected(true);
        }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            nativeNotifyTouchInput();
        }
        return super.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        // Volume is owned by Android even while a movie is playing.  SDL also
        // leaves these keys unhandled so the system can adjust media volume.
        // Do this before the movie's any-key-to-skip handling.
        final int keyCode = event.getKeyCode();
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
            keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
            keyCode == KeyEvent.KEYCODE_VOLUME_MUTE) {
            return super.dispatchKeyEvent(event);
        }

        synchronized (movieLock) {
            if (moviePlayer != null && event.getAction() == KeyEvent.ACTION_DOWN) {
                finishMovie(true);
                return true;
            }
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    protected void onPause() {
        finishMovie(false);
        if (touchControls != null) {
            touchControls.releaseAll();
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        finishMovie(false);
        if (touchControls != null) {
            touchControls.releaseAll();
            nativeSetTouchControllerConnected(false);
        }
        super.onDestroy();
        if (activeInstance == this) {
            activeInstance = null;
        }
    }

    static boolean isRunning() {
        return activeInstance != null;
    }

    /** Called on SDL's native thread while Dark's movie panel is active. */
    public boolean playMovieSynchronously(String path, int millibels) {
        if (path == null || path.isEmpty()) {
            return false;
        }

        final CountDownLatch finished = new CountDownLatch(1);
        final AtomicBoolean result = new AtomicBoolean(false);
        synchronized (movieLock) {
            if (moviePlayer != null) {
                return false;
            }
            movieFinished = finished;
            movieResult = result;
        }

        runOnUiThread(() -> startMovie(path, millibels));
        try {
            finished.await();
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            finishMovie(false);
            return false;
        }
        return result.get();
    }

    private void startMovie(String path, int millibels) {
        try {
            ArrayList<String> options = new ArrayList<>();
            options.add("--no-video-title-show");
            options.add("--avcodec-hw=none");
            // Some Android audio stacks create an AudioTrack successfully but
            // never consume VLC's decoded PCM (observed on the Odin 2 while
            // using Bluetooth).  OpenSL ES is also used successfully by the
            // engine's OpenAL backend and avoids that silent AudioTrack path.
            options.add("--aout=opensles");
            movieLib = new LibVLC(this, options);
            moviePlayer = new MediaPlayer(movieLib);
            movieSurface = new SurfaceView(this);
            moviePlaybackStarted = false;

            // SDL renders into its own SurfaceView.  A newly added SurfaceView is
            // otherwise allowed to remain behind it, which produces movie audio
            // over SDL's black movie canvas.  The media-overlay layer explicitly
            // places VLC above SDL without putting it above normal Android UI.
            movieSurface.setZOrderMediaOverlay(true);
            movieSurface.setOnTouchListener((view, event) -> {
                if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                    finishMovie(true);
                }
                return true;
            });

            if (touchControls != null) {
                movieTouchVisibility = touchControls.getVisibility();
                touchControls.setVisibility(View.GONE);
            }
            mLayout.addView(movieSurface, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

            moviePlayer.setEventListener(event -> {
                switch (event.type) {
                    case MediaPlayer.Event.Playing:
                        synchronized (movieLock) {
                            if (movieResult != null) {
                                movieResult.set(true);
                            }
                        }
                        break;
                    case MediaPlayer.Event.EndReached:
                        finishMovie(true);
                        break;
                    case MediaPlayer.Event.EncounteredError:
                        finishMovie(false);
                        break;
                    default:
                        break;
                }
            });
            movieSurface.getHolder().addCallback(new SurfaceHolder.Callback() {
                @Override
                public void surfaceCreated(SurfaceHolder holder) {
                    beginMoviePlayback(path, millibels);
                }

                @Override
                public void surfaceChanged(SurfaceHolder holder, int format,
                                           int width, int height) {
                }

                @Override
                public void surfaceDestroyed(SurfaceHolder holder) {
                    synchronized (movieLock) {
                        if (movieFinished == null || movieSurface == null) {
                            return;
                        }
                    }
                    finishMovie(false);
                }
            });
        } catch (RuntimeException exception) {
            finishMovie(false);
        }
    }

    private void beginMoviePlayback(String path, int millibels) {
        synchronized (movieLock) {
            if (movieFinished == null || moviePlayer == null ||
                movieSurface == null || moviePlaybackStarted) {
                return;
            }
            moviePlaybackStarted = true;
        }

        try {
            final IVLCVout videoOutput = moviePlayer.getVLCVout();
            videoOutput.setVideoView(movieSurface);
            videoOutput.setWindowSize(mLayout.getWidth(), mLayout.getHeight());
            videoOutput.attachViews((vout, width, height, visibleWidth,
                                     visibleHeight, sarNum, sarDen) ->
                runOnUiThread(() -> updateMovieSurface(
                    width, height, visibleWidth, visibleHeight, sarNum, sarDen)));

            Media media = new Media(movieLib, Uri.fromFile(new File(path)));
            media.setHWDecoderEnabled(false, false);
            moviePlayer.setMedia(media);
            media.release();
            final double gain = millibels <= -10000 ? 0.0 :
                millibels >= 0 ? 1.0 : Math.pow(10.0, millibels / 2000.0);
            moviePlayer.setVolume((int)Math.round(gain * 100.0));
            moviePlayer.play();
        } catch (RuntimeException exception) {
            finishMovie(false);
        }
    }

    private void updateMovieSurface(int width, int height, int visibleWidth,
                                    int visibleHeight, int sarNum, int sarDen) {
        if (movieSurface == null || width <= 0 || height <= 0 ||
            visibleWidth <= 0 || visibleHeight <= 0) {
            return;
        }

        final int windowWidth = mLayout.getWidth();
        final int windowHeight = mLayout.getHeight();
        if (windowWidth <= 0 || windowHeight <= 0) {
            return;
        }

        final double pixelAspect = sarNum > 0 && sarDen > 0
            ? (double)sarNum / (double)sarDen : 1.0;
        final double videoAspect = visibleWidth * pixelAspect / visibleHeight;
        int visibleSurfaceWidth = windowWidth;
        int visibleSurfaceHeight = (int)Math.round(windowWidth / videoAspect);
        if (visibleSurfaceHeight > windowHeight) {
            visibleSurfaceHeight = windowHeight;
            visibleSurfaceWidth = (int)Math.round(windowHeight * videoAspect);
        }

        // Account for coded padding around the visible picture, just as VLC's
        // VideoHelper does, then let SurfaceView scale the decoder buffer.
        final int surfaceWidth = (int)Math.ceil(
            visibleSurfaceWidth * (double)width / visibleWidth);
        final int surfaceHeight = (int)Math.ceil(
            visibleSurfaceHeight * (double)height / visibleHeight);
        final ViewGroup.LayoutParams params = movieSurface.getLayoutParams();
        params.width = surfaceWidth;
        params.height = surfaceHeight;
        movieSurface.setLayoutParams(params);
        movieSurface.setX((windowWidth - surfaceWidth) * 0.5f);
        movieSurface.setY((windowHeight - surfaceHeight) * 0.5f);
        movieSurface.invalidate();
    }

    private void finishMovie(boolean successfulOrSkipped) {
        final CountDownLatch finished;
        final AtomicBoolean result;
        synchronized (movieLock) {
            finished = movieFinished;
            result = movieResult;
            if (finished == null) {
                return;
            }
            movieFinished = null;
            movieResult = null;
            if (successfulOrSkipped) {
                result.set(true);
            }
        }

        runOnUiThread(() -> {
            synchronized (movieLock) {
                if (moviePlayer != null) {
                    moviePlayer.stop();
                    moviePlayer.getVLCVout().detachViews();
                    moviePlayer.release();
                    moviePlayer = null;
                }
                if (movieLib != null) {
                    movieLib.release();
                    movieLib = null;
                }
                if (movieSurface != null) {
                    mLayout.removeView(movieSurface);
                    movieSurface = null;
                }
                moviePlaybackStarted = false;
                if (touchControls != null) {
                    touchControls.setVisibility(movieTouchVisibility);
                }
            }
            finished.countDown();
        });
    }

    @Override
    protected String[] getArguments() {
        Intent intent = getIntent();
        String selectedPath = intent.getStringExtra(EXTRA_GAME_PATH);
        File game;
        if (selectedPath != null && !selectedPath.trim().isEmpty()) {
            game = new File(selectedPath);
        } else {
            File root = getExternalFilesDir(null);
            if (root == null) {
                root = getFilesDir();
            }
            game = new File(root, "game");
            game.mkdirs();
        }

        ArrayList<String> arguments = new ArrayList<>();
        arguments.add("game_path=" + game.getAbsolutePath());
        if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
            addDebugArgument(arguments, intent, EXTRA_QUICKSTART,
                             "quickstart");
            addDebugArgument(arguments, intent, EXTRA_QUICKSTART_POS,
                             "quickstart_pos");
            addDebugArgument(arguments, intent, EXTRA_QUICKSTART_ANGLES,
                             "quickstart_angles");
            int autoStop = intent.getIntExtra(EXTRA_AUTO_STOP, 0);
            if (autoStop > 0) {
                arguments.add("auto_stop=" + autoStop);
            }
        }
        return arguments.toArray(new String[0]);
    }

    private static void addDebugArgument(ArrayList<String> arguments,
                                         Intent intent, String extra,
                                         String option) {
        String value = intent.getStringExtra(extra);
        if (value != null && !value.trim().isEmpty()) {
            arguments.add(option + "=" + value.trim());
        }
    }
}
