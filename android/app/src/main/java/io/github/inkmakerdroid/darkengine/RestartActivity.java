package io.github.inkmakerdroid.darkengine;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.os.SystemClock;
import android.view.View;
import android.view.WindowManager;

import java.util.List;

/**
 * Performs a process-wide engine restart without relying on an alarm.
 *
 * This Activity runs in its own process and task. It remains a foreground
 * activity while the SDL/native process is terminated, then clears that dead
 * task and launches a fresh LauncherActivity. Dark's process-global systems
 * are therefore never constructed twice in one process.
 */
public final class RestartActivity extends Activity {
    static final String EXTRA_ENGINE_PID =
        "io.github.inkmakerdroid.darkengine.extra.ENGINE_PID";

    private static final long PROCESS_EXIT_TIMEOUT_MS = 5000L;
    private static final long PROCESS_POLL_INTERVAL_MS = 50L;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private int enginePid;
    private long waitDeadline;
    private boolean restartStarted;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        View background = new View(this);
        background.setBackgroundColor(Color.BLACK);
        setContentView(background);

        enginePid = getIntent().getIntExtra(EXTRA_ENGINE_PID, 0);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (restartStarted) {
            return;
        }
        restartStarted = true;
        waitDeadline = SystemClock.uptimeMillis() + PROCESS_EXIT_TIMEOUT_MS;

        // Let this window become the foreground activity before terminating
        // the process that owns the old launcher and SDL activity.
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (enginePid > 0 && enginePid != Process.myPid()) {
                    Process.killProcess(enginePid);
                }
                waitForEngineExit();
            }
        });
    }

    private void waitForEngineExit() {
        if (isProcessRunning(enginePid) &&
            SystemClock.uptimeMillis() < waitDeadline) {
            mainHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    waitForEngineExit();
                }
            }, PROCESS_POLL_INTERVAL_MS);
            return;
        }

        Intent relaunch = new Intent(this, LauncherActivity.class);
        relaunch.setAction(Intent.ACTION_MAIN);
        relaunch.addCategory(Intent.CATEGORY_LAUNCHER);
        relaunch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK |
                          Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(relaunch);
        finishAndRemoveTask();
    }

    private boolean isProcessRunning(int processId) {
        if (processId <= 0) {
            return false;
        }
        ActivityManager manager =
            (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> processes =
            manager.getRunningAppProcesses();
        if (processes == null) {
            return false;
        }
        for (ActivityManager.RunningAppProcessInfo process : processes) {
            if (process.pid == processId) {
                return true;
            }
        }
        return false;
    }
}
