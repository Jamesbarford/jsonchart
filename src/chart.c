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

/**
 * Current problems:
 *
 * - Will always need to re-calculate the x & y to find the min & max for
 *   all of the lines plotted.
 *
 * - The `chartPointArray` is far to specific, there should be accessors
 *   for getting the values out of an array.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <float.h>

#include "chart.h"


#define X_AXIS 0
#define Y_AXIS 1
#define LINE_COLOR "#0052FF"
#define AXIS_COLOR "#CCCCCC"
#define TICK_COLOR "#333333"

#define abs(x) ((x) >= 0 ? (x) : -1 * (x))

#define chartInitScale(cs)                                                     \
    ((cs)->valMax = DBL_MIN, (cs)->valMin = DBL_MAX, (cs)->rangeMin = 0,       \
            (cs)->rangeMax = 0)

#define linearScale(cs, value)                                                 \
    (((value) - (cs->valMin)) * ((cs)->rangeMax - (cs)->rangeMin) /            \
        ((cs)->valMax - (cs)->valMin) + (cs)->rangeMin)

typedef struct chartPointArray {
    int len;
    double *xValues;
    double *yValues;
} chartPointArray;

typedef struct chartDimensions {
    int width;
    int height;
    int marginLeft;
    int marginRight;
    int marginBottom;
    int marginTop;
} chartDimensions;

typedef struct chartScale {
    double valMin;
    double valMax;
    double rangeMin;
    double rangeMax;
} chartScale;

static void _xAxisDefaultFormatter(double val, char *buf) {
    int len;

    len = snprintf(buf, TICK_BUFSIZ, "%.2f", val);
    buf[len] = '\0';
}

static void _yAxisDefaultFormatter(double val, char *buf) {
    int len;

    len = snprintf(buf, TICK_BUFSIZ, "%.2f", val);
    buf[len] = '\0';
}

/**
 * Gets `outlen` number of doubles evenly incrementing i.e:
 * {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000}
 *
 * Having an outlen of 7 would produce:
 * {100, 250, 400, 550, 700, 850, 1000}
 */
static void _getRange(double min, double max, double *out, int outlen) {
    double difference, _outlen, step;
    unsigned long iters, j, i;

    _outlen = outlen - 1;

    difference = abs(max - min);
    step = difference / _outlen;
    iters = step * (outlen - 1);

    j = 0;
    for (i = 0; i < iters; i += step)
        out[j++] = (min + i);

    out[outlen - 1] = max;
}

static void chartCalculateScales(chartDimensions *dimensions,
        chartPointArray *cp_array, chartScale **scales)
{
    chartScale *csy, *csx;
    int i;
    csy = scales[Y_AXIS];
    csx = scales[X_AXIS];

    for (i = 0; i < cp_array->len; ++i) {
        double x = cp_array->xValues[i];
        double y = cp_array->yValues[i];

        if (x > csx->valMax)
            csx->valMax = x;
        if (x < csx->valMin)
            csx->valMin = x;

        if (y > csy->valMax)
            csy->valMax = y;
        if (y < csy->valMin)
            csy->valMin = y;
    }

    csy->rangeMin = 0;
    csy->rangeMax = dimensions->height - dimensions->marginTop;

    csx->rangeMin = 0;
    csx->rangeMax = dimensions->width - dimensions->marginLeft;
}

/*================ Axis plotting functions =================*/
/**
 * `int *outlen` needs to be zero'd out before pumping into these functions
 * and is assumed to be keeping track of the larger buffer for the whole
 * chart.
 */

/**
 * This returns a buffer that will be the co-ordinates of where to plot a line
 * on an SVG chart.
 */
static char *_chartLinePointsToString(chartPointArray *cpArr,
        chartDimensions *cDim, chartScale *csy, int *len)
{
    int i, offset;
    char *outstr;
    double xSpace, acc, x, y;

    /* TODO: do something better than this!*/
    if ((outstr = malloc(sizeof(char) * cpArr->len << 10)) == NULL)
        return NULL;

    /* need to know space between x <-> points */
    xSpace = (double)cDim->width / cpArr->len;
    acc = 0;
    offset = 0;

    x = (cDim->marginLeft + (cDim->width - acc)) - cDim->marginRight;
    y = cDim->height - linearScale(csy, cpArr->yValues[0]);
    acc += xSpace;

    offset += sprintf(outstr,
            "<path fill=\"none\" "
            "stroke=\"%s\" stroke-width=\"1.3\" "
            "d=\"M%.10f,%.10f",
            LINE_COLOR, x, y);

    for (i = 1; i < cpArr->len; ++i) {
        x = (cDim->marginLeft + (cDim->width - acc)) - cDim->marginRight;
        y = cDim->height - linearScale(csy, cpArr->yValues[i]);

        offset += sprintf(outstr + offset, "L%.10f,%.10f", x, y);
        acc += xSpace;
    }
    offset += sprintf(outstr + offset, "\"/>");
    outstr[offset] = '\0';
    *len = offset;

    return outstr;
}

static char *chartXAxisCreate(chartDimensions *cDims, int numTicks,
        chartFormatter *formatter, double *xTicks, int *outlen)
{
    char *xAxis, tickBuf[200];
    double acc, tickSpace;
    int i;

    /* very generous allocation */
    if ((xAxis = malloc(sizeof(char) * BUFSIZ * numTicks)) == NULL)
        return NULL;

    // bottom x axis line
    *outlen += snprintf(xAxis + *outlen, BUFSIZ,
            "<g>"
            // bottom
            "<line fill=\"none\" stroke=\"%s\" stroke-width=\"1\""
            " x1=\"%d\" x2=\"%d\" y1=\"%d\" y2=\"%d\"></line>"
            "</g>",
            AXIS_COLOR,
            // bottom positioning
            cDims->marginLeft - cDims->marginRight,
            cDims->width + cDims->marginLeft - cDims->marginRight,
            cDims->height, cDims->height);

    // The label calculations
    acc = 0;
    tickSpace =
            (double)(cDims->width + cDims->marginRight + cDims->marginRight) /
            numTicks;

    *outlen += snprintf(xAxis + *outlen, BUFSIZ,
            "<g transform=\"translate(0, %d)\" fill=\"none\" font-size=\"10\""
            " font-family=\"sans-serif\" text-anchor=\"middle\">",
            cDims->height);

    // bottom ticks
    for (i = 0; i < numTicks; ++i) {
        formatter(xTicks[numTicks - 1 - i], tickBuf);
        *outlen += snprintf(xAxis + *outlen, BUFSIZ,
                "<g opactity=\"1\" transform=\"translate(%10.f, 0)\">"
                "<line stroke=\"%s\" y2=\"%d\"></line>"
                "<text style=\"font-size: 8px; text-anchor: end;\" "
                "fill=\"currentColor\""
                " transform=\"rotate(-60)\" y=\"%d\" dy=\"-.1em\">%s</text>"
                "</g>",
                cDims->width - acc,
                TICK_COLOR,
                // the little dash
                6,
                // the number by the dash positioning
                10,
                // the value to display
                tickBuf);
        acc += tickSpace;
    }

    *outlen += snprintf(xAxis + *outlen, 5, "</g>");

    xAxis[*outlen] = '\0';
    return xAxis;
}

static char *chartYAxisCreate(chartDimensions *cDims, int numTicks,
        chartFormatter *formatter, double *yTicks, int *outlen)
{
    char *yAxis, tickBuf[200];
    double acc, tickSpace;
    int i;

    /* very generous allocation */
    if ((yAxis = malloc(sizeof(char) * BUFSIZ * numTicks)) == NULL)
        return NULL;

    // left y axis line
    *outlen += snprintf(yAxis + *outlen, BUFSIZ,
            "<g>"
            "<line fill=\"none\" stroke=\"%s\" stroke-width=\"1\""
            " x1=\"%d\" x2=\"%d\" y1=\"%d\" y2=\"%d\"></line>"
            "</g>",
            AXIS_COLOR,
            cDims->marginLeft - cDims->marginRight,
            cDims->marginLeft - cDims->marginRight, cDims->marginTop,
            cDims->height);

    *outlen += snprintf(yAxis + *outlen, BUFSIZ,
            "<g transform=\"translate(%d, 0)\" fill=\"none\" font-size=\"10\""
            " font-family=\"sans-serif\" text-anchor=\"middle\">",
            cDims->marginLeft - cDims->marginRight - 7);

    acc = 0;
    tickSpace = (((double)cDims->height) / numTicks) + 0.5;
    // y axis ticks
    for (i = 0; i < numTicks; ++i) {
        formatter(yTicks[i], tickBuf);
        *outlen += snprintf(yAxis + *outlen, BUFSIZ,
                "<g opactity=\"1\" transform=\"translate(0, %.10f)\">"
                "<line stroke=\"%s\" x2=\"%d\"></line>"
                "<text style=\"font-size: 8px; text-anchor: end;\" "
                "fill=\"currentColor\" "
                "dy=\"0.32em\">%s</text>"
                "</g>",
                // spacing
                (cDims->height - acc),
                TICK_COLOR,
                // the little dash
                6,
                // the value to display
                tickBuf);
        acc += tickSpace;
    }
    *outlen += snprintf(yAxis + *outlen, 5, "</g>");

    yAxis[*outlen] = '\0';

    return yAxis;
}

char *chartLineCreateSVG(double *x_values, double *y_values, int arr_len,
        chartAxisFormatters *formatters, int width, int height, int *outlen)
{
    chartDimensions cDims;
    chartFormatter *yFormatter, *xFormatter;
    chartPointArray cpArr;
    char *svgbuf, *lineBuf, *xAxis, *yAxis;
    int lineBufLen, yAxisLen, xAxisLen;
    double yTicks[12], xTicks[5];

    svgbuf = NULL;
    lineBuf = NULL;
    xAxis = NULL;
    yAxis = NULL;
    *outlen = 0;
    chartScale csy, csx, *scales[2];

    cDims.marginBottom = 80;
    cDims.marginLeft = 60;
    cDims.marginTop = 10;
    cDims.marginRight = 10;
    cDims.width = width - cDims.marginLeft - cDims.marginRight;
    cDims.height = height - cDims.marginBottom - cDims.marginTop;

    chartInitScale(&csy);
    chartInitScale(&csx);

    scales[X_AXIS] = &csx;
    scales[Y_AXIS] = &csy;

    cpArr.len = arr_len;
    cpArr.xValues = x_values;
    cpArr.yValues = y_values;

    chartCalculateScales(&cDims, &cpArr, scales);

    if (formatters == NULL || (yFormatter = formatters->yFormatter) == NULL)
        yFormatter = _yAxisDefaultFormatter;

    if (formatters == NULL || (xFormatter = formatters->xFormatter) == NULL)
        xFormatter = _xAxisDefaultFormatter;

    _getRange(csx.valMin, csx.valMax, xTicks, 5);
    _getRange(csy.valMin, csy.valMax, yTicks, 12);

    lineBufLen = 0;
    yAxisLen = 0;
    xAxisLen = 0;

    if ((xAxis = chartXAxisCreate(&cDims, 5, xFormatter, xTicks, &xAxisLen)) ==
            NULL)
        goto svg_finalise;

    if ((yAxis = chartYAxisCreate(&cDims, 12, yFormatter, yTicks, &yAxisLen)) ==
            NULL)
        goto svg_finalise;

    if ((lineBuf = _chartLinePointsToString(&cpArr, &cDims, &csy, &lineBufLen)) == NULL)
        goto svg_finalise;

    if ((svgbuf = malloc(sizeof(char *) *
                         (BUFSIZ + yAxisLen + xAxisLen + lineBufLen))) == NULL)
        goto svg_finalise;

    *outlen = snprintf(svgbuf, BUFSIZ << 10,
            "<svg width=\"%d\" height=\"%d\" font-family=\"sans-serif\" "
            "xmlns=\"http://www.w3.org/2000/svg\">"
            "<rect width=\"100%%\" height=\"100%%\" fill=\"white\" />"
            "%s%s%s</svg>",
            width, height, xAxis, yAxis, lineBuf);

    svgbuf[*outlen] = '\0';

svg_finalise:
    if (xAxis)
        free(xAxis);
    if (yAxis)
        free(yAxis);
    if (lineBuf)
        free(lineBuf);

    return svgbuf;
}

/* Takes all of the computed values creating an SVG */
static char *_chartMultiCreateSVG(int arrayCount, chartPointArray *cpArrays,
        chartDimensions *cDims, chartFormatter *yFormatter,
        chartFormatter *xFormatter, double *xTicks, int xTickCount,
        double *yTicks, int yTickCount, chartScale *csy, int *outlen)
{
    chartPointArray *cpArr;
    int i, j, lineBufLen, xAxisLen, yAxisLen, svgbufSize;
    char **lineChartBuffers, *xAxis, *yAxis, *svgbuf;

    xAxis = NULL;
    yAxis = NULL;
    svgbuf = NULL;
    lineChartBuffers = NULL;

    if ((lineChartBuffers = malloc(sizeof(char *) * arrayCount)) == NULL)
        return NULL;

    /* Step 1: create line chart string buffers */
    for (i = 0; i < arrayCount; ++i) {
        cpArr = &cpArrays[i];

        if ((lineChartBuffers[i] = _chartLinePointsToString(
                     cpArr, cDims, csy, &lineBufLen)) == NULL)
            goto chart_finalise;
    }

    /* Step 2: create axis buffers */
    if ((xAxis = chartXAxisCreate(
                 cDims, xTickCount, xFormatter, xTicks, &xAxisLen)) == NULL)
        goto chart_finalise;

    if ((yAxis = chartYAxisCreate(
                 cDims, yTickCount, yFormatter, yTicks, &yAxisLen)) == NULL)
        goto chart_finalise;

    /* Step 3: create SVG buffer */
    svgbufSize =
            sizeof(char) * ((lineBufLen * arrayCount) + xAxisLen + yAxisLen);

    if ((svgbuf = malloc(svgbufSize)) == NULL)
        goto chart_finalise;

    *outlen = snprintf(svgbuf, svgbufSize,
            "<svg width=\"%d\" height=\"%d\" font-family=\"sans-serif\" "
            "xmlns=\"http://www.w3.org/2000/svg\">%s%s",
            cDims->width, cDims->height, xAxis, yAxis);

    for (j = 0; j < arrayCount; ++j) {
        *outlen += snprintf(
                svgbuf + *outlen, svgbufSize, "%s", lineChartBuffers[j]);
    }

    *outlen += snprintf(svgbuf + *outlen, 7, "</svg>");
    svgbuf[*outlen] = '\0';

chart_finalise:
    if (xAxis)
        free(xAxis);
    if (yAxis)
        free(yAxis);
    if (arrayCount > 0 && lineChartBuffers != NULL)
        for (j = 0; j < arrayCount; ++j)
            free(lineChartBuffers[j]);
    return svgbuf;
}

/**
 * We try and assemble as much as possible in this function to be plotted
 */
static char *_chartMultiCalculateAxisAndCreateSVG(chartDimensions *cDims,
        int arrayCount, chartPointArray *cpArrays, chartFormatter *yFormatter,
        chartFormatter *xFormatter, int *outlen)
{
    chartPointArray *cpArr;
    chartScale csy;
    int i, j;
    double x, y;
    double yTicks[12], xTicks[5], minX, maxX, minY, maxY;
    char *svgbuf;

    minX = minY = 100000000;
    maxX = maxY = -100000000;

    for (i = 0; i < arrayCount; ++i) {
        cpArr = &cpArrays[i];

        for (j = 0; j < cpArr->len; ++j) {
            x = cpArr->xValues[j];
            y = cpArr->yValues[j];

            if (x > maxX)
                maxX = x;
            if (x < minX)
                minX = x;
            if (y > maxY)
                maxY = y;
            if (y < minY)
                minY = y;
        }
    }

    _getRange(minX, maxX, xTicks, 5);
    _getRange(minY, maxY, yTicks, 12);

    csy.rangeMin = 0;
    csy.rangeMax = cDims->height - cDims->marginTop;
    csy.valMin = minY;
    csy.valMax = maxY;

    svgbuf = _chartMultiCreateSVG(arrayCount, cpArrays, cDims, yFormatter,
            xFormatter, xTicks, 5, yTicks, 12, &csy, outlen);

    return svgbuf;
}

/**
 * This does not yet work
 */
char *chartLineMultiCreateSVG(int width, int height, int arrayCount,
        double **x_values_array, double **y_values_array, int array_len,
        chartAxisFormatters *formatters, int *outlen)
{
    char *svgbuf;
    int i;
    chartDimensions cDims;
    chartFormatter *yFormatter, *xFormatter;
    chartPointArray *cp_arrays;

    if ((cp_arrays = malloc(sizeof(chartPointArray) * arrayCount)) == NULL)
        return NULL;

    for (i = 0; i < arrayCount; ++i) {
        cp_arrays[i].len = array_len;
        cp_arrays[i].xValues = x_values_array[i];
        cp_arrays[i].yValues = y_values_array[i];
    }

    if (formatters == NULL || (yFormatter = formatters->yFormatter) == NULL)
        yFormatter = _yAxisDefaultFormatter;

    if (formatters == NULL || (xFormatter = formatters->xFormatter) == NULL)
        xFormatter = _xAxisDefaultFormatter;

    cDims.marginBottom = 80;
    cDims.marginLeft = 60;
    cDims.marginTop = 10;
    cDims.marginRight = 10;
    cDims.width = width - cDims.marginLeft - cDims.marginRight;
    cDims.height = height - cDims.marginBottom - cDims.marginTop;

    svgbuf = _chartMultiCalculateAxisAndCreateSVG(
            &cDims, arrayCount, cp_arrays, yFormatter, xFormatter, outlen);

    free(cp_arrays);

    return svgbuf;
}

int chartCreateFile(char *filename, char *svgbuf, int outlen) {
    int total, bytes_left, sent_bytes, fd;

    total = 0;
    bytes_left = outlen;
    sent_bytes = 0;
    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);

    if (fd == -1) {
        return -1;
    }

    while (total < outlen) {
        sent_bytes = write(fd, svgbuf, bytes_left);

        if (sent_bytes == -1) {
            close(fd);
            return -1;
        }

        total += sent_bytes;
        bytes_left -= sent_bytes;
    }

    close(fd);
    return 1;
}

#ifdef JSON_CHART_CLI
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "jpath.h"
static char *progname;

static void printUsage() {
    fprintf(stderr,
            "\nUsage: %s [OPTIONS]\n\n"
            "Create an svg chart from JSON. JSON must be an array and\n"
            "both values must be numeric \n\n"
            " --file <string>                          Path to the json file\n"
            " --out-file <string>                      Name of outfile\n"
            " --x-name <string>                        Name of JSON key for x "
            "values\n"
            " --x-type <stueing|long|int|float|double>  Data type for x values\n"
            " --y-name <string>                        Name of JSON key for y "
            "values\n"
            " --y-type <string|long|int|float|double>  Data type for y values\n"
            "\nOptional:\n\n"
            "  --width <int>               Width of the chart\n"
            "  --height <int>              Height of the chart\n"
            "  --reverse <'true'|'false'>  Should the data be plotted in reverse?"
            " default false\n"
            "",
            progname);
    exit(EXIT_FAILURE);
}

static int getValueType(char *strtype) {
    if (strncmp(strtype, "string", 5) == 0)
        return J_STRING;
    if (strncmp(strtype, "long", 4) == 0 || strncmp(strtype, "int", 3) == 0)
        return J_LONG;
    if (strncmp(strtype, "float", 5) == 0 || strncmp(strtype, "double", 6) == 0)
        return J_FLOAT;
    return -1;
}

static char *getValueName(int j_type) {
    switch (j_type) {
    case J_FLOAT:
        return "float";
    case J_LONG:
        return "long";
    case J_STRING:
        return "string";
    default:
        return "invalid";
    }
}

static int getBoolean(char *boolean) {
    if (strncasecmp(boolean, "true", 4) == 0) return 1;
    if (strncasecmp(boolean, "false", 5) == 0) return 0;
    return 0;
}

static int printAxisTypeWarning(char axis) {
    fprintf(stderr,
            "ERROR: --%c-type must be one of "
            "<\"string\"|\"long\"|\"int\"|\"float\"|\"double\">\n",
            axis);
    return 1;
}

static int printMissingArgWarning(char *argname) {
    fprintf(stderr, "ERROR: %s must be defined\n", argname);
    return 1;
}

static void printJsonPathError(char *path, int j_type, char *strvalue) {
    fprintf(stderr, "ERROR: Failed to parse key '%s' to '%s', value was: %s\n",
            path, getValueName(j_type), strvalue);
}

/* This assumes an array of json is being passed in, and both must be numeric */
int main(int argc, char **argv) {
    progname = argv[0];

    cJSON *json, *el;
    int x_type, y_type, has_err, arr_size, reverse;
    char *x_value_name, *y_value_name, *filename, *out_filename, *raw_json,
            *svgbuf;
    char chartname[200];
    int i, infd, chartname_len, width, height, svgbuf_len;
    double *xValues, *yValues;
    struct stat sb;

    width = 300;
    height = 200;
    has_err = 0;
    reverse = 0;
    x_type = y_type = -1;
    x_value_name = y_value_name = filename = out_filename = NULL;

    /* Get command line inputs */
    for (i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--file", 6) == 0) {
            filename = argv[++i];
        } else if (strncmp(argv[i], "--y-type", 8) == 0) {
            y_type = getValueType(argv[++i]);
        } else if (strncmp(argv[i], "--x-type", 8) == 0) {
            x_type = getValueType(argv[++i]);
        } else if (strncmp(argv[i], "--y-name", 8) == 0) {
            y_value_name = argv[++i];
        } else if (strncmp(argv[i], "--x-name", 8) == 0) {
            x_value_name = argv[++i];
        } else if (strncmp(argv[i], "--out-file", 10) == 0) {
            out_filename = argv[++i];
        } else if (strncmp(argv[i], "--width", 7) == 0) {
            width = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--height", 8) == 0) {
            height = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--reverse", 9) == 0) {
            reverse = getBoolean(argv[++i]);
        }
    }

    /* Validate inputs */
    if (x_type == -1)
        has_err = printAxisTypeWarning('x');
    if (y_type == -1)
        has_err = printAxisTypeWarning('y');
    if (x_value_name == NULL)
        has_err = printMissingArgWarning("--x-name");
    if (y_value_name == NULL)
        has_err = printMissingArgWarning("--y-name");
    if (filename == NULL)
        has_err = printMissingArgWarning("--file");
    if (out_filename == NULL)
        has_err = printMissingArgWarning("--out-file");

    if (has_err == 1)
        printUsage();

    /* Parse JSON */
    if ((infd = open(filename, O_RDONLY, 0666)) == -1) {
        fprintf(stderr, "ERROR: Failed to open file '%s': %s\n", filename,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fstat(infd, &sb) == -1) {
        fprintf(stderr, "ERROR: Failed to stat file '%s': %s\n", filename,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    raw_json = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, infd, 0);
    if (raw_json == MAP_FAILED) {
        fprintf(stderr, "ERROR: Failed to mmap file '%s': %s\n", filename,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((json = jpathParse(raw_json)) == NULL) {
        fprintf(stderr, "ERROR: Failed to parse JSON\n");
        exit(EXIT_FAILURE);
    }

    if (json->type != cJSON_Array) {
        fprintf(stderr, "ERROR: JSON must be an array of JSON\n");
        exit(EXIT_FAILURE);
    }

    arr_size = cJSON_GetArraySize(json);

    if ((xValues = malloc(sizeof(double) * arr_size)) == NULL) {
        fprintf(stderr, "ERROR: Failed to malloc %dbytes for xValues: %s\n",
                arr_size, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((yValues = malloc(sizeof(double) * arr_size)) == NULL) {
        fprintf(stderr, "ERROR: Failed to malloc %dbytes for yValues: %s\n",
                arr_size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    (void)reverse;

    i = arr_size;
    cJSON_ArrayForEach(el, json) {
        double x, y;
        int idx = i-1;

        if (jpathGetValueFromPath(el, x_value_name, x_type, &x) == JPATH_ERR)
            printJsonPathError(x_value_name, x_type, el->string);

        if (jpathGetValueFromPath(el, y_value_name, y_type, &y) == JPATH_ERR)
            printJsonPathError(y_value_name, y_type, el->string);

        xValues[idx] = (double)x;
        yValues[idx] = (double)y;
        --i;
    }

    /* Create SVG Chart */
    chartname_len = snprintf(chartname, sizeof(chartname) * sizeof(char),
            "%s.svg", out_filename);
    chartname[chartname_len] = '\0';

    if ((svgbuf = chartLineCreateSVG(xValues, yValues, arr_size, NULL, width,
                 height, &svgbuf_len)) == NULL)
    {
        fprintf(stderr, "ERROR: Failed to create svg buffer\n");
        exit(EXIT_FAILURE);
    }

    if (chartCreateFile(chartname, svgbuf, svgbuf_len) == -1) {
        fprintf(stderr, "ERROR: Failed to write chart to file: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    cJSON_Delete(json);
    munmap(raw_json, sb.st_size);
    return 0;
}
#endif
