#include "utilities/utility.h"
typedef std::chrono::steady_clock clk;

namespace Simulator {
    class Config;
    class InterveneConfig;

    Config::Config() {
        hoursPerDay = 14;
        stepToTrack = 28;
        followMallProb = 1.0; //probability of going to routine mall
        followTimeProb = 1.0; //probability of following time duration split {4,8,2}
        followContactProb = 1.0;
        isTraceTrajectory = true;
        numContactPerMan  = 1000;
        isTraceContact = false;
        manNum = 100000;
        locNum = 99;
        houseLocNum = 33;
        workLocNum = 33;
        mallLocNum = 33;
        isSetIntervention = false;
        OVERLAP_RATIO = 0.0;
        startIdx = 3;
        acqtGroupNum = 2;
    }

    EpiConfig::EpiConfig() {
        incubationHours = 14;
        init_infected_num = 10;
        pInfection = 0.20;
        pFastSIRInfection = 0.00001;
        pEpiFastInfection = 0.0001;
        fastSIRThreshold = 0.9;
        pAcqtInfection = 0.3;
    }

    InterveneConfig::InterveneConfig() {
        quarantineHours = 5;
        confineHours = 5;
        isolateHours = 5;
        treatHours = 5;
        fastTraceContact = true;
        traceContactOrder = 1;
        stepToTrace = 28;
    }

// random number generator
    unsigned long milliseconds_since_epoch=std::chrono::duration_cast<std::chrono::milliseconds>(clk::now().time_since_epoch()).count();
    static unsigned long Rx = 123456789,Ry = 362436069 ,Rz = 521288629;
    //Rx = 123456789
    unsigned long xorshf96(){
        unsigned  long t;
        Rx ^= Rx<<16; Rx ^= Rx >>5; Rx ^=  Rx<<1;
        t = Rx; Rx = Ry; Ry = Rz; Rz = t ^ Rx ^ Ry;
        return Rz;
    }

    Config* map2Config(std::unordered_map<std::string, std::string> configMap){
        auto *config_ = new Config();
        config_->hoursPerDay = std::stoi(configMap["hoursPerDay"]);
        config_->stepToTrack = std::stoi(configMap["stepToTrack" ]);
        config_->followMallProb = std::stof(configMap["followMallProb"]);
        config_->followTimeProb = std::stof(configMap["followTimeProb"]);
        config_->followContactProb = std::stof(configMap["followContactProb"]);
        config_->isTraceTrajectory = (configMap["isTraceTrajectory"] == "True");
        config_->numContactPerMan = std::stoi(configMap["numContactPerMan"]);
        config_->isTraceContact = (configMap["isTraceContact"] == "True");
        config_->manNum = std::stoi(configMap["manNum"]);
        config_->locNum = std::stoi(configMap["locNum"]);
        config_->houseLocNum = std::stoi(configMap["houseLocNum"]);
        config_->workLocNum = std::stoi(configMap["workLocNum"]);
        config_->mallLocNum = std::stoi(configMap["mallLocNum"]);
        config_->isSetIntervention = (configMap["isTraceTrajectory"] == "True");
        config_->OVERLAP_RATIO= std::stof(configMap["OVERLAP_RATIO"]);
        config_->startIdx = std::stoi(configMap["startIdx"]);
        config_->acqtGroupNum = std::stoi(configMap["acqtGroupNum"]);
        return config_;

    }

    EpiConfig* map2EpiConfig(std::unordered_map<std::string,std::string> EpiConfigMap){
        auto *epiconfig_ = new EpiConfig();
        epiconfig_->incubationHours = std::stoi(EpiConfigMap["incubationHours"]);
        epiconfig_->init_infected_num= std::stoi(EpiConfigMap["init_infected_num" ]);
        epiconfig_->pInfection = std::stof(EpiConfigMap["pInfection"]);
        epiconfig_->pFastSIRInfection = std::stof(EpiConfigMap["pFastSIRInfection"]);
        epiconfig_->pEpiFastInfection = std::stof(EpiConfigMap["pEpiFastInfection"]);
        epiconfig_->fastSIRThreshold = std::stof(EpiConfigMap["fastSIRThreshold"]);
        epiconfig_->pAcqtInfection = std::stof(EpiConfigMap["pAcqtInfection"]);
        return epiconfig_;
    }

    InterveneConfig* map2InterveneConfig(std::unordered_map<std::string,std::string> InterveneMap)    {
        auto *intConfig_ =  new InterveneConfig();
        intConfig_->quarantineHours = std::stoi(InterveneMap["quarantineHours"]);
        intConfig_->confineHours= std::stoi(InterveneMap["confineHours" ]);
        intConfig_->isolateHours = std::stoi(InterveneMap["isolateHours"]);
        intConfig_->treatHours= std::stoi(InterveneMap["treatHours" ]);
        intConfig_->fastTraceContact = (InterveneMap["fastTraceContact"] == "True");
        intConfig_->traceContactOrder= std::stoi(InterveneMap["traceContactOrder" ]);
        return intConfig_;
    }

//    uint32_t xorshift32(struct xorshift32_state *state){
//        uint32_t x = state->a;
//        x ^= x<<13;
//        x ^= x>>17;
//        x ^= x<<5;
//        return state->a = x;
//    }
}