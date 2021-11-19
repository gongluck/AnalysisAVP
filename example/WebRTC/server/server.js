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

var ws = require("nodejs-websocket");

//房间ID->节点组
var RoomMap = new Map();

var server = ws.createServer(function (conn) {
    console.info("新连接到达...");
    conn.on("text", function (str) {
        console.info("接收到信息:" + str);
        jsonMsg = JSON.parse(str);
        var roomid = jsonMsg.roomid;
        var uid = jsonMsg.uid;
        var sdp = jsonMsg.sdp;
        var connections = RoomMap.get(roomid);
        if (connections == null) {
            connections = new Map();
        }
        console.info("connections size : " + connections.size);
        var jsonResMsg = {};

        switch (jsonMsg.cmd) {
            case SIGNAL_TYPE_JOIN://进入房间
                {
                    if (connections.size < 2) {
                        var remoteuid = connections.keys().next().value;
                        jsonResMsg.cmd = SIGNAL_TYPE_RESP_JOIN;
                        jsonResMsg.roomid = roomid;
                        jsonResMsg.uid = uid
                        jsonResMsg.remoteuid = remoteuid;
                        var message = JSON.stringify(jsonResMsg);
                        conn.send(message);

                        if (connections.size == 1) {
                            var jsonResPeerMsg = jsonResMsg;
                            jsonResPeerMsg.cmd = SIGNAL_TYPE_NEW_PEER;
                            jsonResPeerMsg.roomid = roomid;
                            jsonResPeerMsg.uid = remoteuid
                            jsonResPeerMsg.remoteuid = uid;
                            var message = JSON.stringify(jsonResPeerMsg);
                            connections.get(remoteuid).send(message);
                        }
                        connections.set(uid, conn);
                    } else {
                        jsonResMsg.cmd = SIGNAL_TYPE_RESP_JOIN;
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
                        connections.forEach(function (value, key) {
                            if (key != uid) {
                                var jsonResPeerMsg = jsonResMsg;
                                jsonResPeerMsg.cmd = SIGNAL_TYPE_PEER_LEAVE;
                                jsonResPeerMsg.roomid = roomid;
                                jsonResPeerMsg.uid = key;
                                jsonResPeerMsg.remoteuid = uid;
                                var message = JSON.stringify(jsonResPeerMsg);
                                value.send(message);
                            } else {
                                jsonResMsg.cmd = SIGNAL_TYPE_RESP_LEAVE;
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
                        jsonResMsg.cmd = SIGNAL_TYPE_RESP_LEAVE;
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
                        connections.forEach(function (value, key) {
                            if (key != uid) {
                                var jsonResPeerMsg = jsonResMsg;
                                jsonResPeerMsg.cmd = jsonMsg.cmd;
                                jsonResPeerMsg.roomid = roomid;
                                jsonResPeerMsg.uid = key;
                                jsonResPeerMsg.remoteuid = uid;
                                jsonResPeerMsg.sdp = sdp;
                                if (jsonMsg.cmd == SIGNAL_TYPE_CANDIDATE) {
                                    var before = sdp.split(" ");;
                                    console.info("got candidate: " + JSON.stringify(sdp));
                                    var arr = [];
                                    for (var i = 0; i < before.length; i++) {
                                        if (i == 4 && before[i] == "555.555.555.555") {
                                            arr.push(conn.socket.remoteAddress);
                                            console.info(conn.socket.remoteAddress);
                                        }
                                        else {
                                            arr.push(before[i]);
                                        }
                                    }
                                    jsonResPeerMsg.sdp = arr.join(" ");
                                }
                                var message = JSON.stringify(jsonResPeerMsg);
                                value.send(message);
                            }
                        });
                    }
                }
                break;
        }

        //更新房间信息
        RoomMap.set(roomid, connections);
        console.info("response : " + JSON.stringify(jsonResMsg));
    });

    conn.on("close", function (code, reason) {
        console.info("连接关闭:" + code + "," + reason);
        RoomMap.forEach(function (connections, key) {
            connections.forEach(function (value, key) {
                if (value == conn) {
                    connections.delete(key);
                }
            });
        });
    });

    conn.on("error", function (ev) {
        console.error("错误:" + ev);
    });
}).listen(8001);
