#include "utilities/location.h"
namespace Simulator {
    class Config;
    Location::Location(unsigned short idx, unsigned short typeIdx, Config *config)
            : idx(idx),
              loc2ManIncre(std::vector<std::vector<unsigned int>>(config->stepToTrack, std::vector<unsigned int>(0, 0))),
              Loc2ManRemove(std::vector<std::vector<unsigned int>>(config->stepToTrack, std::vector<unsigned int>(0, 0)))
    {
        type[0] = false;
        type[1] = false;
        type[2] = false;
        type.flip(typeIdx);
    }

    short Location::getType() {
        for (short placeId =0; placeId<3;++placeId){
            if(type[placeId]) return placeId;
        }
        return 0;
    }

    std::unordered_set<unsigned int>& Location::getRoutineVisits() {
        return loc2ManRoutine;
    }
}
