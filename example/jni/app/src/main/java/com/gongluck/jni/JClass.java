package com.gongluck.jni;

import android.util.Log;

public class JClass {
    private int value;

    public void setValue(int value) {
        Log.i("GONGLUCK","JClass::setValue" + value);
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
