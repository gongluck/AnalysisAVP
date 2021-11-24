package com.gongluck.webrtc_android;

import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

//websocket
import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;
import org.json.JSONException;
import org.json.JSONObject;

//webrtc
import org.webrtc.Camera1Enumerator;
import org.webrtc.Camera2Enumerator;
import org.webrtc.CameraEnumerator;
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
import org.webrtc.SdpObserver;
import org.webrtc.SessionDescription;
import org.webrtc.SurfaceTextureHelper;
import org.webrtc.SurfaceViewRenderer;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoDecoderFactory;
import org.webrtc.VideoEncoderFactory;
import org.webrtc.VideoSource;
import org.webrtc.VideoTrack;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {
    static final String TAG = "webrtc-android";
    public static final String SIGNAL_TYPE_JOIN = "join";
    public static final String SIGNAL_TYPE_RESP_JOIN = "resp-join";
    public static final String SIGNAL_TYPE_LEAVE = "leave";
    public static final String SIGNAL_TYPE_RESP_LEAVE = "resp-leave";
    public static final String SIGNAL_TYPE_NEW_PEER = "new-peer";
    public static final String SIGNAL_TYPE_PEER_LEAVE = "peer-leave";
    public static final String SIGNAL_TYPE_OFFER = "offer";
    public static final String SIGNAL_TYPE_ANSWER = "answer";
    public static final String SIGNAL_TYPE_CANDIDATE = "candidate";
    //控件
    private SurfaceViewRenderer mLocal = null;
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
    private VideoCapturer mVideoCapturer = null;
    private VideoTrack mVtrack = null;

    //stream
    private MediaStream mStream = null;

    //Opengl
    private EglBase mGlbace = null;

    private String mRoomid = null;
    private String mUid = UUID.randomUUID().toString();
    private String mRemoteid = null;

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

        mGlbace = EglBase.create();
        //控件
        mStart = (Button) findViewById(R.id.btn_start);
        mStop = (Button) findViewById(R.id.btn_stop);
        mAddr = (EditText) findViewById(R.id.addr_edit);
        mRoom = (EditText) findViewById(R.id.roomid_edit);
        //webrtc预览控件
        mLocal = findViewById(R.id.view_local);
        mRemote = findViewById(R.id.view_remote);

        doCreatePeerconnection();
        doOpenLocal();
        doOpenRemote();

        //开始
        mStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
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

                mWs = new WebSocketClient(uri) {
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
                                case SIGNAL_TYPE_RESP_JOIN://加入房间结果
                                {
                                    if (rid == mRoomid && uid == mUid) {
                                        mRemoteid = remoteuid;
                                    }
                                }
                                break;
                                case SIGNAL_TYPE_NEW_PEER://新节点进入房间
                                {
                                    if (rid.equals(mRoomid) && uid.equals(mUid)) {
                                        mRemoteid = remoteuid;
                                        if (mPeerConn == null) {
                                            doCreatePeerconnection();
                                        }
                                        //CreateOffer
                                        mPeerConn.createOffer(new MySdpObserver("createOffer") {
                                            @Override
                                            public void onCreateSuccess(SessionDescription sessionDescription) {
                                                Log.i(TAG, "createOffer - onCreateSuccess");
                                                mPeerConn.setLocalDescription(new MySdpObserver("setLocalDescription"), sessionDescription);
                                                JSONObject json = new JSONObject();
                                                JSONObject jsonsdp = new JSONObject();
                                                try {
                                                    json.put("cmd", SIGNAL_TYPE_OFFER);
                                                    json.put("roomid", mRoomid);
                                                    json.put("uid", mUid);
                                                    json.put("remoteid", mRemoteid);
                                                    jsonsdp.put("type", sessionDescription.type.toString().toLowerCase());
                                                    jsonsdp.put("sdp", sessionDescription.description.toString());
                                                    json.put("sdp", jsonsdp.toString());
                                                    mWs.send(json.toString());
                                                    Log.i(TAG, "send offer msg : " + json.toString());
                                                } catch (JSONException e) {
                                                    throw new RuntimeException(e);
                                                }
                                            }
                                        }, new MediaConstraints());
                                    }
                                }
                                break;
                                case SIGNAL_TYPE_OFFER: {
                                    JSONObject sdpJson = new JSONObject(sdp);
                                    try {
                                        SessionDescription desc = new SessionDescription(SessionDescription.Type.OFFER, sdpJson.getString("sdp"));
                                        // 设置对端SDP
                                        mPeerConn.setRemoteDescription(new MySdpObserver("setRemoteDescription"), desc);
                                        mPeerConn.createAnswer(new MySdpObserver("createAnswer") {
                                            @Override
                                            public void onCreateSuccess(SessionDescription sessionDescription) {
                                                Log.i(TAG, "createAnswer - onCreateSuccess");
                                                mPeerConn.setLocalDescription(new MySdpObserver("setLocalDescription"), sessionDescription);
                                                JSONObject json = new JSONObject();
                                                JSONObject jsonsdp = new JSONObject();
                                                try {
                                                    json.put("cmd", SIGNAL_TYPE_ANSWER);
                                                    json.put("roomid", mRoomid);
                                                    json.put("uid", mUid);
                                                    json.put("remoteid", mRemoteid);
                                                    jsonsdp.put("type", sessionDescription.type.toString().toLowerCase());
                                                    jsonsdp.put("sdp", sessionDescription.description.toString());
                                                    json.put("sdp", jsonsdp.toString());
                                                    mWs.send(json.toString());
                                                    Log.i(TAG, "send answer msg : " + json.toString());
                                                } catch (JSONException e) {
                                                    throw new RuntimeException(e);
                                                }
                                            }
                                        }, new MediaConstraints());
                                    } catch (JSONException e) {
                                        throw new RuntimeException(e);
                                    }
                                }
                                break;
                                case SIGNAL_TYPE_ANSWER: {
                                    JSONObject sdpJson = new JSONObject(sdp);
                                    try {
                                        SessionDescription desc = new SessionDescription(SessionDescription.Type.ANSWER, sdpJson.getString("sdp"));
                                        mPeerConn.setRemoteDescription(new MySdpObserver("setRemoteDescription"), desc);
                                    } catch (JSONException e) {
                                        throw new RuntimeException(e);
                                    }
                                }
                                break;
                                case SIGNAL_TYPE_CANDIDATE: {
                                    JSONObject candidate = new JSONObject(sdp);
                                    IceCandidate icecandidate = new IceCandidate(candidate.getString("sdpMid"), candidate.getInt("sdpMLineIndex"), candidate.getString("candidate"));
                                    mPeerConn.addIceCandidate(icecandidate);
                                }
                                break;
                                case SIGNAL_TYPE_RESP_LEAVE:
                                case SIGNAL_TYPE_PEER_LEAVE: {
                                    if (rid.equals(mRoomid) && uid.equals(mUid)) {
                                        // 将流移除出RTC连接
                                        mPeerConn.removeStream(mStream);
                                        mStream = null;
//                                        if (mPeerConn != null) {
//                                            Log.i(TAG, "close mPeerConn");
//                                            mPeerConn.close();
//                                            mPeerConn = null;
//                                        }
//                                        if (mPeerConnFactory != null) {
//                                            Log.i(TAG, "dispose mPeerConnFactory");
//                                            mPeerConnFactory.dispose();
//                                            mPeerConnFactory = null;
//                                        }
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
                        Log.i(TAG, "websocket onError : " + ex.toString());
                    }
                };
                mWs.connect();
                mVideoCapturer.startCapture(mLocal.getWidth(), mLocal.getHeight(), 30);
            }
        });

        //结束
        mStop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
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
        });
    }

    void doCreatePeerconnection() {
        Log.i(TAG, "create new peerconnection");
        //PeerConnectionFactory
        PeerConnectionFactory.initialize(
                PeerConnectionFactory.InitializationOptions.builder(this)
                        .createInitializationOptions());
        //编解码器必须要设置，否则createoffer会报错：Failed to set local offer sdp: Failed to set local video description recv parameters for m-section with mid='video'
        VideoEncoderFactory encoderFactory = new DefaultVideoEncoderFactory(
                mGlbace.getEglBaseContext(), false, true);
        VideoDecoderFactory decoderFactory = new DefaultVideoDecoderFactory(mGlbace.getEglBaseContext());
        PeerConnectionFactory.Builder builder = PeerConnectionFactory.builder()
                .setVideoEncoderFactory(encoderFactory)
                .setVideoDecoderFactory(decoderFactory);
        builder.setOptions(null);
        mPeerConnFactory = builder.createPeerConnectionFactory();
        //ice
        ArrayList<PeerConnection.IceServer> iceServers = new ArrayList<>();
        //ice-turn
        PeerConnection.IceServer iceServer = PeerConnection.IceServer
                .builder(turn)
                .setUsername(iceuser)
                .setPassword(icepwd)
                .createIceServer();
        iceServers.add(iceServer);
        //ice-stun
        iceServer = PeerConnection.IceServer
                .builder(stun)
                .createIceServer();
        iceServers.add(iceServer);
        //PeerConnection
        PeerConnection.RTCConfiguration config = new PeerConnection.RTCConfiguration(iceServers);
        //PeerConnection.RTCConfiguration config = new PeerConnection.RTCConfiguration(new ArrayList<>());
        //config.bundlePolicy = PeerConnection.BundlePolicy.MAXBUNDLE;
        //config.rtcpMuxPolicy = PeerConnection.RtcpMuxPolicy.REQUIRE;
        //config.iceTransportsType = PeerConnection.IceTransportsType.ALL;

        mPeerConn = mPeerConnFactory.createPeerConnection(config, new PeerConnection.Observer() {
            @Override
            public void onSignalingChange(PeerConnection.SignalingState signalingState) {
                Log.i(TAG, "PeerConnection.Observer - onSignalingChange : " + signalingState);
            }

            @Override
            public void onIceConnectionChange(PeerConnection.IceConnectionState iceConnectionState) {
                Log.i(TAG, "PeerConnection.Observer - onIceConnectionChange : " + iceConnectionState);
            }

            @Override
            public void onIceConnectionReceivingChange(boolean b) {
                Log.i(TAG, "PeerConnection.Observer - onIceConnectionReceivingChange : " + b);
            }

            @Override
            public void onIceGatheringChange(PeerConnection.IceGatheringState iceGatheringState) {
                Log.i(TAG, "PeerConnection.Observer - onIceGatheringChange : " + iceGatheringState);
            }

            @Override
            public void onIceCandidate(IceCandidate iceCandidate) {
                Log.i(TAG, "PeerConnection.Observer - onIceCandidate : " + iceCandidate);
                try {
                    JSONObject json = new JSONObject();
                    JSONObject jsonSdp = new JSONObject();
                    json.put("cmd", SIGNAL_TYPE_CANDIDATE);
                    json.put("roomid", mRoomid);
                    json.put("uid", mUid);
                    json.put("remotid", mRemoteid);
                    jsonSdp.put("sdpMid", iceCandidate.sdpMid);
                    jsonSdp.put("sdpMLineIndex", iceCandidate.sdpMLineIndex);
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
                Log.i(TAG, "PeerConnection.Observer - onIceCandidatesRemoved : " + iceCandidates);
            }

            @Override
            public void onAddStream(MediaStream mediaStream) {
                Log.i(TAG, "PeerConnection.Observer - onAddStream : " + mediaStream);
                mStream = mediaStream;
            }

            @Override
            public void onRemoveStream(MediaStream mediaStream) {
                Log.i(TAG, "PeerConnection.Observer - onRemoveStream : " + mediaStream);
            }

            @Override
            public void onDataChannel(DataChannel dataChannel) {
                Log.i(TAG, "PeerConnection.Observer - onDataChannel : " + dataChannel);
            }

            @Override
            public void onRenegotiationNeeded() {
                Log.i(TAG, "PeerConnection.Observer - onRenegotiationNeeded");
            }

            @Override
            public void onAddTrack(RtpReceiver rtpReceiver, MediaStream[] mediaStreams) {
                Log.i(TAG, "PeerConnection.Observer - onAddTrack : " + mediaStreams);
                MediaStreamTrack track = rtpReceiver.track();
                if (track instanceof VideoTrack) {
                    Log.i(TAG, "onAddVideoTrack");
                    VideoTrack remoteVideoTrack = (VideoTrack) track;
                    remoteVideoTrack.setEnabled(true);
                    remoteVideoTrack.addSink(mRemote);
                }
            }
        });
    }

    void doOpenLocal() {
        mLocal.init(mGlbace.getEglBaseContext(), null);
        mLocal.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FIT);
        //mLocal.setMirror(true);
        mLocal.setEnableHardwareScaler(true);

        //音视频采集
        CameraEnumerator enumerator = null;
        if (Camera2Enumerator.isSupported(this)) {
            enumerator = new Camera2Enumerator(this);
        } else {
            enumerator = new Camera1Enumerator(true);
        }
        //获取设备名称
        final String[] deviceNames = enumerator.getDeviceNames();
        for (String deviceName : deviceNames) {
            if (enumerator.isFrontFacing(deviceName)) {
                mVideoCapturer = enumerator.createCapturer(deviceName, null);
                if (mVideoCapturer != null) {
                    break;
                }
            }
        }

        //VideoCapturer
        VideoSource vsource = mPeerConnFactory.createVideoSource(false);
        SurfaceTextureHelper helper = SurfaceTextureHelper.create("capturethread", mGlbace.getEglBaseContext());
        mVideoCapturer.initialize(helper, getApplicationContext(), vsource.getCapturerObserver());

        //VideoTrack
        mVtrack = mPeerConnFactory.createVideoTrack("webrtc-android-video", vsource);
        mVtrack.setEnabled(true);
        mVtrack.addSink(mLocal);//预览到控件
        mPeerConn.addTrack(mVtrack);
    }

    void doOpenRemote() {
        mRemote.init(mGlbace.getEglBaseContext(), null);
        mRemote.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FIT);
        //mRemote.setMirror(true);
        mRemote.setEnableHardwareScaler(true);
    }
}
