package com.gongluck.microphone;

//参考
//https://www.xp.cn/b.php/88310.html

import androidx.appcompat.app.AppCompatActivity;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.util.Log;

class RecordThread extends Thread {
  static String Tag = "gongluck";
  static final int frequency = 48000;
  static final int channelConfiguration = AudioFormat.CHANNEL_CONFIGURATION_MONO;
  static final int audioEncoding = AudioFormat.ENCODING_PCM_16BIT;
  private boolean recording = true;

  @Override
  public void run() {
    // TODO Auto-generated method stub
    int recBufSize = AudioRecord.getMinBufferSize(frequency, channelConfiguration, audioEncoding) * 2;
    int plyBufSize = AudioTrack.getMinBufferSize(frequency, channelConfiguration, audioEncoding) * 2;
    AudioRecord audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, frequency, channelConfiguration, audioEncoding, recBufSize);
    AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, frequency, channelConfiguration, audioEncoding, plyBufSize, AudioTrack.MODE_STREAM);
    byte[] recBuf = new byte[recBufSize];
    audioRecord.startRecording();
    audioTrack.play();
    int index = 0;
    while (recording) {
      int readLen = audioRecord.read(recBuf, 0, recBufSize);
      if(index++ % 100 == 0){
        Log.i(Tag, "audio record read " + readLen);
      }
      audioTrack.write(recBuf, 0, readLen);
    }
    audioTrack.stop();
    audioRecord.stop();
  }
}

public class MainActivity extends AppCompatActivity {

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    RecordThread rec = new RecordThread();
    rec.start();
  }
}
