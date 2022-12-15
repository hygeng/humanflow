#ifndef SIMULATOR_UTILITY_H
#define SIMULATOR_UTILITY_H
#include <vector>
#include <bitset>
#include <cmath>
#include <random>
#include <typeinfo>
#include <stdexcept>
#include <map>
#include <set>
#include <queue>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include "iostream"


const int random_N=9999;
const long xorshift_max = 0xbfffffff;
namespace Simulator {

    unsigned long xorshf96();

    class Config{
    // configuration for mobility
    public:
        int  hoursPerDay ;
        int stepToTrack;
        float followMallProb;
        float followTimeProb;
        float followContactProb;
        bool isTraceTrajectory;
        int numContactPerMan;
        bool isTraceContact;

        unsigned int manNum;
        unsigned short locNum;
        unsigned short houseLocNum;
        unsigned short workLocNum;
        unsigned short mallLocNum;
        bool isSetIntervention;
        float OVERLAP_RATIO;
        unsigned short startIdx;
        int acqtGroupNum; //number of acquaintance group members for each person
    public:
        Config();
    };

    class EpiConfig{
        // configuration for epidemic propagation
    public:
        int incubationHours;
        int init_infected_num;
        float pInfection = 0.05;
        double pFastSIRInfection = 0.00001;
        float pEpiFastInfection = 0.0001;
        double fastSIRThreshold = 0.9;
        float pAcqtInfection = 0.3;

    public:
        EpiConfig();
    };

    class InterveneConfig{
    // intervention configuration
    public:
        int quarantineHours;
        int confineHours;
        int isolateHours;
        int treatHours;
        bool fastTraceContact;
        int traceContactOrder;
        int interveneType;
        int stepToTrace;

    public:
        InterveneConfig();
    };

// functions to read configs fromm file
    Config* map2Config(std::unordered_map<std::string,std::string> configMap);
    EpiConfig* map2EpiConfig(std::unordered_map<std::string,std::string> EpiConfigMap);
    InterveneConfig* map2InterveneConfig(std::unordered_map<std::string,std::string> InterveneMap);
}

#endif //SIMULATOR_UTILITY_H
