package org.opendarkengine.thief2;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.provider.DocumentsContract;
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
 * Finds or imports the separately installed retail game data before SDL starts.
 *
 * Android's Storage Access Framework exposes a selected directory as content
 * URIs, not as a POSIX directory the native engine can chdir into.  Import the
 * selected tree into this application's external-files directory, where the
 * native launcher can use the files normally without broad storage permission.
 */
public final class LauncherActivity extends Activity {
    private static final int REQUEST_GAME_DIRECTORY = 1;
    private static final String GAME_DIRECTORY = "game";
    private static final String IMPORT_DIRECTORY = "game-import";
    private static final String BACKUP_DIRECTORY = "game-previous";

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private TextView statusView;
    private ProgressBar progressView;
    private Button chooseButton;
    private boolean pickerOpen;
    private boolean importRunning;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        if (isRecognizableGame(gameDirectory())) {
            startGame();
            return;
        }

        createImportView();
        if (savedInstanceState == null) {
            openDirectoryPicker();
        }
    }

    private File storageRoot() {
        File root = getExternalFilesDir(null);
        return root != null ? root : getFilesDir();
    }

    private File gameDirectory() {
        return new File(storageRoot(), GAME_DIRECTORY);
    }

    private void createImportView() {
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

        beginImport(treeUri);
    }

    private void beginImport(final Uri treeUri) {
        importRunning = true;
        chooseButton.setEnabled(false);
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
                            startGame();
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
        chooseButton.setEnabled(true);
        statusView.setText(message);
    }

    private void startGame() {
        Intent intent = new Intent(this, ThiefActivity.class);
        if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
            Intent launchIntent = getIntent();
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_QUICKSTART);
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_QUICKSTART_POS);
            copyDebugStringExtra(launchIntent, intent,
                                 ThiefActivity.EXTRA_QUICKSTART_ANGLES);
            if (launchIntent.hasExtra(ThiefActivity.EXTRA_AUTO_STOP)) {
                intent.putExtra(ThiefActivity.EXTRA_AUTO_STOP,
                    launchIntent.getIntExtra(ThiefActivity.EXTRA_AUTO_STOP, 0));
            }
        }
        startActivity(intent);
        finish();
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
