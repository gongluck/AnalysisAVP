var ws = require("nodejs-websocket");
var port = 8010;
var user = 0;
// 创建一个连接
var server = ws
  .createServer(function (conn) {
    console.log("创建一个新的连接‐‐‐‐‐‐‐‐");
    user++;
    conn.nickname = "user" + user;
    conn.fd = "user" + user;
    var mes = {};
    mes.type = "enter";
    mes.data = conn.nickname + " 进来啦";
    broadcast(JSON.stringify(mes)); // 广播
    //向客户端推送消息
    conn.on("text", function (str) {
      console.log("回复 " + str);
      mes.type = "message";
      mes.data = conn.nickname + " 说: " + str;
      broadcast(JSON.stringify(mes));
    });
    //监听关闭连接操作
    conn.on("close", function (code, reason) {
      console.log("关闭连接");
      mes.type = "leave";
      mes.data = conn.nickname + " 离开了";
      broadcast(JSON.stringify(mes));
    });
    //错误处理
    conn.on("error", function (err) {
      console.log("监听到错误");
      console.log(err);
    });
  })
  .listen(port);
function broadcast(str) {
  server.connections.forEach(function (connection) {
    connection.sendText(str);
  });
}
