#include "engine/intervene.h"

#define SIM_DAY 32
using namespace Simulator;

std::vector<int> run_ours(Config *configAll, EpiConfig *epiConfigAll, InterveneConfig *interveneConfigAll, float HMESPInfection, int interveneStartDay) {
    std::vector<int> daily_result;
    auto *config = configAll;
    auto *epiConfig = epiConfigAll;
    auto *interveneConfig = interveneConfigAll;

    //mobility
    config->isTraceContact = false; //for network-based
    config->isTraceTrajectory = true;//for location-based method

    //epidemic config
    epiConfig->incubationHours = 14; //incubation
    epiConfig->pInfection = HMESPInfection;

    // intervene config
    config->isSetIntervention = true;
    // int interveneStartDay = 21;

    //init
    auto startTime = std::chrono::high_resolution_clock::now();
    float mobilityTime=0; //module Mobility timer
    float epidemicTime=0; //module Epidemic timer
    float interventionTime=0; //module Intervention timer

    auto *MobilityGenerator0 = new MobilityGenerator(config);
    auto mobilityInitTime = std::chrono::high_resolution_clock::now();
    mobilityTime += ((float)(mobilityInitTime - startTime).count())/1000000.0F;

//    __gnu_cxx::hash_map<unsigned int, Man*> idx2Man = MobilityGenerator0->getIdx2Man();
    auto idx2Man = MobilityGenerator0->getIdx2Man();
    std::unordered_map<unsigned short, Location*> idx2Loc = MobilityGenerator0->getIdx2Loc();

    auto *EpiGenerator0 = new EpiGenerator(config, epiConfig);
    EpiGenerator0->MobilityB = MobilityGenerator0;
    std::vector<unsigned int> initInfectedMan;
    for(int manIdx=config->startIdx; manIdx<config->startIdx + epiConfig->init_infected_num; manIdx++){
        initInfectedMan.emplace_back(manIdx);
    }
    EpiGenerator0->setInitInfection(initInfectedMan); //set initial infected man
    auto epidemicInitTime = std::chrono::high_resolution_clock::now();
    epidemicTime += ((float)(epidemicInitTime - mobilityInitTime).count())/1000000.0F;

    auto intervene = new Intervene(config, interveneConfig);
    auto interventionInitTime = std::chrono::high_resolution_clock::now();
    interventionTime += ((float)(interventionInitTime - epidemicInitTime).count())/1000000.0F;

    auto endTime1 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime1 - startTime).count())/1000000.0F;
    std::cout << "Init Time: " << totalElapsedTime/1000 << "s" << std::endl;

    std::vector<unsigned int> dailyInfectedRecord;

    //simulation start
    int total_recovered=0;
    for(int day=0;day<SIM_DAY;day++){
        std::cout<<"Day: "<<day+1<<"\t";

        //set mobility
        auto mobilityTime1 = std::chrono::high_resolution_clock::now();
        MobilityGenerator0->setTodayDestination();
        MobilityGenerator0->setTodayDuration();
        auto mobilityTime2 = std::chrono::high_resolution_clock::now();
        mobilityTime += ((float) (mobilityTime2 - mobilityTime1).count()) / 1000000.0F;

        // two-stage intervention
        int dayinterveneType = 0;
        if(day>interveneStartDay) dayinterveneType = interveneConfig->interveneType;

        //update people's health status
        if(day!=0){
            dailyInfectedRecord.clear();
            std::vector<unsigned int> incubationManList = EpiGenerator0->getIncubationManIdxList();
            for(int position=incubationManList.size()-1; position>=0; position--){
                int manIdx = incubationManList[position];
                if(MobilityGenerator0->getManIncubationHourLeft(manIdx) <= 0){ //incubation end, turn to critical
                    EpiGenerator0->eraseIncubationManIdxList(position);
                    EpiGenerator0->addInfectedManIdxList(manIdx);
                    MobilityGenerator0->setManHealthStatus(manIdx,2);
                    dailyInfectedRecord.emplace_back(manIdx);
                }
                else MobilityGenerator0->DecreaseManIncubationDayLeft(manIdx);
            }

            // intervene
            if(config->isSetIntervention){
                auto interventionTime1 = std::chrono::high_resolution_clock::now();
                // get location visiting history
                for(unsigned short locIdx= config->startIdx ; locIdx< config->startIdx + config->locNum; locIdx++)
                    intervene->hisLoc[locIdx] = MobilityGenerator0->getLoc2ManHis(idx2Loc[locIdx], day * config->hoursPerDay,
                                                                                config->stepToTrack);

                // set intervention
                std::vector<unsigned int> concernedList =dailyInfectedRecord;// EpiGenerator0->getInfectedIdxList(); //EpiGenerator0->dailyIncubationIdxList;//
                if (!interveneConfig->fastTraceContact){
                    for(unsigned int manIdx= config->startIdx; manIdx< config->startIdx+idx2Man.size(); manIdx++)
                    {
                        intervene->hisMan[manIdx] = MobilityGenerator0->getMan2LocHis(idx2Man[manIdx],day * config->hoursPerDay,
                                                                                    interveneConfig->stepToTrace);
                    }
                    intervene->contactIntervene(concernedList, interveneConfig->interveneType, MobilityGenerator0);
                }
                else{
                    intervene->fastContactIntervene(concernedList, dayinterveneType,  MobilityGenerator0, day*config->hoursPerDay,
                                                    interveneConfig->traceContactOrder, interveneConfig->stepToTrace);
                }
                auto interventionTime2 = std::chrono::high_resolution_clock::now();
                interventionTime += ((float) (interventionTime2 - interventionTime1).count()) / 1000000.0F;
            }
        }

//        EpiGenerator0->dailyIncubationIdxList.clear();

        for(int hour=0; hour<config->hoursPerDay; hour++){
            auto mobilityTime1 = std::chrono::high_resolution_clock::now();
            MobilityGenerator0->generateHourTrajectory(1);
            int hourly_recovered = MobilityGenerator0->getDailyRecoveredCount();
            total_recovered += hourly_recovered;
            EpiGenerator0->minusCurInfectedNum(MobilityGenerator0->getDailyRecoveredCount());
            auto mobilityTime2 = std::chrono::high_resolution_clock::now();
            mobilityTime += ((float) (mobilityTime2 - mobilityTime1).count()) / 1000000.0F;

            auto epidemicTime1 = std::chrono::high_resolution_clock::now();
            for(unsigned short locIdx= config->startIdx; locIdx< config->startIdx + config->locNum; locIdx++){
                std::vector<unsigned int> hourloc2man = MobilityGenerator0->getHourLoc2Man(idx2Loc[locIdx], MobilityGenerator0->getGlobalStep());
                if(hourloc2man.size()==0) continue;
                EpiGenerator0->computeInfection(hourloc2man);
            }
            auto epidemicTime2 = std::chrono::high_resolution_clock::now();
            epidemicTime += ((float) (epidemicTime2 - epidemicTime1).count()) / 1000000.0F;
        }

        auto epidemicTime3 = std::chrono::high_resolution_clock::now();
        EpiGenerator0->setDailyInfection();
        auto epidemicTime4 = std::chrono::high_resolution_clock::now();
        epidemicTime += ((float) (epidemicTime4 - epidemicTime3).count()) / 1000000.0F;


        std::cout<< EpiGenerator0->getIncubationCount() <<"\t"<< EpiGenerator0->getInfectedCount()<<"\n";
        daily_result.emplace_back(EpiGenerator0->getInfectedCount());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    totalElapsedTime = ((float)(endTime - startTime).count())/1000000.0F;

//    std::cout << "Mobility traversal Time: " << MobilityGenerator0->queryLocTime/1000 << "s" << std::endl;
    std::cout << "Mobility Time: " << mobilityTime/1000 << "s" << std::endl;
    std::cout << "Epidemic Time: " << epidemicTime/1000 << "s" << std::endl;
    std::cout << "Intervention Time: " << interventionTime/1000 << "s" << std::endl;
    std::cout << "Total Time: " << totalElapsedTime/1000 << "s" << std::endl;

    return daily_result;
}

std::vector<int> run_epifast(Config *configAll, EpiConfig *epiConfigAll, InterveneConfig *interveneConfigAll){
    std::vector<int> daily_result;
    auto *config = configAll;
    auto *epiConfig = epiConfigAll;
    auto *interveneConfig = interveneConfigAll;

    //network-based mobility config
    config->isTraceContact = true;
    config->isTraceTrajectory  = false;

    // epi config
    epiConfig->incubationHours = 14; 

    //intervene
    config->isSetIntervention = false;

    auto startTime = std::chrono::high_resolution_clock::now();
    float mobilityTime=0;
    float epidemicTime=0;
    float interventionTime=0;

    auto *NetworkGenerator0 = new NetworkGenerator(config);
    auto mobilityInitTime = std::chrono::high_resolution_clock::now();
    mobilityTime += ((float)(mobilityInitTime - startTime).count())/1000000.0F;

    auto idx2Man = NetworkGenerator0->getIdx2Man();
    auto *EpiGenerator0 = new EpiGenerator(config, epiConfig);
    std::cout<<"epifast p infection:"<<epiConfig->pEpiFastInfection <<"\n";
    EpiGenerator0->MobilityB = NetworkGenerator0;
    std::vector<unsigned int> initInfectedMan;
    for(int manIdx=config->startIdx; manIdx<config->startIdx + epiConfig->init_infected_num; manIdx++){
        initInfectedMan.emplace_back(manIdx);
    }
    EpiGenerator0->setInitInfection(initInfectedMan); //set initial infected man
    auto epidemicInitTime = std::chrono::high_resolution_clock::now();
    epidemicTime += ((float)(epidemicInitTime - mobilityInitTime).count())/1000000.0F;

    //intervention initialization

    auto intervene = new Intervene(config, interveneConfig);
    auto interventionInitTime = std::chrono::high_resolution_clock::now();
    interventionTime += ((float)(interventionInitTime - epidemicInitTime).count())/1000000.0F;

    auto endTime1 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime1 - startTime).count())/1000000.0F;
    std::cout << "Init Time: " << totalElapsedTime/1000 << "s" << std::endl;
    for(int day=0;day<SIM_DAY;day++) {
        std::cout<<"Day: "<<day+1<<"\t";
        // auto daytime1 = std::chrono::high_resolution_clock::now();

        //update people's health status
        if(day!=0) {
            std::vector<unsigned int> incubationManList = EpiGenerator0->getIncubationManIdxList();
            for (int position = incubationManList.size() - 1; position >= 0; position--) {
                int manIdx = incubationManList[position];
                Man *man = idx2Man[manIdx];
                if (man->incubationHourLeft <= 0) { //incubation end, turn to critical
                    EpiGenerator0->eraseIncubationManIdxList(position);
                    EpiGenerator0->addInfectedManIdxList(manIdx);
                    man->setHealthStatus(2);
                } else man->incubationHourLeft-= config->hoursPerDay;
            }

            // intervene
            if(config->isSetIntervention ){
                // get man visiting history
                auto interventionTime1 = std::chrono::high_resolution_clock::now();
                std::vector<unsigned int> concernedList = EpiGenerator0->getInfectedManIdxList();
                for(auto manIdx:concernedList){
                    auto contactHis = NetworkGenerator0->getManContactHis(idx2Man[manIdx], day*config->hoursPerDay, config->stepToTrack);
                    intervene->hisManNetwork[manIdx].clear();
                    intervene->hisManNetwork[manIdx] = contactHis;
                }

                // set intertnevion
                intervene->ContactInterveneNetwork(concernedList, 2, NetworkGenerator0,
                                                   interveneConfig->traceContactOrder, config->stepToTrack,  idx2Man);
                auto interventionTime2 = std::chrono::high_resolution_clock::now();
                interventionTime += ((float)(interventionTime2 - interventionTime1).count())/1000000.0F;

            }
        }

//        EpiGenerator0->dailyIncubationIdxList.clear();
        for (int hour = 0; hour < config->hoursPerDay; hour++) {
            auto trajectoryTime1 = std::chrono::high_resolution_clock::now();
            NetworkGenerator0->generateHourContact(1);
//            std::cout << "Hour " << hour + 1 << " finished generating contact." << std::endl;
            auto trajectoryTime2 = std::chrono::high_resolution_clock::now();
            mobilityTime += ((float) (trajectoryTime2 - trajectoryTime1).count()) / 1000000.0F;

            auto epidemicTime1 = std::chrono::high_resolution_clock::now();
            for (unsigned int manIdx = config->startIdx; manIdx < config->startIdx+ idx2Man.size(); manIdx++) {
                Man *man = idx2Man[manIdx];
//                if(man->getHealthStatus()>0)    std::cout<<"hour man contact: "<<NetworkGenerator0->getHourManContact(man,NetworkGenerator0->getGlobalStep()).size()<<std::endl;
                EpiGenerator0->computeEpiFastInfection(man, NetworkGenerator0->getHourManContact(man, NetworkGenerator0->getGlobalStep()));
            }
            auto epidemicTime2 = std::chrono::high_resolution_clock::now();
            epidemicTime += ((float) (epidemicTime2 - epidemicTime1).count()) / 1000000.0F;

//            auto hourtime2 = std::chrono::high_resolution_clock::now();
//            float hourlyTime = ((float) (hourtime2 - hourtime1).count()) / 1000000.0F;
//            std::cout << "time consumed in hour " << hour + 1 << ": " << hourlyTime / 1000 << "s" << std::endl;
        }


        EpiGenerator0->setDailyInfection();

        std::cout<< EpiGenerator0->getIncubationCount() <<"\t"<< EpiGenerator0->getInfectedCount()<<"\n";
        daily_result.emplace_back(EpiGenerator0->getInfectedCount());

    }
    auto endTime = std::chrono::high_resolution_clock::now();
    totalElapsedTime = ((float)(endTime - startTime).count())/1000000.0F;
    std::cout << "Trajectory Time: " << mobilityTime/1000 << "s" << std::endl;
    std::cout << "Epidemic Time: " << epidemicTime/1000 << "s" << std::endl;
    std::cout << "Intervention Time: " << interventionTime/1000 << "s" << std::endl;
    std::cout << "Total Time: " << totalElapsedTime/1000 << "s" << std::endl;
    return daily_result;
}

std::vector<int> run_fastSIR(Config *configAll, EpiConfig *epiConfigAll, InterveneConfig *interveneConfigAll, float fastSIRrate1, float fastSIRrate2){
    std::vector<int> daily_result;
    auto *config = configAll;
    auto *epiConfig = epiConfigAll;
    auto *interveneConfig = interveneConfigAll;

    //network-based mobility config
    config->isTraceContact = true;
    config->isTraceTrajectory  = false;

    // epi config
    epiConfig->incubationHours = 14; 

    // interevene
    config->isSetIntervention = false;

    auto startTime = std::chrono::high_resolution_clock::now();
    float mobilityTime=0;
    float epidemicTime=0;
    float interventionTime=0;

    auto *NetworkGenerator0 = new NetworkGenerator(config);
    std::cout<<"start here\n";
    auto mobilityInitTime = std::chrono::high_resolution_clock::now();
    mobilityTime += ((float)(mobilityInitTime - startTime).count())/1000000.0F;

    auto idx2Man = NetworkGenerator0->getIdx2Man();
    auto EpiGenerator0 = new EpiGenerator(config, epiConfig);
    EpiGenerator0->MobilityB = NetworkGenerator0;
    std::vector<unsigned int> initInfectedMan;
    for(int manIdx=config->startIdx; manIdx<config->startIdx + epiConfig->init_infected_num; manIdx++){
        initInfectedMan.emplace_back(manIdx);
    }
    EpiGenerator0->setInitInfection(initInfectedMan); //set initial infected man

    auto epidemicInitTime = std::chrono::high_resolution_clock::now();
    epidemicTime += ((float)(epidemicInitTime - mobilityInitTime).count())/1000000.0F;

    //intervention initialization
    auto intervene = new Intervene(config, interveneConfig);
    auto interventionInitTime = std::chrono::high_resolution_clock::now();
    interventionTime += ((float)(interventionInitTime - epidemicInitTime).count())/1000000.0F;

    auto endTime1 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime1 - startTime).count())/1000000.0F;
    std::cout << "Init Time: " << totalElapsedTime/1000 << "s" << std::endl;

    for(int day=0;day<SIM_DAY;day++) {
        std::cout<<"Day: "<<day+1<<"\t";
//        auto daytime1 = std::chrono::high_resolution_clock::now();
        //update people's health status
        if(day!=0) {
            std::vector<unsigned int> incubationManList = EpiGenerator0->getIncubationManIdxList();
            for (int position = incubationManList.size() - 1; position >= 0; position--) {
                int manIdx = incubationManList[position];
                Man *man = idx2Man[manIdx];
                if (man->incubationHourLeft <= 0) { //incubation end, turn to critical
                    EpiGenerator0->eraseIncubationManIdxList(position);
                    EpiGenerator0->addInfectedManIdxList(manIdx);
                    man->setHealthStatus(2);
                } else man->incubationHourLeft-= config->hoursPerDay;
            }
        }

        if(config->isSetIntervention ){
            // get man visiting history
            auto interventionTime1 = std::chrono::high_resolution_clock::now();
            std::vector<unsigned int> concernedList = EpiGenerator0->getInfectedManIdxList();
            for(unsigned int manIdx=config->startIdx; manIdx< config->startIdx + idx2Man.size(); manIdx++){
                auto contactHis = NetworkGenerator0->getManContactHis(idx2Man[manIdx], day*config->hoursPerDay, config->stepToTrack);
                intervene->hisManNetwork[manIdx].clear();
                intervene->hisManNetwork[manIdx] = contactHis;
            }

            // set intertnevion
            intervene->ContactInterveneNetwork(concernedList, 2, NetworkGenerator0,
                                               interveneConfig->traceContactOrder, config->stepToTrack,  idx2Man);
            auto interventionTime2 = std::chrono::high_resolution_clock::now();
            interventionTime += ((float)(interventionTime2 - interventionTime1).count())/1000000.0F;
        }

//        EpiGenerator0->dailyIncubationIdxList.clear();

        for (int hour = 0; hour < config->hoursPerDay; hour++) {

            auto trajectoryTime1 = std::chrono::high_resolution_clock::now();
            NetworkGenerator0->generateHourContact(1);
            auto trajectoryTime2 = std::chrono::high_resolution_clock::now();
            mobilityTime += ((float) (trajectoryTime2 - trajectoryTime1).count()) / 1000000.0F;

            auto epidemicTime1 = std::chrono::high_resolution_clock::now();
            for (unsigned int manIdx = config->startIdx; manIdx < config->startIdx + idx2Man.size(); manIdx++) {
                Man *man = idx2Man[manIdx];
                if (man->getHealthStatus()!=0) {

                EpiGenerator0->computeFastSIRInfection(man, NetworkGenerator0->getHourManContact(man,NetworkGenerator0->getGlobalStep()), fastSIRrate1, fastSIRrate2);                }
            }
            auto epidemicTime2 = std::chrono::high_resolution_clock::now();
            epidemicTime += ((float) (epidemicTime2 - epidemicTime1).count()) / 1000000.0F;

        }


        auto epidemicTime3 = std::chrono::high_resolution_clock::now();
        EpiGenerator0->setDailyInfection();
        auto epidemicTime4 = std::chrono::high_resolution_clock::now();
        epidemicTime += ((float) (epidemicTime4 - epidemicTime3).count()) / 1000000.0F;


        std::cout<< EpiGenerator0->getIncubationCount() <<"\t"<< EpiGenerator0->getInfectedCount()<<"\n";
        daily_result.emplace_back(EpiGenerator0->getInfectedCount());
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    totalElapsedTime = ((float)(endTime - startTime).count())/1000000.0F;
     std::cout << "Trajectory Time: " << mobilityTime/1000 << "s" << std::endl;
     std::cout << "Epidemic Time: " << epidemicTime/1000 << "s" << std::endl;
     std::cout << "Intervention Time: " << interventionTime/1000 << "s" << std::endl;
    std::cout << "Total Time: " << totalElapsedTime/1000 << "s" << std::endl;
    return daily_result;
}

std::vector<int> run_fred(Config *configAll, EpiConfig *epiConfigAll, InterveneConfig *interveneConfigAll, float epiSimPInfection){
    auto *config = configAll;
    auto *epiConfig = epiConfigAll;
    auto *interveneConfig = interveneConfigAll;

    config->hoursPerDay = 14; 
    config->stepToTrack  = 14;
    config->startIdx = 3;

    //randomness
    config->followMallProb = 1.0;
    config->followTimeProb = 1.0;

    //
    config->isTraceContact = false; //for network-based
    config->isTraceTrajectory = true;//for location-based method

    //intervention config
    // interevene
    config->isSetIntervention = false;

    auto startTime = std::chrono::high_resolution_clock::now();
    float mobilityTime=0;
    float epidemicTime=0;
    float interventionTime=0; //module Intervention timer

    auto  *EpiSim0 = new EpiSim(config);
    auto mobilityInitTime = std::chrono::high_resolution_clock::now();
    mobilityTime += ((float)(mobilityInitTime - startTime).count())/1000000.0F;

    auto idx2Man = EpiSim0->getIdx2Man();
    std::unordered_map<unsigned short, Location*> idx2Loc = EpiSim0->getIdx2Loc();

    std::vector<int> daily_result;
    auto *EpiGenerator0 = new EpiGenerator(config, epiConfig);
    EpiGenerator0->MobilityB =EpiSim0;
    std::vector<unsigned int> initInfectedMan;
    for(int manIdx=config->startIdx; manIdx<config->startIdx + epiConfig->init_infected_num; manIdx++){
        initInfectedMan.emplace_back(manIdx);
    }
    EpiGenerator0->setInitInfection(initInfectedMan); //set initial infected man
    auto epidemicInitTime = std::chrono::high_resolution_clock::now();
    epidemicTime += ((float)(epidemicInitTime - mobilityInitTime).count())/1000000.0F;

    auto intervene = new Intervene(config, interveneConfig);
    auto interventionInitTime = std::chrono::high_resolution_clock::now();
    interventionTime += ((float)(interventionInitTime - epidemicInitTime).count())/1000000.0F;

    auto endTime1 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime1 - startTime).count())/1000000.0F;
    std::cout << "Init Time: " << totalElapsedTime/1000 << "s" << std::endl;

    //simulation start
    for(int day=0;day<SIM_DAY;day++){
        std::cout<<"Day: "<<day+1<<"\t";
//        auto daytime1 = std::chrono::high_resolution_clock::now();

        //set mobility
        EpiSim0->setDayRoutine();
        EpiSim0->resetRoutineDuration();

        //update people's health status
        if(day!=0) {
            std::vector<unsigned int> incubationManList = EpiGenerator0->getIncubationManIdxList();
            for (int position = incubationManList.size() - 1; position >= 0; position--) {
                int manIdx = incubationManList[position];
                Man *man = idx2Man[manIdx];
                if (man->incubationHourLeft <= 0) { //incubation end, turn to critical
                    EpiGenerator0->eraseIncubationManIdxList(position);
                    EpiGenerator0->addInfectedManIdxList(manIdx);
                    man->setHealthStatus(2);
                } else man->incubationHourLeft-= config->hoursPerDay;
            }

            // init intervene
            if (config->isSetIntervention) {
                auto interventionTime1 = std::chrono::high_resolution_clock::now();
                // get location visiting history
                for (unsigned short locIdx = config->startIdx; locIdx < config->startIdx + idx2Loc.size(); locIdx++)
                    intervene->hisLoc[locIdx] = EpiSim0->getLoc2ManHis(idx2Loc[locIdx],day * config->hoursPerDay,
                                                                        config->stepToTrack);
                // set intervention
                std::vector<unsigned int> concernedList = EpiGenerator0->getInfectedManIdxList();
                if (!interveneConfig->fastTraceContact) {
                    for (unsigned int manIdx = config->startIdx; manIdx < config->startIdx + idx2Man.size(); manIdx++) {
                        intervene->hisMan[manIdx] = EpiSim0->getMan2LocHis(idx2Man[manIdx], day * config->hoursPerDay,
                                                                            config->stepToTrack);
                    }
                    intervene->contactInterveneEpiSim(concernedList, 2, EpiSim0);

                } else {

                    intervene->fastContactInterveneEpiSim(concernedList, 2, EpiSim0, day * config->hoursPerDay,
                                                        interveneConfig->traceContactOrder, config->stepToTrack);
                }
                auto interventionTime2 = std::chrono::high_resolution_clock::now();
                interventionTime += ((float) (interventionTime2 - interventionTime1).count()) / 1000000.0F;
            }
        }

//        EpiGenerator0->dailyIncubationIdxList.clear();

        for(int hour=0; hour<config->hoursPerDay; hour++){
            //auto hourtime1 = std::chrono::high_resolution_clock::now();
            auto mobilityTime1 = std::chrono::high_resolution_clock::now();
            EpiSim0->generateHourTrajectory(1);
            auto mobilityTime2 = std::chrono::high_resolution_clock::now();
            mobilityTime += ((float) (mobilityTime2 - mobilityTime1).count()) / 1000000.0F;

            auto epidemicTime1 = std::chrono::high_resolution_clock::now();
            for(unsigned short locIdx= config->startIdx; locIdx< config->startIdx + idx2Loc.size()-1; locIdx++){  // todo why loc has only 99?
//                std::cout<<locIdx<<"\t";
                std::vector<unsigned int> hourloc2man = EpiSim0->getHourLoc2Man(idx2Loc[locIdx], EpiSim0->getGlobalStep());
                if(hourloc2man.size()==0) continue;
                EpiGenerator0->computeFREDInfection( hourloc2man);
            }

            auto epidemicTime2 = std::chrono::high_resolution_clock::now();
            epidemicTime += ((float) (epidemicTime2 - epidemicTime1).count()) / 1000000.0F;
        }

        auto epidemicTime3 = std::chrono::high_resolution_clock::now();
        EpiGenerator0->setDailyInfection();
        auto epidemicTime4 = std::chrono::high_resolution_clock::now();
        epidemicTime += ((float) (epidemicTime4 - epidemicTime3).count()) / 1000000.0F;

        std::cout<< EpiGenerator0->getIncubationCount() <<"\t"<< EpiGenerator0->getInfectedCount()<<"\n";
        daily_result.emplace_back(EpiGenerator0->getInfectedCount());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    totalElapsedTime = ((float)(endTime - startTime).count())/1000000.0F;
    std::cout << "Trajectory Time: " << mobilityTime/1000 << "s" << std::endl;
    std::cout << "Epidemic Time: " << epidemicTime/1000 << "s" << std::endl;
    std::cout << "Intervention Time: " << interventionTime/1000 << "s" << std::endl;
    std::cout << "Total Time: " << totalElapsedTime/1000 << "s" << std::endl;

    return daily_result;
}

std::vector<int> run_episimdemics(Config *configAll, EpiConfig *epiConfigAll, InterveneConfig *interveneConfigAll, float epiSimPInfection){
    std::vector<int> daily_result;
    auto *config = configAll;
    auto *epiConfig = epiConfigAll;
    auto *interveneConfig = interveneConfigAll;

    //mobility
    config->isTraceContact = false; //for network-based
    config->isTraceTrajectory = true;//for location-based method

    //epidemic config
    epiConfig->pInfection = epiSimPInfection;
    epiConfig->incubationHours = 14;

    // interevene
    config->isSetIntervention = false;

    auto startTime = std::chrono::high_resolution_clock::now();
    float mobilityTime=0;
    float epidemicTime=0;
    float interventionTime=0; //module Intervention timer

    auto  *EpiSim0 = new EpiSim(config);
    auto mobilityInitTime = std::chrono::high_resolution_clock::now();
    mobilityTime += ((float)(mobilityInitTime - startTime).count())/1000000.0F;

    auto idx2Man = EpiSim0->getIdx2Man();
    std::unordered_map<unsigned short, Location*> idx2Loc = EpiSim0->getIdx2Loc();

    auto *EpiGenerator0 = new EpiGenerator(config, epiConfig);
    EpiGenerator0->MobilityB = EpiSim0;
    std::vector<unsigned int> initInfectedMan;
    for(int manIdx=config->startIdx; manIdx<config->startIdx + epiConfig->init_infected_num; manIdx++){
        initInfectedMan.emplace_back(manIdx);
    }
    EpiGenerator0->setInitInfection(initInfectedMan); //set initial infected man
    auto epidemicInitTime = std::chrono::high_resolution_clock::now();
    epidemicTime += ((float)(epidemicInitTime - mobilityInitTime).count())/1000000.0F;

    auto intervene = new Intervene(config, interveneConfig);
    auto interventionInitTime = std::chrono::high_resolution_clock::now();
    interventionTime += ((float)(interventionInitTime - epidemicInitTime).count())/1000000.0F;

    auto endTime1 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime1 - startTime).count())/1000000.0F;
    std::cout << "Init Time: " << totalElapsedTime/1000 << "s" << std::endl;

    //simulation start
    for(int day=0;day<SIM_DAY;day++){
        std::cout<<"Day: "<<day+1<<"\t";
        // auto daytime1 = std::chrono::high_resolution_clock::now();

        //set mobility
        EpiSim0->setDayRoutine();
        EpiSim0->resetRoutineDuration();

        //update people's health status
        if(day!=0) {
            std::vector<unsigned int> incubationManList = EpiGenerator0->getIncubationManIdxList();
            for (int position = incubationManList.size() - 1; position >= 0; position--) {
                int manIdx = incubationManList[position];
                Man *man = idx2Man[manIdx];
                if (man->incubationHourLeft <= 0) { //incubation end, turn to critical
                    EpiGenerator0->eraseIncubationManIdxList(position);
                    EpiGenerator0->addInfectedManIdxList(manIdx);
                    man->setHealthStatus(2);
                } else man->incubationHourLeft-= config->hoursPerDay;
            }

            // intervene
            if (config->isSetIntervention) {
                auto interventionTime1 = std::chrono::high_resolution_clock::now();
                // get location visiting history
                for (unsigned short locIdx = config->startIdx; locIdx < config->startIdx + idx2Loc.size(); locIdx++)
                    intervene->hisLoc[locIdx] = EpiSim0->getLoc2ManHis(idx2Loc[locIdx],
                                                                       day * config->hoursPerDay,
                                                                       config->stepToTrack);
                // set intervention
                std::vector<unsigned int> concernedList = EpiGenerator0->getInfectedManIdxList();
                if (!interveneConfig->fastTraceContact) {
                    for (unsigned int manIdx = config->startIdx; manIdx < config->startIdx + idx2Man.size(); manIdx++) {
                        intervene->hisMan[manIdx] = EpiSim0->getMan2LocHis(idx2Man[manIdx], day * config->hoursPerDay,
                                                                           config->stepToTrack);
                    }
                    intervene->contactInterveneEpiSim(concernedList, 2, EpiSim0);

                } else {

                    intervene->fastContactInterveneEpiSim(concernedList, 2, EpiSim0, day * config->hoursPerDay,
                                                          interveneConfig->traceContactOrder, config->stepToTrack);
                }
                auto interventionTime2 = std::chrono::high_resolution_clock::now();
                interventionTime += ((float) (interventionTime2 - interventionTime1).count()) / 1000000.0F;
            }
        }
//        EpiGenerator0->dailyIncubationIdxList.clear();

        for(int hour=0; hour<config->hoursPerDay; hour++){
            //auto hourtime1 = std::chrono::high_resolution_clock::now();
//            std::cout<<"Hour "<<hour+1<<" start"<<std::endl;
            auto mobilityTime1 = std::chrono::high_resolution_clock::now();
            EpiSim0->generateHourTrajectory(1);
            auto mobilityTime2 = std::chrono::high_resolution_clock::now();
            mobilityTime += ((float) (mobilityTime2 - mobilityTime1).count()) / 1000000.0F;

            auto epidemicTime1 = std::chrono::high_resolution_clock::now();
            for(unsigned short locIdx= config->startIdx; locIdx< config->startIdx + idx2Loc.size()-1; locIdx++){  // todo why loc has only 99?
                std::vector<unsigned int> hourloc2man = EpiSim0->getHourLoc2Man(idx2Loc[locIdx], EpiSim0->getGlobalStep());
                if(hourloc2man.size()==0) continue;

                EpiGenerator0->computeInfection( hourloc2man);
            }

            auto epidemicTime2 = std::chrono::high_resolution_clock::now();
            epidemicTime += ((float) (epidemicTime2 - epidemicTime1).count()) / 1000000.0F;
        }

        auto epidemicTime3 = std::chrono::high_resolution_clock::now();
        EpiGenerator0->setDailyInfection();
        auto epidemicTime4 = std::chrono::high_resolution_clock::now();
        epidemicTime += ((float) (epidemicTime4 - epidemicTime3).count()) / 1000000.0F;


        std::cout<< EpiGenerator0->getIncubationCount() <<"\t"<< EpiGenerator0->getInfectedCount()<<"\n";
        daily_result.emplace_back(EpiGenerator0->getInfectedCount());
    }
     auto endTime = std::chrono::high_resolution_clock::now();
     totalElapsedTime = ((float)(endTime - startTime).count())/1000000.0F;
     std::cout << "Trajectory Time: " << mobilityTime/1000 << "s" << std::endl;
     std::cout << "Epidemic Time: " << epidemicTime/1000 << "s" << std::endl;
     std::cout << "Intervention Time: " << interventionTime/1000 << "s" << std::endl;
     std::cout << "Total Time: " << totalElapsedTime/1000 << "s" << std::endl;

    return daily_result;
}


int run_methods(int top_manNum, int top_locNum, std::string countyName){
    std::cout<<"experiments for: "<<countyName<<"\n";
//    srand(std::time(0));
    auto *config = new Config();
    auto *epiConfig = new EpiConfig();
    auto *interveneConfig = new InterveneConfig();
    //config
    config->manNum = top_manNum;
    config->houseLocNum = top_locNum/3;
    config->workLocNum = top_locNum/3;
    config->mallLocNum = top_locNum/3;
    config->locNum = config->houseLocNum + config->workLocNum + config->mallLocNum;
    config->hoursPerDay = 14; 
    config->stepToTrack  = 28;
    config->startIdx = 3;

    config->followMallProb = 1;
    config->followTimeProb = 1;
    config->OVERLAP_RATIO = 0;
    config->followContactProb = 1.0;

    config->isTraceContact = false; //for network-based
    config->isTraceTrajectory = true;//for location-based method
    config->numContactPerMan = top_manNum/top_locNum;

    //epidemic config
    epiConfig->init_infected_num = 10;
    epiConfig->incubationHours = 28;
    epiConfig->pAcqtInfection = 0.0;

    epiConfig->pInfection = 0.0085;
    float HMESPInfection = 0.0172;
    float fredPInfection =0.00000093;
    float epiSimPInfection = 0.0128;
    epiConfig->pEpiFastInfection = 0.000003;
    epiConfig->pFastSIRInfection = 0.0001;
    epiConfig->fastSIRThreshold = 0.001;

    float fastSIRrate1 = 0.0075;
    float fastSIRrate2 = 0.00065;

    //intervention config
    int interveneStartDay = 18; //for hmes
    config->isSetIntervention = false;
    interveneConfig->interveneType = 2;
    interveneConfig->fastTraceContact = true;
    interveneConfig->traceContactOrder = 2;
    interveneConfig->treatHours  = 280;
    interveneConfig->confineHours = 14;
    interveneConfig->isolateHours = 14;

    /////!!!! config that has been once changed will stay changed, must be re-config in each call
    /////-------------------------------------------------------------------------------------
    std::cout<<"----------run HMES----------"<<std::endl;
    std::vector<int> hmes_result = run_ours(config, epiConfig, interveneConfig, HMESPInfection, interveneStartDay);

    std::cout<<"----------run_epifast----------"<<std::endl;
    std::vector<int> epifast_result = run_epifast(config, epiConfig, interveneConfig);

    std::cout<<"----------run_fastSIR----------"<<std::endl;
    std::vector<int> fastSIR_result = run_fastSIR(config, epiConfig, interveneConfig, fastSIRrate1, fastSIRrate2);
    //  fastSIR implementation is unrobust: either no infection or explosion; take rate and rate1 as workaround when no infection occurred

    std::cout<<"----------run_fred----------"<<std::endl;
    std::vector<int> fred_result = run_fred(config, epiConfig, interveneConfig, fredPInfection);

    std::cout<<"----------run_episimdemics----------"<<std::endl;
    std::vector<int> episim_result = run_episimdemics(config, epiConfig, interveneConfig, epiSimPInfection);

    std::string save_path = "./logs/infection/";
    std::fstream f(save_path + countyName +".csv", std::ios::out);
    f<<"HMES\tepiFast\tfastSIR\tFRED\tEpiSim\n";
    for (int day=0;day<SIM_DAY;day++){
        f<<std::to_string(hmes_result[day])<<"\t"<<std::to_string(epifast_result[day])<<"\t"<<std::to_string(fastSIR_result[day])<<"\t"<<std::to_string(fred_result[day])<<"\t"<<std::to_string(episim_result[day])<<"\n";
    }
    f.close();
    return 0;
    }

int main(){
    // tiny examples:
    run_methods(1000, 30, "virtual");

    // small cities
     std::vector<unsigned int> s_manNumList = {103009, 116111, 94258, 136606, 88880};
     std::vector<int> s_locNumList = {23, 29, 24,36,23};
     std::vector<std::string> s_nameList = {"PA_adam", "pa_Lycoming", "pa_Northumberland", "pa_Fayette", "pa_Indiana"};

    // medium cities
     std::vector<int> m_manNumList = {320918, 349497, 411442, 519445, 625249};
     std::vector<int> m_locNumList = {104, 76, 90, 98,143};
     std::vector<std::string> m_nameList = {"pa_Luzerne", "pa_Lehigh", "pa_Berks", "pa_Lancaster", "pa_Bucks"};

    //large cities
     std::vector<int> l_manNumList = {1216045, 1226698, 1958578, 2054475, 799874};
     std::vector<int> l_locNumList = {402, 218, 366, 357, 211};
     std::vector<std::string> l_nameList = {"pa_Allegheny", "tx_Travis",  "tx_Bexar", "tx_Tarrant", "tx_Montgomery"};

     for (int epoch =0; epoch<s_nameList.size();++epoch){
         run_methods(s_manNumList[epoch], s_locNumList[epoch], s_nameList[epoch]);
     }

    return 0;
}
