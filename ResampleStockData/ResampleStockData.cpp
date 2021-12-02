//
// ResampleStockData.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <fstream>
#include <map>
#include <cassert>

using std::cout;
using std::endl;
using std::string;

bool ProcessCommandLine(int argc, const char* argv[], std::filesystem::path& directory, char& interval, std::vector<int>& ratio, float& min_value);
bool ProcessCSVFile(std::vector<int> ratio, std::ifstream& input_file, std::ofstream& output_file);
bool DateStringsToTime_t(const string& line, const string& date, std::time_t& t);
std::vector<string> split(const string& line, const char* delimiter);
int dayOfWeek(int day, int month, int year);
int weekNumber(int start_weel_number, time_t curDate);
int weekNumber(time_t t);

time_t writeOutputFile(std::ofstream& output_file, const std::vector<std::vector<string>>& days, time_t initial_time_t, const string& dataset_name);

constexpr int seconds_in_day = 24 * 60 * 60;

int main(int argc, const char* argv[])
{
    // process command line arguments;
    std::filesystem::path directory;
    char interval;
    std::vector<int> ratio;
    float min_value = 1.0f;
    bool rc = ProcessCommandLine(argc, argv, directory, interval, ratio, min_value);
    if (!rc)
        return -1;

    // process each file in directory
    int file_count = 0;
    std::vector<std::string> file_list;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        const auto full_name = entry.path().string();
        if (entry.is_regular_file())
        {
            if (entry.path().extension().string() != ".csv")
                continue;
            const string input_filename = entry.path().filename().string();
            file_count++;

            // open input file
            std::ifstream csv_file(entry.path());
            // Make sure the file is open
            if (!csv_file.is_open() || !csv_file.good()) {
                cout << "***Error*** Unable to open '" << entry.path() << "' for reading." << endl;
                continue;
            }

            // if necessary, create output directory ResampledData, then create output file
            std::filesystem::path output_directory = entry.path().parent_path().concat("/ResampledData/");
            if (!std::filesystem::exists(output_directory)) {
                // create output directory
                if (!std::filesystem::create_directory(output_directory)) {
                    cout << "***Error*** Unable to create output directory '" << output_directory << "'" << endl;
                    return -1;;
                }
                cout << "Created output directory '" << output_directory << "'" << endl;
            }
            const string output_filename = input_filename.substr(0, input_filename.size() - 4) + "_resampled.csv";
            string full_output_filename = entry.path().parent_path().string() + "/ResampledData/" + input_filename.substr(0, input_filename.size() - 4) + "_resampled.csv";
            std::ofstream resampled_csv_file(full_output_filename);
            // if we can't create an output file, we quit, because we probably can't create any other output files either
            if (!resampled_csv_file.is_open() || !resampled_csv_file.good()) {
                cout << "***Error*** Unable to create '" << full_output_filename << "' for writing. It might be locked by another program." << endl;
                return -1;
            }

            // now process input file to output file
            cout << "Resampling '" << input_filename << "' to create '" << output_filename << endl;
            if (!ProcessCSVFile(ratio, csv_file, resampled_csv_file))
                continue;
        }
    }
    if (file_count == 0) {
        cout << "***Error*** No valid .csv files found in specified directory" << endl;
        return -1;
    }

    return 0;
}

bool ProcessCommandLine(int argc, const char* argv[], std::filesystem::path& directory, char& interval, std::vector<int>& ratio, float& min_value) {
    bool directorySpecified = false;
    bool intervalIsSpecified = false;
    bool ratioIsSpecified = false;
    bool minIsSpecified = false;

    for (int i = 1; i < argc; i += 2) {
        const string parm(argv[i]);
        if (parm == "-d" || parm == "--directory") {
            if (directorySpecified)
            {
                cout << "***Error*** directory specified more than once." << endl;
                return false;
            }
            directorySpecified = true;

            if (i + 1 < argc) {
                string dirname{ argv[i + 1] };
                cout << "input directory = " << dirname << endl;
                directory = std::filesystem::path(dirname);
                std::filesystem::path folder(dirname);
                if (!std::filesystem::is_directory(folder)) {
                    cout << "***Error*** Specified directory does not exist or is not actually a directory." << endl;
                    return false;
                }
                cout << "output directory = " << dirname << "/ResampledData" << endl;
            }
            else {
                cout << "***Error*** No directory specified after -d" << endl;
                return false;
            }
        }
        else if (parm == "-i" || parm == "--interval") {
            if (intervalIsSpecified)
            {
                cout << "***Error*** interval specified more than once." << endl;
                return false;
            }
            intervalIsSpecified = true;

            if (i + 1 < argc) {
                if (strlen(argv[i + 1]) != 1 || (*argv[i + 1] != 'd' && *argv[i + 1] != 'w')) {
                    cout << "***Error*** Invalid interval. Must be d or w" << endl;
                    return false;
                }
                interval = *argv[i + 1];
            }
            else {
                cout << "***Error*** No interval (d or w) specified after -i" << endl;
                return false;
            }
        }
        else if (parm == "-r" || parm == "--ratio") {
            if (ratioIsSpecified)
            {
                cout << "***Error*** ratio specified more than once." << endl;
                return false;
            }
            ratioIsSpecified = true;

            if (i + 1 < argc) {
                char* next_token = nullptr;
                const char* ctoken = strtok_s((char*)argv[i + 1], ":", &next_token);
                while (ctoken != nullptr) {
                    // convert token to integer
                    string token{ ctoken };
                    if (token.find_first_not_of("0123456789") == string::npos)
                        ratio.push_back(atoi(token.c_str()));
                    else {
                        cout << "***Error*** Invalid ratio specification (train#:valid#:test#)" << endl;
                        return false;
                    }
                    ctoken = strtok_s(0, ":", &next_token);
                }
                if (ratio.size() != 3) {
                    cout << "***Error*** Invalid ratio specification (train#:valid#:test#). Must specify 3 integer values" << endl;
                    return false;
                }
            }
            else {
                cout << "***Error*** No ratio (train#:valid#:test#) specified after -r" << endl;
                return false;
            }
        }
        else if (parm == "-m" || parm == "--min") {
            if (minIsSpecified)
            {
                cout << "***Error*** min specified more than once." << endl;
                return false;
            }
            minIsSpecified = true;

            if (i + 1 < argc) {
                string minstr{ argv[i + 1] };
                cout << "minimum value = " << minstr << endl;

                if (minstr.find_first_not_of("0123456789.") == string::npos) {
                    double dval = atof(minstr.c_str());
                    if (dval < 0.0 || dval > 1000.0) {
                        cout << "***Error*** Invalid minimum value" << endl;
                        return false;
                    }
                    min_value = (float)dval;
                }
                else {
                    cout << "***Error*** Invalid minimum value" << endl;
                    return false;
                }

            }
            else {
                cout << "***Error*** No value specified after -m" << endl;
                return false;
            }

        }
        else {
            cout << "'" << parm << "' is not a recognized parameter. Try ResampleStockData.cpp -h...but not yet" << endl;
            return false;
        }
    }

    return true;
}

bool ProcessCSVFile(std::vector<int> ratio, std::ifstream& input_file, std::ofstream& output_file) {
    time_t t;
    string line;
    std::map<std::time_t, std::vector<string>> bars; // time_t is the date, the vector contains a string for each time in the day

    // read header; 
    const string expected_header{"\"Date\",\"Time\",\"Open\",\"High\",\"Low\",\"Close\",\"Up\",\"Down\"" };
    if (!std::getline(input_file, line)) {
        cout << "***Error*** Inout file is empty" << endl;
        return false;
    }
    if (line != expected_header) 
        cout << "***Warning*** First line is not expected header: " << expected_header << endl;

    // Read data, line by line and create dictionary<DateTime, Tick>
    char* next_token = nullptr;
    time_t cur_date_time_t = -1;
    time_t initial_time_t = -1;
    std::vector<string> bars_for_day;
    int num_days = 0;
    while (std::getline(input_file, line))
    {
        // split line into fields
        std::vector<string> fields = split(line, ",");
        if (fields.size() != 8) {
            cout << "***Error*** There must be 8 comma separated fields: " << line << endl;
            return false;
        }

        // convert date field to time_t;
        if (!DateStringsToTime_t(line, fields[0], t))
            return false;
        if (initial_time_t == -1)
            initial_time_t = t;

        // if new day, save previous day in bars map and clear bars_for_day vector so it can be used for next day
        if (t != cur_date_time_t) {
            if (!bars_for_day.empty()) {
                auto retval = bars.emplace(cur_date_time_t, std::move(bars_for_day));
                num_days++;
                if (cur_date_time_t == 1637902800) {
                    int xxx = 1;
                }
                bars_for_day.clear(); // resurrect moved bars_for_day
                if (!retval.second)
                    cout << "***Error*** Duplicate date: " << line << endl;
            }
            cur_date_time_t = t;
        }

        //
        // now add this bar to current bars_for_day vector
        //

        // add seconds to time field if it doesn't exist
        if (fields[1].size() == 5)
            fields[1] += ":00";

        // create bar string without date field
        std::ostringstream oss;
        for (int i = 1; i < 8; i++)
            oss << ',' << fields[i];

        // append bar string to bars_for_day vector
        bars_for_day.emplace_back(oss.str());
    }

    // save last day
    if (!bars_for_day.empty()) {
        auto retval = bars.emplace(cur_date_time_t, std::move(bars_for_day));
        if (!retval.second)
            cout << "***Error*** Duplicate date: " << line << endl;
    }

    // check for no valid days
    if (bars.empty()) {
        cout << "***Error*** Input file is empty" << endl;
        return false;
    }

    //
    // here's where the magic occurs. We split up the data into train, validate and test sets in the requested ratio
    //

    // now place entries from main map into one of three vectors (training, validation, test)
    // each vector contains a vector of ticks for a whole day
    std::vector<std::vector<string>> training_set;
    std::vector<std::vector<string>> validation_set;
    std::vector<std::vector<string>> test_set;
    std::vector<std::vector<string>>* pSelectedSet{ nullptr };
#if 0   
    auto day_iterator = bars.begin();
    while (true) {
        for (int i = 0; i < 3; i++) {
            switch (i) {
                case 0: pSelectedSet = &training_set; break;
                case 1: pSelectedSet = &validation_set; break;
                case 2: pSelectedSet = &test_set; break;
            }

            // place ratio[i] # of days into selected vector
            for (int j = 0; j < ratio[i]; j++) {
                if (day_iterator == bars.end())
                    goto end;
                std::vector<string>& day = (*day_iterator++).second;
                pSelectedSet->emplace_back(std::move(day)); // we will never ever try and use this entry in bars map
            }
        }
    }
#endif

#if 1   
    auto day_iterator = bars.begin();
    time_t start_day = (*day_iterator).first;
    int start_week_number = weekNumber(start_day);
    int cur_week = 0;
    while (true) {
        for (int i = 0; i < 3; i++) {
            switch (i) {
            case 0: pSelectedSet = &training_set; break;
            case 1: pSelectedSet = &validation_set; break;
            case 2: pSelectedSet = &test_set; break;
            }

            // place ratio[i] # of weeks into selected vector
            for (int j = 0; j < ratio[i]; j++, cur_week++) {
                // put a whole weeks worth of days into selected vector
                while ((day_iterator != bars.end()) && weekNumber(start_week_number, (*day_iterator).first) == cur_week) {
                    std::vector<string>& day = (*day_iterator++).second;
                    pSelectedSet->emplace_back(std::move(day)); // we will never ever try and use this entry in bars map
                }
                if (day_iterator == bars.end())
                    goto end;
            }
        }
    }
#endif

end:

    // write output file
    time_t next_day_time_t = writeOutputFile(output_file, training_set, initial_time_t, "training");
    next_day_time_t = writeOutputFile(output_file, validation_set, next_day_time_t, "validation");
    writeOutputFile(output_file, test_set, next_day_time_t, "test");

    // Close files
    input_file.close();
    output_file.close();
    return true;
}

time_t writeOutputFile(std::ofstream& output_file, const std::vector<std::vector<string>>& days, time_t day_time_t, const string& dataset_name) {
    struct tm timeinfo;
    char init_time_buffer[32];
    char buffer[32];
    time_t initial_day_time_t = day_time_t;
    localtime_s(&timeinfo, &day_time_t);  // convert time_t in kvp.first to tm in timeinfo
    std::strftime(init_time_buffer, 32, "%m/%d/%Y", &timeinfo); // mm/dd/yyyy,

    for (std::vector<string> day : days) {
        assert(!day.empty());
        for (string& bar : day) {
            localtime_s(&timeinfo, &day_time_t);  // convert time_t in kvp.first to tm in timeinfo
            std::strftime(buffer, 32, "%m/%d/%Y,", &timeinfo); // mm/dd/yyyy,
            output_file << buffer << bar << endl;
        }
        day_time_t += seconds_in_day;
    }

    time_t last_time_t = day_time_t - seconds_in_day;
    localtime_s(&timeinfo, &last_time_t);  // convert time_t in kvp.first to tm in timeinfo
    std::strftime(buffer, 32, "%m/%d/%Y", &timeinfo); // mm/dd/yyyy,
    cout << "Writing " << dataset_name << " dataset from " << init_time_buffer << "  to " << buffer << endl;
    return day_time_t;
}

// returns time_t corresponding to given date string
bool DateStringsToTime_t(const string& line, const string& date, std::time_t& t) {
    std::tm datetime_tm;
    std::istringstream ss(date + " 00:00:00");
    ss >> std::get_time(&datetime_tm, "%m/%d/%Y %H:%M:%S");
    if (ss.fail()) {
        cout << "***Error*** Invalid date: " << line << endl;
        return false;
    }

    t = std::mktime(&datetime_tm);
    if (t == -1) {
        cout << "***Error*** Invalid date: " << line << endl;
        return false;
    }

    return true;
}

// poor man's string split function
std::vector<string> split(const string& line, const char* delimiter) {
    std::vector<string> fields;

    char* next_token = nullptr;
    const char* ctoken = strtok_s((char*)line.c_str(), delimiter, &next_token);
    while (ctoken != nullptr) {
        // save token
        string token{ ctoken };
        fields.emplace_back(string(ctoken));
        ctoken = strtok_s(0, delimiter, &next_token);
    }

    return fields;
}

// uses Zeller's algorithm to find day of week; 0 = Saturday
int dayOfWeek(int day, int month, int year) {
    int mon;
    if (month > 2)
        mon = month; //for march to december month code is same as month
    else {
        mon = (12 + month); // for Jan and Feb, month code will be 13 and 14
        year--; // decrease year for Jan and Feb
    }
    int y = year % 100; // last two digits
    int c = year / 100; // first two digits
    int w = (day + ((13 * (mon + 1)) / 5) + y + (y / 4) + (c / 4) + (5 * c));
    return w % 7;
}

// returns week number relative to week number of starting date
// week begins on Sunday, ends on Saturday
int weekNumber(int start_week_number, time_t curDate) {
    return weekNumber(curDate) - start_week_number;
}

// returns week number relative to Jan 1, 1970 - where time_t is 0
int weekNumber(time_t t) {
    // get day of week of startDate
    std::tm date_tm;
    localtime_s(&date_tm, &t);
    const int start_day_of_week = date_tm.tm_wday;
    // compute # of days since Dec 28, 1969, a Sunday
    // then, week number is just the day number divided by 7
    const long day_num = (long)t / seconds_in_day + 4;
    return day_num / 7;
}

