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
        
        // Parse passenger_count
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

// Structure for grouping statistics per VendorID
struct VendorStats {
    long long trips = 0;
    long long totalPassengers = 0;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <queryName> <data_file>\n";
        cerr << "Example: " << argv[0] << " query3 taxi-trips-data.json\n";
        return EXIT_FAILURE;
    }
    
    string queryName = argv[1];
    string filename = argv[2];
    
    if (queryName != "query3") {
        cerr << "Error: Only query3 is implemented in this version.\n";
        return EXIT_FAILURE;
    }
    
    ifstream infile(filename);
    if (!infile) {
        cerr << "Error opening file: " << filename << "\n";
        return EXIT_FAILURE;
    }
    
    unordered_map<int, VendorStats> vendorGroups;
    string line;
    long long totalLines = 0;
    long long parseErrors = 0;
    
    // Process each line without storing all records in memory
    while (getline(infile, line)) {
        totalLines++;
        if (line.empty()) continue;
        
        TaxiTrip trip;
        if (parseJsonLine(line, trip)) {
            // Apply filtering: store_and_fwd_flag must be 'Y'
            // and pickup date must be between "2024-01-01" (inclusive) and "2024-02-01" (exclusive)
            if (trip.store_and_fwd_flag == 'Y') {
                string pickupDate = trip.tpep_pickup_datetime.substr(0, 10);
                if (pickupDate >= "2024-01-01" && pickupDate < "2024-02-01") {
                    int vendor = trip.VendorID;
                    vendorGroups[vendor].trips++;
                    vendorGroups[vendor].totalPassengers += trip.passenger_count;
                }
            }
        } else {
            parseErrors++;
        }
    }
    infile.close();
    
    // Output results (for now, simple output)
    cout << "VendorID  Trips  Avg_Passengers\n";
    for (const auto &entry : vendorGroups) {
        int vendor = entry.first;
        const VendorStats &stats = entry.second;
        double avgPassengers = (stats.trips > 0) ? static_cast<double>(stats.totalPassengers) / stats.trips : 0.0;
        cout << vendor << "  " << stats.trips << "  " << avgPassengers << "\n";
    }
    
    // Optionally print parse statistics:
    cout << "\nTotal lines processed: " << totalLines << "\n";
    cout << "Total parse errors: " << parseErrors << "\n";
    
    return EXIT_SUCCESS;
}
