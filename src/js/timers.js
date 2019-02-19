/* Copyright 2015-present Samsung Electronics Co., Ltd. and other contributors
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

var util = require('util');

var TIMEOUT_MAX = '2147483647.0' - 0; // 2^31-1


function Timeout(after) {
  this.after = after;
  this.isrepeat = false;
  this.callback = null;
  this.handler = null;
}


native.prototype.handleTimeout = function() {
  var timeout = this.timeoutObj; // 'this' is native object
  if (timeout && timeout.callback) {
    try {
      timeout.callback();
    } catch (e) {
      timeout.unref();
      throw e;
    }

    if (!timeout.isrepeat) {
      timeout.unref();
    }
  }
};


Timeout.prototype.ref = function() {
  var repeat = 0;
  var handler = new native();

  if (this.isrepeat) {
    repeat = this.after;

  }

  handler.timeoutObj = this;
  this.handler = handler;

  handler.start(this.after, repeat);
};


Timeout.prototype.unref = function() {
  this.callback = undefined;
  if (this.handler) {
    this.handler.timeoutObj = undefined;
    this.handler.stop();
    this.handler = undefined;
  }
};
// TODO: add proper name
var types = {
  setTimeout : 0,
  setInterval : 1,
  setImmediet : 2,
}

function timeoutConfigurator(type, callback, delay) {
  if (!util.isFunction(callback)) {
    throw new TypeError('Bad arguments: callback must be a Function');
  }

  if (type === types.setImmediet) {
    delay = 0;
  } else {
    delay *= 1;
    if (delay < 1 || delay > TIMEOUT_MAX) {
      delay = 1;
    }
  }

  var timeout = new Timeout(delay);

  // set timeout handler.
  if (arguments.length <= 3) {
    timeout.callback = callback;
  } else {
    var args = Array.prototype.slice.call(arguments, 3);
    args.splice(0, 0, timeout);
    timeout.callback = callback.bind.apply(callback, args);
  }
  timeout.isrepeat = type == types.setInterval;
  timeout.ref();

  return timeout;
}

exports.setTimeout = timeoutConfigurator.bind(undefined, types.setTimeout);
exports.setInterval = timeoutConfigurator.bind(undefined, types.setInterval);
exports.setImmediet = timeoutConfigurator.bind(undefined, types.setImmediet);

function clearTimeoutBase(timeoutType, timeout) {
  if (timeout) {
    if (timeout instanceof Timeout) {
      timeout.unref();
    } else {
       throw new Error(timeoutType + '() - invalid timeout');
    }
  }
}

exports.clearTimeout = clearTimeoutBase.bind(undefined, 'clearTimeout');
exports.clearInterval = clearTimeoutBase.bind(undefined, 'clearInterval');
