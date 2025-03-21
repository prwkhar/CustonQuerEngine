#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
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

// Helper: Extracts a value (as a string) from a JSON line given a key.
// Returns empty string if key is not found.
string getJsonValue(const string &line, const string &key) {
    string searchKey = "\"" + key + "\"";
    size_t keyPos = line.find(searchKey);
    if (keyPos == string::npos) return "";
    size_t colonPos = line.find(":", keyPos + searchKey.length());
    if (colonPos == string::npos) return "";
    size_t valueStart = line.find_first_not_of(" \t", colonPos + 1);
    if (valueStart == string::npos) return "";
    if (line[valueStart] == '"') { // quoted string
        size_t valueEnd = line.find("\"", valueStart + 1);
        if (valueEnd == string::npos) return "";
        return line.substr(valueStart + 1, valueEnd - valueStart - 1);
    } else {
        size_t valueEnd = line.find_first_of(",}", valueStart);
        return line.substr(valueStart, valueEnd - valueStart);
    }
}

// Improved JSON parser that never drops a record because of missing data.
// It uses default values (0 for numbers, empty for strings, 'N' for the flag)
// for any missing fields. It returns false only if tpep_pickup_datetime is missing.
bool parseJsonLine(const string &line, TaxiTrip &trip) {
    // For each field, try to extract the value. If not found or empty, use a default.
    
    // tpep_pickup_datetime is required.
    string pickup_dt = getJsonValue(line, "tpep_pickup_datetime");
    if (pickup_dt.empty() || pickup_dt == "null") {
        return false; // Cannot process a record without a pickup datetime.
    }
    trip.tpep_pickup_datetime = pickup_dt;
    
    // Optional string field: tpep_dropoff_datetime
    string dropoff_dt = getJsonValue(line, "tpep_dropoff_datetime");
    trip.tpep_dropoff_datetime = (dropoff_dt.empty() || dropoff_dt == "null") ? "" : dropoff_dt;
    
    // Optional numeric fields: use 0 if missing.
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
    
    // For flag, default to 'N'
    string flagStr = getJsonValue(line, "store_and_fwd_flag");
    trip.store_and_fwd_flag = (flagStr.empty() || flagStr == "null") ? 'N' : flagStr[0];
    
    return true;
}

// Structure to accumulate daily statistics.
struct DailyStats {
    long long total_trips = 0;
    long long totalPassengers = 0;
    double totalDistance = 0.0;
    double totalFare = 0.0;
    double totalTip = 0.0;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <queryName> <data_file>\n";
        cerr << "Example: " << argv[0] << " query4 taxi-trips-data.json\n";
        return EXIT_FAILURE;
    }
    
    string queryName = argv[1];
    string filename = argv[2];
    
    if (queryName != "query4") {
        cerr << "Error: Only query4 is implemented in this version.\n";
        return EXIT_FAILURE;
    }
    
    ifstream infile(filename);
    if (!infile) {
        cerr << "Error opening file: " << filename << "\n";
        return EXIT_FAILURE;
    }
    
    unordered_map<string, DailyStats> dailyStatsMap;
    string line;
    long long totalLines = 0;
    long long parseErrors = 0;
    long long successCount = 0;
    
    // Process file line by line.
    while (getline(infile, line)) {
        totalLines++;
        if (line.empty())
            continue;
        
        TaxiTrip trip;
        if (parseJsonLine(line, trip)) {
            successCount++;
            // Extract the date part from tpep_pickup_datetime (first 10 characters).
            string pickupDate = trip.tpep_pickup_datetime.substr(0, 10);
            // Filter: only consider trips in January 2024.
            if (pickupDate >= "2024-01-01" && pickupDate < "2024-02-01") {
                DailyStats &stats = dailyStatsMap[pickupDate];
                stats.total_trips++;
                stats.totalPassengers += trip.passenger_count;
                stats.totalDistance += trip.trip_distance;
                stats.totalFare += trip.fare_amount;
                stats.totalTip += trip.tip_amount;
            }
        } else {
            parseErrors++;
        }
    }
    infile.close();
    
    // Log overall parsing statistics.
    cout << "Total lines processed: " << totalLines << "\n";
    cout << "Successfully parsed: " << successCount << "\n";
    cout << "Total parse errors: " << parseErrors << "\n";
    
    // Sort dates in ascending order.
    vector<string> dates;
    for (const auto &entry : dailyStatsMap) {
        dates.push_back(entry.first);
    }
    sort(dates.begin(), dates.end());
    
    // Output the results.
    cout << "Trip_date\tTotal_trips\tAvg_passengers\tAvg_distance\tAvg_fare\tTotal_tip\n";
    for (const string &date : dates) {
        const DailyStats &stats = dailyStatsMap[date];
        double avgPassengers = (stats.total_trips > 0) ? static_cast<double>(stats.totalPassengers) / stats.total_trips : 0.0;
        double avgDistance   = (stats.total_trips > 0) ? stats.totalDistance / stats.total_trips : 0.0;
        double avgFare       = (stats.total_trips > 0) ? stats.totalFare / stats.total_trips : 0.0;
        cout << date << "\t" 
             << stats.total_trips << "\t"
             << fixed << setprecision(15) << avgPassengers << "\t"
             << fixed << setprecision(15) << avgDistance << "\t"
             << fixed << setprecision(15) << avgFare << "\t"
             << fixed << setprecision(15) << stats.totalTip << "\n";
    }
    
    return EXIT_SUCCESS;
}
