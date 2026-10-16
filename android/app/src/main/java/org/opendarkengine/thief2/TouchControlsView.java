package org.opendarkengine.thief2;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.util.SparseArray;
import android.view.MotionEvent;
import android.view.View;

/** Thief-specific, multi-touch controls layered over the SDL surface. */
final class TouchControlsView extends View {
    private static final int CONTROL_NONE = -1;
    private static final int CONTROL_LEFT_STICK = 100;
    private static final int CONTROL_RIGHT_STICK = 101;
    private static final int CONTROL_WEAPON = 102;
    private static final int CONTROL_INVENTORY = 103;

    private static final int BUTTON_A = 0;
    private static final int BUTTON_B = 1;
    private static final int BUTTON_X = 2;
    private static final int BUTTON_Y = 3;
    private static final int BUTTON_LB = 4;
    private static final int BUTTON_RB = 5;
    private static final int BUTTON_VIEW = 6;
    private static final int BUTTON_MENU = 7;
    private static final int BUTTON_DPAD_UP = 10;
    private static final int BUTTON_DPAD_DOWN = 11;
    private static final int BUTTON_DPAD_LEFT = 12;
    private static final int BUTTON_DPAD_RIGHT = 13;
    private static final int BUTTON_LT = 14;
    private static final int BUTTON_RT = 15;
    private static final int BUTTON_LEAN_FORWARD = 16;
    private static final int BUTTON_CLEAR_WEAPON = 17;
    private static final int BUTTON_DROP_ITEM = 18;
    private static final int BUTTON_CYCLE_WEAPON = 19;
    private static final int BUTTON_CYCLE_ITEM = 20;

    private static final long LONG_PRESS_MILLIS = 550L;
    private static final long PULSE_MILLIS = 100L;

    private static final int[] RELEASE_BUTTONS = {
        BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y, BUTTON_LB, BUTTON_RB,
        BUTTON_VIEW, BUTTON_MENU, BUTTON_DPAD_UP, BUTTON_DPAD_DOWN,
        BUTTON_DPAD_LEFT, BUTTON_DPAD_RIGHT, BUTTON_LT, BUTTON_RT,
        BUTTON_LEAN_FORWARD, BUTTON_CLEAR_WEAPON, BUTTON_DROP_ITEM,
        BUTTON_CYCLE_WEAPON, BUTTON_CYCLE_ITEM
    };

    private static final class TouchButton {
        final int button;
        final String label;
        final RectF bounds = new RectF();
        boolean down;

        TouchButton(int button, String label) {
            this.button = button;
            this.label = label;
        }
    }

    private static final class Gesture {
        final int control;
        final float startX;
        final long startTime;

        Gesture(int control, float x, long time) {
            this.control = control;
            startX = x;
            startTime = time;
        }
    }

    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint guide = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint hint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SparseArray<Gesture> gestures = new SparseArray<>();
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final RectF weaponBounds = new RectF();
    private final RectF inventoryBounds = new RectF();
    private boolean controlsVisible;
    private boolean weaponDown;
    private boolean inventoryDown;

    private final Runnable visibilityPoll = new Runnable() {
        @Override
        public void run() {
            boolean active = ThiefActivity.nativeTouchControlsActive();
            boolean visible = active && ThiefActivity.nativeTouchControlsVisible();
            if (visible != controlsVisible) {
                if (controlsVisible && !visible) {
                    releaseAll();
                }
                controlsVisible = visible;
                if (visible) {
                    bringToFront();
                }
                invalidate();
            }
            setVisibility(active ? View.VISIBLE : View.INVISIBLE);
            handler.postDelayed(this, 200L);
        }
    };

    private final TouchButton[] buttons = {
        new TouchButton(BUTTON_LB, "LEAN L"),
        new TouchButton(BUTTON_B, "CROUCH"),
        new TouchButton(BUTTON_RB, "LEAN R"),
        new TouchButton(BUTTON_LEAN_FORWARD, "LEAN F"),
        new TouchButton(BUTTON_X, "USE"),
        new TouchButton(BUTTON_A, "JUMP"),
        new TouchButton(BUTTON_RT, "HIT"),
        new TouchButton(BUTTON_LT, "BLOCK"),
        new TouchButton(BUTTON_VIEW, "MAP"),
        new TouchButton(BUTTON_MENU, "PAUSE")
    };

    private float stickRadius;
    private float leftX;
    private float leftY;
    private float rightX;
    private float rightY;
    private float leftKnobX;
    private float leftKnobY;
    private float rightKnobX;
    private float rightKnobY;

    TouchControlsView(Context context) {
        super(context);
        setWillNotDraw(false);
        setFocusable(false);
        setClickable(false);
        fill.setStyle(Paint.Style.FILL);
        line.setColor(Color.argb(165, 220, 225, 235));
        line.setStyle(Paint.Style.STROKE);
        line.setStrokeWidth(3.0f * getResources().getDisplayMetrics().density);
        guide.setColor(Color.argb(65, 220, 225, 235));
        guide.setStyle(Paint.Style.STROKE);
        guide.setStrokeWidth(1.25f * getResources().getDisplayMetrics().density);
        text.setColor(Color.argb(220, 245, 245, 250));
        text.setTextAlign(Paint.Align.CENTER);
        text.setFakeBoldText(true);
        hint.setColor(Color.argb(170, 235, 235, 240));
        hint.setTextAlign(Paint.Align.CENTER);
        setVisibility(View.INVISIBLE);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        handler.post(visibilityPoll);
    }

    @Override
    protected void onDetachedFromWindow() {
        handler.removeCallbacksAndMessages(null);
        releaseAll();
        super.onDetachedFromWindow();
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        final float unit = Math.min(width, height);
        // Gameplay uses the complete Android surface.  Fixed-resolution
        // menus hide these controls and retain their own native aspect.
        final float gameWidth = width;
        final float gameHeight = height;
        final float gameLeft = 0.0f;
        final float gameTop = 0.0f;
        stickRadius = unit * 0.112f;
        leftX = gameLeft + gameWidth * 0.10f;
        leftY = gameTop + gameHeight * 0.61f;
        rightX = gameLeft + gameWidth * 0.90f;
        rightY = gameTop + gameHeight * 0.61f;
        leftKnobX = leftX;
        leftKnobY = leftY;
        rightKnobX = rightX;
        rightKnobY = rightY;

        weaponBounds.set(gameLeft, gameTop + gameHeight * 0.77f,
                         gameLeft + gameWidth * 0.20f,
                         gameTop + gameHeight * 0.95f);
        inventoryBounds.set(gameLeft + gameWidth * 0.80f,
                            gameTop + gameHeight * 0.77f,
                            gameLeft + gameWidth,
                            gameTop + gameHeight * 0.95f);

        final float smallWidth = unit * 0.105f;
        final float buttonHeight = unit * 0.072f;
        final float gap = unit * 0.014f;
        final float leanY = gameTop + gameHeight * 0.40f;
        setRect(buttons[0], leftX - smallWidth - gap, leanY,
                smallWidth, buttonHeight);
        setRect(buttons[1], leftX, leanY, smallWidth, buttonHeight);
        setRect(buttons[2], leftX + smallWidth + gap, leanY,
                smallWidth, buttonHeight);
        setRect(buttons[3], leftX, gameTop + gameHeight * 0.31f,
                smallWidth, buttonHeight);

        final float actionWidth = unit * 0.10f;
        final float actionStep = actionWidth + gap;
        final float actionY = gameTop + gameHeight * 0.40f;
        final float actionCenterX = Math.min(rightX,
            gameLeft + gameWidth - unit * 0.02f - actionWidth * 0.5f -
            actionStep * 1.5f);
        for (int i = 0; i < 4; ++i) {
            setRect(buttons[4 + i], actionCenterX + (i - 1.5f) * actionStep,
                    actionY, actionWidth, buttonHeight);
        }

        setRect(buttons[8], gameLeft + gameWidth * 0.065f,
                gameTop + gameHeight * 0.085f,
                unit * 0.135f, buttonHeight);
        setRect(buttons[9], gameLeft + gameWidth * 0.935f,
                gameTop + gameHeight * 0.085f,
                unit * 0.15f, buttonHeight);
        text.setTextSize(unit * 0.026f);
        hint.setTextSize(unit * 0.019f);
    }

    private static void setRect(TouchButton button, float x, float y,
                                float width, float height) {
        button.bounds.set(x - width * 0.5f, y - height * 0.5f,
                          x + width * 0.5f, y + height * 0.5f);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (!controlsVisible) {
            return;
        }
        drawHudRegion(canvas, weaponBounds, weaponDown,
                      "WEAPON  -  TAP CYCLE  -  HOLD HOLSTER");
        drawHudRegion(canvas, inventoryBounds, inventoryDown,
                      "ITEM  -  TAP CYCLE  -  HOLD DROP");
        drawStick(canvas, leftX, leftY, leftKnobX, leftKnobY, true);
        drawStick(canvas, rightX, rightY, rightKnobX, rightKnobY, false);
        for (TouchButton button : buttons) {
            fill.setColor(button.down
                ? Color.argb(125, 120, 170, 230)
                : Color.argb(58, 18, 22, 28));
            float radius = button.bounds.height() * 0.5f;
            canvas.drawRoundRect(button.bounds, radius, radius, fill);
            canvas.drawRoundRect(button.bounds, radius, radius, line);
            drawCenteredText(canvas, button.label, button.bounds, text);
        }
    }

    private void drawHudRegion(Canvas canvas, RectF bounds, boolean down,
                               String label) {
        float radius = bounds.height() * 0.18f;
        if (down) {
            fill.setColor(Color.argb(65, 110, 160, 220));
            canvas.drawRoundRect(bounds, radius, radius, fill);
        }
        canvas.drawRoundRect(bounds, radius, radius, line);
        Paint.FontMetrics fm = hint.getFontMetrics();
        float topBaseline = bounds.top - fm.ascent + 2.0f;
        canvas.drawText(label, bounds.centerX(), topBaseline, hint);
        float centerBaseline = bounds.centerY() - (fm.ascent + fm.descent) * 0.5f;
        canvas.drawText("<", bounds.left + bounds.height() * 0.22f,
                        centerBaseline, hint);
        canvas.drawText(">", bounds.right - bounds.height() * 0.22f,
                        centerBaseline, hint);
    }

    private static void drawCenteredText(Canvas canvas, String value,
                                         RectF bounds, Paint paint) {
        Paint.FontMetrics metrics = paint.getFontMetrics();
        float baseline = bounds.centerY() -
            (metrics.ascent + metrics.descent) * 0.5f;
        canvas.drawText(value, bounds.centerX(), baseline, paint);
    }

    private void drawStick(Canvas canvas, float centerX, float centerY,
                           float knobX, float knobY, boolean movement) {
        fill.setColor(Color.argb(45, 18, 22, 28));
        canvas.drawCircle(centerX, centerY, stickRadius, fill);
        canvas.drawCircle(centerX, centerY, stickRadius, line);
        if (movement) {
            canvas.drawCircle(centerX, centerY, stickRadius * 0.42f, guide);
            canvas.drawCircle(centerX, centerY, stickRadius * 0.78f, guide);
        }
        fill.setColor(Color.argb(90, 150, 165, 185));
        canvas.drawCircle(knobX, knobY, stickRadius * 0.40f, fill);
        canvas.drawCircle(knobX, knobY, stickRadius * 0.40f, line);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int action = event.getActionMasked();
        final int actionIndex = event.getActionIndex();
        if (action == MotionEvent.ACTION_DOWN ||
            action == MotionEvent.ACTION_POINTER_DOWN) {
            final int pointerId = event.getPointerId(actionIndex);
            final float x = event.getX(actionIndex);
            final float y = event.getY(actionIndex);
            final int control = findControl(x, y);
            if (control == CONTROL_NONE && action == MotionEvent.ACTION_DOWN) {
                return false;
            }
            if (control != CONTROL_NONE) {
                controlsVisible = true;
                gestures.put(pointerId,
                    new Gesture(control, x, event.getEventTime()));
                if (control == CONTROL_WEAPON) {
                    weaponDown = true;
                    invalidate();
                } else if (control == CONTROL_INVENTORY) {
                    inventoryDown = true;
                    invalidate();
                } else {
                    updateControl(control, x, y, true);
                }
            }
            return true;
        }
        if (action == MotionEvent.ACTION_MOVE) {
            for (int i = 0; i < event.getPointerCount(); ++i) {
                Gesture gesture = gestures.get(event.getPointerId(i));
                if (gesture != null &&
                    (gesture.control == CONTROL_LEFT_STICK ||
                     gesture.control == CONTROL_RIGHT_STICK)) {
                    updateControl(gesture.control, event.getX(i), event.getY(i), true);
                }
            }
            return gestures.size() != 0;
        }
        if (action == MotionEvent.ACTION_UP ||
            action == MotionEvent.ACTION_POINTER_UP) {
            final int pointerId = event.getPointerId(actionIndex);
            Gesture gesture = gestures.get(pointerId);
            if (gesture != null) {
                float x = event.getX(actionIndex);
                float y = event.getY(actionIndex);
                if (gesture.control == CONTROL_WEAPON ||
                    gesture.control == CONTROL_INVENTORY) {
                    finishHudGesture(gesture, x, event.getEventTime());
                } else if (gesture.control == CONTROL_LEFT_STICK ||
                           gesture.control == CONTROL_RIGHT_STICK) {
                    updateControl(gesture.control, x, y, false);
                } else {
                    finishButtonGesture(gesture, event.getEventTime());
                }
                gestures.remove(pointerId);
            }
            return true;
        }
        if (action == MotionEvent.ACTION_CANCEL) {
            releaseAll();
            return true;
        }
        return false;
    }

    private int findControl(float x, float y) {
        for (int i = 0; i < buttons.length; ++i) {
            if (buttons[i].bounds.contains(x, y)) {
                return i;
            }
        }
        if (distanceSquared(x, y, leftX, leftY) <= stickRadius * stickRadius) {
            return CONTROL_LEFT_STICK;
        }
        if (distanceSquared(x, y, rightX, rightY) <= stickRadius * stickRadius) {
            return CONTROL_RIGHT_STICK;
        }
        if (weaponBounds.contains(x, y)) {
            return CONTROL_WEAPON;
        }
        if (inventoryBounds.contains(x, y)) {
            return CONTROL_INVENTORY;
        }
        return CONTROL_NONE;
    }

    private static float distanceSquared(float x1, float y1, float x2, float y2) {
        float x = x1 - x2;
        float y = y1 - y2;
        return x * x + y * y;
    }

    private void updateControl(int control, float x, float y, boolean down) {
        if (control == CONTROL_LEFT_STICK || control == CONTROL_RIGHT_STICK) {
            final float centerX = control == CONTROL_LEFT_STICK ? leftX : rightX;
            final float centerY = control == CONTROL_LEFT_STICK ? leftY : rightY;
            float axisX = down ? (x - centerX) / stickRadius : 0.0f;
            float axisY = down ? (centerY - y) / stickRadius : 0.0f;
            float magnitude = (float)Math.sqrt(axisX * axisX + axisY * axisY);
            if (magnitude > 1.0f) {
                axisX /= magnitude;
                axisY /= magnitude;
            }
            if (control == CONTROL_LEFT_STICK) {
                leftKnobX = centerX + axisX * stickRadius;
                leftKnobY = centerY - axisY * stickRadius;
                ThiefActivity.nativeSetTouchAxis(0, axisX, axisY);
            } else {
                rightKnobX = centerX + axisX * stickRadius;
                rightKnobY = centerY - axisY * stickRadius;
                ThiefActivity.nativeSetTouchAxis(1, axisX, axisY);
            }
        } else if (control >= 0 && control < buttons.length) {
            TouchButton button = buttons[control];
            if (button.down != down) {
                button.down = down;
                ThiefActivity.nativeSetTouchButton(button.button, down);
            }
        }
        invalidate();
    }

    private void finishHudGesture(Gesture gesture, float x, long eventTime) {
        final boolean weapon = gesture.control == CONTROL_WEAPON;
        final RectF bounds = weapon ? weaponBounds : inventoryBounds;
        final float deltaX = x - gesture.startX;
        final float swipeThreshold = bounds.width() * 0.18f;
        final long duration = eventTime - gesture.startTime;

        if (weapon) {
            weaponDown = false;
        } else {
            inventoryDown = false;
        }

        if (Math.abs(deltaX) >= swipeThreshold) {
            if (weapon) {
                pulseButton(deltaX < 0.0f ? BUTTON_DPAD_UP : BUTTON_DPAD_DOWN);
            } else {
                pulseButton(deltaX < 0.0f ? BUTTON_DPAD_RIGHT : BUTTON_DPAD_LEFT);
            }
        } else if (duration >= LONG_PRESS_MILLIS) {
            pulseButton(weapon ? BUTTON_CLEAR_WEAPON : BUTTON_DROP_ITEM);
        } else {
            pulseButton(weapon ? BUTTON_CYCLE_WEAPON : BUTTON_CYCLE_ITEM);
        }
        invalidate();
    }

    private void pulseButton(final int button) {
        ThiefActivity.nativeSetTouchButton(button, true);
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                ThiefActivity.nativeSetTouchButton(button, false);
            }
        }, PULSE_MILLIS);
    }

    private void finishButtonGesture(final Gesture gesture, long eventTime) {
        final int control = gesture.control;
        final long remaining = PULSE_MILLIS - (eventTime - gesture.startTime);
        if (remaining <= 0L) {
            updateControl(control, 0.0f, 0.0f, false);
            return;
        }
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                updateControl(control, 0.0f, 0.0f, false);
            }
        }, remaining);
    }

    void releaseAll() {
        gestures.clear();
        weaponDown = false;
        inventoryDown = false;
        ThiefActivity.nativeSetTouchAxis(0, 0.0f, 0.0f);
        ThiefActivity.nativeSetTouchAxis(1, 0.0f, 0.0f);
        leftKnobX = leftX;
        leftKnobY = leftY;
        rightKnobX = rightX;
        rightKnobY = rightY;
        for (TouchButton button : buttons) {
            button.down = false;
        }
        for (int button : RELEASE_BUTTONS) {
            ThiefActivity.nativeSetTouchButton(button, false);
        }
        invalidate();
    }
}
