// ResampleStockData.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <fstream>

using std::cout;
using std::endl;
using std::string;
bool ProcessCommandLine(int argc, const char* argv[], std::filesystem::path& directory, char& interval, std::vector<int>& ratio);
bool ProcessCSVFile(std::ifstream& input_file, std::ofstream& output_file);

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
    std::vector<std::string> file_list;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        const auto full_name = entry.path().string();
        if (entry.is_regular_file())
        {
            if (entry.path().extension().string() != "csv")
                continue;

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
            ProcessCSVFile(csv_file, resampled_csv_file);
        }
    }
    if (file_list.empty()) {
        cout << "***Error*** No valid .csv files found in specified directory" << endl;
        return -1;
    }

    return 0;
}

bool ProcessCSVFile(std::ifstream& input_file, std::ofstream& output_file) {
    string line;
    float val;

    // Read data, line by line
    while (std::getline(input_file, line))
    {
        // Create a stringstream of the current line
        std::stringstream ss(line);

        // Keep track of the current column index
        int colIdx = 0;

        // Extract each value
        while (ss >> val) {

            // Add the current integer to the 'colIdx' column's values vector
            //result.at(colIdx).second.push_back(val);

            // If the next token is a comma, ignore it and move on
            if (ss.peek() == ',') ss.ignore();

            // Increment the column index
            colIdx++;
        }
    }

    // Close files
    input_file.close();
    output_file.close();
    return true;
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
                const char* ctoken = strtok_s((char*)argv[i + 1] , ":", &next_token);
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
