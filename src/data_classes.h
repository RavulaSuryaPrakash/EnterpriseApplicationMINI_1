#ifndef DATA_CLASSES_H
#define DATA_CLASSES_H

#include <vector>
#include <string>
#include <tuple>
#include <utility>

// Class representing a single collision record.
class CollisionRecord {
public:
    int crash_date;      // YYYYMMDD format
    int crash_time;      // HHMM format
    int persons_injured;
    int persons_killed;
    int pedestrians_injured;
    int pedestrians_killed;
    int cyclists_injured;
    int cyclists_killed;
    int motorists_injured;
    int motorists_killed;

    CollisionRecord(int date, int time, int p_inj, int p_kill, int ped_inj, int ped_kill, 
                    int cyc_inj, int cyc_kill, int mot_inj, int mot_kill);
};

// Class to manage collision data and provide query methods.
class CollisionDataManager {
private:
    std::vector<CollisionRecord> data;

public:
    // Loads collision records from a CSV file.
    void loadFromCSV(const std::string &filename);

    // Query methods updated to use OpenMP for parallelization.
    int getTotalInjuriesInRange(int startDate, int endDate);
    int getTotalFatalitiesInRange(int startDate, int endDate);
    std::vector<CollisionRecord> getMostSevereAccidents(int startDate, int endDate);
    std::pair<int, int> getPeakAccidentHour(int startDate, int endDate);
};

#endif 
