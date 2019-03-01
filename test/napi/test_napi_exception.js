var addon = require('./build/Release/napi_error_handling');
var assert = require('assert');

var ERROR_MSG = "ErrorMSG";

process.on("uncaughtException", function (e) {
  assert(e.message === ERROR_MSG);
});

assert(addon.GetandClearLastException() === undefined);

var err = new Error(ERROR_MSG);

addon.FatalException(err);
