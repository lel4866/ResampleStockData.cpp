// ResampleStockData.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>

using std::cout;
using std::endl;
using std::string;

int main(int argc, const char* argv[])
{
    // process command line arguments;
    std::string directory;
    bool directorySpecified = false;
    for (int i = 0; i < argc; ++i) {
        const string parm(argv[i]);
        if (parm == "-d" || parm == "--directory") {
            if (directorySpecified)
            {
                cout << "***Error*** -d specified more than once." << endl;
                return -1;
            }
            directorySpecified = true;
            if (i < argc) {
                directory = argv[i + 1];
                cout << "directory = " << directory << endl;
                std::filesystem::path folder(directory);
                if (!std::filesystem::is_directory(folder)) {
                    cout << "***Error*** specified directory does not exist or is not actually a directory." << endl;
                    return - 1;
                }
            }
            else {
                cout << "***Error*** No directory specified after -d" << endl;
                return -1;
            }
        }
        else if (parm == "-i" || parm == "--interval") {
        }
        else if (parm == "-r" || parm == "--ratio") {
        }
        else {
            cout << "'" << parm << "' is not a recognized parameter. Try ResampleStockData.cpp -h" << endl;
        }
    }

    std::filesystem::path folder(directory);

    std::vector<std::string> file_list;
    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        const auto full_name = entry.path().string();

        if (entry.is_regular_file())
        {
            const auto base_name = entry.path().filename().string();
            file_list.push_back(full_name);
        }
    }

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
