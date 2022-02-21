package com.gongluck.webrtc_android;

import android.content.Intent;
import android.graphics.Point;
import android.media.MediaRecorder;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
//websocket
import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;
import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.AudioSource;
//webrtc
import org.webrtc.AudioTrack;
import org.webrtc.DataChannel;
import org.webrtc.DefaultVideoDecoderFactory;
import org.webrtc.DefaultVideoEncoderFactory;
import org.webrtc.EglBase;
import org.webrtc.IceCandidate;
import org.webrtc.MediaConstraints;
import org.webrtc.MediaStream;
import org.webrtc.MediaStreamTrack;
import org.webrtc.PeerConnection;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.RendererCommon;
import org.webrtc.RtpReceiver;
import org.webrtc.ScreenCapturerAndroid;
import org.webrtc.SdpObserver;
import org.webrtc.SessionDescription;
import org.webrtc.SurfaceTextureHelper;
import org.webrtc.SurfaceViewRenderer;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoDecoderFactory;
import org.webrtc.VideoEncoderFactory;
import org.webrtc.VideoSource;
import org.webrtc.VideoTrack;

public class MainActivity extends AppCompatActivity {

  static final String TAG = "webrtc-android";
  //自定义信令
  public static final String SIGNAL_TYPE_JOIN = "join"; //加入房间
  public static final String SIGNAL_TYPE_RESP_JOIN = "resp-join"; //加入命令回复
  public static final String SIGNAL_TYPE_NEW_PEER = "new-peer"; //通知有对端加入
  public static final String SIGNAL_TYPE_LEAVE = "leave"; //离开房间
  public static final String SIGNAL_TYPE_RESP_LEAVE = "resp-leave"; //离开命令回复
  public static final String SIGNAL_TYPE_PEER_LEAVE = "peer-leave"; //通知有对端离开
  public static final String SIGNAL_TYPE_OFFER = "offer"; //发送offer
  public static final String SIGNAL_TYPE_ANSWER = "answer"; //发送answer
  public static final String SIGNAL_TYPE_CANDIDATE = "candidate"; //发送ice candidate
  //控件
  private SurfaceViewRenderer mLocal = null; //画面预览控件
  private SurfaceViewRenderer mRemote = null;
  private Button mStart = null;
  private Button mStop = null;
  private EditText mAddr = null;
  private EditText mRoom = null;
  //ice
  private String stun = "stun:192.168.0.142:3478";
  private String turn = "turn:192.168.0.142:3478";
  private String iceuser = "gongluck";
  private String icepwd = "123456";
  //实例
  private WebSocketClient mWs = null;
  private PeerConnectionFactory mPeerConnFactory = null;
  private PeerConnection mPeerConn = null;
  private VideoCapturer mVideoCapturer = null; //webrtc屏幕采集对象
  private VideoSource mVsource = null;
  private VideoTrack mVtrack = null;
  private AudioSource mAsource = null;
  private AudioTrack mAtrack = null;

  //Opengl
  private EglBase mGlbace = null;

  //Screen
  private int PROJECTION_REQUEST_CODE = 100; //自定义屏幕采集权限获取标识码
  private Intent mIntent;

  private String mRoomid = null;
  private String mUid = UUID.randomUUID().toString();
  private String mRemoteid = null;

  //设置本地画面控件
  void doOpenLocal() {
    //webrtc预览控件
    mLocal = findViewById(R.id.view_local);
    //本地预览控件设置
    mLocal.init(mGlbace.getEglBaseContext(), null);
    mLocal.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FIT); //SCALE_ASPECT_FIT等比缩放
    mLocal.setEnableHardwareScaler(true);
  }

  //设置远端画面控件
  void doOpenRemote() {
    //webrtc预览控件
    mRemote = findViewById(R.id.view_remote);
    //远端预览控件设置
    mRemote.init(mGlbace.getEglBaseContext(), null);
    mRemote.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FIT);
    mRemote.setEnableHardwareScaler(true);
  }

  @Override
  protected void onActivityResult( //权限获取结果回调
    int requestCode, //请求标识码
    int resultCode, //请求结果
    @Nullable Intent data //消息传递对象
  ) {
    super.onActivityResult(requestCode, resultCode, data);
    if (requestCode != PROJECTION_REQUEST_CODE) {
      Log.w(TAG, "unknow request code : " + requestCode);
      return;
    }
    if (resultCode != RESULT_OK) {
      Log.e(TAG, "permission denied !");
      return;
    }
    Log.w(TAG, "got result : " + data);
    mIntent = data;
  }

  //创建PeerconnectionFactory
  void doCreatePeerconnectionFactory() {
    Log.i(TAG, "create new peerconnection");
    //PeerConnectionFactory
    PeerConnectionFactory.initialize(
      PeerConnectionFactory.InitializationOptions
        .builder(this)
        .createInitializationOptions()
    );
    //编解码器必须要设置，否则createoffer会报错：Failed to set local offer sdp: Failed to set local video description recv parameters for m-section with mid='video'
    VideoEncoderFactory encoderFactory = new DefaultVideoEncoderFactory(
      mGlbace.getEglBaseContext(),
      true,
      true
    );
    VideoDecoderFactory decoderFactory = new DefaultVideoDecoderFactory(
      mGlbace.getEglBaseContext()
    );
    PeerConnectionFactory.Builder builder = PeerConnectionFactory
      .builder()
      .setVideoEncoderFactory(encoderFactory)
      .setVideoDecoderFactory(decoderFactory);
    mPeerConnFactory = builder.createPeerConnectionFactory();
  }

  //开始采集屏幕
  void doStartCapture() {
    doStopCapture();
    //数据：capture -> source -> track -> SurfaceViewRenderer & peerconnection
    //VideoCapturer
    mVideoCapturer =
      new ScreenCapturerAndroid(
        mIntent,
        new MediaProjection.Callback() {
          @Override
          public void onStop() {
            super.onStop();
          }
        }
      );
    //VideoSource
    mVsource =
      mPeerConnFactory.createVideoSource(mVideoCapturer.isScreencast());
    SurfaceTextureHelper helper = SurfaceTextureHelper.create(
      "capturethread",
      mGlbace.getEglBaseContext()
    );
    //初始化capturer，绑定SurfaceTextureHelper和Source的观察者
    mVideoCapturer.initialize(
      helper,
      getApplicationContext(),
      mVsource.getCapturerObserver() //采集数据回调给vsource
    );
    //获取屏幕大小
    Display defaultDisplay = getWindowManager().getDefaultDisplay();
    Point point = new Point();
    defaultDisplay.getSize(point);
    Log.i(TAG, "display pos : " + point.x + "," + point.y);
    //开始采集
    mVideoCapturer.startCapture(point.x, point.y, 30/*ignored*/);
    //AudioSource
    mAsource = mPeerConnFactory.createAudioSource(new MediaConstraints());
  }

  //停止采集屏幕
  void doStopCapture() {
    if (mVideoCapturer == null) return;
    try {
      //停止采集
      mVideoCapturer.stopCapture();
      mVideoCapturer.dispose();
      mVideoCapturer = null;
      mVsource = null;
    } catch (InterruptedException e) {
      e.printStackTrace();
    }
  }

  //创建PeerConnection
  void doCreatePeerconnection() {
    if (mPeerConn != null) return;
    //ice
    ArrayList<PeerConnection.IceServer> iceServers = new ArrayList<>();
    //ice-stun
    PeerConnection.IceServer iceServer = PeerConnection.IceServer
      .builder(stun)
      .createIceServer();
    iceServers.add(iceServer);
    //ice-turn
    iceServer =
      PeerConnection.IceServer
        .builder(turn)
        .setUsername(iceuser)
        .setPassword(icepwd)
        .createIceServer();
    iceServers.add(iceServer);
    //PeerConnection
    PeerConnection.RTCConfiguration config = new PeerConnection.RTCConfiguration(
      iceServers
    );
    config.bundlePolicy = PeerConnection.BundlePolicy.MAXBUNDLE;
    config.rtcpMuxPolicy = PeerConnection.RtcpMuxPolicy.REQUIRE;
    config.iceTransportsType = PeerConnection.IceTransportsType.ALL;

    doDestroyPeerconnection();
    mPeerConn =
      mPeerConnFactory.createPeerConnection(
        config,
        null,
        new PeerConnection.Observer() {
          @Override
          public void onSignalingChange(
            PeerConnection.SignalingState signalingState
          ) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onSignalingChange : " + signalingState
            );
          }

          @Override
          public void onIceConnectionChange(
            PeerConnection.IceConnectionState iceConnectionState
          ) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onIceConnectionChange : " +
              iceConnectionState
            );
          }

          @Override
          public void onIceConnectionReceivingChange(boolean b) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onIceConnectionReceivingChange : " + b
            );
          }

          @Override
          public void onIceGatheringChange(
            PeerConnection.IceGatheringState iceGatheringState
          ) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onIceGatheringChange : " +
              iceGatheringState
            );
          }

          @Override
          public void onIceCandidate(IceCandidate iceCandidate) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onIceCandidate : " + iceCandidate
            );
            try {
              JSONObject json = new JSONObject();
              JSONObject jsonSdp = new JSONObject();
              json.put("cmd", SIGNAL_TYPE_CANDIDATE);
              json.put("roomid", mRoomid);
              json.put("uid", mUid);
              json.put("remotid", mRemoteid);
              jsonSdp.put("sdpMid", iceCandidate.sdpMid);
              jsonSdp.put("sdpMLineIndex", iceCandidate.sdpMLineIndex);
              Log.i(TAG, "use candidate : " + iceCandidate.sdp);
              jsonSdp.put("candidate", iceCandidate.sdp);
              json.put("sdp", jsonSdp.toString());
              mWs.send(json.toString());
              Log.i(TAG, "send candidate msg : " + json.toString());
            } catch (JSONException e) {
              e.printStackTrace();
            }
          }

          @Override
          public void onIceCandidatesRemoved(IceCandidate[] iceCandidates) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onIceCandidatesRemoved : " +
              iceCandidates
            );
          }

          @Override
          public void onAddStream(MediaStream mediaStream) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onAddStream : " + mediaStream
            );
          }

          @Override
          public void onRemoveStream(MediaStream mediaStream) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onRemoveStream : " + mediaStream
            );
          }

          @Override
          public void onDataChannel(DataChannel dataChannel) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onDataChannel : " + dataChannel
            );
          }

          @Override
          public void onRenegotiationNeeded() {
            Log.i(TAG, "PeerConnection.Observer - onRenegotiationNeeded");
          }

          @Override
          public void onAddTrack(
            RtpReceiver rtpReceiver,
            MediaStream[] mediaStreams
          ) {
            Log.i(
              TAG,
              "PeerConnection.Observer - onAddTrack : " + mediaStreams
            );
            MediaStreamTrack track = rtpReceiver.track();
            if (track instanceof VideoTrack) {
              Log.i(TAG, "onAddVideoTrack");
              VideoTrack remoteVideoTrack = (VideoTrack) track;
              remoteVideoTrack.setEnabled(true);
              remoteVideoTrack.addSink(mRemote); //预览到控件
            }
          }
        }
      );
    List<String> mediaStreamLabels = Collections.singletonList("msid-gongluck");
    //VideoTrack
    mVtrack =
      mPeerConnFactory.createVideoTrack("webrtc-android-video", mVsource);
    mVtrack.setEnabled(true);
    mVtrack.addSink(mLocal); //预览到控件
    mPeerConn.addTrack(mVtrack, mediaStreamLabels);
    //AudioTrack
    mAtrack =
      mPeerConnFactory.createAudioTrack("webrtc-android-audio", mAsource);
    mAtrack.setEnabled(true);
    mPeerConn.addTrack(mAtrack, mediaStreamLabels);
  }

  //销毁peerconnection
  void doDestroyPeerconnection() {
    if (mPeerConn == null) return;
    mPeerConn.close();
    mPeerConn = null;
  }

  //sdp信息观察者
  public static class MySdpObserver implements SdpObserver {

    private String Name;

    public MySdpObserver(String Name) {
      this.Name = Name;
    }

    @Override
    public void onCreateSuccess(SessionDescription sessionDescription) {
      Log.i(TAG, Name + " - onCreateSuccess : " + sessionDescription);
    }

    @Override
    public void onSetSuccess() {
      Log.i(TAG, Name + " - onSetSuccess");
    }

    @Override
    public void onCreateFailure(String msg) {
      Log.e(TAG, Name + " - onCreateFailure : " + msg);
    }

    @Override
    public void onSetFailure(String msg) {
      Log.e(TAG, Name + " - onSetFailure : " + msg);
    }
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    //OpenGL ES上下文
    mGlbace = EglBase.create();
    //控件
    mStart = (Button) findViewById(R.id.btn_start);
    mStop = (Button) findViewById(R.id.btn_stop);
    mAddr = (EditText) findViewById(R.id.addr_edit);
    mRoom = (EditText) findViewById(R.id.roomid_edit);
    doCreatePeerconnectionFactory();
    doOpenLocal();
    doOpenRemote();

    //采集屏幕权限获取
    MediaProjectionManager projectionManager = (MediaProjectionManager) getSystemService(
      MEDIA_PROJECTION_SERVICE
    );
    Intent intent = projectionManager.createScreenCaptureIntent();
    startActivityForResult(intent, PROJECTION_REQUEST_CODE);

    //开始
    mStart.setOnClickListener(
      new View.OnClickListener() {
        @Override
        public void onClick(View v) {
          mStop.callOnClick();
          doStartCapture();
          doCreatePeerconnection();

          String wsaddr = mAddr.getText().toString();
          mRoomid = mRoom.getText().toString();
          Log.i(TAG, "websocket addr : " + wsaddr);
          Log.i(TAG, "room id : " + mRoomid);
          URI uri = null;
          try {
            uri = new URI(wsaddr);
          } catch (URISyntaxException e) {
            e.printStackTrace();
          }
          if (uri == null) {
            Log.e(TAG, "create uri failed");
            return;
          }

          // Create SDP constraints.
          MediaConstraints sdpMediaConstraints = new MediaConstraints();
          sdpMediaConstraints.mandatory.add(
            new MediaConstraints.KeyValuePair("OfferToReceiveAudio", "true")
          );
          sdpMediaConstraints.mandatory.add(
            new MediaConstraints.KeyValuePair("OfferToReceiveVideo", "true")
          );

          //连接信令服务
          mWs =
            new WebSocketClient(uri) {
              @Override
              public void onOpen(ServerHandshake handshakedata) {
                Log.i(TAG, "websocket onOpen");
                try {
                  JSONObject json = new JSONObject();
                  json.put("cmd", SIGNAL_TYPE_JOIN);
                  json.put("roomid", mRoomid);
                  json.put("uid", mUid);
                  mWs.send(json.toString());
                  Log.i(TAG, "send join msg : " + json.toString());
                } catch (JSONException e) {
                  e.printStackTrace();
                }
              }

              @Override
              public void onMessage(String message) {
                Log.i(TAG, "websocket onMessage : " + message);
                try {
                  JSONObject jsonMsg = new JSONObject(message);
                  String rid = jsonMsg.optString("roomid");
                  String uid = jsonMsg.optString("uid");
                  String remoteuid = jsonMsg.optString("remoteuid");
                  String sdp = jsonMsg.optString("sdp");
                  switch (jsonMsg.getString("cmd")) {
                    case SIGNAL_TYPE_RESP_JOIN: //加入房间结果
                      {
                        if (rid.equals(mRoomid) && uid.equals(mUid)) {
                          mRemoteid = remoteuid;
                        }
                      }
                      break;
                    case SIGNAL_TYPE_NEW_PEER: //新节点进入房间
                      {
                        if (rid.equals(mRoomid) && uid.equals(mUid)) {
                          mRemoteid = remoteuid;
                          doCreatePeerconnection();
                          //CreateOffer
                          mPeerConn.createOffer(
                            new MySdpObserver("createOffer") {
                              @Override
                              public void onCreateSuccess(
                                SessionDescription sessionDescription
                              ) {
                                Log.i(TAG, "createOffer - onCreateSuccess");
                                //设置本地媒体sdp
                                mPeerConn.setLocalDescription(
                                  new MySdpObserver("setLocalDescription"),
                                  sessionDescription
                                );
                                JSONObject json = new JSONObject();
                                JSONObject jsonsdp = new JSONObject();
                                try {
                                  json.put("cmd", SIGNAL_TYPE_OFFER);
                                  json.put("roomid", mRoomid);
                                  json.put("uid", mUid);
                                  json.put("remoteid", mRemoteid);
                                  jsonsdp.put(
                                    "type",
                                    sessionDescription.type
                                      .toString()
                                      .toLowerCase()
                                  );
                                  jsonsdp.put(
                                    "sdp",
                                    sessionDescription.description.toString()
                                  );
                                  json.put("sdp", jsonsdp.toString());
                                  mWs.send(json.toString());
                                  Log.i(
                                    TAG,
                                    "send offer msg : " + json.toString()
                                  );
                                } catch (JSONException e) {
                                  throw new RuntimeException(e);
                                }
                              }
                            },
                            sdpMediaConstraints
                          );
                        }
                      }
                      break;
                    case SIGNAL_TYPE_OFFER: //收到offer
                      {
                        JSONObject sdpJson = new JSONObject(sdp);
                        try {
                          SessionDescription desc = new SessionDescription(
                            SessionDescription.Type.OFFER,
                            sdpJson.getString("sdp")
                          );
                          doCreatePeerconnection();
                          // 设置对端SDP
                          mPeerConn.setRemoteDescription(
                            new MySdpObserver("setRemoteDescription"),
                            desc
                          );
                          mPeerConn.createAnswer(
                            new MySdpObserver("createAnswer") {
                              @Override
                              public void onCreateSuccess(
                                SessionDescription sessionDescription
                              ) {
                                Log.i(TAG, "createAnswer - onCreateSuccess");
                                mPeerConn.setLocalDescription(
                                  new MySdpObserver("setLocalDescription"),
                                  sessionDescription
                                );
                                JSONObject json = new JSONObject();
                                JSONObject jsonsdp = new JSONObject();
                                try {
                                  json.put("cmd", SIGNAL_TYPE_ANSWER);
                                  json.put("roomid", mRoomid);
                                  json.put("uid", mUid);
                                  json.put("remoteid", mRemoteid);
                                  jsonsdp.put(
                                    "type",
                                    sessionDescription.type
                                      .toString()
                                      .toLowerCase()
                                  );
                                  jsonsdp.put(
                                    "sdp",
                                    sessionDescription.description.toString()
                                  );
                                  json.put("sdp", jsonsdp.toString());
                                  mWs.send(json.toString());
                                  Log.i(
                                    TAG,
                                    "send answer msg : " + json.toString()
                                  );
                                } catch (JSONException e) {
                                  throw new RuntimeException(e);
                                }
                              }
                            },
                            sdpMediaConstraints
                          );
                        } catch (JSONException e) {
                          throw new RuntimeException(e);
                        }
                      }
                      break;
                    case SIGNAL_TYPE_ANSWER: //收到answer
                      {
                        JSONObject sdpJson = new JSONObject(sdp);
                        try {
                          SessionDescription desc = new SessionDescription(
                            SessionDescription.Type.ANSWER,
                            sdpJson.getString("sdp")
                          );
                          doCreatePeerconnection();
                          mPeerConn.setRemoteDescription(
                            new MySdpObserver("setRemoteDescription"),
                            desc
                          );
                        } catch (JSONException e) {
                          throw new RuntimeException(e);
                        }
                      }
                      break;
                    case SIGNAL_TYPE_CANDIDATE:
                      { //收到ice candidate
                        JSONObject candidate = new JSONObject(sdp);
                        IceCandidate icecandidate = new IceCandidate(
                          candidate.getString("sdpMid"),
                          candidate.getInt("sdpMLineIndex"),
                          candidate.getString("candidate")
                        );
                        doCreatePeerconnection();
                        mPeerConn.addIceCandidate(icecandidate);
                      }
                      break;
                    case SIGNAL_TYPE_RESP_LEAVE: //离开结果
                    case SIGNAL_TYPE_PEER_LEAVE: //对端离开房间
                      {
                        if (rid.equals(mRoomid) && uid.equals(mUid)) {
                          doDestroyPeerconnection();
                        }
                      }
                      break;
                  }
                } catch (JSONException e) {
                  e.printStackTrace();
                }
              }

              @Override
              public void onClose(int code, String reason, boolean remote) {
                Log.i(TAG, "websocket onClose : " + reason);
              }

              @Override
              public void onError(Exception ex) {
                Log.e(TAG, "websocket onError : " + ex.toString());
              }
            };
          // wss需添加
          try {
            SSLContext sslContext = SSLContext.getInstance("TLS");
            sslContext.init(
              null,
              new TrustManager[] {
                new X509TrustManager() {
                  @Override
                  public void checkClientTrusted(
                    X509Certificate[] chain,
                    String authType
                  ) {}

                  @Override
                  public void checkServerTrusted(
                    X509Certificate[] chain,
                    String authType
                  ) {}

                  @Override
                  public X509Certificate[] getAcceptedIssuers() {
                    return new X509Certificate[0];
                  }
                },
              },
              new SecureRandom()
            );
            SSLSocketFactory factory = sslContext.getSocketFactory();
            mWs.setSocket(factory.createSocket());
          } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
          } catch (KeyManagementException e) {
            e.printStackTrace();
          } catch (IOException e) {
            e.printStackTrace();
          }
          mWs.connect();
        }
      }
    );

    //结束
    mStop.setOnClickListener(
      new View.OnClickListener() {
        @Override
        public void onClick(View v) {
          doStopCapture();
          doDestroyPeerconnection();
          if (mWs != null) {
            try {
              JSONObject json = new JSONObject();
              json.put("cmd", SIGNAL_TYPE_LEAVE);
              json.put("roomid", mRoomid);
              json.put("uid", mUid);
              mWs.send(json.toString());
              Log.i(TAG, "send leave msg : " + json.toString());
            } catch (JSONException e) {
              e.printStackTrace();
            }
          }
        }
      }
    );
  }
}
