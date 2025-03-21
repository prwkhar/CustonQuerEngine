#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <iomanip>

using namespace std;

bool parseJsonLine(const string &line) {
    return (line.find("\"tpep_pickup_datetime\"") != string::npos);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <queryName> <data_file>\n";
        cerr << "Example: " << argv[0] << " query1 taxi-trips-data.json\n";
        return EXIT_FAILURE;
    }
    
    string queryName = argv[1];
    string filename = argv[2];
    if (queryName != "query1") {
        cerr << "Error: Only query1 (total trips count) is implemented in this version.\n";
        return EXIT_FAILURE;
    }

    ifstream infile(filename);
    if (!infile) {
        cerr << "Error opening file: " << filename << "\n";
        return EXIT_FAILURE;
    }

    long long total_trips = 0;
    string line;
    while (getline(infile, line)) {
        if (!line.empty() && parseJsonLine(line)) {
            total_trips++;
        }
    }
    infile.close();

    cout << "totatripscount-" << total_trips <<  "\n";

    return EXIT_SUCCESS;
}
