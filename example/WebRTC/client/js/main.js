/*
 * @Author: gongluck 
 * @Date: 2021-11-24 16:52:12 
 * @Last Modified by: gongluck
 * @Last Modified time: 2022-03-03 12:39:11
 */

// 自定义信令
const SIGNAL_TYPE_JOIN = "join";//加入房间
const SIGNAL_TYPE_RESP_JOIN = "resp-join";//加入命令回复
const SIGNAL_TYPE_NEW_PEER = "new-peer";//通知有对端加入
const SIGNAL_TYPE_LEAVE = "leave";//离开房间
const SIGNAL_TYPE_RESP_LEAVE = "resp-leave";//离开命令回复
const SIGNAL_TYPE_PEER_LEAVE = "peer-leave";//通知有对端离开
const SIGNAL_TYPE_OFFER = "offer";//发送offer
const SIGNAL_TYPE_ANSWER = "answer";//发送answer
const SIGNAL_TYPE_CANDIDATE = "candidate";//发送ice candidate

//房间id
var RoomID = "";
//本地连接id
var LocalID = Math.random().toString(16).substr(2);
//远端连接id
var RemoteID = "";

//本地视频控件
var localVideo = document.querySelector("#localVideo");
//远端视频控件
var remoteVideo = document.querySelector("#remoteVideo");

//新建一个websocket连接
var websocket = new WebSocket("wss://127.0.0.1:55555");

//ice配置
var iceip = "127.0.0.1";//coturn地址
var config = {
  //收集ICE候选时要使用的媒体捆绑策略
  bundlePolicy: "max-bundle",
  //仅为RTP收集ICE候选，并在RTP候选上复用RTCP。如果远程端点不支持rtcp-mux，会话协商将失败。
  rtcpMuxPolicy: "require",
  //收集ICE候选的类型
  iceTransportPolicy: "all", //relay all
  //ICE服务地址
  iceServers: [{
    "urls": ["turn:" + iceip + ":3478"],
    "username": "gongluck",
    "credential": "123456",
  },
  {
    "urls": ["stun:" + iceip + ":3478"],
  }
  ]
};

//连接对象
var peerconn = null;

//创建连接
function doCreatePeerconnection() {
  peerconn = new RTCPeerConnection(config);
  //获取到打洞信息(candidate)回调
  peerconn.onicecandidate = function (event) {
    if (event.candidate) {
      console.info("got candidate: " + JSON.stringify(event.candidate));
      //构造SIGNAL_TYPE_CANDIDATE消息
      var jsonMsg = {
        cmd: SIGNAL_TYPE_CANDIDATE,
        roomid: RoomID,
        uid: LocalID,
        remoteid: RemoteID,
        sdp: JSON.stringify(event.candidate),
      };
      //发送消息
      var message = JSON.stringify(jsonMsg);
      websocket.send(message);
    } else {
      console.info("end of icecandidate");
    }
  };
  //获取到对方码流的对象句柄回调
  peerconn.ontrack = function (event) {
    remoteVideo.srcObject = event.streams[0];
  };
  //流连接状态回调
  peerconn.onconnectionstatechange = function (event) {
    console.info("connection state : " + peerconn.connectionState);
    switch (peerconn.connectionState) {
      case "disconnected":
      case "failed":
      case "closed":
        {
          //移除本地流
          peerconn.removeStream(localVideo.srcObject);
          //移除远端流
          peerconn.removeStream(remoteVideo.srcObject);
          //清空远端窗口控件
          remoteVideo.srcObject = null;
        }
        break;
    }
  };
  //ICE连接回调
  peerconn.oniceconnectionstatechange = function (event) {
    console.info("ice connection state : " + peerconn.connectionState);
  };
}

//打开websocket连接回调
websocket.onopen = function (ev) {
  console.info("websocket open");
};
//关闭websocket连接回调
websocket.onclose = function (ev) {
  console.info("websocket close, code : " + ev.code + ", data : " + ev.data);
};
//接收服务器返回的数据回调
websocket.onmessage = function (ev) {
  console.info("websocket message : " + ev.data);
  jsonMsg = JSON.parse(ev.data);
  var roomid = jsonMsg.roomid;
  var uid = jsonMsg.uid;
  var remoteuid = jsonMsg.remoteuid;
  var sdp = jsonMsg.sdp;

  //解析命令
  switch (jsonMsg.cmd) {
    case SIGNAL_TYPE_RESP_JOIN: //加入房间结果
      {
        if (roomid == RoomID && uid == LocalID) {//加入成功
          RemoteID = remoteuid;//保存对端id
        }
      }
      break;
    case SIGNAL_TYPE_NEW_PEER: //新节点进入房间
      {
        if (roomid == RoomID && uid == LocalID) {//加入成功
          RemoteID = remoteuid;//保存对端id
          if (peerconn == null) {
            doCreatePeerconnection();//创建连接
          }
          // 将本地流加入到RTC连接
          peerconn.addStream(localVideo.srcObject);
          // 创建RTC的offer
          peerconn.createOffer()
            .then(function (localsdp) {
              console.info("got sdp: " + JSON.stringify(localsdp));
              // 设置本地SDP
              peerconn.setLocalDescription(localsdp)
                .then(function () {
                  //构造SIGNAL_TYPE_OFFER消息
                  var jsonMsg = {
                    cmd: SIGNAL_TYPE_OFFER,
                    roomid: RoomID,
                    uid: LocalID,
                    remoteid: RemoteID,
                    sdp: JSON.stringify(localsdp),
                  };
                  //发送消息
                  var message = JSON.stringify(jsonMsg);
                  websocket.send(message);
                })
                .catch(function (error) {
                  console.error("offer setLocalDescription failed: " + error);
                });
            })
            .catch(function (error) {
              console.error("create offer failed: " + error);
            });
        }
      }
      break;
    case SIGNAL_TYPE_OFFER://收到offer
      {
        if (peerconn == null) {
          doCreatePeerconnection();//创建连接
        }
        //设置是否收发数据
        peerconn.addTransceiver("video", { direction: "recvonly" });
        peerconn.addTransceiver("audio", { direction: "recvonly" });
        // 将本地流加入到RTC连接
        //peerconn.addStream(localVideo.srcObject);
        var desc = JSON.parse(sdp);
        // 设置对端SDP
        peerconn.setRemoteDescription(desc)
          .then(function () {
            // 创建RTC的answer
            peerconn.createAnswer()
              .then(function (localsdp) {
                //设置本地sdp
                peerconn.setLocalDescription(localsdp)
                  .then(function () {
                    //构造SIGNAL_TYPE_ANSWER消息
                    var jsonMsg = {
                      cmd: SIGNAL_TYPE_ANSWER,
                      roomid: RoomID,
                      uid: LocalID,
                      remoteid: RemoteID,
                      sdp: JSON.stringify(localsdp),
                    };
                    //发送消息
                    var message = JSON.stringify(jsonMsg);
                    websocket.send(message);
                    console.info("send answer message: " + message);
                  })
                  .catch(function (error) {
                    console.error("answer setLocalDescription failed: " + error);
                  });
              })
              .catch(function (error) {
                console.error("create answer failed: " + error);
              });
          })
          .catch(function (error) {
            console.error("answer setRemoteDescription failed: " + error);
          });
      }
      break;
    case SIGNAL_TYPE_ANSWER://收到answer
      {
        var desc = JSON.parse(sdp);
        //设置远端sdp
        peerconn.setRemoteDescription(desc);
      }
      break;
    case SIGNAL_TYPE_CANDIDATE://收到ICE消息
      {
        var candidate = JSON.parse(sdp);
        // 添加网络协商信息
        peerconn.addIceCandidate(candidate)
          .then(function () {
            console.info("addIceCandidate succeed:" + sdp);
          })
          .catch(function (e) {
            console.error("addIceCandidate failed:" + e);
          });
      }
      break;
    case SIGNAL_TYPE_RESP_LEAVE://离开房间结果
    case SIGNAL_TYPE_PEER_LEAVE://对端离开消息
      {
        if (roomid == RoomID && uid == LocalID) {
          //清空远端窗口
          remoteVideo.srcObject = null;
          //关闭RTC连接
          peerconn.close();
          peerconn = null;
        }
      }
      break;
  }
};
//websocket错误
websocket.onerror = function (ev) {
  console.error("websocket error, code : " + ev.code + ", data : " + ev.data);
};

//加入按钮
document.getElementById("joinBtn")
  .onclick = function () {
    console.log("joinBtn onclick");
    RoomID = document.getElementById("roomId").value;
    if (RoomID == "" || RoomID == "请输入房间ID") {
      alert("请输入房间ID");
      return;
    }
    //在用户通过提示许可的情况下，打开系统上的摄像头和/或麦克风，并提供MediaStream包含视频轨道和/或音频轨道的输入
    //navigator.mediaDevices.getUserMedia({
    //提示用户选择一个显示或显示的一部分（例如窗口）以作为MediaStream共享或记录目的进行捕获。返回一个解析为a的承诺MediaStream
    navigator.mediaDevices.getDisplayMedia({
      audio: true,
      video: {
        mandatory: {
          maxWidth: 640,
          maxHeight: 480
        }
      }
    })
      .then(function (stream) {
        //设置本地窗口视频
        localVideo.srcObject = stream;
        console.info("set stream : " + stream)
        console.info(stream);
        //构造SIGNAL_TYPE_JOIN消息
        var jsonMsg = {
          cmd: SIGNAL_TYPE_JOIN,
          roomid: RoomID,
          uid: LocalID,
        };
        //发送消息
        var message = JSON.stringify(jsonMsg);
        websocket.send(message);
      })
      .catch(function (e) {
        alert("getUserMedia error : " + e);
      });
  }

//离开按钮
document.getElementById("leaveBtn")
  .onclick = function () {
    console.log("leaveBtn onclick");
    RoomID = document.getElementById("roomId").value;
    if (RoomID == "" || RoomID == "请输入房间ID") {
      alert("请输入房间ID");
      return;
    }
    //构造SIGNAL_TYPE_LEAVE消息
    var jsonMsg = {
      cmd: SIGNAL_TYPE_LEAVE,
      roomid: RoomID,
      uid: LocalID,
    };
    //发送消息
    var message = JSON.stringify(jsonMsg);
    websocket.send(message);
  }