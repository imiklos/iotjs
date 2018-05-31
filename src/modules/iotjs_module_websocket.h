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

#include "iotjs_def.h"
#include "iotjs_module_buffer.h"
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"
#include "iotjs_handlewrap.h"
#include "iotjs_reqwrap.h"

enum {
  WS_OP_CONTINUE = 0x00,
  WS_OP_UTF8 = 0x01,
  WS_OP_BINARY = 0x02,
  WS_OP_TERMINATE = 0x08,
  WS_OP_PING = 0x09,
  WS_OP_PONG = 0x0a,
} iotjs_websocket_opcodes;


typedef struct {
  struct {
    uint64_t length;
    unsigned char *buffer;
  } tcp_buff;

  struct {
    uint64_t length;
    unsigned char *buffer;
  } ws_buff;
} iotjs_ws_client_t;
