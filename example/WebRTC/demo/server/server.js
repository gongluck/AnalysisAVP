//"use strict"的目的是指定代码在严格条件下执行。
//严格模式下你不能使用未声明的变量。
"use strict";

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

// 引入websocket模块
var ws = require("nodejs-websocket");

// 端口
var port = 8099;

// map
var RTCMap = function () {
  // 内部数组
  this._entrys = new Array();

  this.put = function (key, value) {
    if (key == null || key == undefined) {
      return;
    }
    var index = this._getIndex(key);
    if (index == -1) {
      var entry = new Object();
      entry.key = key;
      entry.value = value;
      this._entrys[this._entrys.length] = entry;
    } else {
      this._entrys[index].value = value;
    }
  };
  this.get = function (key) {
    var index = this._getIndex(key);
    return index != -1 ? this._entrys[index].value : null;
  };

  this.remove = function (key) {
    var index = this._getIndex(key);
    if (index != -1) {
      // splice()方法用于添加或删除数组中的元素。
      this._entrys.splice(index, 1);
    }
  };
  this.clear = function () {
    this._entrys.length = 0;
  };

  this.contains = function (key) {
    var index = this._getIndex(key);
    return index != -1 ? true : false;
  };

  this.size = function () {
    return this._entrys.length;
  };
  this.getEntrys = function () {
    return this._entrys;
  };

  // 查找匹配key的元素的索引
  this._getIndex = function (key) {
    if (key == null || key == undefined) {
      return -1;
    }
    var _length = this._entrys.length;
    for (var i = 0; i < _length; i++) {
      var entry = this._entrys[i];
      if (entry == null || entry == undefined) {
        continue;
      }
      if (entry.key === key) {
        return i;
      }
    }
    return -1;
  };
};

// 创建房间map
var roomTableMap = new RTCMap();

// 定义Client
function Client(uid, conn, roomid) {
  this.uid = uid; // 用户id
  this.conn = conn; // websocket连接
  this.roomid = roomid;
}

// 处理join信令
function handleJoin(message, conn) {
  var roomid = message.roomid;
  var uid = message.uid;
  console.info("uid: " + uid + " try to join room " + roomid);

  var roomMap = roomTableMap.get(roomid);
  if (roomMap == null) {
    roomMap = new RTCMap();
    roomTableMap.put(roomid, roomMap);
  }

  if (roomMap.size() >= 2) {
    console.error("roomid:" + roomid + "已经有两人存在，请使用其他房间");
    return;
  }

  var client = new Client(uid, conn, roomid);
  roomMap.put(uid, client);

  // 获取所有该房间下的客户端
  var clients = roomMap.getEntrys();
  if (clients.length <= 0) return;

  // 遍历客户端
  for (var i in clients) {
    var id = clients[i].key;
    if (id != uid) {
      // 通知先前客户端有新客户端加入
      var jsonMsg = {
        cmd: SIGNAL_TYPE_NEW_PEER,
        uid: uid,
      };
      var msg = JSON.stringify(jsonMsg);
      var client = roomMap.get(id);
      console.info("new-peer: " + msg);
      client.conn.sendText(msg);

      // 通知客户端加入成功并返回对端id
      jsonMsg = {
        cmd: SIGNAL_TYPE_RESP_JOIN,
        uid: id,
      };
      msg = JSON.stringify(jsonMsg);
      console.info("resp-join: " + msg);
      conn.sendText(msg);
    } else if (clients.length < 2) {
      // 通知客户端加入成功
      jsonMsg = {
        cmd: SIGNAL_TYPE_RESP_JOIN,
        uid: null,
      };
      msg = JSON.stringify(jsonMsg);
      console.info("resp-join: " + msg);
      conn.sendText(msg);
    }
  }

  conn.client = client;
}

// 处理leave信令
function handleLeave(message, force) {
  var roomid = message.roomid;
  var uid = message.uid;
  console.info("uid: " + uid + " leave room " + roomid);

  var roomMap = roomTableMap.get(roomid);
  if (roomMap == null) {
    console.error("无法查找roomid " + roomid);
    return;
  }

  // 获取房间下的客户端数组
  var clients = roomMap.getEntrys();
  // 遍历房间下的客户端
  for (var i in clients) {
    var id = clients[i].key;
    var client = roomMap.get(id);
    if (client) {
      if (uid == id) {
        if (!force) {
          // 通知删除成功
          var jsonmsg = {
            cmd: SIGNAL_TYPE_RESP_LEAVE,
            uid: uid,
          };
          var msg = JSON.stringify(jsonmsg);
          client.conn.sendText(msg);
        }
      } else {
        // 通知房间中的其他客户端
        var jsonMsg = {
          cmd: SIGNAL_TYPE_PEER_LEAVE,
          uid: uid,
        };
        var msg = JSON.stringify(jsonMsg);
        console.info("notify peer:" + client.uid + ", uid:" + uid + " leave");
        client.conn.sendText(msg);
      }
    }
  }

  // 删除发送者
  roomMap.remove(uid);
}

// 透传
function sendData(roomid, uid, remoteid, message) {
  var roomMap = roomTableMap.get(roomid);
  if (roomMap == null) {
    console.error("无法查找roomid " + roomid);
    return;
  }

  if (roomMap.get(uid) == null) {
    console.error("can't find the uid " + uid);
    return;
  }

  var client = roomMap.get(remoteid);
  if (client) {
    var msg = JSON.stringify(message);
    client.conn.sendText(msg);
  } else {
    console.error("can't find remoteid：" + remoteid);
  }
}

// 处理offer信令
function handleOffer(message) {
  var roomid = message.roomid;
  var uid = message.uid;
  var remoteid = message.remoteid;
  console.info("uid: " + uid + " send offer to remoteid " + remoteid);

  sendData(roomid, uid, remoteid, message);
}

// 处理answer指令
function handleAnswer(message) {
  var roomid = message.roomid;
  var uid = message.uid;
  var remoteid = message.remoteid;
  console.info("uid: " + uid + " send answer to remoteid " + remoteid);

  sendData(roomid, uid, remoteid, message);
}

// 处理candidate指令
function handleCandidate(message) {
  var roomid = message.roomid;
  var uid = message.uid;
  var remoteid = message.remoteid;
  console.info("uid: " + uid + " send answer to remoteid " + remoteid);

  sendData(roomid, uid, remoteid, message);
}

// 创建websocket服务
var server = ws
  .createServer(
    // 客户端连接成功的回调
    function (conn) {
      console.log("创建一个新连接");
      // 向客户端发送消息
      conn.sendText("连接成功");
      // 设置客户端连接的接收回调
      conn.on("text", function (str) {
        console.info("recv msg : " + str);
        var jsonMsg = null;
        try {
          jsonMsg = JSON.parse(str);
        } catch (e) {
          console.warn("parse " + str + " to json failed.");
          return;
        }
        switch (jsonMsg.cmd) {
          case SIGNAL_TYPE_JOIN:
            handleJoin(jsonMsg, conn);
            break;
          case SIGNAL_TYPE_LEAVE:
            handleLeave(jsonMsg, false);
            break;
          case SIGNAL_TYPE_OFFER:
            handleOffer(jsonMsg);
            break;
          case SIGNAL_TYPE_ANSWER:
            handleAnswer(jsonMsg);
            break;
          case SIGNAL_TYPE_CANDIDATE:
            handleCandidate(jsonMsg);
        }
      });
      // 设置客户端连接的关闭回调
      conn.on("close", function (code, reason) {
        console.info(
          conn.client!=null
            ? conn.client.uid
            : "local" + " close : " + code + ", reason : " + reason
        );
        if (conn.client != null) {
          handleLeave(
            {
              cmd: SIGNAL_TYPE_LEAVE,
              roomid: conn.client.roomid,
              uid: conn.client.uid,
            },
            true
          );
        }
      });
      // 设置客户端连接的错误回调
      conn.on("error", function (err) {
        console.error("error : " + err);
      });
    }
  )
  // 监听
  .listen(port);
