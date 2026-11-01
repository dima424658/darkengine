package io.github.inkmakerdroid.darkengine;

import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import java.io.File;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicBoolean;

import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

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
    static final String EXTRA_TEXTURE_AUDIT =
        "io.github.inkmakerdroid.darkengine.extra.TEXTURE_AUDIT";
    static final String EXTRA_CONTENT =
        "io.github.inkmakerdroid.darkengine.extra.CONTENT";
    static final String EXTRA_FAN_MISSION =
        "io.github.inkmakerdroid.darkengine.extra.FAN_MISSION";

    static native void nativeSetTouchControllerConnected(boolean connected);
    static native void nativeSetTouchAxis(int axis, float x, float y);
    static native void nativeSetTouchButton(int button, boolean down);
    static native void nativeSetConsoleButton(boolean down);
    static native void nativeSendConsoleEscape();
    static native void nativeNotifyTouchInput();
    static native boolean nativeTouchControlsActive();
    static native boolean nativeTouchControlsVisible();

    private TouchControlsView touchControls;
    private ConsoleFrameView consoleFrame;
    private volatile boolean consoleFrameVisible;
    private final Object movieLock = new Object();
    private LibVLC movieLib;
    private MediaPlayer moviePlayer;
    private SurfaceView movieSurface;
    private CountDownLatch movieFinished;
    private AtomicBoolean movieResult;
    private boolean moviePlaybackStarted;
    private int movieTouchVisibility = View.VISIBLE;
    private final Handler consoleButtonHandler =
        new Handler(Looper.getMainLooper());
    private boolean consoleButtonPending;
    private boolean consoleBackGesture;
    private final Runnable consoleButtonFallback = new Runnable() {
        @Override
        public void run() {
            if (consoleButtonPending) {
                consoleButtonPending = false;
                performConsoleButtonAction();
            }
        }
    };

    private static void pulseConsoleButton() {
        nativeSetConsoleButton(true);
        nativeSetConsoleButton(false);
    }

    private void performConsoleButtonAction() {
        if (consoleFrameVisible) {
            sendConsoleEscape();
        } else {
            pulseConsoleButton();
        }
    }

    private void sendConsoleEscape() {
        nativeSendConsoleEscape();
    }

    private boolean handleConsoleBackEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN &&
            event.getRepeatCount() == 0) {
            if (consoleFrameVisible) {
                // Own this complete key gesture before closing the console.
                // The native close can hide the console overlay before the
                // matching ACTION_UP arrives; allowing that key-up to fall
                // through makes SDL's IME resize the SurfaceView and exposes
                // its rotated/tiled backing buffer.  Closing on key-down also
                // follows exactly the same native path as the touch button.
                consoleBackGesture = true;
                consoleButtonPending = false;
                consoleButtonHandler.removeCallbacks(consoleButtonFallback);
                sendConsoleEscape();
                return true;
            }
            consoleButtonPending = true;
            consoleButtonHandler.removeCallbacks(consoleButtonFallback);
            consoleButtonHandler.postDelayed(consoleButtonFallback, 250L);
        } else if (event.getAction() == KeyEvent.ACTION_UP) {
            if (consoleBackGesture) {
                consoleBackGesture = false;
                return true;
            }
            if (consoleButtonPending) {
                consoleButtonPending = false;
                consoleButtonHandler.removeCallbacks(consoleButtonFallback);
                performConsoleButtonAction();
            }
        }
        return true;
    }

    @Override
    protected boolean onTextInputBackPressed(KeyEvent event) {
        return (consoleFrameVisible || consoleBackGesture) &&
            handleConsoleBackEvent(event);
    }

    private static final class ConsoleFrameView extends View {
        private final Object frameLock = new Object();
        private final ThiefActivity activity;
        private final Paint closeFill = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint closeStroke = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint closeText = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final RectF closeBounds = new RectF();
        private Bitmap frame;
        private boolean closePressed;

        ConsoleFrameView(ThiefActivity activity) {
            super(activity);
            this.activity = activity;
            // Be opaque as soon as the console transition starts. The first
            // copied console frame follows on the render thread, but the
            // rotated SDL SurfaceView must never show through in between.
            setBackgroundColor(Color.BLACK);
            closeFill.setColor(0xc0000000);
            closeFill.setStyle(Paint.Style.FILL);
            closeStroke.setColor(Color.WHITE);
            closeStroke.setStrokeWidth(2.0f *
                getResources().getDisplayMetrics().density);
            closeStroke.setStyle(Paint.Style.STROKE);
            closeText.setColor(Color.WHITE);
            closeText.setTextAlign(Paint.Align.CENTER);
            closeText.setTextSize(18.0f *
                getResources().getDisplayMetrics().scaledDensity);
            setClickable(true);
            setFocusable(false);
        }

        private void updateCloseBounds() {
            final float density = getResources().getDisplayMetrics().density;
            final float margin = 12.0f * density;
            final float width = 132.0f * density;
            final float height = 48.0f * density;
            closeBounds.set(getWidth() - margin - width, margin,
                            getWidth() - margin, margin + height);
        }

        void update(ByteBuffer pixels, int width, int height, int rowBytes) {
            if (pixels == null || width <= 0 || height <= 0 ||
                rowBytes != width * 2) {
                return;
            }
            synchronized (frameLock) {
                if (frame == null || frame.getWidth() != width ||
                    frame.getHeight() != height) {
                    frame = Bitmap.createBitmap(width, height,
                                                Bitmap.Config.RGB_565);
                }
                pixels.rewind();
                frame.copyPixelsFromBuffer(pixels);
            }
            postInvalidateOnAnimation();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            synchronized (frameLock) {
                if (frame != null) {
                    canvas.drawBitmap(frame, null,
                        new Rect(0, 0, getWidth(), getHeight()), null);
                }
            }
            updateCloseBounds();
            closeFill.setColor(closePressed ? 0xe0404040 : 0xc0000000);
            final float radius = 8.0f *
                getResources().getDisplayMetrics().density;
            canvas.drawRoundRect(closeBounds, radius, radius, closeFill);
            canvas.drawRoundRect(closeBounds, radius, radius, closeStroke);
            final Paint.FontMetrics metrics = closeText.getFontMetrics();
            final float baseline = closeBounds.centerY() -
                (metrics.ascent + metrics.descent) * 0.5f;
            canvas.drawText("CLOSE", closeBounds.centerX(), baseline, closeText);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            final float x = event.getX();
            final float y = event.getY();
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    if (!closeBounds.contains(x, y)) {
                        return false;
                    }
                    closePressed = true;
                    invalidate();
                    return true;
                case MotionEvent.ACTION_UP:
                    if (!closePressed) {
                        return false;
                    }
                    final boolean activate = closeBounds.contains(x, y);
                    closePressed = false;
                    invalidate();
                    if (activate) {
                        activity.sendConsoleEscape();
                    }
                    return true;
                case MotionEvent.ACTION_CANCEL:
                    if (closePressed) {
                        closePressed = false;
                        invalidate();
                        return true;
                    }
                    break;
                default:
                    if (closePressed) {
                        return true;
                    }
                    break;
            }
            return false;
        }
    }

    // Called synchronously by the SDL render thread. Bitmap owns a copy before
    // the native display canvas can change again.
    public void presentConsoleFrame(ByteBuffer pixels, int width, int height,
                                    int rowBytes) {
        if (consoleFrame == null) {
            return;
        }
        consoleFrame.update(pixels, width, height, rowBytes);
    }

    public void setConsoleFrameVisible(boolean visible) {
        consoleFrameVisible = visible;
        runOnUiThread(() -> {
            if (touchControls != null) {
                touchControls.setConsoleSuppressed(visible);
            }
            if (consoleFrame != null) {
                consoleFrame.setVisibility(visible ? View.VISIBLE : View.GONE);
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // SDL initially requests a non-fullscreen window and only promotes it
        // after native startup. Establish immersive landscape while the newly
        // created SurfaceView is still awaiting its first layout instead.
        // Otherwise some Android compositors retain the initial 1920x909
        // rotated buffer and expose its tiled storage when the IME is shown.
        applyImmersiveWindow();
        setWindowStyle(true);
        activeInstance = this;
        if (mLayout != null) {
            touchControls = new TouchControlsView(this);
            mLayout.addView(touchControls, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
            consoleFrame = new ConsoleFrameView(this);
            consoleFrame.setVisibility(View.GONE);
            mLayout.addView(consoleFrame, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
            nativeSetTouchControllerConnected(true);
        }
    }

    private void applyImmersiveWindow() {
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
            View.SYSTEM_UI_FLAG_FULLSCREEN |
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().clearFlags(
            WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }
        mFullscreenModeActive = true;
    }

    @Override
    protected void onResume() {
        // Android clears immersive visibility while the task is behind the
        // launcher. Restore the layout flags before SDL's SurfaceView is
        // resumed so it is born at the native 1920x1080 size instead of first
        // presenting a black 1920x909 surface and expanding it afterward.
        applyImmersiveWindow();
        super.onResume();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (hasFocus) {
            applyImmersiveWindow();
        }
        super.onWindowFocusChanged(hasFocus);
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

        // Android and SDL normally collapse a gamepad's dedicated Back key
        // and its Select/View key into one controller button. Keep Select as
        // automap and route only the distinct hardware Back key to Dark's
        // console semantic. Normally pulse it after the key has been released:
        // opening SDL's text editor on ACTION_DOWN would make that editor
        // interpret the matching ACTION_UP as a request to hide the IME. Some
        // Odin firmware paths omit that key-up, so use a guarded fallback.
        final int source = event.getSource();
        final boolean gamepadBack =
            (source & InputDevice.SOURCE_GAMEPAD) ==
                 InputDevice.SOURCE_GAMEPAD ||
             (source & InputDevice.SOURCE_JOYSTICK) ==
                 InputDevice.SOURCE_JOYSTICK;
        if (keyCode == KeyEvent.KEYCODE_BACK &&
            (consoleFrameVisible || consoleBackGesture || gamepadBack)) {
            return handleConsoleBackEvent(event);
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    protected void onPause() {
        finishMovie(false);
        if (touchControls != null) {
            touchControls.releaseAll();
        }
        consoleButtonPending = false;
        consoleBackGesture = false;
        consoleButtonHandler.removeCallbacks(consoleButtonFallback);
        nativeSetConsoleButton(false);
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
            addDebugArgument(arguments, intent, EXTRA_CONTENT, "content");
            addDebugArgument(arguments, intent, EXTRA_FAN_MISSION, "fm");
            addDebugArgument(arguments, intent, EXTRA_QUICKSTART,
                             "quickstart");
            addDebugArgument(arguments, intent, EXTRA_QUICKSTART_POS,
                             "quickstart_pos");
            addDebugArgument(arguments, intent, EXTRA_QUICKSTART_ANGLES,
                             "quickstart_angles");
            addDebugArgument(arguments, intent, EXTRA_TEXTURE_AUDIT,
                             "texture_audit");
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
