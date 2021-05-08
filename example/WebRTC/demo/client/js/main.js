//"use strict"的目的是指定代码在严格条件下执行。
//严格模式下你不能使用未声明的变量。
"use strict";

// 服务器地址
const ServerWebsocket = "wss://www.gongluck.icu:8098/ws";
const Coturn = "www.gongluck.icu:3478";

// 信令
const SIGNAL_TYPE_JOIN = "join";
const SIGNAL_TYPE_RESP_JOIN = "resp-join";
const SIGNAL_TYPE_NEW_PEER = "new-peer";
const SIGNAL_TYPE_LEAVE = "leave";
const SIGNAL_TYPE_RESP_LEAVE = "resp-leave";
const SIGNAL_TYPE_PEER_LEAVE = "peer-leave";
const SIGNAL_TYPE_OFFER = "offer";
const SIGNAL_TYPE_ANSWER = "answer";
const SIGNAL_TYPE_CANDIDATE = "candidate";

// 获取窗口控件
var LocalVideo = document.querySelector("#LocalVideo");
var RemoteVideo = document.querySelector("#RemoteVideo");

// 本地id
var LocalID = Math.random().toString(36).substr(2);
// 对端id
var RemoteID = -1;
// 房间id
var RoomID = -1;

// 本地流
var LocalStream = null;

// peerconnection
var peerconn = null;

// ice服务配置
var defaultConfiguration = {
  bundlePolicy: "max-bundle",
  rtcpMuxPolicy: "require",
  iceTransportPolicy: "all", //relay all
  iceServers: [
    {
      urls: [
        "turn:"+Coturn+"?transport=udp",
        "turn:"+Coturn+"?transport=tcp", // 可以插入多个进行备选
      ],
      username: "gongluck",
      credential: "123456",
    },
    {
      urls: ["stun:"+Coturn+""],
    },
  ],
};

// RTCEngine类
var RTCEngine = function (wsurl) {
  // 创建websocket连接
  this.ws = new WebSocket(wsurl);
  // websocket连接建立时触发
  this.ws.onopen = function () {
    console.info("websocket open");
  };
  // websocket客户端接收服务端数据时触发
  this.ws.onmessage = function (ev) {
    console.info("recv websocket message : " + ev.data);
    var jsonMsg = null;
    try {
      jsonMsg = JSON.parse(ev.data);
    } catch (e) {
      console.warn("parse " + ev.data + " to json failed.");
      return;
    }
    switch (jsonMsg.cmd) {
      case SIGNAL_TYPE_NEW_PEER:
        handleRemoteNewPeer(jsonMsg);
        break;
      case SIGNAL_TYPE_RESP_JOIN:
        handleResponseJoin(jsonMsg);
        break;
      case SIGNAL_TYPE_PEER_LEAVE:
        handleRemotePeerLeave(jsonMsg);
        break;
      case SIGNAL_TYPE_OFFER:
        handleRemoteOffer(jsonMsg);
        break;
      case SIGNAL_TYPE_ANSWER:
        handleRemoteAnswer(jsonMsg);
        break;
      case SIGNAL_TYPE_CANDIDATE:
        handleRemoteCandidate(jsonMsg);
        break;
    }
  };
  // websocket通信发生错误时触发
  this.ws.onerror = function (ev) {
    console.error("websocket error : " + ev.data);
  };
  // websocket连接关闭时触发
  this.ws.onclose = function (ev) {
    console.info(
      "websocket close : " + ev.code + ", reason : " + EventTarget.reason
    );
  };
  return this;
};

// 加入房间回复
function handleResponseJoin(message) {
  console.info("join succeed, remote : " + message.uid);
  RemoteID = message.uid;
}

// 对端离开房间消息
function handleRemotePeerLeave(message) {
  console.info("other leave, remote : " + message.uid);
  RemoteVideo.srcObject = null;
  if (peerconn != null) {
    peerconn.close();
    peerconn = null;
  }
}

// 获取到对方码流的对象句柄
function track(event) {
  RemoteVideo.srcObject = event.streams[0];
}

// 获取到打洞信息(candidate)
function iceCandidate(event) {
  if (event.candidate) {
    var jsonMsg = {
      cmd: SIGNAL_TYPE_CANDIDATE,
      roomid: RoomID,
      uid: LocalID,
      remoteid: RemoteID,
      msg: JSON.stringify(event.candidate),
    };
    var message = JSON.stringify(jsonMsg);
    rtcEngine.ws.send(message);
    console.info("send candiate message: " + message);
  } else {
    console.warn("End of candidates");
  }
}

// 对端加入房间消息回调
function handleRemoteNewPeer(message) {
  console.info("other join succeed, remote : " + message.uid);
  RemoteID = message.uid;

  if (peerconn == null) {
    // 创建RTC连接
    peerconn = new RTCPeerConnection(defaultConfiguration);
    peerconn.onicecandidate = iceCandidate;
    peerconn.ontrack = track;
    // 将本地流加入到RTC连接
    LocalStream.getTracks().forEach((track) =>
      peerconn.addTrack(track, LocalStream)
    );
  }

  // 发送RTC的offer
  peerconn
    .createOffer()
    .then(function (session) {
      // 设置本地SDP
      peerconn
        .setLocalDescription(session)
        .then(function () {
          var jsonMsg = {
            cmd: SIGNAL_TYPE_OFFER,
            roomid: RoomID,
            uid: LocalID,
            remoteid: RemoteID,
            msg: JSON.stringify(session),
          };
          var message = JSON.stringify(jsonMsg);
          rtcEngine.ws.send(message);
          console.info("send offer message: " + message);
        })
        .catch(function (error) {
          console.error("offer setLocalDescription failed: " + error);
        });
    })
    .catch(function (error) {
      console.error("create offer failed: " + error);
    });
}

// 对端发送offer消息回调
function handleRemoteOffer(message) {
  if (peerconn == null) {
    // 创建RTC连接
    peerconn = new RTCPeerConnection(defaultConfiguration);
    peerconn.onicecandidate = iceCandidate;
    peerconn.ontrack = track;
    // 将本地流加入到RTC连接
    LocalStream.getTracks().forEach((track) =>
      peerconn.addTrack(track, LocalStream)
    );
  }

  var desc = JSON.parse(message.msg);
  // 设置对端SDP
  peerconn.setRemoteDescription(desc);
  // 发送RTC的answer
  peerconn
    .createAnswer()
    .then(function (session) {
      peerconn
        .setLocalDescription(session)
        .then(function () {
          var jsonMsg = {
            cmd: SIGNAL_TYPE_ANSWER,
            roomid: RoomID,
            uid: LocalID,
            remoteid: RemoteID,
            msg: JSON.stringify(session),
          };
          var message = JSON.stringify(jsonMsg);
          rtcEngine.ws.send(message);
          console.info("send answer message: " + message);
        })
        .catch(function (error) {
          console.error("answer setLocalDescription failed: " + error);
        });
    })
    .catch(function (error) {
      console.error("create answer failed: " + error);
    });
}

// 对端发送answer回调
function handleRemoteAnswer(message) {
  var desc = JSON.parse(message.msg);
  peerconn.setRemoteDescription(desc);
}

// 对端发送candidate回调
function handleRemoteCandidate(message) {
  var candidate = JSON.parse(message.msg);
  // 添加网络协商信息
  peerconn.addIceCandidate(candidate).catch(function (e) {
    console.error("addIceCandidate failed:" + e.name);
  });
}

// “加入”控件点击事件
document.getElementById("JoinBtn").onclick = function () {
  console.info("加入按钮被点击");
  RoomID = document.getElementById("RoomID").value;
  if (RoomID == "" || RoomID == "请输入房间ID") {
    alert("请输入房间ID");
    return;
  }
  // 初始化本地流
  navigator.mediaDevices
    .getUserMedia({
      audio: true,
      video: true,
    })
    .then(function (stream) {
      var jsonMsg = {
        cmd: SIGNAL_TYPE_JOIN,
        roomid: RoomID,
        uid: LocalID,
      };
      var message = JSON.stringify(jsonMsg);
      rtcEngine.ws.send(message);
      LocalVideo.srcObject = stream;
      LocalStream = stream;
    })
    .catch(function (e) {
      alert("getUserMedia error : " + e.name);
    });
};

// “离开”控件点击事件
document.getElementById("LeaveBtn").onclick = function () {
  console.info("离开按钮被点击");
  var jsonMsg = {
    cmd: SIGNAL_TYPE_LEAVE,
    roomid: RoomID,
    uid: LocalID,
  };
  var message = JSON.stringify(jsonMsg);
  rtcEngine.ws.send(message);

  LocalVideo.srcObject = null;
  RemoteVideo.srcObject = null;
  if (peerconn != null) {
    peerconn.close();
    peerconn = null;
  }
};

// 启动服务
var rtcEngine = new RTCEngine(ServerWebsocket);
