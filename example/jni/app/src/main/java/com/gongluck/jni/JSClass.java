package com.gongluck.jni;

public class JSClass {
    private static int value;

    public static void setValue(int value) {
        JSClass.value = value;
    }

    public static int getValue() {
        return value;
    }
}
