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
    if (keyPos == string::npos) return "";
    size_t colonPos = line.find(":", keyPos + searchKey.length());
    if (colonPos == string::npos) return "";
    size_t valueStart = line.find_first_not_of(" \t", colonPos + 1);
    if (valueStart == string::npos) return "";
    if (line[valueStart] == '"') { // Value is quoted
        size_t valueEnd = line.find("\"", valueStart + 1);
        if (valueEnd == string::npos) return "";
        return line.substr(valueStart + 1, valueEnd - valueStart - 1);
    } else {
        size_t valueEnd = line.find_first_of(",}", valueStart);
        return line.substr(valueStart, valueEnd - valueStart);
    }
}

// Improved JSON parser that never drops a row.
// It assigns default values (0 for numbers, empty for strings, 'N' for the flag)
// if a key is missing or its value is "null".
// It always returns true (as long as at least tpep_pickup_datetime is present).
bool parseJsonLine(const string &line, TaxiTrip &trip) {
    // tpep_pickup_datetime is required; if missing, we return an empty string.
    string pickup_dt = getJsonValue(line, "tpep_pickup_datetime");
    trip.tpep_pickup_datetime = (pickup_dt.empty() || pickup_dt == "null") ? "" : pickup_dt;
    
    // For other fields, assign defaults if missing.
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
    
    // Even if some fields are missing, we return true (so we don't drop the record).
    return true;
}

// Structure to accumulate statistics per VendorID.
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
    long long successCount = 0;
    
    // Process each line.
    while (getline(infile, line)) {
        totalLines++;
        if (line.empty())
            continue;
        
        TaxiTrip trip;
        // Parse the JSON row (always returns true, with defaults if needed).
        if (parseJsonLine(line, trip)) {
            successCount++;
            // Filter: only consider records with store_and_fwd_flag 'Y'
            // and a pickup date in January 2024.
            if (trip.store_and_fwd_flag == 'Y') {
                // We assume tpep_pickup_datetime is in "YYYY-MM-DD HH:MM:SS" format.
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
    
    // Log parsing statistics.
    cout << "Total lines processed: " << totalLines << "\n";
    cout << "Successfully parsed: " << successCount << "\n";
    cout << "Total parse errors: " << parseErrors << "\n";
    
    // Output results.
    cout << "VendorID\tTrips\tAvg_Passengers\n";
    for (const auto &entry : vendorGroups) {
        int vendor = entry.first;
        const VendorStats &stats = entry.second;
        double avgPassengers = (stats.trips > 0) ? static_cast<double>(stats.totalPassengers) / stats.trips : 0.0;
        cout << vendor << "\t" << stats.trips << "\t" << fixed << setprecision(15) << avgPassengers << "\n";
    }
    
    return EXIT_SUCCESS;
}
