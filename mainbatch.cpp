#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cstdlib>
#include <chrono>

using namespace std;
using namespace std::chrono;

// --- Data Structures --- //

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

// --- JSON Parsing --- //

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
// Strings: empty string.
// Numbers: 0 (or 0.0).
// store_and_fwd_flag: 'N'
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
    
    return true;
}

// --- Aggregation Structures --- //

// Query 1: Overall record count (just a counter).

// Query 2: Group by payment_type.
struct GroupStats {
    long long count = 0;
    double totalFare = 0.0;
    double totalTip = 0.0;
};

// Query 3: Group by VendorID for store-and-fwd trips in Jan 2024.
struct VendorStats {
    long long trips = 0;
    long long totalPassengers = 0;
};

// Query 4: Daily statistics for January 2024.
struct DailyStats {
    long long total_trips = 0;
    long long totalPassengers = 0;
    double totalDistance = 0.0;
    double totalFare = 0.0;
    double totalTip = 0.0;
};

// --- Batch Processing Function --- //

void processBatch(const string &batch,
                  long long &totalTripCount,
                  unordered_map<int, GroupStats> &groups,
                  unordered_map<int, VendorStats> &vendorGroups,
                  unordered_map<string, DailyStats> &dailyStatsMap,
                  long long &successCount)
{
    // Split the batch string into lines.
    size_t pos = 0, newlinePos;
    string line, pickupDate;
    TaxiTrip trip;
    while ((newlinePos = batch.find('\n', pos)) != string::npos) {
        line = batch.substr(pos, newlinePos - pos);
        pos = newlinePos + 1;
        if(line.empty()) continue;
        if(parseJsonLine(line, trip)) {
            successCount++;
            // Query 1: overall count.
            if(!trip.tpep_pickup_datetime.empty())
                totalTripCount++;
            // Query 2: trips with trip_distance > 5 grouped by payment_type.
            if(trip.trip_distance > 5.0) {
                int payment = trip.payment_type;
                groups[payment].count++;
                groups[payment].totalFare += trip.fare_amount;
                groups[payment].totalTip += trip.tip_amount;
            }
            // Query 3: store_and_fwd_flag 'Y' and pickup date in Jan 2024, group by VendorID.
            if(trip.store_and_fwd_flag == 'Y' && trip.tpep_pickup_datetime.size() >= 10) {
                pickupDate = trip.tpep_pickup_datetime.substr(0, 10);
                if(pickupDate >= "2024-01-01" && pickupDate < "2024-02-01") {
                    int vendor = trip.VendorID;
                    vendorGroups[vendor].trips++;
                    vendorGroups[vendor].totalPassengers += trip.passenger_count;
                }
            }
            // Query 4: For records with pickup date in Jan 2024, group by day.
            if(trip.tpep_pickup_datetime.size() >= 10) {
                pickupDate = trip.tpep_pickup_datetime.substr(0, 10);
                if(pickupDate >= "2024-01-01" && pickupDate < "2024-02-01") {
                    DailyStats &stats = dailyStatsMap[pickupDate];
                    stats.total_trips++;
                    stats.totalPassengers += trip.passenger_count;
                    stats.totalDistance += trip.trip_distance;
                    stats.totalFare += trip.fare_amount;
                    stats.totalTip += trip.tip_amount;
                }
            }
        }
    }
}

// --- Main --- //

int main(int argc, char* argv[]) {
    if(argc < 3) {
        cerr << "Usage: " << argv[0] << " <queryName> <data_file>\n";
        cerr << "Allowed queryNames: query1, query2, query3, query4\n";
        return EXIT_FAILURE;
    }
    
    string queryName = argv[1];
    string filename = argv[2];
    
    if(queryName != "query1" && queryName != "query2" && queryName != "query3" && queryName != "query4") {
        cerr << "Error: Invalid queryName. Allowed: query1, query2, query3, query4\n";
        return EXIT_FAILURE;
    }
    
    ifstream infile(filename, ios::in | ios::binary);
    if(!infile) {
        cerr << "Error opening file: " << filename << "\n";
        return EXIT_FAILURE;
    }
    
    // Aggregation variables.
    long long totalTripCount = 0;                           // Query 1.
    unordered_map<int, GroupStats> groups;                  // Query 2.
    unordered_map<int, VendorStats> vendorGroups;           // Query 3.
    unordered_map<string, DailyStats> dailyStatsMap;        // Query 4.
    
    // Counters for logging.
    long long totalLines = 0, parseErrors = 0, successCount = 0;
    
    // Batch processing: Read the file in fixed-size blocks.
    const size_t BUFFER_SIZE = 1 << 16; // 64KB
    char buffer[BUFFER_SIZE];
    string leftover = "";
    
    // Start timer.
    auto startTime = high_resolution_clock::now();
    
    while(!infile.eof()) {
        infile.read(buffer, BUFFER_SIZE);
        size_t bytesRead = infile.gcount();
        if(bytesRead == 0) break;
        // Append the block to leftover from previous iteration.
        string block = leftover + string(buffer, bytesRead);
        size_t pos = 0;
        size_t newlinePos = 0;
        while((newlinePos = block.find('\n', pos)) != string::npos) {
            string line = block.substr(pos, newlinePos - pos);
            pos = newlinePos + 1;
            totalLines++;
            // Process each line in the batch.
            TaxiTrip trip;
            if(parseJsonLine(line, trip)) {
                successCount++;
                // Query 1: overall count.
                if(!trip.tpep_pickup_datetime.empty())
                    totalTripCount++;
                // Query 2: trip_distance > 5 grouped by payment_type.
                if(trip.trip_distance > 5.0) {
                    int payment = trip.payment_type;
                    groups[payment].count++;
                    groups[payment].totalFare += trip.fare_amount;
                    groups[payment].totalTip += trip.tip_amount;
                }
                // Query 3: store_and_fwd_flag 'Y' and pickup date in Jan 2024, group by VendorID.
                if(trip.store_and_fwd_flag == 'Y' && trip.tpep_pickup_datetime.size() >= 10) {
                    string pickupDate = trip.tpep_pickup_datetime.substr(0, 10);
                    if(pickupDate >= "2024-01-01" && pickupDate < "2024-02-01") {
                        int vendor = trip.VendorID;
                        vendorGroups[vendor].trips++;
                        vendorGroups[vendor].totalPassengers += trip.passenger_count;
                    }
                }
                // Query 4: Group by day for January 2024.
                if(trip.tpep_pickup_datetime.size() >= 10) {
                    string pickupDate = trip.tpep_pickup_datetime.substr(0, 10);
                    if(pickupDate >= "2024-01-01" && pickupDate < "2024-02-01") {
                        DailyStats &stats = dailyStatsMap[pickupDate];
                        stats.total_trips++;
                        stats.totalPassengers += trip.passenger_count;
                        stats.totalDistance += trip.trip_distance;
                        stats.totalFare += trip.fare_amount;
                        stats.totalTip += trip.tip_amount;
                    }
                }
            } else {
                parseErrors++;
            }
        }
        // Any remaining data after the last newline is saved as leftover.
        leftover = block.substr(pos);
    }
    // Process any leftover as one last line.
    if(!leftover.empty()) {
        totalLines++;
        TaxiTrip trip;
        if(parseJsonLine(leftover, trip)) {
            successCount++;
            if(!trip.tpep_pickup_datetime.empty())
                totalTripCount++;
            if(trip.trip_distance > 5.0) {
                int payment = trip.payment_type;
                groups[payment].count++;
                groups[payment].totalFare += trip.fare_amount;
                groups[payment].totalTip += trip.tip_amount;
            }
            if(trip.store_and_fwd_flag == 'Y' && trip.tpep_pickup_datetime.size() >= 10) {
                string pickupDate = trip.tpep_pickup_datetime.substr(0, 10);
                if(pickupDate >= "2024-01-01" && pickupDate < "2024-02-01") {
                    int vendor = trip.VendorID;
                    vendorGroups[vendor].trips++;
                    vendorGroups[vendor].totalPassengers += trip.passenger_count;
                }
            }
            if(trip.tpep_pickup_datetime.size() >= 10) {
                string pickupDate = trip.tpep_pickup_datetime.substr(0, 10);
                if(pickupDate >= "2024-01-01" && pickupDate < "2024-02-01") {
                    DailyStats &stats = dailyStatsMap[pickupDate];
                    stats.total_trips++;
                    stats.totalPassengers += trip.passenger_count;
                    stats.totalDistance += trip.trip_distance;
                    stats.totalFare += trip.fare_amount;
                    stats.totalTip += trip.tip_amount;
                }
            }
        } else {
            parseErrors++;
        }
    }
    
    // Stop timer.
    auto endTime = high_resolution_clock::now();
    duration<double> elapsed = endTime - startTime;
    
    // Log parsing statistics and elapsed time.
    cout << "Total lines processed: " << totalLines << "\n";
    cout << "Successfully parsed: " << successCount << "\n";
    cout << "Total parse errors: " << parseErrors << "\n";
    cout << "Elapsed time: " << elapsed.count() << " seconds\n\n";
    
    // Dispatch output based on queryName.
    if(queryName == "query1") {
        // Query 1: Overall record count.
        cout << "Total_trips: " << totalTripCount << "\n";
    }
    else if(queryName == "query2") {
        // Query 2: Payment type aggregation.
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
    }
    else if(queryName == "query3") {
        // Query 3: Vendor grouping.
        cout << "VendorID\tTrips\tAvg_Passengers\n";
        for(const auto &entry : vendorGroups) {
            int vendor = entry.first;
            const VendorStats &stats = entry.second;
            double avgPassengers = (stats.trips > 0) ? static_cast<double>(stats.totalPassengers) / stats.trips : 0.0;
            cout << vendor << "\t" 
                 << stats.trips << "\t" 
                 << fixed << setprecision(15) << avgPassengers << "\n";
        }
    }
    else if(queryName == "query4") {
        // Query 4: Daily statistics.
        vector<string> dates;
        for(const auto &entry : dailyStatsMap)
            dates.push_back(entry.first);
        sort(dates.begin(), dates.end());
        
        cout << "Trip_date\tTotal_trips\tAvg_passengers\tAvg_distance\tAvg_fare\tTotal_tip\n";
        for(const string &date : dates) {
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
    }
    
    return EXIT_SUCCESS;
}
