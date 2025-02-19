#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include "data_classes.h"

#define NUM_RUNS 100  
long long totalTimeAll =0;
void benchmarkMethod(const std::string &name, std::function<void()> func) {
    std::vector<long long> times;
    long long time =0;
    for (int i = 0; i < NUM_RUNS; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
        time = time + std::accumulate(times.begin(), times.end(), 0LL);
    }

    
    std::cout << name << ": " << time/1e+6 << " Seconds (time  over " << NUM_RUNS << " runs)" << std::endl;
    totalTimeAll =  totalTimeAll +time/1e+6;
}

int main() {
    std::vector<long long> timesToLoad;
    CollisionDataManager manager;
    auto start = std::chrono::high_resolution_clock::now();
    manager.loadFromCSV("../data/Motor_Vehicle_Collisions_-_Crashes_20250218.csv");
    auto end = std::chrono::high_resolution_clock::now();
    timesToLoad.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    long long time = std::accumulate(timesToLoad.begin(), timesToLoad.end(), 0LL) /1e+6;

    std::cout <<"Time Taken to Load Data : "<< time << " Seconds" << std::endl;
    std::cout << "================== Performance Benchmark ==================" << std::endl;

    benchmarkMethod("Total Injuries (2023)", [&]() {
        manager.getTotalInjuriesInRange(20210101, 20231231);
    });

    benchmarkMethod("Total Fatalities (2023)", [&]() {
        manager.getTotalFatalitiesInRange(20230101, 20231231);
    });

    benchmarkMethod("Most Severe Accidents (2023)", [&]() {
        manager.getMostSevereAccidents(20230101, 20231231);
    });

    benchmarkMethod("Peak Accident Hour (2023)", [&]() {
        manager.getPeakAccidentHour(20230101, 20231231);
    });

    totalTimeAll  = totalTimeAll+time;
    std::cout << "Total Time for all Quries: " << totalTimeAll << " Seconds (time  over " << NUM_RUNS << " runs)"<< std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
