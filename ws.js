var ws = require('websocket');
var net = require('net');

var websocket = new ws.Websocket();
websocket.connect('10.109.187.1');
websocket.on('data', function(msg) {
  console.log(msg);
  websocket._socket.write("asdd");
  websocket.send("almafa");
});
