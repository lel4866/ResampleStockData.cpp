// ResampleStockData.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>

using std::cout;
using std::endl;
using std::string;
bool processCommandLine(int argc, const char* argv[], std::filesystem::path& directory, char& interval, std::vector<int>& ratio);

int main(int argc, const char* argv[])
{
    // process command line arguments;
    std::filesystem::path directory;
    char interval;
    std::vector<int> ratio{ 0, 0, 0 };
    bool rc = processCommandLine(argc, argv, directory, interval, ratio);
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
            const string base_name = entry.path().filename().string();
            file_list.push_back(full_name);
        }
    }

    return 0;
}

bool processCommandLine(int argc, const char* argv[], std::filesystem::path& directory, char& interval, std::vector<int>& ratio) {
    bool directorySpecified = false;
    bool intervalIsSpecified = false;
    bool ratioIsSpecified = false;
    for (int i = 1; i < argc; ++i) {
        const string parm(argv[i]);
        if (parm == "-d" || parm == "--directory") {
            if (directorySpecified)
            {
                cout << "***Error*** directory specified more than once." << endl;
                return false;
            }
            directorySpecified = true;
            if (i < argc) {
                directory = argv[i + 1];
                cout << "directory = " << directory << endl;
                std::filesystem::path folder(directory);
                if (!std::filesystem::is_directory(folder)) {
                    cout << "***Error*** specified directory does not exist or is not actually a directory." << endl;
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
        }
        else if (parm == "-r" || parm == "--ratio") {
            if (ratioIsSpecified)
            {
                cout << "***Error*** ratio specified more than once." << endl;
                return false;
            }
            ratioIsSpecified = true;
        }
        else {
            cout << "'" << parm << "' is not a recognized parameter. Try ResampleStockData.cpp -h" << endl;
            return false;
        }
    }

    return true;
}
