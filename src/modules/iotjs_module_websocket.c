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

#include "iotjs_module_websocket.h"
#include <time.h>
#include <stdlib.h>

static char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/**
 * The protocol as follows:
 * method + USER_ENDPOINT + protocol
 * host + USER_HOST + line_end
 * upgrade
 * connection
 * sec_websocket_key + line_end
 * sec_websocket_ver
 */
static char method[] = "GET ";
static char protocol[] = " HTTP/1.1\r\n";
static char host[] = "Host: ";
static char line_end[] = "\r\n";
static char upgrade[] = "Upgrade: websocket\r\n";
static char connection[] = "Connection: Upgrade\r\n";
static char sec_websocket_key[] = "Sec-WebSocket-Key: ";
static char sec_websocket_ver[] = "Sec-WebSocket-Version: 13\r\n\r\n";
static size_t header_fixed_size = sizeof(method) + sizeof(protocol) +
                                  sizeof(host) + sizeof(upgrade) +
                                  sizeof(connection) + sizeof(sec_websocket_key)
                                  + sizeof(sec_websocket_ver) - 9;

static unsigned char *generated_key = NULL;

static unsigned char *ws_generate_key() {
  srand(time(NULL));
  unsigned char *key = IOTJS_CALLOC(16, unsigned char);
  for (uint8_t i = 0; i < 16; i++) {
    key[i] = rand() % 256;
  }

  size_t buff_len;
  unsigned char *ret_val = NULL;
  mbedtls_base64_encode(ret_val, 0, &buff_len, key, 16);

  ret_val = IOTJS_CALLOC(buff_len, unsigned char);

  if (mbedtls_base64_encode(ret_val, buff_len, &buff_len, key, 16)) {

  }
  IOTJS_RELEASE(key);

  return ret_val;
}


static char *ws_write_header(char *dst, char *src, size_t *offset) {
  memcpy(dst, src, strlen(src));
  *offset += strlen(src);
  return dst + strlen(src);
}


JS_FUNCTION(PrepareHandshakeRequest) {
  DJS_CHECK_THIS();
  DJS_CHECK_ARGS(2, object, object);

  jerry_value_t jendpoint = JS_GET_ARG(0, object);
  jerry_value_t jhost = JS_GET_ARG(1, object);

  iotjs_string_t l_endpoint;
  iotjs_string_t l_host;
  if (!(iotjs_jbuffer_as_string(jendpoint, &l_endpoint)) ||
      !(iotjs_jbuffer_as_string(jhost, &l_host))) {
        return JS_CREATE_ERROR(COMMON, "err");
  };

  generated_key = ws_generate_key();

  jerry_value_t jfinal =
      iotjs_bufferwrap_create_buffer(header_fixed_size + iotjs_string_size(&l_endpoint) + iotjs_string_size(&l_host)
                                     + (sizeof(line_end) * 2) + 20);

  iotjs_bufferwrap_t *final_wrap = iotjs_bufferwrap_from_jbuffer(jfinal);

  char *buff_ptr = final_wrap->buffer;
  size_t offset = 0;
  buff_ptr = ws_write_header(buff_ptr, method, &offset);
  memcpy(buff_ptr, iotjs_string_data(&l_endpoint), iotjs_string_size(&l_endpoint));
  buff_ptr += iotjs_string_size(&l_endpoint);
  buff_ptr = ws_write_header(buff_ptr, protocol, &offset);
  buff_ptr = ws_write_header(buff_ptr, host, &offset);
  memcpy(buff_ptr, iotjs_string_data(&l_host), iotjs_string_size(&l_host));
  buff_ptr += iotjs_string_size(&l_host);
  buff_ptr = ws_write_header(buff_ptr, line_end, &offset);
  buff_ptr = ws_write_header(buff_ptr, upgrade, &offset);
  buff_ptr = ws_write_header(buff_ptr, connection, &offset);
  buff_ptr = ws_write_header(buff_ptr, sec_websocket_key, &offset);
  memcpy(buff_ptr, generated_key, 20);
  buff_ptr += 20;
  buff_ptr = ws_write_header(buff_ptr, line_end, &offset);
  buff_ptr = ws_write_header(buff_ptr, sec_websocket_ver, &offset);

  iotjs_string_destroy(&l_endpoint);
  iotjs_string_destroy(&l_host);

  return jfinal;
}


JS_FUNCTION(CheckHandshakeKey) {
  DJS_CHECK_THIS();
  DJS_CHECK_ARGS(1, string);

  iotjs_string_t server_key = JS_GET_ARG(0, string);

  unsigned char out_buff[20] = {0};

  mbedtls_sha1_context sha_ctx;
  mbedtls_sha1_init(&sha_ctx);
  mbedtls_sha1_starts_ret(&sha_ctx);
  unsigned char *concatenated =
      IOTJS_CALLOC(strlen(WS_GUID) + strlen((char *)generated_key), unsigned char);

  memcpy(concatenated, generated_key, strlen((char *)generated_key));
  memcpy(concatenated + strlen((char *)generated_key), WS_GUID, strlen(WS_GUID));
  mbedtls_sha1_update_ret(&sha_ctx, concatenated, strlen((char *)concatenated));
  mbedtls_sha1_finish_ret(&sha_ctx, out_buff);
  mbedtls_sha1_free(&sha_ctx);

  size_t buff_len;
  unsigned char *ret_val = NULL;
  mbedtls_base64_encode(ret_val, 0, &buff_len, out_buff, sizeof(out_buff));

  ret_val = IOTJS_CALLOC(buff_len, unsigned char);

  if (mbedtls_base64_encode(ret_val, buff_len, &buff_len, out_buff, sizeof(out_buff))) {

  }

  IOTJS_UNUSED(server_key);
  IOTJS_RELEASE(generated_key);

  return jerry_create_boolean(true);
}

jerry_value_t InitWebsocket() {
 IOTJS_UNUSED(WS_GUID);
 jerry_value_t jws = jerry_create_object();
 iotjs_jval_set_method(jws, "checkHandshakeKey", CheckHandshakeKey);
 iotjs_jval_set_method(jws, "prepareHandshake", PrepareHandshakeRequest);

 return jws;
}
