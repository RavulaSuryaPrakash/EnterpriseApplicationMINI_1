#include "data_classes.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <omp.h>
#include <thread>
#include <mutex>

// Constructor for CollisionRecord.
CollisionRecord::CollisionRecord(int date, int time, int p_inj, int p_kill, int ped_inj, int ped_kill,
	int cyc_inj, int cyc_kill, int mot_inj, int mot_kill)
	: crash_date(date), crash_time(time), persons_injured(p_inj), persons_killed(p_kill),
	pedestrians_injured(ped_inj), pedestrians_killed(ped_kill),
	cyclists_injured(cyc_inj), cyclists_killed(cyc_kill),
	motorists_injured(mot_inj), motorists_killed(mot_kill) {
}

std::mutex data_mutex;  // Mutex for safe access to `data`

// Load data from a CSV file.
void CollisionDataManager::loadFromCSV(const std::string &filename) {
    std::ifstream file(filename);
    std::cerr << "Started Loading Data Set ....."  << std::endl;
    if (!file.is_open()) {
        std::cerr << "[Error] Unable to open file: " << filename << std::endl;
        return;
    }

    std::vector<std::thread> threads;
    std::vector<CollisionRecord> local_data;  // Local vector for each thread

    std::string line;
    std::getline(file, line);  // Skip header

    auto worker = [&](const std::vector<std::string> &lines) {
        std::vector<CollisionRecord> local_records;
        for (const std::string &line : lines) {
            std::stringstream ss(line);
            std::string token;
            
            try {
                std::string dateStr, timeStr;
                int date, time, persons_inj, persons_kill, ped_inj, ped_kill, cyc_inj, cyc_kill, mot_inj, mot_kill;

                std::getline(ss, dateStr, ','); 
                std::getline(ss, timeStr, ','); 

                if (!dateStr.empty()) {
                    dateStr.erase(std::remove(dateStr.begin(), dateStr.end(), '/'), dateStr.end());
                    if (dateStr.size() == 8) {
                        date = std::stoi(dateStr.substr(4, 4) + dateStr.substr(0, 2) + dateStr.substr(2, 2));
                    } else continue;
                } else continue;

                if (!timeStr.empty()) {
                    int colonIndex = timeStr.find(':');
                    if (colonIndex == 1) timeStr = "0" + timeStr;
                    timeStr.erase(std::remove(timeStr.begin(), timeStr.end(), ':'), timeStr.end());
                    if (timeStr.size() == 4) time = std::stoi(timeStr);
                    else continue;
                } else continue;

                for (int i = 0; i < 8; i++) std::getline(ss, token, ',');

                std::getline(ss, token, ','); persons_inj = token.empty() ? 0 : std::stoi(token);
                std::getline(ss, token, ','); persons_kill = token.empty() ? 0 : std::stoi(token);
                std::getline(ss, token, ','); ped_inj = token.empty() ? 0 : std::stoi(token);
                std::getline(ss, token, ','); ped_kill = token.empty() ? 0 : std::stoi(token);
                std::getline(ss, token, ','); cyc_inj = token.empty() ? 0 : std::stoi(token);
                std::getline(ss, token, ','); cyc_kill = token.empty() ? 0 : std::stoi(token);
                std::getline(ss, token, ','); mot_inj = token.empty() ? 0 : std::stoi(token);
                std::getline(ss, token, ','); mot_kill = token.empty() ? 0 : std::stoi(token);

                local_records.emplace_back(date, time, persons_inj, persons_kill, ped_inj, ped_kill, cyc_inj, cyc_kill, mot_inj, mot_kill);
            } catch (...) {}
        }

        // Merge local thread data into the main data vector
        std::lock_guard<std::mutex> lock(data_mutex);
        data.insert(data.end(), local_records.begin(), local_records.end());
    };

    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        lines.push_back(line);
        if (lines.size() >= 10000) {  // Process every 10,000 lines in a new thread
            threads.emplace_back(worker, lines);
            lines.clear();
        }
    }
    if (!lines.empty()) threads.emplace_back(worker, lines);  // Process remaining lines

    for (auto &t : threads) t.join();
    
    std::cerr << "Data Loaded Successfully: " << data.size() << " records.\n";
}

// -----------------------------------------------------------------
// OpenMP-parallelized query methods
// -----------------------------------------------------------------

// getTotalInjuriesInRange: Sum persons_injured using OpenMP reduction.
int CollisionDataManager::getTotalInjuriesInRange(int startDate, int endDate) const {
	return memoize("getTotalInjuriesInRange", [this](int start, int end) {
		int total = 0;
#pragma omp parallel for reduction(+:total)
		for (size_t i = 0; i < data.size(); i++) {
			if (data[i].crash_date >= start && data[i].crash_date <= end) {
				total += data[i].persons_injured;
			}
		}
		return total;
		}, startDate, endDate);
}


// getTotalFatalitiesInRange: Sum persons_killed using OpenMP reduction.
int CollisionDataManager::getTotalFatalitiesInRange(int startDate, int endDate) const {
	return memoize("getTotalFatalitiesInRange", [this](int start, int end) {
		int total = 0;
#pragma omp parallel for reduction(+:total)
		for (size_t i = 0; i < data.size(); i++) {
			if (data[i].crash_date >= start && data[i].crash_date <= end) {
				total += data[i].persons_killed;
			}
		}
		return total;
		}, startDate, endDate);
}

// getMostSevereAccidents: Collect records with high injury or fatality counts.
std::vector<CollisionRecord> CollisionDataManager::getMostSevereAccidents(int startDate, int endDate) const {
	return memoize("getMostSevereAccidents", [this](int start, int end) {
		std::vector<CollisionRecord> severeAccidents;

#pragma omp parallel
		{
			std::vector<CollisionRecord> localSevere;
#pragma omp for nowait
			for (size_t i = 0; i < data.size(); i++) {
				if (data[i].crash_date >= start && data[i].crash_date <= end &&
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
		}, startDate, endDate);
}


// getPeakAccidentHour: Determine the hour with the most accidents.
std::pair<int, int> CollisionDataManager::getPeakAccidentHour(int startDate, int endDate) const {
	return memoize("getPeakAccidentHour", [this](int start, int end) {
		const int NUM_HOURS = 24;
		int hourCount[NUM_HOURS] = { 0 };

#pragma omp parallel for
		for (size_t i = 0; i < data.size(); i++) {
			if (data[i].crash_date >= start && data[i].crash_date <= end) {
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
		}, startDate, endDate);
}
