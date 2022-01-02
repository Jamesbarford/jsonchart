# JSON Charts

Plot an SVG Line chart from the commandline.

Currently only a line chart is supported.

# Build
```sh
make
```

And if you would like to install the program:

```sh
make && sudo make install
```

# Usage
```bash
Usage: ./jsonchart [OPTIONS]

Create an svg chart from JSON. JSON must be an array and
both values must be numeric

 --file <string>                          Path to the json file
 --out-file <string>                      Name of outfile
 --x-name <string>                        Name of JSON key for x values e.g .x
 --x-type <string|long|int|float|double>  Data type for x values
 --y-name <string>                        Name of JSON key for y values e.g .y
 --y-type <string|long|int|float|double>  Data type for y values

Optional:

  --width <int>   Width of the chart
  --height <int>  Height of the chart
  --reverse <'true'|'false'>  Should the data be plotted in reverse? Defaults to false
```

# Example
Given some JSON:

*stored in a file called `data.json`*
```json
[
  {"x":1, "y":2},
  {"x":2, "y":3},
  {"x":3, "y":4},
  {"x":4, "y":5},
  {"x":5, "y":6},
  {"x":6, "y":7}
]
```

Running the following would create a line chart:
```sh
./jsonchart \
  --file ./data.json \
  --x-name .x \
  --x-type double \
  --y-name .y \
  --y-type double \
  --out-file test \
  --width 600 \
  --height 400
```
