package dev.glist.android;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.SurfaceView;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.text.InputType;
import dev.glist.android.lib.GlistNative;

public class GlistSurfaceView extends SurfaceView {
    public GlistSurfaceView(Context context) {
        super(context);
        initView();
    }

    public GlistSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
    }

    public GlistSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initView();
    }

    private void initView() {
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    @Override
    public boolean onCheckIsTextEditor() {
        return true;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        outAttrs.inputType = InputType.TYPE_CLASS_TEXT;
        outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE;
        return new BaseInputConnection(this, false) {
            private String composingText = "";

            @Override
            public boolean setComposingText(CharSequence text, int newCursorPosition) {
                int len = composingText.codePointCount(0, composingText.length());
                for (int i = 0; i < len; i++) {
                    GlistNative.onKeyDown(KeyEvent.KEYCODE_DEL);
                    GlistNative.onKeyUp(KeyEvent.KEYCODE_DEL);
                }
                composingText = text.toString();
                int textLen = composingText.length();
                for (int i = 0; i < textLen; ) {
                    int codepoint = Character.codePointAt(composingText, i);
                    GlistNative.onCharPressed(codepoint);
                    i += Character.charCount(codepoint);
                }
                return true;
            }

            @Override
            public boolean commitText(CharSequence text, int newCursorPosition) {
                int len = composingText.codePointCount(0, composingText.length());
                for (int i = 0; i < len; i++) {
                    GlistNative.onKeyDown(KeyEvent.KEYCODE_DEL);
                    GlistNative.onKeyUp(KeyEvent.KEYCODE_DEL);
                }
                composingText = "";
                int textLen = text.length();
                for (int i = 0; i < textLen; ) {
                    int codepoint = Character.codePointAt(text, i);
                    GlistNative.onCharPressed(codepoint);
                    i += Character.charCount(codepoint);
                }
                return true;
            }

            @Override
            public boolean finishComposingText() {
                composingText = "";
                return super.finishComposingText();
            }

            @Override
            public boolean sendKeyEvent(KeyEvent event) {
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    GlistNative.onKeyDown(event.getKeyCode());
                } else if (event.getAction() == KeyEvent.ACTION_UP) {
                    GlistNative.onKeyUp(event.getKeyCode());
                }
                return true;
            }

            @Override
            public boolean deleteSurroundingText(int beforeLength, int afterLength) {
                for (int i = 0; i < beforeLength; i++) {
                    GlistNative.onKeyDown(KeyEvent.KEYCODE_DEL);
                    GlistNative.onKeyUp(KeyEvent.KEYCODE_DEL);
                }
                return true;
            }
        };
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        GlistNative.onKeyDown(keyCode);
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        GlistNative.onKeyUp(keyCode);
        return super.onKeyUp(keyCode, event);
    }

    private android.view.ActionMode actionMode;

    public void showSelectionMenu(final boolean canCut, final boolean canCopy, final boolean canPaste) {
        if (actionMode != null) {
            actionMode.finish();
            actionMode = null;
        }

        android.view.ActionMode.Callback callback = new android.view.ActionMode.Callback() {
            @Override
            public boolean onCreateActionMode(android.view.ActionMode mode, android.view.Menu menu) {
                if (canCut) menu.add(0, android.R.id.cut, 0, android.R.string.cut);
                if (canCopy) menu.add(0, android.R.id.copy, 0, android.R.string.copy);
                if (canPaste) menu.add(0, android.R.id.paste, 0, android.R.string.paste);
                return true;
            }

            @Override
            public boolean onPrepareActionMode(android.view.ActionMode mode, android.view.Menu menu) {
                return false;
            }

            @Override
            public boolean onActionItemClicked(android.view.ActionMode mode, android.view.MenuItem item) {
                int id = item.getItemId();
                if (id == android.R.id.cut) {
                    GlistNative.onCutPressed();
                    mode.finish();
                    return true;
                } else if (id == android.R.id.copy) {
                    GlistNative.onCopyPressed();
                    mode.finish();
                    return true;
                } else if (id == android.R.id.paste) {
                    GlistNative.onPastePressed();
                    mode.finish();
                    return true;
                }
                return false;
            }

            @Override
            public void onDestroyActionMode(android.view.ActionMode mode) {
                actionMode = null;
            }
        };

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            actionMode = startActionMode(callback, android.view.ActionMode.TYPE_FLOATING);
        } else {
            actionMode = startActionMode(callback);
        }
    }

    public void hideSelectionMenu() {
        if (actionMode != null) {
            actionMode.finish();
            actionMode = null;
        }
    }
}
