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
#ifndef __CHART_H__
#define __CHART_H__

#define TICK_BUFSIZ 20

typedef void chartFormatter(double value, char *svgbuf);

typedef struct chartAxisFormatters {
    chartFormatter *xFormatter;
    chartFormatter *yFormatter;
} chartAxisFormatters;

char *chartLineCreateSVG(double *x_values, double *y_values, int datalen,
        chartAxisFormatters *formatters, int width, int height, int *outlen);
char *chartLineMultiCreateSVG(int width, int height, int arrayCount,
        double **x_values_array, double **y_values_array, int array_len,
        chartAxisFormatters *formatters,
        int *outlen);

/* Write SVG Buffer to a file */
int chartCreateFile(char *filename, char *svgbuf, int outlen);
#endif
