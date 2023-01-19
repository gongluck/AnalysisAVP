/*
 * @Author: gongluck 
 * @Date: 2021-11-26 16:50:37 
 * @Last Modified by: gongluck
 * @Last Modified time: 2023-01-19 10:31:44
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

//引入https模块
var https = require('https');
//引入websocket模块
var ws = require("ws");
//引入fs模块
var fs = require('fs');

// CA私钥
// openssl genrsa -out key.pem
// 自签名证书
// openssl req -new -x509 -key key.pem -out cert.pem -days 365

// openssl genrsa -out key.pem
// openssl req -new -key key.pem -out cert.pem
// openssl x509 -req -in cert.pem -signkey key.pem -out cert.pem -days 365

var keypath = './key.pem';
var certpath = './cert.pem';

//房间ID->节点组
var RoomMap = new Map();

//创建服务
var options = {
  key: fs.readFileSync(keypath),
  cert: fs.readFileSync(certpath),
  passphrase: ''
};
var https = https.createServer(options, function (req, res) {
  res.writeHead(200);
  res.end("This is a WebSockets server!");
}).listen(55555);
var wss = new ws.Server({ server: https });

wss.on("connection", function (conn) {
  console.info("新连接到达...");
  //收掉消息
  conn.on("message", function (str) {
    console.info("接收到信息:" + str);
    //解析消息
    jsonMsg = JSON.parse(str);
    var roomid = jsonMsg.roomid;
    var uid = jsonMsg.uid;
    var remoteuid = jsonMsg.remoteuid;
    var sdp = jsonMsg.sdp;
    var connections = RoomMap.get(roomid);
    if (connections == null) {
      connections = new Map();
    }
    console.info("connections size : " + connections.size);
    connections.forEach(function (value, key) {
      console.info("id : " + key);
    });
    var jsonResMsg = {};
    //处理命令
    switch (jsonMsg.cmd) {
      case SIGNAL_TYPE_JOIN://进入房间
        {
          if (connections.size < 2) {
            remoteuid = connections.keys().next().value;
            jsonResMsg.cmd = SIGNAL_TYPE_RESP_JOIN;//进入结果
            jsonResMsg.roomid = roomid;
            jsonResMsg.uid = uid
            jsonResMsg.remoteuid = remoteuid;
            //发送消息
            var message = JSON.stringify(jsonResMsg);
            conn.send(message);
            //处理原来房间的连接
            if (connections.size == 1) {
              var jsonResPeerMsg = jsonResMsg;
              jsonResPeerMsg.cmd = SIGNAL_TYPE_NEW_PEER;//对端加入消息
              jsonResPeerMsg.roomid = roomid;
              jsonResPeerMsg.uid = remoteuid
              jsonResPeerMsg.remoteuid = uid;
              //发送消息
              var message = JSON.stringify(jsonResPeerMsg);
              connections.get(remoteuid).send(message);
            }
            //保存conn到房间
            connections.set(uid, conn);
          } else {//该房间已满
            jsonResMsg.cmd = SIGNAL_TYPE_RESP_JOIN;//进入结果
            jsonResMsg.roomid = roomid;
            jsonResMsg.uid = "";
            jsonResMsg.remoteuid = "";
            var message = JSON.stringify(jsonResMsg);
            conn.send(message);
          }
        }
        break;
      case SIGNAL_TYPE_LEAVE://离开房间
        {
          if (connections.get(uid) != null) {
            //遍历房间中的连接
            connections.forEach(function (value, key) {
              if (key != uid) {
                var jsonResPeerMsg = jsonResMsg;
                jsonResPeerMsg.cmd = SIGNAL_TYPE_PEER_LEAVE;//对端离开消息
                jsonResPeerMsg.roomid = roomid;
                jsonResPeerMsg.uid = key;
                jsonResPeerMsg.remoteuid = uid;
                var message = JSON.stringify(jsonResPeerMsg);
                value.send(message);
              } else {
                jsonResMsg.cmd = SIGNAL_TYPE_RESP_LEAVE;//离开结果
                jsonResMsg.roomid = roomid;
                jsonResMsg.uid = key;
                jsonResMsg.remoteuid = "";
                var message = JSON.stringify(jsonResMsg);
                value.send(message);
              }
            });
            connections.delete(uid);
          }
          else {
            jsonResMsg.cmd = SIGNAL_TYPE_RESP_LEAVE;//离开结果
            jsonResMsg.roomid = roomid;
            jsonResMsg.uid = "";//本来无一物，何处惹尘埃
            jsonResMsg.remoteuid = "";
            var message = JSON.stringify(jsonResMsg);
            conn.send(message);
          }
        }
        break;
      //透传转发
      case SIGNAL_TYPE_OFFER://通话offer
      case SIGNAL_TYPE_ANSWER://offer应答
      case SIGNAL_TYPE_CANDIDATE://网络打洞信息
        {
          if (connections.get(uid) != null) {
            connections.forEach(function (connection, id) {
              console.info("dueling with id : " + id);
              if (id != uid) {
                jsonResMsg.cmd = jsonMsg.cmd;
                jsonResMsg.roomid = roomid;
                jsonResMsg.uid = id;
                jsonResMsg.remoteuid = uid;
                jsonResMsg.sdp = sdp;//浅拷贝
                var message = JSON.stringify(jsonResMsg);
                connection.send(message);
              }
            });
          }
          break;
        }
    }
    //更新房间信息
    RoomMap.set(roomid, connections);
    console.info("response : " + JSON.stringify(jsonResMsg));
  });
  //连接关闭
  conn.on("close", function (code, reason) {
    console.info("连接关闭:" + code + "," + reason);
    //遍历节点组
    RoomMap.forEach(function (connections, roomid) {
      //遍历组内每个连接
      connections.forEach(function (connection, id) {
        if (connection == conn) {
          connections.delete(id);
        } else {
          var jsonResMsg = {};
          jsonResMsg.cmd = SIGNAL_TYPE_PEER_LEAVE;//对端离开消息
          jsonResMsg.roomid = roomid;
          jsonResMsg.uid = id;
          jsonResMsg.remoteuid = "";
          var message = JSON.stringify(jsonResMsg);
          connection.send(message);
        }
      });
    });
  });
  //连接错误
  conn.on("error", function (ev) {
    console.error("错误:" + ev);
  });
});
