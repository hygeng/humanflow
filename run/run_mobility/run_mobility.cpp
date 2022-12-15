#include "utilities/mobility.h"

using namespace Simulator;

int run_ours(){
    std::cout<<"----------run HMES----------"<<std::endl;
    auto *config = new Config();
    config->followMallProb = 1.0;
    config->followTimeProb  = 1.0;
    config->hoursPerDay = 14;
    config->stepToTrack = 28;
    config->isTraceTrajectory  = true;
    config->isTraceContact = false;
    config->manNum = 99;
    config->locNum = 9;
    config->houseLocNum = config->locNum/3;
    config->workLocNum = config->locNum/3;
    config->mallLocNum = config->locNum/3;

    auto startTime = std::chrono::high_resolution_clock::now();

    MobilityGenerator MobilityGenerator0(config);

    auto endTime1 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime1 - startTime).count())/1000000.0F;
    std::cout << "Init Time: " << totalElapsedTime/1000 << "s" << std::endl;

    MobilityGenerator0.generateDayTrajectory(10);

    auto endTime2 = std::chrono::high_resolution_clock::now();
//    std::cout << "Total Step: " << MobilityGenerator0::getGlobalStep() << std::endl;
    totalElapsedTime = ((float)(endTime2 - endTime1).count())/1000000.0F;
    std::cout << "Running Time: " << totalElapsedTime/1000 << "s" << std::endl;

    totalElapsedTime = ((float)(endTime2 - startTime).count())/1000000.0F;
    std::cout << "Total Time: " << totalElapsedTime/1000 << "s" << std::endl;
    return 0;
}

int run_episim(){
    std::cout<<"----------run EpiSim----------"<<std::endl;
    auto *config = new Config();
    config->followMallProb = 0.0;
    config->followTimeProb  = 0.0;
    config->hoursPerDay = 14;
    config->stepToTrack = 70;
    config->isTraceTrajectory  = true;
    config->isTraceContact = false;
    config->manNum = 99;
    config->locNum = 9;

    auto startTime = std::chrono::high_resolution_clock::now();

    EpiSim EpiSim0(config);

    auto endTime1 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime1 - startTime).count())/1000000.0F;
    std::cout << "Init Time: " << totalElapsedTime/1000 << "s" << std::endl;

    EpiSim0.generateDayTrajectory(10);

    auto endTime2 = std::chrono::high_resolution_clock::now();
//    std::cout << "Total Step: " << MobilityGenerator0::getGlobalStep() << std::endl;
    totalElapsedTime = ((float)(endTime2 - endTime1).count())/1000000.0F;
    std::cout << "Running Time: " << totalElapsedTime/1000 << "s" << std::endl;

    totalElapsedTime = ((float)(endTime2 - startTime).count())/1000000.0F;
    std::cout << "Total Time: " << totalElapsedTime/1000 << "s" << std::endl;
    return 0;
}

int run_epifast(){
    std::cout<<"----------run EpiFast----------"<<std::endl;
    auto *config = new Config();
    config->hoursPerDay = 1;
    config->stepToTrack = 1;
    config->isTraceTrajectory  = false;
    config->isTraceContact = true;
    config->numContactPerMan  = 1000;
    config->manNum = 99;

    auto startTime = std::chrono::high_resolution_clock::now();

    NetworkGenerator NetworkGenerator0(config);
    std::unordered_map<unsigned int, Man*> idx2Man = NetworkGenerator0.getIdx2Man();

    auto endTime1 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime1 - startTime).count())/1000000.0F;
    std::cout << "Init Time: " << totalElapsedTime/1000 << "s" << std::endl;

    for(int hour=0; hour<140; hour++){
        auto hourtime1 = std::chrono::high_resolution_clock::now();

        //network-based model
        NetworkGenerator0.generateHourContact(1);

        auto hourtime2 = std::chrono::high_resolution_clock::now();
        float hourlyTime = ((float)(hourtime2 - hourtime1).count())/1000000.0F;
    }

    auto endTime2 = std::chrono::high_resolution_clock::now();
    totalElapsedTime = ((float)(endTime2 - endTime1).count())/1000000.0F;
    std::cout << "Running Time: " << totalElapsedTime/1000 << "s" << std::endl;

    totalElapsedTime = ((float)(endTime2 - startTime).count())/1000000.0F;
    std::cout << "Total Time: " << totalElapsedTime/1000 << "s" << std::endl;
    return 0;
}

int main(){
    run_ours();
    run_episim();
    run_epifast();
    return 0;
}