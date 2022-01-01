# JSON Charts

Plot an SVG chart quickly from the commandline

```bash
Usage: ./jsonchart [OPTIONS]

Create an svg chart from JSON. JSON must be an array and
both values must be numeric

 --file <string>                          Path to the json file
 --out-file <string>                      Name of outfile
 --x-name <string>                        Name of JSON key for x values
 --x-type <string|long|int|float|double>  Data type for x values
 --y-name <string>                        Name of JSON key for y values
 --y-type <string|long|int|float|double>  Data type for y values

Optional:

  --width <int>   Width of the chart
  --height <int>  Height of the chart
```
