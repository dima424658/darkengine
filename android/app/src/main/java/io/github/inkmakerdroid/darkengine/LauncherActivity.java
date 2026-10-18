package io.github.inkmakerdroid.darkengine;

import android.Manifest;
import android.app.AlarmManager;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Selects separately installed retail game data before SDL starts.
 *
 * Android's Storage Access Framework exposes a selected directory as content
 * URIs, not as a POSIX directory the native engine can chdir into. A physical
 * external-storage tree can be used directly with All files access; any other
 * provider can be imported transactionally into the app's external-files area.
 */
public final class LauncherActivity extends Activity {
    private static final int REQUEST_GAME_DIRECTORY = 1;
    private static final int REQUEST_STORAGE_ACCESS = 2;
    private static final int REQUEST_LEGACY_STORAGE_ACCESS = 3;
    private static final String GAME_DIRECTORY = "game";
    private static final String IMPORT_DIRECTORY = "game-import";
    private static final String BACKUP_DIRECTORY = "game-previous";
    private static final String ACTION_CHOOSE_GAME_FOLDER =
        "io.github.inkmakerdroid.darkengine.action.CHOOSE_GAME_FOLDER";
    private static final String PREFERENCES = "game-storage";
    private static final String PREF_STORAGE_MODE = "storage-mode";
    private static final String PREF_GAME_PATH = "game-path";
    private static final String PREF_TREE_URI = "tree-uri";
    private static final String MODE_DIRECT = "direct";
    private static final String MODE_COPY = "copy";
    private static final String STATE_PENDING_TREE_URI = "pending-tree-uri";
    private static final String STATE_PENDING_DIRECT_PATH = "pending-direct-path";

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private TextView statusView;
    private ProgressBar progressView;
    private Button chooseButton;
    private Button directButton;
    private Button copyButton;
    private boolean pickerOpen;
    private boolean importRunning;
    private boolean awaitingStorageAccess;
    private Uri pendingTreeUri;
    private String pendingDirectPath;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        if (savedInstanceState != null) {
            String treeUri = savedInstanceState.getString(STATE_PENDING_TREE_URI);
            if (treeUri != null) {
                pendingTreeUri = Uri.parse(treeUri);
            }
            pendingDirectPath = savedInstanceState.getString(
                STATE_PENDING_DIRECT_PATH);
        }

        File activeGame = activeGameDirectory();
        if (!isFolderSelectionRequested(getIntent()) && activeGame != null) {
            startGame(activeGame);
            return;
        }
        createSelectionView();
        if (pendingTreeUri != null) {
            showStorageChoices();
        } else if (savedInstanceState == null) {
            openDirectoryPicker();
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        if (!isFolderSelectionRequested(intent)) {
            File activeGame = activeGameDirectory();
            if (activeGame != null) {
                startGame(activeGame);
                return;
            }
        }
        createSelectionView();
        openDirectoryPicker();
    }

    @Override
    protected void onSaveInstanceState(Bundle state) {
        super.onSaveInstanceState(state);
        if (pendingTreeUri != null) {
            state.putString(STATE_PENDING_TREE_URI, pendingTreeUri.toString());
        }
        if (pendingDirectPath != null) {
            state.putString(STATE_PENDING_DIRECT_PATH, pendingDirectPath);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!awaitingStorageAccess) {
            return;
        }
        awaitingStorageAccess = false;
        if (hasDirectStorageAccess()) {
            activateDirectSelection();
        } else if (statusView != null) {
            statusView.setText(R.string.direct_access_denied);
            setChoiceButtonsEnabled(true);
        }
    }

    private File storageRoot() {
        File root = getExternalFilesDir(null);
        return root != null ? root : getFilesDir();
    }

    private File gameDirectory() {
        return new File(storageRoot(), GAME_DIRECTORY);
    }

    private SharedPreferences preferences() {
        return getSharedPreferences(PREFERENCES, MODE_PRIVATE);
    }

    private static boolean isFolderSelectionRequested(Intent intent) {
        return intent != null && ACTION_CHOOSE_GAME_FOLDER.equals(intent.getAction());
    }

    private File activeGameDirectory() {
        SharedPreferences preferences = preferences();
        String mode = preferences.getString(PREF_STORAGE_MODE, "");
        if (MODE_DIRECT.equals(mode)) {
            String path = preferences.getString(PREF_GAME_PATH, "");
            if (!path.isEmpty()) {
                File directory = new File(path);
                if (isRecognizableGame(directory)) {
                    return directory;
                }
            }
            return null;
        }

        File imported = gameDirectory();
        if (isRecognizableGame(imported)) {
            // Upgrade the original one-shot importer without forcing its data
            // to be selected and copied again.
            if (!MODE_COPY.equals(mode)) {
                rememberSelection(MODE_COPY, imported, null);
            }
            return imported;
        }
        return null;
    }

    private void rememberSelection(String mode, File directory, Uri treeUri) {
        SharedPreferences.Editor editor = preferences().edit()
            .putString(PREF_STORAGE_MODE, mode)
            .putString(PREF_GAME_PATH, directory.getAbsolutePath());
        if (treeUri != null) {
            editor.putString(PREF_TREE_URI, treeUri.toString());
        } else {
            editor.remove(PREF_TREE_URI);
        }
        editor.apply();
    }

    private void createSelectionView() {
        final int padding = Math.round(32.0f * getResources().getDisplayMetrics().density);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(padding, padding, padding, padding);
        layout.setBackgroundColor(Color.rgb(16, 16, 16));

        TextView title = new TextView(this);
        title.setText(R.string.import_title);
        title.setTextColor(Color.WHITE);
        title.setTextSize(26.0f);
        title.setGravity(Gravity.CENTER);
        layout.addView(title, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT));

        statusView = new TextView(this);
        statusView.setText(R.string.import_description);
        statusView.setTextColor(Color.LTGRAY);
        statusView.setTextSize(17.0f);
        statusView.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT);
        statusParams.setMargins(0, padding / 2, 0, padding / 2);
        layout.addView(statusView, statusParams);

        progressView = new ProgressBar(this);
        progressView.setIndeterminate(true);
        progressView.setVisibility(View.GONE);
        layout.addView(progressView);

        directButton = new Button(this);
        directButton.setText(R.string.use_folder_directly);
        directButton.setVisibility(View.GONE);
        directButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                requestDirectAccess();
            }
        });
        layout.addView(directButton);

        copyButton = new Button(this);
        copyButton.setText(R.string.copy_for_faster_loading);
        copyButton.setVisibility(View.GONE);
        copyButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (pendingTreeUri != null) {
                    beginImport(pendingTreeUri);
                }
            }
        });
        layout.addView(copyButton);

        chooseButton = new Button(this);
        chooseButton.setText(R.string.choose_game_folder);
        chooseButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                openDirectoryPicker();
            }
        });
        layout.addView(chooseButton);

        setContentView(layout);
    }

    private void openDirectoryPicker() {
        if (pickerOpen || importRunning) {
            return;
        }

        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION |
                        Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION |
                        Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        try {
            pickerOpen = true;
            startActivityForResult(intent, REQUEST_GAME_DIRECTORY);
        } catch (ActivityNotFoundException exception) {
            pickerOpen = false;
            showFailure(getString(R.string.folder_picker_unavailable));
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_STORAGE_ACCESS) {
            return;
        }
        if (requestCode != REQUEST_GAME_DIRECTORY) {
            return;
        }

        pickerOpen = false;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            statusView.setText(R.string.no_folder_selected);
            return;
        }

        final Uri treeUri = data.getData();
        try {
            if ((data.getFlags() & Intent.FLAG_GRANT_READ_URI_PERMISSION) != 0) {
                getContentResolver().takePersistableUriPermission(
                    treeUri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
            }
        } catch (SecurityException | IllegalArgumentException ignored) {
            // The transient read grant remains valid for this import.
        }

        try {
            if (!isRecognizableTree(treeUri)) {
                throw new IOException(
                    "The selected folder does not directly contain cam.cfg " +
                    "and recognizable Thief mission data.");
            }
        } catch (IOException exception) {
            showFailure(getString(R.string.selection_failed,
                                  safeMessage(exception)));
            return;
        }

        pendingTreeUri = treeUri;
        pendingDirectPath = directPathForTree(treeUri);
        showStorageChoices();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != REQUEST_LEGACY_STORAGE_ACCESS) {
            return;
        }
        if (hasDirectStorageAccess()) {
            activateDirectSelection();
        } else {
            statusView.setText(R.string.direct_access_denied);
            setChoiceButtonsEnabled(true);
        }
    }

    private boolean isRecognizableTree(Uri treeUri) throws IOException {
        final String rootDocumentId;
        try {
            rootDocumentId = DocumentsContract.getTreeDocumentId(treeUri);
        } catch (IllegalArgumentException exception) {
            throw new IOException("The selected location is not a directory", exception);
        }

        boolean hasConfiguration = false;
        boolean hasGameSignature = false;
        for (DocumentEntry entry : listChildren(treeUri, rootDocumentId)) {
            if (DocumentsContract.Document.MIME_TYPE_DIR.equals(entry.mimeType)) {
                continue;
            }
            if (entry.name.equalsIgnoreCase("cam.cfg")) {
                hasConfiguration = true;
            } else if (entry.name.equalsIgnoreCase("MISS18.MIS") ||
                       entry.name.equalsIgnoreCase("MISS8.MIS") ||
                       entry.name.equalsIgnoreCase("DARK.GAM")) {
                hasGameSignature = true;
            }
        }
        return hasConfiguration && hasGameSignature;
    }

    private void showStorageChoices() {
        progressView.setVisibility(View.GONE);
        directButton.setVisibility(
            pendingDirectPath == null ? View.GONE : View.VISIBLE);
        copyButton.setVisibility(View.VISIBLE);
        chooseButton.setVisibility(View.VISIBLE);
        chooseButton.setText(R.string.choose_another_folder);
        setChoiceButtonsEnabled(true);

        if (pendingDirectPath == null) {
            statusView.setText(R.string.copy_only_description);
        } else if (isRemovablePath(pendingDirectPath)) {
            statusView.setText(R.string.storage_choice_removable);
        } else {
            statusView.setText(R.string.storage_choice_internal);
        }
    }

    private void setChoiceButtonsEnabled(boolean enabled) {
        if (directButton != null) {
            directButton.setEnabled(enabled);
        }
        if (copyButton != null) {
            copyButton.setEnabled(enabled);
        }
        if (chooseButton != null) {
            chooseButton.setEnabled(enabled);
        }
    }

    private static boolean isRemovablePath(String path) {
        return path.startsWith("/storage/") &&
               !path.startsWith("/storage/emulated/");
    }

    private String directPathForTree(Uri treeUri) {
        if (!"com.android.externalstorage.documents".equals(treeUri.getAuthority())) {
            return null;
        }

        final String documentId;
        try {
            documentId = DocumentsContract.getTreeDocumentId(treeUri);
        } catch (IllegalArgumentException exception) {
            return null;
        }
        int separator = documentId.indexOf(':');
        String volume = separator >= 0 ? documentId.substring(0, separator) : documentId;
        String relative = separator >= 0 ? documentId.substring(separator + 1) : "";
        File volumeRoot = volume.equalsIgnoreCase("primary")
            ? Environment.getExternalStorageDirectory()
            : new File("/storage", volume);
        File selected = relative.isEmpty() ? volumeRoot : new File(volumeRoot, relative);
        try {
            String rootPath = volumeRoot.getCanonicalPath();
            String selectedPath = selected.getCanonicalPath();
            if (!selectedPath.equals(rootPath) &&
                !selectedPath.startsWith(rootPath + File.separator)) {
                return null;
            }
            return selectedPath;
        } catch (IOException exception) {
            return null;
        }
    }

    private boolean hasDirectStorageAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        }
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) !=
            PackageManager.PERMISSION_GRANTED) {
            return false;
        }
        return Build.VERSION.SDK_INT > Build.VERSION_CODES.P ||
               checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) ==
               PackageManager.PERMISSION_GRANTED;
    }

    private void requestDirectAccess() {
        if (pendingDirectPath == null || pendingTreeUri == null) {
            return;
        }
        setChoiceButtonsEnabled(false);
        if (hasDirectStorageAccess()) {
            activateDirectSelection();
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Intent permissionIntent = new Intent(
                Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                Uri.parse("package:" + getPackageName()));
            try {
                awaitingStorageAccess = true;
                startActivityForResult(permissionIntent, REQUEST_STORAGE_ACCESS);
            } catch (ActivityNotFoundException exception) {
                awaitingStorageAccess = true;
                startActivityForResult(
                    new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION),
                    REQUEST_STORAGE_ACCESS);
            }
            return;
        }

        ArrayList<String> permissions = new ArrayList<>();
        permissions.add(Manifest.permission.READ_EXTERNAL_STORAGE);
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
            permissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        }
        requestPermissions(permissions.toArray(new String[0]),
                           REQUEST_LEGACY_STORAGE_ACCESS);
    }

    private void activateDirectSelection() {
        if (pendingDirectPath == null) {
            return;
        }
        File directory = new File(pendingDirectPath);
        if (!isRecognizableGame(directory)) {
            statusView.setText(R.string.direct_access_failed);
            setChoiceButtonsEnabled(true);
            return;
        }
        rememberSelection(MODE_DIRECT, directory, pendingTreeUri);
        startGame(directory);
    }

    private void beginImport(final Uri treeUri) {
        importRunning = true;
        setChoiceButtonsEnabled(false);
        progressView.setVisibility(View.VISIBLE);
        statusView.setText(R.string.checking_game_folder);

        Thread importer = new Thread(new Runnable() {
            @Override
            public void run() {
                final File staging = new File(storageRoot(), IMPORT_DIRECTORY);
                try {
                    if (!deleteTree(staging)) {
                        throw new IOException("Could not clear the previous import directory");
                    }
                    if (!staging.mkdirs() && !staging.isDirectory()) {
                        throw new IOException("Could not create the import directory");
                    }

                    ImportProgress progress = new ImportProgress();
                    copySelectedTree(treeUri, staging, progress);
                    if (!isRecognizableGame(staging)) {
                        throw new IOException(
                            "The selected folder is not a recognizable Thief installation. " +
                            "Select the folder that directly contains cam.cfg and the mission files.");
                    }

                    activateImport(staging);

                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            importRunning = false;
                            File imported = gameDirectory();
                            rememberSelection(MODE_COPY, imported, treeUri);
                            startGame(imported);
                        }
                    });
                } catch (final Exception exception) {
                    deleteTree(staging);
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            importRunning = false;
                            showFailure(getString(
                                R.string.import_failed, safeMessage(exception)));
                        }
                    });
                }
            }
        }, "Thief data importer");
        importer.start();
    }

    private void copySelectedTree(Uri treeUri, File destination,
                                  ImportProgress progress) throws IOException {
        final String rootDocumentId;
        try {
            rootDocumentId = DocumentsContract.getTreeDocumentId(treeUri);
        } catch (IllegalArgumentException exception) {
            throw new IOException("The selected location is not a directory", exception);
        }
        copyDirectory(treeUri, rootDocumentId, destination, "", progress);
    }

    private void activateImport(File staging) throws IOException {
        File root = storageRoot();
        File game = new File(root, GAME_DIRECTORY);
        File backup = new File(root, BACKUP_DIRECTORY);
        if (!deleteTree(backup)) {
            throw new IOException("Could not clear the previous game backup");
        }

        boolean hasBackup = false;
        if (game.exists()) {
            if (!game.renameTo(backup)) {
                throw new IOException("Could not preserve the existing game directory");
            }
            hasBackup = true;
        }

        if (!staging.renameTo(game)) {
            if (hasBackup) {
                backup.renameTo(game);
            }
            throw new IOException("Could not activate the imported game directory");
        }
        if (hasBackup) {
            deleteTree(backup);
        }
    }

    private void copyDirectory(Uri treeUri, String documentId, File destination,
                               String relativePath, ImportProgress progress)
                               throws IOException {
        if (!destination.isDirectory() && !destination.mkdirs()) {
            throw new IOException("Could not create " + destination.getName());
        }

        for (DocumentEntry entry : listChildren(treeUri, documentId)) {
            validateName(entry.name);
            File output = safeChild(destination, entry.name);
            String displayPath = relativePath.isEmpty()
                ? entry.name : relativePath + "/" + entry.name;
            if (DocumentsContract.Document.MIME_TYPE_DIR.equals(entry.mimeType)) {
                copyDirectory(treeUri, entry.documentId, output, displayPath, progress);
            } else {
                copyFile(treeUri, entry, output);
                progress.files++;
                progress.bytes += Math.max(0L, entry.size);
                postProgress(displayPath, progress);
            }
        }
    }

    private List<DocumentEntry> listChildren(Uri treeUri, String documentId)
                                             throws IOException {
        Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(
            treeUri, documentId);
        String[] columns = {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE,
            DocumentsContract.Document.COLUMN_SIZE
        };
        ArrayList<DocumentEntry> entries = new ArrayList<>();
        Cursor cursor = null;
        try {
            cursor = getContentResolver().query(childrenUri, columns,
                                                null, null, null);
            if (cursor == null) {
                throw new IOException("The selected folder could not be read");
            }
            while (cursor.moveToNext()) {
                entries.add(new DocumentEntry(
                    cursor.getString(0), cursor.getString(1),
                    cursor.getString(2), cursor.isNull(3) ? 0L : cursor.getLong(3)));
            }
        } catch (SecurityException exception) {
            throw new IOException("Permission to read the selected folder was denied", exception);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return entries;
    }

    private void copyFile(Uri treeUri, DocumentEntry entry, File output)
                          throws IOException {
        Uri documentUri = DocumentsContract.buildDocumentUriUsingTree(
            treeUri, entry.documentId);
        ContentResolver resolver = getContentResolver();
        try (InputStream source = resolver.openInputStream(documentUri);
             FileOutputStream target = new FileOutputStream(output)) {
            if (source == null) {
                throw new IOException("Could not open " + entry.name);
            }
            byte[] buffer = new byte[128 * 1024];
            int count;
            while ((count = source.read(buffer)) != -1) {
                target.write(buffer, 0, count);
            }
        }
    }

    private void postProgress(final String path, final ImportProgress progress) {
        long now = SystemClock.uptimeMillis();
        if (now - progress.lastUpdate < 200L) {
            return;
        }
        progress.lastUpdate = now;
        final long files = progress.files;
        final long bytes = progress.bytes;
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                statusView.setText(getString(R.string.import_progress,
                    path, files, bytes / (1024.0 * 1024.0)));
            }
        });
    }

    private static File safeChild(File parent, String name) throws IOException {
        File child = new File(parent, name);
        String parentPath = parent.getCanonicalPath() + File.separator;
        if (!child.getCanonicalPath().startsWith(parentPath)) {
            throw new IOException("Unsafe file name in selected folder");
        }
        return child;
    }

    private static void validateName(String name) throws IOException {
        if (name == null || name.isEmpty() || name.equals(".") || name.equals("..") ||
            name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 || name.indexOf('\0') >= 0) {
            throw new IOException("Invalid file name in selected folder");
        }
    }

    private static boolean isRecognizableGame(File directory) {
        File[] files = directory.listFiles();
        if (files == null) {
            return false;
        }

        boolean hasConfiguration = false;
        boolean hasGameSignature = false;
        for (File file : files) {
            if (!file.isFile()) {
                continue;
            }
            String name = file.getName();
            if (name.equalsIgnoreCase("cam.cfg")) {
                hasConfiguration = true;
            } else if (name.equalsIgnoreCase("MISS18.MIS") ||
                       name.equalsIgnoreCase("MISS8.MIS") ||
                       name.equalsIgnoreCase("DARK.GAM")) {
                hasGameSignature = true;
            }
        }
        return hasConfiguration && hasGameSignature;
    }

    private static boolean deleteTree(File file) {
        if (!file.exists()) {
            return true;
        }
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) {
                    if (!deleteTree(child)) {
                        return false;
                    }
                }
            }
        }
        return file.delete();
    }

    private void showFailure(String message) {
        progressView.setVisibility(View.GONE);
        directButton.setVisibility(View.GONE);
        copyButton.setVisibility(View.GONE);
        chooseButton.setEnabled(true);
        chooseButton.setVisibility(View.VISIBLE);
        chooseButton.setText(R.string.choose_game_folder);
        statusView.setText(message);
    }

    private void startGame(File gameDirectory) {
        if (ThiefActivity.isRunning()) {
            restartApplication();
            return;
        }
        Intent intent = new Intent(this, ThiefActivity.class);
        intent.putExtra(ThiefActivity.EXTRA_GAME_PATH,
                        gameDirectory.getAbsolutePath());
        if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
            Intent launchIntent = getIntent();
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_CONTENT);
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_FAN_MISSION);
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_QUICKSTART);
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_QUICKSTART_POS);
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_QUICKSTART_ANGLES);
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_TEXTURE_AUDIT);
            if (launchIntent.hasExtra(ThiefActivity.EXTRA_AUTO_STOP)) {
                intent.putExtra(ThiefActivity.EXTRA_AUTO_STOP,
                    launchIntent.getIntExtra(ThiefActivity.EXTRA_AUTO_STOP, 0));
            }
        }
        startActivity(intent);
        finish();
    }

    private void restartApplication() {
        Intent relaunch = new Intent(this, LauncherActivity.class);
        relaunch.setAction(Intent.ACTION_MAIN);
        relaunch.addCategory(Intent.CATEGORY_LAUNCHER);
        relaunch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK |
                          Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pending = PendingIntent.getActivity(
            this, 0, relaunch,
            PendingIntent.FLAG_CANCEL_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        AlarmManager alarms = (AlarmManager)getSystemService(ALARM_SERVICE);
        alarms.set(AlarmManager.ELAPSED_REALTIME,
                   SystemClock.elapsedRealtime() + 250L, pending);
        android.os.Process.killProcess(android.os.Process.myPid());
    }

    private static void copyDebugStringExtra(Intent source,
                                             Intent destination,
                                             String name) {
        String value = source.getStringExtra(name);
        if (value != null) {
            destination.putExtra(name, value);
        }
    }

    private static String safeMessage(Exception exception) {
        String message = exception.getMessage();
        return message == null || message.isEmpty()
            ? exception.getClass().getSimpleName() : message;
    }

    private static final class DocumentEntry {
        final String documentId;
        final String name;
        final String mimeType;
        final long size;

        DocumentEntry(String documentId, String name, String mimeType, long size) {
            this.documentId = documentId;
            this.name = name;
            this.mimeType = mimeType;
            this.size = size;
        }
    }

    private static final class ImportProgress {
        long files;
        long bytes;
        long lastUpdate;
    }
}
