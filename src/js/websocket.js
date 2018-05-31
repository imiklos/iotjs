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

function Websocket(options) {
  if (!(this instanceof Websocket)) {
    return new Websocket(options);
  }

  EventEmitter.call(this);
  this._firstMessage = true;

  /*
  this._options = Object.create(options, {

  });
  */
}

Websocket.prototype.send = function(msg) {
  var self = this;
  self._socket.write(native.encodeWebSocket(msg));
}

Websocket.prototype.connect = function(host) {
  // CURRENTLY ONLY SUPPORT SIMPLE SOCKET UNTIL NO URL MODULE
  var self = this;
  this._socket = new net.Socket();
  this._socket.on('connect', function() {
    self._socket.write(sendHandshake(host, '/dummy'));
  });
  this._socket.connect(8080, host);

  this._socket.on('data', function(data) {
    if (self._firstMessage) {
      // it must be a valid utf8 string
      if (parseHandshakeData(data.toString())) {
        self._firstMessage = false;
      }
    } else {
      data = native.decodeFrame(data);
      // if utf8
      //data = data.toString();
      self.emit('data', data);
    }
  });
}

// NEED TO BE AN INTERNAL FUNCTION GIVEN TO CALLBACK OF SOCKET CONNECT
function sendHandshake(host, endpoint) {
  host = new Buffer(host);
  endpoint = new Buffer(endpoint);

  return native.prepareHandshake(endpoint, host);
}

// NEED TO BE A PRIVATE FUNCTION CALLED INTERNALLY ON INTERNAL SOCKET
function parseHandshakeData(data) {
  var spl_data = data.split("\r\n");
  var status_code = spl_data[0].substr(9, 3);
  if (parseInt(status_code) !== 101) {
    throw new Error("Expected status code: 101; Got: " + status_code);
  }

  var obj = {};

  for (var i = 1; i < spl_data.length; i++) {
    var ret_val = spl_data[i].split(": ");
    obj[ret_val[0]] = ret_val[1];
  }

  native.checkHandshakeKey(obj["Sec-WebSocket-Accept"]);

  return true;
}

exports.Websocket = Websocket;
