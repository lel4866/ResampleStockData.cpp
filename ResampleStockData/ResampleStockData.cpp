//
// ResampleStockData.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <fstream>
#include <map>

using std::cout;
using std::endl;
using std::string;

bool ProcessCommandLine(int argc, const char* argv[], std::filesystem::path& directory, char& interval, std::vector<int>& ratio);
bool ProcessCSVFile(std::vector<int> ratio, std::ifstream& input_file, std::ofstream& output_file);
bool DateTimeStringsToTime_t(string& line, string& date, string& time, std::time_t& t);
std::vector<string> split(string line, const char* delimiter);
int dayOfWeek(int day, int month, int year);
int weekNumber(time_t startDate, time_t curDate);
time_t writeOutputFile(std::ofstream& output_file, const std::vector<std::vector<string>>& days, time_t initial_time_t);

constexpr int seconds_in_day = 24 * 24 * 60;

int main(int argc, const char* argv[])
{
    // process command line arguments;
    std::filesystem::path directory;
    char interval;
    std::vector<int> ratio;
    bool rc = ProcessCommandLine(argc, argv, directory, interval, ratio);
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
                cout << "***Error*** Unable to create '" << full_output_filename << "' for writing." << endl;
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

bool ProcessCommandLine(int argc, const char* argv[], std::filesystem::path& directory, char& interval, std::vector<int>& ratio) {
    bool directorySpecified = false;
    bool intervalIsSpecified = false;
    bool ratioIsSpecified = false;

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
    std::map<std::time_t, string> bars;

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
    while (std::getline(input_file, line))
    {
        // split line into fields
        std::vector<string> fields = split(line, ",");
        if (fields.size() != 8) {
            cout << "***Error*** There must be 8 comma separated fields: " << line << endl;
            return false;
        }

        // convert date and time fields to tm; if time is just xx:xx, add :00
        if (fields[1].size() == 5)
            fields[1] += ":00";
        if (!DateTimeStringsToTime_t(line, fields[0], fields[1], t))
            return false;

        // now save open, high, low, close,up, down fields in separate string
        std::ostringstream oss;
        for (int i = 3; i < 8; i++)
            oss << ',' << fields[i];

        // returns a std::pair, where if the second element is false, the datetime already exists
        auto retval = bars.emplace(t, oss.str());
        if (!retval.second)
            cout << "***Error*** Duplicate date/time: " << line << endl;
    }
    if (bars.empty()) {
        cout << "***Error*** Inout file is empty" << endl;
        return false;
    }

    // now place entries from main map into one of three vectors (training, validation, test)
    // each vector contains a vector of ticks for a whole day
    std::vector<std::vector<string>> training_set;
    std::vector<std::vector<string>> validation_set;
    std::vector<std::vector<string>> test_set;
    std::vector<std::vector<string>>* pSelectedSet{ nullptr };
   
    int bar_count = 0; // for debugging
    auto bar_iterator = bars.begin();
    time_t initial_time_t = (*bar_iterator).first;
    time_t cur_time_t = initial_time_t; // save first day for when we start writing the output files
    long current_day = (long)cur_time_t / seconds_in_day;
    while (true) {
        for (int i = 0; i < 3; i++) {
            switch (i) {
            case 0: pSelectedSet = &training_set; break;
            case 1: pSelectedSet = &validation_set; break;
            case 2: pSelectedSet = &test_set; break;
            }

            // place ratio[i] # of days into selected vector
            for (int j = 0; j < ratio[i]; j++) {
                std::vector<string> dayVector;
                // move ticks to dayVector so long as the ticks are in the same day as current_day
                while (bar_iterator != bars.end()) {
                    long day = (long)((*bar_iterator).first / seconds_in_day);
                    string& bar = (*bar_iterator++).second; // note: also increments bar_iterator
                    if (day != current_day) {
                        if (!dayVector.empty())
                            pSelectedSet->emplace_back(std::move(dayVector));
                        current_day = day; // current_day now is new day
                        break;
                    }
                    dayVector.emplace_back(std::move(bar));
                    bar_count++; // for debugging
                }
                if (bar_iterator == bars.end())
                    goto end; // no more ticks...break out of outer loop
            }
        }
    }
end:

    // write output file
    time_t next_day_time_t = writeOutputFile(output_file, training_set, initial_time_t);
    next_day_time_t = writeOutputFile(output_file, validation_set, next_day_time_t);
    writeOutputFile(output_file, test_set, next_day_time_t);

    // Close files
    input_file.close();
    output_file.close();
    return true;
}

time_t writeOutputFile(std::ofstream& output_file, const std::vector<std::vector<string>>& days, time_t day_time_t) {
    for (std::vector<string> day : days) {
        for (string& bar : day) {
            std::stringstream xx; // for debugging
            struct tm timeinfo;
            localtime_s(&timeinfo, &day_time_t);  // convert time_t in kvp.first to tm in timeinfo
            char buffer[32];
            //strftime(buffer, 80, "Now it's %I:%M%p.", timeinfo);
            std::strftime(buffer, 32, "%m/%d/%Y,%H:%M:%S", &timeinfo); // mm/dd/yyyy,HH:MM:SS
            xx << buffer << bar; // for debugging
            output_file << buffer << bar << endl;
        }
        day_time_t += seconds_in_day;
    }

    return day_time_t;
}

bool DateTimeStringsToTime_t(string& line, string& date, string& time, std::time_t& t) {
    std::istringstream ss(date + ' ' + time);
    std::tm datetime_tm;
    ss >> std::get_time(&datetime_tm, "%m/%d/%Y %H:%M:%S");
    if (ss.fail()) {
        cout << "***Error*** Invalid date or time: " << line << endl;
        return false;
    }
    t = std::mktime(&datetime_tm);
    return true;
}

// poor man's string split function
std::vector<string> split(string line, const char* delimiter) {
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

// returns week number relative to week number of start date
int weekNumber(time_t startDate, time_t curDate) {
    std::tm startDate_tm;
    localtime_s(&startDate_tm, &startDate);
    return 0;
}
