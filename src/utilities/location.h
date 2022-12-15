#ifndef SIMULATOR_LOCATION_H
#define SIMULATOR_LOCATION_H

#include "utility.h"

namespace Simulator {
    class Config;

    class Location {
        friend class MobilityGenerator;
        friend class EpiGenerator;
        friend class EpiSim;

    private:
        unsigned short idx;
        std::bitset<3> type; // 0:home  1:work  2:entertainment
        std::vector<std::vector<unsigned int>> loc2ManIncre; //incremental visits
        std::unordered_set<unsigned int> loc2ManRoutine; //routine visits of the location (man 1, man2,...)
        std::vector<std::vector<unsigned int>> Loc2ManRemove; //remove people's visits if they deviate from routine trajectory



    public:
        Location(unsigned short idx, unsigned short type, Config *config);//        std::pair<float, float> getCoordinate();
        short getType();
        std::unordered_set<unsigned int>& getRoutineVisits();

    };
}
#endif //SIMULATOR_LOCATION_H
