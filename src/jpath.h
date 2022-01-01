/**
 * jsonchart - A commandline SVG Plotting Tool
 *
 * Version 1.0 Janurary 2022
 *
 * Copyright (c) 2022, James Barford-Evans
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __JPATH_H__
#define __JPATH_H__

#include "cJSON.h"

#define J_LONG 0
#define J_STRING 1
#define J_FLOAT 2

#define JPATH_OK 1
#define JPATH_ERR -1

cJSON *jpathGet(cJSON *json, char *path);
cJSON *jpathParse(char *rawjson);
void jpathPrintValue(cJSON *json);
void jpathPrintType(int type);
void jpathPrintJson(cJSON *json);
int jpathGetValue(cJSON *json, int valuetype, void *result);
int jpathGetValueFromPath(cJSON *json, char *path, int type, void *retval);

#define jpathGetString(json, path, retval) \
    jpathGetValueFromPath((json), (path), J_STRING, (retval))

#define jpathGetFloat(json, path, retval) \
    jpathGetValueFromPath((json), (path), J_FLOAT, (retval))

#define jpathGetLong(json, path, retval) \
    jpathGetValueFromPath((json), (path), J_LONG, (retval))

#endif
