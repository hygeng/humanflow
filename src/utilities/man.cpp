#include "utilities/man.h"

namespace Simulator {
    class Config;

    Man::Man(unsigned int _idx, Config *config)
            : manIdx(_idx),
              isFollowMallRoutine(true),
              isFollowRoutineDuration(true)
              {
        healthStatus[0] = true;
        interveneStrategy[0] = true;
        //      acqtGroup
        int acqtIdx =0;
        while (acqtIdx<config->acqtGroupNum){
            unsigned int acqtManIdx = config->startIdx + xorshf96()%config->manNum ;
            if (acqtManIdx==_idx || std::find(acqtGroup.begin(), acqtGroup.end(), acqtManIdx)!=acqtGroup.end()){
                continue;
            }
            acqtGroup.emplace_back(acqtManIdx);
            acqtIdx++;
        }

        if (config->isTraceTrajectory){
            man2LocIncre  = std::vector<unsigned short>(config->stepToTrack, 0);
            dailyRoutine  = std::vector<unsigned short>(3,0);
            man2LocIncre.reserve(config->stepToTrack);
            dailyRoutine.reserve(3);
            abnormalDuration.reserve(3);
            dailyDuration.reserve(3);
        }
        else if(config->isTraceContact){
            contactHis = std::vector<std::vector<unsigned int>>(config->stepToTrack,std::vector<unsigned int>(0,0));
            routineContact = std::vector<unsigned int>(config->numContactPerMan,0);
        }

    }


    int Man::getHealthStatus() {
        for (int hs =0; hs<4; ++hs){//hs = health status
            if(healthStatus[hs]) return hs;
        }
        return -1;
    }

    int Man::getInterveneStrategy() {
        for(int s = 0; s< 4; ++s){
            if(interveneStrategy[s]) return s;
        }
            
        return -1;
    }

    std::vector<unsigned short> Man::getDailyRoutine() {
        return dailyRoutine;
    }

    std::vector<unsigned short> Man::getManTrajectory() {
        return man2LocIncre;
    }

    bool Man::isHourFollowDuration(int hour, int curPlaceId) {
        if(isFollowRoutineDuration) return true;
        int placeIdOld =0;
        int leftHour = hour;
        while(leftHour >= dailyDuration[placeIdOld]){
            leftHour -= dailyDuration[placeIdOld++];
        }

        return placeIdOld==curPlaceId;
    }

    bool Man::setHealthStatus(int status) {
        healthStatus[getHealthStatus()] = false;
        healthStatus[status] = true;
        return true;
    }
}


