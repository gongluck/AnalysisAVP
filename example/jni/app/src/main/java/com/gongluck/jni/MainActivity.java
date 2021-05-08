package com.gongluck.jni;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;
import android.widget.VideoView;
import android.widget.Button;
import android.view.View;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    // Used to load the 'native-lib' library on application startup.
    static {
        //加载C库
        System.loadLibrary("native-lib");
        //System.loadLibrary("add");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI() + stringFromJNIGONGLUCK("gongluck") + stringFromJNIGONGLUCK(" goodluck!"));//调用C库函数

        Button button = (Button)findViewById(R.id.sample_button);
        button.setOnClickListener(this);

        //VideoView vv = findViewById(R.id.sample_video);
        //nativeWindowView(vv.getHolder().getSurface());
    }

    public void onClick(View v) {
        VideoView vv = findViewById(R.id.sample_video);
        nativeWindowView(vv.getHolder().getSurface());
        JClass obj = new JClass();
        nativeSaveJObject(obj);
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();//声明本地方法
    public native String stringFromJNIGONGLUCK(String str);
    public native void nativeWindowView(Object surface);
    public native void nativeSaveJObject(JClass obj);

}
