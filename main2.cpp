#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <cstdlib>

using namespace std;

// Structure representing a taxi trip record.
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

// Helper function to extract a value (as a string) from a JSON line given a key.
// Returns an empty string if the key is not found.
string getJsonValue(const string &line, const string &key) {
    string searchKey = "\"" + key + "\"";
    size_t keyPos = line.find(searchKey);
    if(keyPos == string::npos) return "";
    size_t colonPos = line.find(":", keyPos + searchKey.length());
    if(colonPos == string::npos) return "";
    size_t valueStart = line.find_first_not_of(" \t", colonPos + 1);
    if(valueStart == string::npos) return "";
    if(line[valueStart] == '"') { // Value is quoted.
        size_t valueEnd = line.find("\"", valueStart + 1);
        if(valueEnd == string::npos) return "";
        return line.substr(valueStart + 1, valueEnd - valueStart - 1);
    } else {
        size_t valueEnd = line.find_first_of(",}", valueStart);
        return line.substr(valueStart, valueEnd - valueStart);
    }
}

// Improved JSON parser that never drops a row.
// Missing or "null" values are replaced with default values:
//   - For strings: empty string.
//   - For numbers: 0 (or 0.0).
//   - For store_and_fwd_flag: 'N'
bool parseJsonLine(const string &line, TaxiTrip &trip) {
    // tpep_pickup_datetime is required; if missing, assign empty.
    string pickup_dt = getJsonValue(line, "tpep_pickup_datetime");
    trip.tpep_pickup_datetime = (pickup_dt.empty() || pickup_dt == "null") ? "" : pickup_dt;
    
    // Other fields.
    string dropoff_dt = getJsonValue(line, "tpep_dropoff_datetime");
    trip.tpep_dropoff_datetime = (dropoff_dt.empty() || dropoff_dt == "null") ? "" : dropoff_dt;
    
    string vendorStr = getJsonValue(line, "VendorID");
    trip.VendorID = (vendorStr.empty() || vendorStr == "null") ? 0 : stoi(vendorStr);
    
    string passengerStr = getJsonValue(line, "passenger_count");
    trip.passenger_count = (passengerStr.empty() || passengerStr == "null") ? 0 : stoi(passengerStr);
    
    string distanceStr = getJsonValue(line, "trip_distance");
    trip.trip_distance = (distanceStr.empty() || distanceStr == "null") ? 0.0 : stod(distanceStr);
    
    string paymentStr = getJsonValue(line, "payment_type");
    trip.payment_type = (paymentStr.empty() || paymentStr == "null") ? 0 : stoi(paymentStr);
    
    string fareStr = getJsonValue(line, "fare_amount");
    trip.fare_amount = (fareStr.empty() || fareStr == "null") ? 0.0 : stod(fareStr);
    
    string tipStr = getJsonValue(line, "tip_amount");
    trip.tip_amount = (tipStr.empty() || tipStr == "null") ? 0.0 : stod(tipStr);
    
    string flagStr = getJsonValue(line, "store_and_fwd_flag");
    trip.store_and_fwd_flag = (flagStr.empty() || flagStr == "null") ? 'N' : flagStr[0];
    
    // Always return true, so we never drop a record.
    return true;
}

// Structure for grouping statistics per payment_type.
struct GroupStats {
    long long count = 0;
    double totalFare = 0.0;
    double totalTip = 0.0;
};

int main(int argc, char* argv[]) {
    if(argc < 3) {
        cerr << "Usage: " << argv[0] << " <queryName> <data_file>\n";
        cerr << "Example: " << argv[0] << " query2 taxi-trips-data.json\n";
        return EXIT_FAILURE;
    }
    
    string queryName = argv[1];
    string filename = argv[2];
    
    if(queryName != "query2") {
        cerr << "Error: Only query2 is implemented in this version.\n";
        return EXIT_FAILURE;
    }
    
    ifstream infile(filename);
    if(!infile) {
        cerr << "Error opening file: " << filename << "\n";
        return EXIT_FAILURE;
    }
    
    unordered_map<int, GroupStats> groups;
    string line;
    long long totalLines = 0;
    long long parseErrors = 0;
    long long successCount = 0;
    
    // Process each line without storing all records in memory.
    while(getline(infile, line)) {
        totalLines++;
        if(line.empty()) continue;
        
        TaxiTrip trip;
        if(parseJsonLine(line, trip)) {
            successCount++;
            // Filter: only include trips with trip_distance > 5 miles.
            if(trip.trip_distance > 5.0) {
                int payment = trip.payment_type;
                groups[payment].count++;
                groups[payment].totalFare += trip.fare_amount;
                groups[payment].totalTip += trip.tip_amount;
            }
        } else {
            parseErrors++;
        }
    }
    infile.close();
    
    // Log parsing statistics.
    cout << "Total lines processed: " << totalLines << "\n";
    cout << "Successfully parsed: " << successCount << "\n";
    cout << "Total parse errors: " << parseErrors << "\n";
    
    // Output aggregated results.
    cout << "Payment_type\tNum_trips\tAvg_fare\tTotal_tip\n";
    for(const auto &entry : groups) {
        int payment = entry.first;
        const GroupStats &stats = entry.second;
        double avgFare = (stats.count > 0) ? stats.totalFare / stats.count : 0.0;
        cout << payment << "\t" 
             << stats.count << "\t" 
             << fixed << setprecision(15) << avgFare << "\t" 
             << fixed << setprecision(15) << stats.totalTip << "\n";
    }
    
    return EXIT_SUCCESS;
}
