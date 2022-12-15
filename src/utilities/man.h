#ifndef SIMULATOR_MAN_H
#define SIMULATOR_MAN_H

#include "utility.h"

namespace Simulator {
    class Man {
        friend class MobilityGenerator;
        friend class NetworkGenerator;
        friend class EpiGenerator;
        friend class EpiSim;
    private:
        unsigned int manIdx;
        bool isFollowMallRoutine; // destination for the current day
        bool isFollowRoutineDuration;//period for the current day
        bool isInDailyInfectedList = false;  // Infection proceeds daily. Avoid duplicate infection
        std::bitset<4> healthStatus; //0:susceptible(S) 1:incubation(E) 2:critical infected(I) 3:recovered (R)
        std::vector<unsigned int> acqtGroup; //acquaintance group
        unsigned short todayMallLoc;
        std::vector<unsigned short> man2LocIncre;//incremental trajectory (when do not follow mall destination)
        std::vector<unsigned short> abnormalDuration; //when do not follow routine duration
        std::vector<unsigned short> dailyRoutine; //size 3 for home, work, mall
        std::vector<unsigned short> dailyDuration = {4, 8, 2};//size 3 for home, work, mall
        std::vector<std::vector<unsigned int>> contactHis; //contact history in network-based mobility
        std::vector<unsigned int> routineContact; //most frequent contacts for each day in network-based mobility
        std::bitset<5> interveneStrategy; // 0: no intervention 1:confine 2:isolate 3/4: hospitalize
        unsigned short strategyHours = 0; // remaining hours for intervention strategy

    public:
        Man(unsigned int idx, Config *config);
        //get mobility pattern
        std::vector<unsigned short> getDailyRoutine();
        std::vector<unsigned short> getManTrajectory();

        // query status
        unsigned int getManIdx() const{return manIdx;}
        int getHealthStatus();
        int getInterveneStrategy();
        bool isHourFollowDuration(int hour, int curPlaceId);

        //set status
        bool setHealthStatus(int status);

        int incubationHourLeft;

    };
}
#endif //SIMULATOR_MAN_H
