# ResampleStockData.cpp
Windows command line program which re-orders stock data in .csv files to obtain interleaved (in time) training, validation, and test data sets

Reads from directory specified in -d option and writes to same filenames as read with additional "_resampled"" postfix

Command line interface:

ResampleStockData.exe -d directory -i [d|w] -r train#:validate#:#test:

Command line options:

-h help
-d specifies folder which contains .csv files to resample. All .csv files in that directory will be read
-i [d | w] interleave factor; d will interleave on a daily basis, w on a weekly basis
-r ratio of training data to validation data to test data; for example 3:2:1; only the last number may be 0; max value of any number is 5 

Example:

ResampleStockData.exe -d C:/Users/lel48/SQXData -i w -r 3:2:1