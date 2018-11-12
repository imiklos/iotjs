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

#include <stdlib.h>
#include "internal/node_api_internal.h"

#include "iotjs_def.h"

#include <dlfcn.h>

#define XSTR(ARG) #ARG
#define STR(ARG) XSTR(ARG)


jerry_value_t iotjs_load_module(const char* path) {
  if (path == NULL) {
    const char* error = "Invalid module path";
    return jerry_create_error(JERRY_ERROR_TYPE, (const jerry_char_t*)error);
  }

  void* dynamic_lib = dlopen(path, RTLD_NOW);

  if (dynamic_lib == NULL) {
    const char* error = "Can not open module";
    return jerry_create_error(JERRY_ERROR_TYPE, (const jerry_char_t*)error);
  }

  void* loader_function = dlsym(dynamic_lib, STR(IOTJS_MODULE_ENTRYPOINT));
  if (loader_function == NULL) {
    dlclose(dynamic_lib);
    const char* error = "Entrypoint not found in module";
    return jerry_create_error(JERRY_ERROR_TYPE, (const jerry_char_t*)error);
  }

  const iotjs_module* module =
      (const iotjs_module*)((iotjs_module_info_getter)loader_function)();

  if (module == NULL) {
    dlclose(dynamic_lib);
    const char* error = "Invalid module info";
    return jerry_create_error(JERRY_ERROR_TYPE, (const jerry_char_t*)error);
  }

  if (module->iotjs_module_version != IOTJS_CURRENT_MODULE_VERSION) {
    dlclose(dynamic_lib);
    const char* error = "Incorrect version requested in the module";
    return jerry_create_error(JERRY_ERROR_TYPE, (const jerry_char_t*)error);
  }

  return module->initializer();
}


JS_FUNCTION(DLload) {
  DJS_CHECK_ARGS(1, string);

  iotjs_string_t file = JS_GET_ARG(0, string);
  const char* filename = iotjs_string_data(&file);

  jerry_value_t jresult = iotjs_load_module(filename);

  iotjs_string_destroy(&file);

  return jresult;
}


JS_FUNCTION(OpenNativeModule) {
  iotjs_string_t location = JS_GET_ARG(0, string);

  void* handle = dlopen(iotjs_string_data(&location), RTLD_LAZY);
  iotjs_string_destroy(&location);

  if (handle == NULL) {
    char* err_msg = dlerror();
    jerry_value_t jval_error =
        jerry_create_error(JERRY_ERROR_COMMON, (jerry_char_t*)err_msg);
    return jval_error;
  }

  jerry_value_t exports;

// #ifdef ENABLE_NAPI
  int status = napi_module_init_pending(&exports);
  if (status == napi_module_load_ok) {
    return exports;
  }
  if (status == napi_pending_exception) {
    /** exports is an error reference */
    return exports;
  }
  if (status == napi_module_no_nm_register_func) {
    jerry_value_t jval_error = jerry_create_error(
        JERRY_ERROR_COMMON,
        (jerry_char_t*)"Module has no declared entry point.");
    return jval_error;
  }
// #endif

  void (*init_fn)(jerry_value_t);
  init_fn = dlsym(handle, "iotjs_module_register");
  // check for dlsym
  if (init_fn == NULL) {
    char* err_msg = dlerror();
    dlclose(handle);
    char* msg_tpl = "dlopen(%s)";
    char msg[strlen(err_msg) + 8];
    sprintf(msg, msg_tpl, err_msg);

    jerry_value_t jval_error =
        jerry_create_error(JERRY_ERROR_COMMON, (jerry_char_t*)msg);
    return jval_error;
  }

  exports = jerry_create_object();
  (*init_fn)(exports);
  return exports;
}


jerry_value_t InitDynamicloader() {

  jerry_value_t loader = jerry_create_object();
  iotjs_jval_set_method(loader, "dynamicloader", DLload);
  iotjs_jval_set_method(loader, "OpenNativeModule", OpenNativeModule);
  return loader;
}
