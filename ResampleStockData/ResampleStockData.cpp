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
int zellersAlgorithm(int day, int month, int year);

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
            file_count++;

            // open input file
            std::ifstream csv_file(entry.path());
            const string input_filename = entry.path().filename().string();
            // Make sure the file is open
            if (!csv_file.is_open() || !csv_file.good()) {
                cout << "***Error*** Unable to open '" << input_filename << "' for reading." << endl;
                continue;
            }

            // create output file
            string output_filename = input_filename.substr(0, input_filename.size() - 4) + "_resampled.csv";
            std::ofstream resampled_csv_file(output_filename);
            if (!resampled_csv_file.is_open() || !resampled_csv_file.good()) {
                cout << "***Error*** Unable to open '" << output_filename << "' for writing." << endl;
                continue;
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
                cout << "directory = " << dirname << endl;
                directory = std::filesystem::path(dirname);
                std::filesystem::path folder(dirname);
                if (!std::filesystem::is_directory(folder)) {
                    cout << "***Error*** Specified directory does not exist or is not actually a directory." << endl;
                    return false;
                }
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
        cout << "***Error*** Inout file appears to be empty" << endl;
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

    // now go through ration vector and place entries from main map into one of three vectors
    for (int i = 1; i < 3; i++) {
        int date_count = ratio[i];
    }

    // write output file
    for (auto& kvp : bars) {
        std::stringstream xx; // for debugging
        struct tm timeinfo;
        localtime_s(&timeinfo, &kvp.first);
        char buffer[32];
        std::strftime(buffer, 32, "%m/%d/%Y,%H:%M:%S", &timeinfo);
        xx << buffer << kvp.second; // for debugging
        output_file << buffer << kvp.second;
    }

    // Close files
    input_file.close();
    output_file.close();
    return true;
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

// to find day of week; 0 = Saturday
int zellersAlgorithm(int day, int month, int year) {
    int mon;
    if (month > 2)
        mon = month; //for march to december month code is same as month
    else {
        mon = (12 + month); //for Jan and Feb, month code will be 13 and 14
        year--; //decrease year for month Jan and Feb
    }
    int y = year % 100; //last two digit
    int c = year / 100; //first two digit
    int w = (day + (int)floor((13 * (mon + 1)) / 5) + y + (int)floor(y / 4) + (int)floor(c / 4) + (5 * c));
    return w % 7;
}
