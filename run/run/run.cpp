#include "engine/intervene.h"
using namespace Simulator;
#define SIM_DAY 10
#define MAN_NUM 9999
#define LOC_NUM 99


int main(){

    auto *config = new Config();
    auto *interveneConfig = new InterveneConfig();
    auto *epiConfig = new EpiConfig();
    //config
    config->manNum = MAN_NUM;
    config->houseLocNum = LOC_NUM/3;
    config->workLocNum = LOC_NUM/3;
    config->mallLocNum = LOC_NUM/3;
    config->locNum = config->houseLocNum + config->workLocNum + config->mallLocNum;
    config->hoursPerDay = 14; //hours per day are supported by 14 only!
    config->stepToTrack  = 28;
    config->startIdx = 3;

    config->followMallProb = 0.5;
    config->followTimeProb = 0.5;
    config->OVERLAP_RATIO = 0;

    config->isTraceContact = false; //for network-based
    config->isTraceTrajectory = true;//for location-based method

    //intervention config
    config->isSetIntervention = false;
    interveneConfig->interveneType = 2;
    interveneConfig->fastTraceContact = true;
    interveneConfig->traceContactOrder = 2;
    interveneConfig->treatHours  = 280;
    interveneConfig->confineHours = 14;
    interveneConfig->isolateHours = 14;
    // int interveneStartDay = 20;
    //epidemic config
    epiConfig->incubationHours = 56; // Incubation Period
    epiConfig->init_infected_num = 10;
    epiConfig->pInfection = 0.02;
    epiConfig->pAcqtInfection = 0.00;
    //intervention config
    config->isSetIntervention = false;
    interveneConfig->interveneType = 2;
    interveneConfig->fastTraceContact = true;
    interveneConfig->traceContactOrder = 2;
    interveneConfig->treatHours  = 280;
    interveneConfig->confineHours = 14;
    interveneConfig->isolateHours = 14;

    auto endTime1 = std::chrono::high_resolution_clock::now();

    auto *mg = new MobilityGenerator(config, epiConfig);
    int tracing_step = 1;
    std::cout<<"tracing steps: "<<tracing_step<<"\n";
    mg->generateDayInfection(2);
    for (auto day =0; day<10; ++day){
        mg->generateDayInfection(1);
        std::cout <<"number of incubation period: "<< mg->getIncubationCount() << "\t number of infected:" << mg->getInfectedCount() << "\n";
    }

    auto endTime2 = std::chrono::high_resolution_clock::now();
    float totalElapsedTime = ((float)(endTime2 - endTime1).count())/1000000.0F;
    std::cout << "Elapsed Time: " << totalElapsedTime/1000 << "s" << std::endl;



}
