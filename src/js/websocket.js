/* Copyright 2018-present Samsung Electronics Co., Ltd. and other contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 var net = require('net');
 var util = require('util');
 var EventEmitter = require('events').EventEmitter;
 var tls = require('tls');

util.inherits(Websocket, EventEmitter);
util.inherits(Server, EventEmitter);
util.inherits(WebsocketClient, EventEmitter);


function WebSocketHandle(client) {
  this.client = client;
  this.pings = [];
  this.connected = false;

  native.wsInit(this);
}

function Websocket(options) {
  if (!(this instanceof Websocket)) {
    return new Websocket(options);
  }

  EventEmitter.call(this);
  this._firstMessage = true;
  this._handle = new WebSocketHandle(this);
  this._secure = false;
}

function ServerHandle(server) {
  this.clients = [];
  this._server = server;
  console.log("server: " + Object.keys(this));
  console.log("server: " + Object.keys(this._server));
  console.log("server: " + Object.keys(this._server._netserver));

  native.wsInit(this);
}

function WebsocketClient(socket) {
  if (!(this instanceof WebsocketClient)) {
    return new WebsocketClient(socket);
  }
  this._socket = socket;

  this.readyState = 'CONNECTING';
  console.log(this.readyState);
  this._secure = false;
}

function connectionListener(socket) {
  console.log("Client connected");
  var websocket = new WebsocketClient(socket);
  console.log(this instanceof net.Server);
  console.log("connLis" + Object.keys(this));
  this._serverHandle.clients.push(websocket);
  var self = this;
  websocket._socket.on('data', function (data) {
    var id = self._serverHandle.clients.indexOf(websocket);
    if (self._serverHandle.clients[id].readyState == 'CONNECTING') {
      parseServerHandshakeData(data, websocket);
    } else {
      self._serverHandle.ondata(data, websocket)
    }
  })
  console.log(Object.keys(this));
}

function parseServerHandshakeData(data, client) {
  console.log("serverhand " + Object.keys(this));
  data = data.toString();
  var res = data.split("\r\n");
  var method = res[0].split(" ");

  var headers = { 'Connection': '',
                  'Upgrade': '',
                  'Host': '',
                  'Sec-WebSocket-Key': '',
                  'Sec-WebSocket-Version': -1
                };

  for(var i = 1; i < res.length; i++) {
    var temp = res[i].split(": ");
    headers[temp[0]] = temp[1];
  }

  var response = '';
  if (method[0] == 'GET' &&
      method[2] == 'HTTP/1.1' &&
      headers['Connection'] == 'Upgrade' &&
      headers['Upgrade'] == 'websocket' && (
      headers['Sec-WebSocket-Version'] > 7 &&
      headers['Sec-WebSocket-Version'] <= 13)) {
        response = native.ReceiveHandshakeData(
                            headers['Sec-WebSocket-Key'], // key
                            method[1] // path
                          ).toString();
  } else {
    // TODO: Throw error (not valid headers)
  }
  client.readyState = 'OPEN';
  client._socket.write(response);
  // serverHandshakeDone(client, server);
}

function Server(options, listener) {
  if (!(this instanceof Server)) {
    return new Server(options);
  }
  EventEmitter.call(this);
  if (options.port) {
    this._netserver = net.createServer(options, connectionListener);
    this._netserver._serverHandle = new ServerHandle(this);
    this._netserver.listen(options.port);
    console.log('Listening on port: ' + options.port);
  }

  // if (listener) {
  //   this._server.on("open", listener);
  // }

}

ServerHandle.prototype.ondata = function (data, client) {
  console.log(data);
  console.log(Object.keys(client));
  console.log(Object.keys(this));
  // var id = this.clients.indexOf(client);
  native.wsReceive(this, data, client);
}

// ServerHandle.prototype.onmessage = function (msg, client) {
// // ￼   client._socket.emit('message', msg);
// ￼ }

WebSocketHandle.prototype.onmessage = function(msg) {
  this.client.emit('message', msg);
};

WebSocketHandle.prototype.ondata = function(data) {
  native.wsReceive(this, data, this);
};

WebSocketHandle.prototype.onhandshakedone = function(remaining) {
  this.client.emit('open');
  this.client._firstMessage = false;
  if (remaining) {
    this.ondata(remaining);
  }
};

WebSocketHandle.prototype.onError = function(err) {
  this.client.emit('error', err);
};

WebSocketHandle.prototype.onclose = function(msg) {
  if (msg) {
    // If there is msg we know the following:
    // 4 characters status code (1000-4999)
    // rest is msg payload
    var msg_str = msg.toString();
    msg = {
      code: msg_str.substr(0, 4),
      reason: msg_str.substr(4, msg_str.length),
    };
  } else {
    msg = {};
  }

  this.client.emit('close', msg);
  for (var i = 0; i < this.pings.length; i++) {
    clearInterval(this.pings[i].timer);
  }
  this.client._socket.end();
};

WebSocketHandle.prototype.sendFrame = function(msg, cb) {
  if (this.connected) {
    if (typeof cb == 'function') {
      this.client._socket.write(msg, cb);
    } else {
      this.client._socket.write(msg);
    }
  } else {
    this.onError('Underlying socket connection is closed');
  }
};

WebSocketHandle.prototype.pong = function(msg) {
  this.client._socket.write(native.ping(false, msg, true));
};

WebSocketHandle.prototype.onpingresp = function(msg) {
  for (var i = 0; i < this.pings.length; i++) {
    if (this.pings[i].id == msg) {
      clearInterval(this.pings[i].timer);
      this.pings[i].callback(msg);
      this.pings.splice(i, 1);
      return;
    }
  }
};

function sendHandshake(jsref, host, path) {
  return native.prepareHandshake(jsref, host, path);
}

Websocket.prototype.connect = function(url, port, path, callback) {
  var host = url.toString() || '127.0.0.1';
  path = path || '/';

  var emit_type = 'connect';

  if (host.substr(0, 3) == 'wss') {
    this._secure = true;
    if (!tls) {
      this._handle.onError('TLS module was not found!');
    }
    port = port || 443;
    host = host.substr(6);
    this._socket = tls.connect(port, host);
    emit_type = 'secureConnect';
  } else if (host.substr(0, 2) == 'ws') {
    port = port || 80;
    this._socket = new net.Socket();
    host = host.substr(5);
  } else {
    port = port || 80;
    this._socket = new net.Socket();
  }

  if (typeof callback == 'function') {
    this.on('open', callback);
  }

  var self = this;

  this._socket.on(emit_type, function() {
    self._handle.connected = true;
    self._socket.write(sendHandshake(self._handle, host, path));
  });

  this._socket.on('end', function() {
    self._handle.connected = false;
  });
  if (emit_type == 'connect') {
    this._socket.connect(port, host);
  }

  this._socket.on('data', function(data) {
    if (self._firstMessage) {
      var remaining_data = native.parseHandshakeData(data, self._handle);
      self._handle.onhandshakedone(remaining_data);
    } else {
      self._handle.ondata(data);
    }
  });
};

Websocket.prototype.close = function(message, code, cb) {
  this._handle.sendFrame(native.close(message, code), cb);
};

Websocket.prototype.ping = function(message, mask, cb) {
  var self = this;
  var obj = {
    id: message,
    callback: cb,
    timer: setTimeout(function() {
      self.close('Ping timeout limit exceeded', 1002);
    }, 30000),
  };
  this._handle.pings.push(obj);
  this._handle.sendFrame(native.ping(true, message, mask));
};

Websocket.prototype.send = function(message, opts, cb) {
  if (opts) {
    var mask = opts.mask;
    var binary = opts.binary;
    var compress = opts.compress;
    if (compress) {
      // Currently not supported, needs zlib
      this._handle.onError('Compression is not supported');
    }
  }
  var buff = native.send(message, binary, mask, compress);
  if (buff) {
    this._handle.sendFrame(buff, cb);
  }
};
exports.Websocket = Websocket;
exports.Server = Server;
