#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <cstdlib>

using namespace std;

// Structure representing a taxi trip record
struct TaxiTrip {
    string tpep_pickup_datetime;
    string tpep_dropoff_datetime;
    int VendorID;
    int passenger_count;
    double trip_distance;
    int payment_type;
    double fare_amount;
    double tip_amount;
    char store_and_fwd_flag;
};

// Custom JSON parser for one line (assumes fixed keys and a simple format)
bool parseJsonLine(const string &line, TaxiTrip &trip) {
    try {
        size_t pos, start, end;
        
        // Parse tpep_pickup_datetime
        pos = line.find("\"tpep_pickup_datetime\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find("\"", pos);
        end = line.find("\"", start + 1);
        trip.tpep_pickup_datetime = line.substr(start + 1, end - start - 1);
        
        // Parse tpep_dropoff_datetime
        pos = line.find("\"tpep_dropoff_datetime\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find("\"", pos);
        end = line.find("\"", start + 1);
        trip.tpep_dropoff_datetime = line.substr(start + 1, end - start - 1);
        
        // Parse VendorID
        pos = line.find("\"VendorID\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find_first_not_of(" ", pos + 1);
        end = line.find_first_of(",}", start);
        trip.VendorID = stoi(line.substr(start, end - start));
        
        // Parse passenger_count (note the key is all lowercase)
        pos = line.find("\"passenger_count\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find_first_not_of(" ", pos + 1);
        end = line.find_first_of(",}", start);
        trip.passenger_count = stoi(line.substr(start, end - start));
        
        // Parse trip_distance
        pos = line.find("\"trip_distance\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find_first_not_of(" ", pos + 1);
        end = line.find_first_of(",}", start);
        trip.trip_distance = stod(line.substr(start, end - start));
        
        // Parse payment_type
        pos = line.find("\"payment_type\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find_first_not_of(" ", pos + 1);
        end = line.find_first_of(",}", start);
        trip.payment_type = stoi(line.substr(start, end - start));
        
        // Parse fare_amount
        pos = line.find("\"fare_amount\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find_first_not_of(" ", pos + 1);
        end = line.find_first_of(",}", start);
        trip.fare_amount = stod(line.substr(start, end - start));
        
        // Parse tip_amount
        pos = line.find("\"tip_amount\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find_first_not_of(" ", pos + 1);
        end = line.find_first_of(",}", start);
        trip.tip_amount = stod(line.substr(start, end - start));
        
        // Parse store_and_fwd_flag
        pos = line.find("\"store_and_fwd_flag\"");
        if (pos == string::npos) return false;
        pos = line.find(":", pos);
        start = line.find("\"", pos);
        end = line.find("\"", start + 1);
        string flag = line.substr(start + 1, end - start - 1);
        trip.store_and_fwd_flag = (!flag.empty()) ? flag[0] : 'N';
        
        return true;
    } catch (...) {
        return false;
    }
}

// Structure for grouping statistics per payment_type
struct GroupStats {
    long long count = 0;
    double totalFare = 0.0;
    double totalTip = 0.0;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <queryName> <data_file>\n";
        cerr << "Example: " << argv[0] << " query2 taxi-trips-data.json\n";
        return EXIT_FAILURE;
    }
    
    string queryName = argv[1];
    string filename = argv[2];

    if (queryName != "query2") {
        cerr << "Error: Only query2 is implemented in this version.\n";
        return EXIT_FAILURE;
    }
    
    ifstream infile(filename);
    if (!infile) {
        cerr << "Error opening file: " << filename << "\n";
        return EXIT_FAILURE;
    }
    
    unordered_map<int, GroupStats> groups;
    string line;
    long long totalLines = 0;
    long long errorCount = 0;
    
    // Process each line without storing all records in memory
    while (getline(infile, line)) {
        totalLines++;
        if (line.empty())
            continue;
        
        TaxiTrip trip;
        if (parseJsonLine(line, trip)) {
            // Filter: only include trips with trip_distance > 5 miles
            if (trip.trip_distance > 5.0) {
                int payment = trip.payment_type;
                groups[payment].count++;
                groups[payment].totalFare += trip.fare_amount;
                groups[payment].totalTip += trip.tip_amount;
            }
        } else {
            errorCount++;
        }
    }
    infile.close();
    
    
    // Output each group's statistics
    for (const auto &entry : groups) {
        int payment = entry.first;
        const GroupStats &stats = entry.second;
        double avgFare = (stats.count > 0) ? stats.totalFare / stats.count : 0.0;
        cout << payment << " ";
        cout << stats.count << " ";
        cout << avgFare <<" ";
        cout << stats.totalTip <<endl;
    }
    
    // Optional: Report parsing statistics
    cout << "\nTotal lines processed: " << totalLines << "\n";
    cout << "Total parse errors: " << errorCount << "\n";
    
    return EXIT_SUCCESS;
}
