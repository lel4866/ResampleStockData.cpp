# ResampleStockData.cpp
Windows command line program which re-orders stock data in .csv files to obtain interleaved (in time) training, validation, and test data sets

## Command line arguments

Reads all files with .csv extension from directory specified in -d option and writes to same filenames as read with additional "_resampled" postfix
in the ResampledData sub-directory of the specified directory. Creates the ResampledData sub-directory if it doesn't exist.

### Command line interface:

ResampleStockData.exe -d directory -i [d|w|m] -r train#:validate#:#test [-m minimum value]:

### Command line options:

- -d {directory} specifies folder which contains the .csv files to resample. All .csv files in that directory will be read
- -i {d | w | m} specifies the interleave factor; d will interleave on a daily basis, w on a weekly basis, m on a monthly basis
- -r {train#:validate#:test#} specifies the ratio of training data to validation data to test data; for example 3:2:1; only the last number may be 0;
max value of any number is 9; all 3 numbers must be specified.
- -m {number} specifies the minimum value of open, high, low, or close values before bar will be written to output file. If one bar has values that exceed
that minimum and a later bar has values below it, all prior bars will still be discarded. That is, the output file will only contain bars whose open,
high, low, and close are greater than or equal to the specified minimum value.

Example:
```
ResampleStockData.exe -d C:/Users/lel48/SQXData -i w -r 3:2:1
```