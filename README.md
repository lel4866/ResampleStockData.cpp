# ResampleStockData.cpp
Windows command line program which re-orders stock data in .csv files to obtain interleaved (in time) training (In Sample Training),
validation (In Sample Validation), and test data sets (Out of Sample)

Why would you want to do this? Because for non-stationary time series, such as those found in stock/futures data, we would like to both train and test
on the most recent samples. But most software requires us to specify contiguous segments of time to separate these data sets.

If the training software allows us to use/generate trading systems that always exit at the end of day, end of week, end of month, or even end of
year, instead of using contiguous sets of data, we could use interleaved sets of data.

For example, if I had a system that exited end of week, and if my total data set consisted of 50 weeks of data, instead of specifying, for instance,
that the training data was weeks 1-30, the validation was weeks 31-40, and the test was weeks 41-50, we could speciy a ratio for the training,
validation, and test, say, 3:2:1. Then the training data would consist of weeks 1-3, 7-9, 13-15, etc., the validation data would be weeks 4-5, 10-11,
16-17, and the test data would consists of weeks 6, 12, 18. But...most software does not allow you to do this. So... This program will enable you
to do this with any software that allows both separation of the data into continguous segments and specification of end of day/week/month exits.
It does this by assembling the pieces (each piece being a day/week/month) into a contiguous segment and then remapping the dates of the bars so that
the software sees contiguous segments. The only extra requireent is that the trading system does not use any data from the prior piece, since that
piece is no longer the piece that was in fact, the prior piece.

## Command line arguments

Reads all files with .csv extension from directory specified in -d option and writes to same filenames as read with additional "_resampled" postfix
in the ResampledData sub-directory of the specified directory. Creates the ResampledData sub-directory if it doesn't exist.

### Command line interface:

ResampleStockData.exe -d directory -i [d|w|m] -r train#:validate#:#test [-m minimum value]:

### Command line options:

- -d {directory} specifies folder which contains the .csv files to resample. All .csv files in that directory will be read and resampled.
- -i {d | w | m} specifies the interleave factor; d will interleave on a daily basis, w on a weekly basis, m on a monthly basis
- -r {train#:validate#:test#} specifies the ratio of training data to validation data to test data; for example 3:2:1; only the last number may be 0;
max value of any number is 9; all 3 numbers must be specified.
- -m {number} specifies the minimum value of open, high, low, or close values before bar will be written to output file. If one bar has values that exceed
that minimum and a later bar has values below it, all prior bars will still be discarded. That is, the output file will only contain bars whose open,
high, low, and close are greater than or equal to the specified minimum value.

Example:
```
ResampleStockData.exe -d C:/Users/lel48/SQXData -i w -r 3:2:1 -m 1.0
```