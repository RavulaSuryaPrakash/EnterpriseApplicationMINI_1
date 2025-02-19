#include "data_classes.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <tuple>

// Constructor
CollisionRecord::CollisionRecord(int date, int time, int p_inj, int p_kill, int ped_inj, int ped_kill, 
                                 int cyc_inj, int cyc_kill, int mot_inj, int mot_kill)
    : crash_date(date), crash_time(time), persons_injured(p_inj), persons_killed(p_kill),
      pedestrians_injured(ped_inj), pedestrians_killed(ped_kill),
      cyclists_injured(cyc_inj), cyclists_killed(cyc_kill),
      motorists_injured(mot_inj), motorists_killed(mot_kill) {}

// Load data from CSV
void CollisionDataManager::loadFromCSV(const std::string &filename) {
    std::ifstream file(filename);
    std::cerr << "Started Loading Data Set ....."  << std::endl;
    if (!file.is_open()) {
        std::cerr << "[Error] Unable to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line);  

    int count = 0, failed = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        
        try {
            std::string dateStr, timeStr;
            int date, time, persons_inj, persons_kill, ped_inj, ped_kill, cyc_inj, cyc_kill, mot_inj, mot_kill;

            std::getline(ss, dateStr, ','); // CRASH DATE
            std::getline(ss, timeStr, ','); // CRASH TIME
            
            // 🛠 FIX: Convert Date Format MM/DD/YYYY → YYYYMMDD
            if (!dateStr.empty()) {
                // Remove '/' using erase-remove idiom
                dateStr.erase(std::remove(dateStr.begin(), dateStr.end(), '/'), dateStr.end());

                // Validate and convert MM/DD/YYYY to YYYYMMDD
                if (dateStr.size() == 8) {
                    date = std::stoi(dateStr.substr(4, 4) + dateStr.substr(0, 2) + dateStr.substr(2, 2));
                } else {
                    throw std::runtime_error("Invalid Date Format");
                }
            } else {
                throw std::runtime_error("Missing Date");
            }

            if (!timeStr.empty()) {
                int colonIndex = timeStr.find(':');
                if (colonIndex == std::string::npos || colonIndex > 2 || timeStr.size() < 3) {
                    throw std::runtime_error("Invalid Time Format");
                }

                // Ensure time is always in HH:MM format (e.g., "7:40" → "07:40")
                if (colonIndex == 1) {  // Single-digit hour case (H:MM)
                    timeStr = "0" + timeStr;  // Convert to HH:MM
                }

                // Remove ':' using erase-remove idiom
                timeStr.erase(std::remove(timeStr.begin(), timeStr.end(), ':'), timeStr.end());

                // Convert string to integer
                if (timeStr.size() == 4) {
                    time = std::stoi(timeStr);
                } else {
                    throw std::runtime_error("Invalid Time Format After Processing");
                }
            } else {
                throw std::runtime_error("Missing Time");
            }

            // Skip unnecessary fields until we reach "NUMBER OF PERSONS INJURED"
            for (int i = 0; i < 8; i++) std::getline(ss, token, ',');

            std::getline(ss, token, ','); persons_inj = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); persons_kill = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); ped_inj = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); ped_kill = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); cyc_inj = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); cyc_kill = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); mot_inj = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); mot_kill = token.empty() ? 0 : std::stoi(token);

            // Store the record
            data.emplace_back(date, time, persons_inj, persons_kill, ped_inj, ped_kill, cyc_inj, cyc_kill, mot_inj, mot_kill);
            count++;
        } catch (const std::exception &e) {
            //std::cerr << "[Warning] Skipped a row due to error: " << e.what() << std::endl;
            failed++;
        }
    }

    //std::cout << "Successfully loaded " << count << " records. Failed: " << failed << std::endl;
    file.close();
}

// Method 1: Total Injuries
int CollisionDataManager::getTotalInjuriesInRange(int startDate, int endDate) {
    int total = 0;
    for (const auto &record : data) {
        if (record.crash_date >= startDate && record.crash_date <= endDate) {
            total += record.persons_injured;
        }
    }
    return total;
}

// Method 2: Total Fatalities
int CollisionDataManager::getTotalFatalitiesInRange(int startDate, int endDate) {
    int total = 0;
    for (const auto &record : data) {
        if (record.crash_date >= startDate && record.crash_date <= endDate) {
            total += record.persons_killed;
        }
    }
    return total;
}

// Method 3: Most Severe Accidents
std::vector<CollisionRecord> CollisionDataManager::getMostSevereAccidents(int startDate, int endDate) {
    std::vector<CollisionRecord> severeAccidents;
    for (const auto &record : data) {
        if (record.crash_date >= startDate && record.crash_date <= endDate && (record.persons_injured > 5 || record.persons_killed > 1)) {
            severeAccidents.push_back(record);
        }
    }
    return severeAccidents;
}

// Method 4: Peak Accident Hour
std::pair<int, int> CollisionDataManager::getPeakAccidentHour(int startDate, int endDate) {
    std::unordered_map<int, int> hourCount;
    for (const auto &record : data) {
        if (record.crash_date >= startDate && record.crash_date <= endDate) {
            hourCount[record.crash_time / 100]++;
        }
    }
    return *std::max_element(hourCount.begin(), hourCount.end(), [](auto &a, auto &b) { return a.second < b.second; });
}
