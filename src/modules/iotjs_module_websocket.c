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
#include <math.h>

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
JS_FUNCTION(encodeWebSocket) {
  DJS_CHECK_THIS();
  DJS_CHECK_ARGS(1, string);
  // jerry_value_t jnum = JS_GET_ARG(0, number);
  // int frame_type = jerry_get_number_value(jnum);
  iotjs_string_t jstring = JS_GET_ARG(0, string);
  const char *msg = iotjs_string_data(&jstring);
  uint32_t size = iotjs_string_size(&jstring);
  int pos = 0;
  unsigned char frame_type = 0x1;
  unsigned char *buffer = IOTJS_CALLOC(256 ,unsigned char);
  buffer[pos++] = (unsigned char)frame_type; // text frame

  if(size <= 125) {
    buffer[pos++] = size;
  }
  else if(size <= 65535) {
    buffer[pos++] = 126; //16 bit length follows

    buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
    buffer[pos++] = size & 0xFF;
  }
  else { // >2^16-1 (65535)
    buffer[pos++] = 127; //64 bit length follows

    // write 8 bytes length (significant first)

    // since msg_length is int it can be no longer than 4 bytes = 2^32-1
    // padd zeroes for the first 4 bytes
    for(int i=3; i>=0; i--) {
      buffer[pos++] = 0;
    }
    // write the actual 32bit msg_length in the next 4 bytes
    for(int i=3; i>=0; i--) {
      buffer[pos++] = ((size >> 8*i) & 0xFF);
    }
  }
  memcpy((void*)(buffer+pos), msg, size);

  return jerry_create_string_from_utf8(buffer+pos);

}

JS_FUNCTION(decodeFrame) {
  DJS_CHECK_THIS();
  DJS_CHECK_ARGS(1, object);

  jerry_value_t jbuffer = JS_GET_ARG(0, object);
  iotjs_bufferwrap_t *buffer_wrap = iotjs_bufferwrap_from_jbuffer(jbuffer);

  unsigned char *buff_ptr = (unsigned char *)buffer_wrap->buffer;

  uint8_t fin_bit = (buff_ptr[0] >> 7) & 0x01;
  uint8_t opcode = buff_ptr[0] & 0x0F;

  uint8_t payload_byte = (buff_ptr[1]) & 0x7F;

  buff_ptr += 2;

  IOTJS_UNUSED(fin_bit);

  uint8_t *payload_len = NULL;
  if (!(payload_byte ^ 0x7E)) {
    payload_len = (uint8_t *)malloc(sizeof(uint8_t) * sizeof(uint16_t));
    payload_len[0] = buff_ptr[1];
    payload_len[1] = buff_ptr[0];
    buff_ptr += sizeof(uint16_t);
  } else if (!(payload_byte ^ 0x7F)) {
    payload_len = (uint8_t *)malloc(sizeof(uint8_t) * sizeof(uint64_t));
    for (uint8_t i = 0; i < sizeof(uint64_t); i++) {
      payload_len[i] = buff_ptr[sizeof(uint64_t) - 1 - i];
    }
    buff_ptr += sizeof(uint64_t);
  } else {
    payload_len = &payload_byte;
  }

  uint8_t mask = (buff_ptr[1] >> 7) & 0x01;
  uint32_t *mask_key = NULL;
  if (mask) {
    mask_key = (uint32_t *)buff_ptr;
    buff_ptr += sizeof(uint32_t);

    for (uint64_t i = 0; i < *payload_len; i++) {
      buff_ptr[i] ^= ((unsigned char *)(mask_key))[i % 4];
    }
  }

  jerry_value_t ret_buff = iotjs_bufferwrap_create_buffer(*payload_len);
  iotjs_bufferwrap_t *ret_wrap = iotjs_bufferwrap_from_jbuffer(ret_buff);

  switch (opcode) {
    case WS_OP_CONTINUE: {
      // concat buffers
      break;
    }

    case WS_OP_UTF8: {
      // assert to have utf8
      jerry_release_value(ret_buff);
      if (!jerry_is_valid_utf8_string(buff_ptr, *payload_len)) {
        JS_CREATE_ERROR(COMMON, "not valid utf8 string in websocket");
      }
      return jerry_create_string_sz_from_utf8((const jerry_char_t *)buff_ptr, *payload_len);
    }

    case WS_OP_BINARY: {
      memcpy(ret_wrap->buffer, buff_ptr, *payload_len);
      ret_wrap->length = *payload_len;
      // binary data
    }

    case WS_OP_TERMINATE: {
      // close connection
    }

    case WS_OP_PING: {
      //
    }

    case WS_OP_PONG: {
      //
    }

    default:
      return JS_CREATE_ERROR(COMMON, "Unknown WebSocket opcode");
      break;
  }



  return jerry_create_undefined();
}

jerry_value_t InitWebsocket() {
 IOTJS_UNUSED(WS_GUID);
 jerry_value_t jws = jerry_create_object();
 iotjs_jval_set_method(jws, "checkHandshakeKey", CheckHandshakeKey);
 iotjs_jval_set_method(jws, "prepareHandshake", PrepareHandshakeRequest);
 iotjs_jval_set_method(jws, "decodeFrame", decodeFrame);

 return jws;
}
