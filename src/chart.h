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
