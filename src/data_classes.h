#ifndef DATA_CLASSES_H
#define DATA_CLASSES_H

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <any>
#include <tuple>
#include <functional>

// Implement hash function for tuples
namespace std {
	namespace {
		// Helper function to combine hash values
		template <class T>
		inline void hash_combine(std::size_t& seed, const T& v) {
			seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		}

		// Recursive template to compute hash of tuple
		template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
		struct HashValueImpl {
			static void apply(size_t& seed, const Tuple& tuple) {
				HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
				hash_combine(seed, std::get<Index>(tuple));
			}
		};

		template <class Tuple>
		struct HashValueImpl<Tuple, 0> {
			static void apply(size_t& seed, const Tuple& tuple) {
				hash_combine(seed, std::get<0>(tuple));
			}
		};
	}

	template <typename... TT>
	struct hash<std::tuple<TT...>> {
		size_t operator()(const std::tuple<TT...>& tt) const {
			size_t seed = 0;
			HashValueImpl<std::tuple<TT...>>::apply(seed, tt);
			return seed;
		}
	};
}

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
	// Memoization cache
	mutable std::unordered_map<std::string, std::any> cache;

	// Helper function to generate cache key
	template<typename... Args>
	std::string getCacheKey(const std::string& methodName, Args... args) const {
		return methodName + "_" + std::to_string(std::hash<std::tuple<Args...>>()(std::make_tuple(args...)));
	}

	// Generic memoization function
	template<typename Func, typename... Args>
	auto memoize(const std::string& methodName, Func func, Args... args) const {
		std::string key = getCacheKey(methodName, args...);
		if (cache.find(key) != cache.end()) {
			return std::any_cast<std::invoke_result_t<Func, Args...>>(cache[key]);
		}
		auto result = func(args...);
		cache[key] = result;
		return result;
	}
public:
	// Loads collision records from a CSV file.
	void loadFromCSV(const std::string& filename);

	// Query methods updated to use OpenMP for parallelization.
	int getTotalInjuriesInRange(int startDate, int endDate) const;
	int getTotalFatalitiesInRange(int startDate, int endDate) const;
	std::vector<CollisionRecord> getMostSevereAccidents(int startDate, int endDate) const;
	std::pair<int, int> getPeakAccidentHour(int startDate, int endDate) const;
};

#endif 
