#include "data_classes.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <omp.h>

// Constructor for CollisionRecord.
CollisionRecord::CollisionRecord(int date, int time, int p_inj, int p_kill, int ped_inj, int ped_kill, 
                                 int cyc_inj, int cyc_kill, int mot_inj, int mot_kill)
    : crash_date(date), crash_time(time), persons_injured(p_inj), persons_killed(p_kill),
      pedestrians_injured(ped_inj), pedestrians_killed(ped_kill),
      cyclists_injured(cyc_inj), cyclists_killed(cyc_kill),
      motorists_injured(mot_inj), motorists_killed(mot_kill) {}

// Load data from a CSV file.
void CollisionDataManager::loadFromCSV(const std::string &filename) {
    std::ifstream file(filename);
    std::cerr << "Started Loading Data Set ....." << std::endl;
    if (!file.is_open()) {
        std::cerr << "[Error] Unable to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line);  // Skip header line

    int count = 0, failed = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        try {
            std::string dateStr, timeStr;
            int date, time, persons_inj, persons_kill, ped_inj, ped_kill, cyc_inj, cyc_kill, mot_inj, mot_kill;
            
            // Read crash date and time.
            std::getline(ss, dateStr, ','); // CRASH DATE
            std::getline(ss, timeStr, ','); // CRASH TIME

            // Convert date from MM/DD/YYYY to YYYYMMDD.
            if (!dateStr.empty()) {
                // Remove '/'
                dateStr.erase(std::remove(dateStr.begin(), dateStr.end(), '/'), dateStr.end());
                if (dateStr.size() == 8) {
                    // Rearrange to YYYYMMDD.
                    date = std::stoi(dateStr.substr(4, 4) + dateStr.substr(0, 2) + dateStr.substr(2, 2));
                } else {
                    throw std::runtime_error("Invalid Date Format");
                }
            } else {
                throw std::runtime_error("Missing Date");
            }

            // Process time string to ensure HHMM format.
            if (!timeStr.empty()) {
                int colonIndex = timeStr.find(':');
                if (colonIndex == std::string::npos || timeStr.size() < 3) {
                    throw std::runtime_error("Invalid Time Format");
                }
                if (colonIndex == 1) {  // For example, "7:40" → "07:40"
                    timeStr = "0" + timeStr;
                }
                timeStr.erase(std::remove(timeStr.begin(), timeStr.end(), ':'), timeStr.end());
                if (timeStr.size() == 4) {
                    time = std::stoi(timeStr);
                } else {
                    throw std::runtime_error("Invalid Time Format After Processing");
                }
            } else {
                throw std::runtime_error("Missing Time");
            }

            // Skip the next 8 fields (unused columns).
            for (int i = 0; i < 8; i++) {
                std::getline(ss, token, ',');
            }

            // Parse the injury and fatality data.
            std::getline(ss, token, ','); persons_inj = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); persons_kill = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); ped_inj = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); ped_kill = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); cyc_inj = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); cyc_kill = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); mot_inj = token.empty() ? 0 : std::stoi(token);
            std::getline(ss, token, ','); mot_kill = token.empty() ? 0 : std::stoi(token);

            // Add the record to the data vector.
            data.emplace_back(date, time, persons_inj, persons_kill, ped_inj, ped_kill, 
                              cyc_inj, cyc_kill, mot_inj, mot_kill);
            count++;
        } catch (const std::exception &e) {
            failed++;
        }
    }
    file.close();
    std::cout << "Successfully loaded " << count << " records. Failed: " << failed << std::endl;
}

// -----------------------------------------------------------------
// OpenMP-parallelized query methods
// -----------------------------------------------------------------

// getTotalInjuriesInRange: Sum persons_injured using OpenMP reduction.
int CollisionDataManager::getTotalInjuriesInRange(int startDate, int endDate) {
    int total = 0;
    #pragma omp parallel for reduction(+:total)
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i].crash_date >= startDate && data[i].crash_date <= endDate) {
            total += data[i].persons_injured;
        }
    }
    return total;
}

// getTotalFatalitiesInRange: Sum persons_killed using OpenMP reduction.
int CollisionDataManager::getTotalFatalitiesInRange(int startDate, int endDate) {
    int total = 0;
    #pragma omp parallel for reduction(+:total)
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i].crash_date >= startDate && data[i].crash_date <= endDate) {
            total += data[i].persons_killed;
        }
    }
    return total;
}

// getMostSevereAccidents: Collect records with high injury or fatality counts.
std::vector<CollisionRecord> CollisionDataManager::getMostSevereAccidents(int startDate, int endDate) {
    std::vector<CollisionRecord> severeAccidents;

    #pragma omp parallel
    {
        std::vector<CollisionRecord> localSevere;
        #pragma omp for nowait
        for (size_t i = 0; i < data.size(); i++) {
            if (data[i].crash_date >= startDate && data[i].crash_date <= endDate &&
               (data[i].persons_injured > 5 || data[i].persons_killed > 1)) {
                localSevere.push_back(data[i]);
            }
        }
        #pragma omp critical
        {
            severeAccidents.insert(severeAccidents.end(), localSevere.begin(), localSevere.end());
        }
    }
    return severeAccidents;
}

// getPeakAccidentHour: Determine the hour with the most accidents.
std::pair<int, int> CollisionDataManager::getPeakAccidentHour(int startDate, int endDate) {
    const int NUM_HOURS = 24;
    int hourCount[NUM_HOURS] = {0};

    #pragma omp parallel for
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i].crash_date >= startDate && data[i].crash_date <= endDate) {
            int hour = data[i].crash_time / 100;
            if (hour >= 0 && hour < NUM_HOURS) {
                #pragma omp atomic
                hourCount[hour]++;
            }
        }
    }

    int maxHour = 0, maxCount = 0;
    for (int i = 0; i < NUM_HOURS; i++) {
        if (hourCount[i] > maxCount) {
            maxCount = hourCount[i];
            maxHour = i;
        }
    }
    return std::make_pair(maxHour, maxCount);
}
