# ResampleStockData.cpp
Re-orders stock data in .csv files to obtain interleaved (in time) training, validation, and test data

Command line interface:

-h help
-i [d | w] interleave factor; d will interleave on a daily basis, w on a weekly basis
-r ratio of training data to validation data to test data; for example 3:2:1; only the last number may be 0; max value of any number is 5 
