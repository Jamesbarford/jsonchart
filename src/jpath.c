#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "jpath.h"
#include "cstr.h"

#define PATH_INDICIES_SIZE 20


typedef struct jpath {
    int type;
    /* shared pointer DO NOT FREE */
    char *key;
    /**
     * seems absurd to have more than 20 nested arrays
     * ie.
     * [1][2][3][4][5][6][7][8][9][10][11][12][13][14][15][16][17][18][19][20]
     * could malloc and realloc if needed but seems pointless
     */
    int array_indicies[PATH_INDICIES_SIZE];
    int idx_count;
} jpath;

void jpathPrintPath(jpath *jp) {
    printf("key: %s\n", jp->key);
    for (int i = 0; i < jp->idx_count; ++i) {
        printf("array_indicies[%d] = %d\n", i, jp->array_indicies[i]);
    }
}

void jpathPrintJson(cJSON *json) {
    if (json) {
        char *readable;
        readable = cJSON_Print(json);
        printf("%s\n", readable);
        free(readable);
    }
}

/**
 * We first assign the string buffer to jp->key. Then check to see if the key is
 * array access. If so we fill the array indicies. Example:
 *
 * Non array:
 * "foo":
 *   jp->key = "foo"
 *
 * Array:
 * "foo[0][1]":
 *   jp->key = "foo"
 *   jp->array_indicies = [0, 1]
 *   jp->idx_count = 2;
 */
void jpathParsePath(cstr *str, jpath *jp) {
    unsigned int j;
    char c;
    /* this is going to get converted to an int */
    char tempStrInt[BUFSIZ];
    int pos, i, len;

    memset(jp->array_indicies, 0, PATH_INDICIES_SIZE * sizeof(int));
    jp->key = str;
    jp->idx_count = 0;
    len = cstrlen(str);

    for (i = 0; i < len; ++i) {
        if (str[i] == '[') {
            j = i + 1;
            pos = 0;
            // peak ahead to see if it is succeeded by an int
            while (isdigit(c = str[j++])) {
                tempStrInt[pos++] = c;
            }

            tempStrInt[pos] = '\0';
            if (str[j - 1] == ']') {
                str[i] = '\0';
                jp->key = str;
                jp->array_indicies[jp->idx_count++] = atoi(tempStrInt);
            }
            i += pos;
        }
    }
}

char *jpathTypeString(int type) {
    switch (type) {
    case cJSON_False:
        return "false";
    case cJSON_True:
        return "true";
    case cJSON_NULL:
        return "null";
    case cJSON_String:
        return "string";
    case cJSON_Number:
        return "number";
    case cJSON_Array:
        return "array";
    case cJSON_Object:
        return "object";
    case cJSON_Raw:
        return "raw";
    }
    return "invalid";
}

void jpathPrintType(int type) {
    char *strType = jpathTypeString(type);
    printf("%s\n", strType);
}

void jpathPrintValue(cJSON *json) {
    if (json == NULL) {
        printf("null\n");
        return;
    }
    switch (json->type) {
    case cJSON_False:
        printf("false\n");
        return;
    case cJSON_True:
        printf("true\n");
        return;
    case cJSON_NULL:
        printf("null\n");
        return;
    case cJSON_String:
        printf("%s\n", json->valuestring);
        return;
    case cJSON_Number:
        printf("%.2f\n", json->valuedouble);
        return;
    case cJSON_Object:
    case cJSON_Array: {
        char *str = cJSON_Print(json);
        printf("%s\n", str);
        free(str);
        return;
    }
    case cJSON_Raw:
        printf("raw\n");
        return;
    }
    printf("null\n");
}

int jpathGetValue(cJSON *json, int valuetype, void *result) {
    cJSON *desiredJson = json;

    switch (valuetype) {
        case J_FLOAT: {
            double *floatResult = result;
            switch (desiredJson->type) {
                case cJSON_String: {
                    *floatResult = (double)atof(desiredJson->valuestring);
                    return JPATH_OK;
                }
                case cJSON_Number: {
                    *floatResult = desiredJson->valuedouble;
                    return JPATH_OK;
                }
                case cJSON_NULL: {
                    *floatResult = 0;
                    return JPATH_OK;
                }
                case cJSON_True: {
                    *floatResult = 1;
                    return JPATH_OK;
                }
                case cJSON_False: {
                    *floatResult = 0;
                    return JPATH_OK;
                }
                default: goto error;
            }
        }
        case J_LONG: {
            long *longResult = result;
            switch (desiredJson->type) {
                case cJSON_String: {
                    *longResult = (long)atol(desiredJson->valuestring);
                    return JPATH_OK;
                }
                case cJSON_Number: {
                    *longResult = (long)desiredJson->valuedouble;
                    return JPATH_OK;
                }
                case cJSON_NULL: {
                    *longResult = 0;
                    return JPATH_OK;
                }
                case cJSON_True: {
                    *longResult = 1;
                    return JPATH_OK;
                }
                case cJSON_False: {
                    *longResult = 0;
                    return JPATH_OK;
                }
                default: goto error;
            }
        }
        case J_STRING: {
            char **strResult = (char **)result;
            switch (desiredJson->type) {
                case cJSON_String: {
                    *strResult = desiredJson->valuestring;
                    return JPATH_OK;
                }
                case cJSON_Number: {
                    char buf[100] = {'\0'};
                    snprintf(buf, 100, "%10.f", desiredJson->valuedouble);
                    *strResult = buf;
                    return JPATH_OK;
                }
                case cJSON_NULL: {
                    *strResult = "null";
                    return JPATH_OK;
                }
                case cJSON_True: {
                    *strResult = "true";
                    return JPATH_OK;
                }
                case cJSON_False: {
                    *strResult = "false";
                    return JPATH_OK;
                }
                default: goto error;
            }
        }
    }

error:
    result = NULL;
    return JPATH_ERR;
}

/**
 * For handling array access for example:
 *
 * A nested array of NxN:
 * array[0][1]
 *
 * to get the second element of the first row
 */
void jpathTraverseArray(cJSON **cur, jpath *jp) {
    int arrsize, i;

    for (i = 0; i < jp->idx_count; ++i) {
        if ((*cur)->type != cJSON_Array) {
            fprintf(stderr, "Error: expected Array recieved: %s\n",
                jpathTypeString((*cur)->type));
            *cur = NULL;
            return;
        }
        arrsize = cJSON_GetArraySize(*cur);

        if (i >= arrsize || i < 0) {
            fprintf(
                stderr, "idx out of bounds: %d array size: %d\n", i, arrsize);
            *cur = NULL;
            return;
        } else {
            *cur = cJSON_GetArrayItem(*cur, jp->array_indicies[i]);
        }
    }
}

int jpathVisit(cJSON **cur, jpath *jp, cJSON *tmp) {
    switch ((*cur)->type) {
    case cJSON_False:
    case cJSON_True:
    case cJSON_NULL:
    case cJSON_String:
        break;
    /* keep digging through the arrays*/
    case cJSON_Array: {
        jpathTraverseArray(cur, jp);
        break;
    }
    case cJSON_Object:
        tmp = cJSON_GetObjectItemCaseSensitive(*cur, jp->key);
        if (tmp == NULL)
            return 1;
        if ((*cur)->type == cJSON_Array && jp->idx_count > 0)
            jpathTraverseArray(cur, jp);
        break;
    case cJSON_Raw:
        break;
    }
    return -1;
}

cJSON *jpathGet(cJSON *json, char *path) {
    int partCount;
    cstr **parts;
    jpath jp;

    if (path[0] == '.')
        path++;

    if ((parts = cstrSplit(path, '.', &partCount)) == NULL)
        return NULL;

    for (int i = 0; i < partCount; ++i) {
        jpathParsePath(parts[i], &jp);
        switch (json->type) {
            case cJSON_False:
            case cJSON_True:
            case cJSON_NULL:
            case cJSON_String:
                break;
            /* keep digging through the arrays*/
            case cJSON_Array: {
                jpathTraverseArray(&json, &jp);
                break;
            }
            case cJSON_Object:
                json = cJSON_GetObjectItemCaseSensitive(json, jp.key);
                if (json == NULL)
                    goto notfound;
                if (json->type == cJSON_Array && jp.idx_count > 0)
                    jpathTraverseArray(&json, &jp);
                break;
            case cJSON_Raw:
                break;
        }
    }

cleanup:
    cstrArrayRelease(parts, partCount);
    return json;

notfound:
    goto cleanup;
}

int jpathGetValueFromPath(cJSON *json, char *path, int type, void *retval) {
    cJSON *needle;

    if ((needle = jpathGet(json, path)) == NULL)
        return JPATH_ERR;

    return jpathGetValue(needle, type, retval);
}

cJSON *jpathParse(char *rawjson) {
    if (rawjson == NULL)
        return NULL;
    return cJSON_Parse(rawjson);
}
