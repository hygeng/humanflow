#pragma once
#ifndef SIMULATOR_EPIDEMIC_H
#define SIMULATOR_EPIDEMIC_H
#include "../utilities/mobility.h"

namespace Simulator {

    class EpiGenerator{
    private:
        Config *config;
        EpiConfig *epiConfig;
        int cur_infected_cnt = 0; //incubation and critical
        std::vector<unsigned int> InfectedManIdxList; //infected people list
        std::vector<unsigned int> IncubationManIdxList; //inbubation people list
//        std::vector<unsigned int> dailyIncubationIdxList; //everyday newly infected people
        std::unordered_map<unsigned int, Man*> idx2Man;
        unsigned int stateCnt[3] = { 0 };  //stateCnt[0]:number of susceptible, stateCnt[1]:number of infected, stateCnt[2]:number of recovered

    public:
        std::vector<unsigned int> dailyIncubationManList;
        MobilityBase *MobilityB;
        // construction
        explicit EpiGenerator();
        EpiGenerator( Config *config_, EpiConfig* epiConfig_);//, const __gnu_cxx::hash_map<unsigned int, Man*>& idx2Man);
        //set infection
        bool setInitInfection(std::vector<unsigned int> initInfectedMan);
        bool setDailyInfection();

        //compute infection
        bool computeInfection(const std::vector<unsigned int>& HourManInLoc);
        bool computeEpiFastInfection(Man* man, const std::vector<unsigned int>& contactMan);
        bool computeFREDInfection(const std::vector<unsigned int>& HourManInLoc);
        bool computeFastSIRInfection(Man* man, const std::vector<unsigned int>& contactMan, float rate = 0.007, float rate2 = 0.0003);
        float pKInfection(int n,int k);

        //query
        int getCurInfectedCount() const;
        std::vector<unsigned int> getInfectedManIdxList();
        std::vector<unsigned int> getIncubationManIdxList();
        int getInfectedCount(){return InfectedManIdxList.size();}
        int getRecoveredCount(){return 0;} // TODO: not implemented
        int getIncubationCount(){return IncubationManIdxList.size();}

        // operators
        bool addInfectedManIdxList(int manIdx);
        bool eraseInfectedManIdxList(int position);
        bool eraseIncubationManIdxList(int position);
        bool minusCurInfectedNum(int recovered_cnt);

        float computeTime = 0.0;
        float stateCntTime = 0.0;
    };
};

#endif //SIMULATOR_EPIDEMIC_H
