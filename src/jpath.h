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
