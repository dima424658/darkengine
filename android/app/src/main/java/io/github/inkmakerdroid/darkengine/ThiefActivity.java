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
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

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
    static native boolean nativeMoviePlaybackActive();
    static native void nativeNotifyTouchInput();
    static native boolean nativeTouchControlsActive();
    static native boolean nativeTouchControlsVisible();

    private TouchControlsView touchControls;
    private ConsoleFrameView consoleFrame;
    private volatile boolean consoleFrameVisible;
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
        final int keyCode = event.getKeyCode();
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
            keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
            keyCode == KeyEvent.KEYCODE_VOLUME_MUTE) {
            return super.dispatchKeyEvent(event);
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
        if (!nativeMoviePlaybackActive() && keyCode == KeyEvent.KEYCODE_BACK &&
            (consoleFrameVisible || consoleBackGesture || gamepadBack)) {
            return handleConsoleBackEvent(event);
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    protected void onPause() {
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
