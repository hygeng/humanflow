#include "utilities/mobility.h"
#include <cmath>
#define RD_PRECISION 1e9
//
namespace Simulator {
    MobilityGenerator::MobilityGenerator(Config *config_)
    :globalStep(-1)
    {
        config = config_;
        build_mobility_class();

        epiConfig = new Simulator::EpiConfig();
        build_epidemic_class();
    }

    bool MobilityGenerator::generateLocation(unsigned short LocNum, int typeIdx, Config *config_){
        for(unsigned short i = 0;i < LocNum; ++i){
            unsigned short mapIdx = idx2Loc.size() + config_->startIdx;
            auto *loc = new Location(mapIdx, typeIdx, config_);
            idx2Loc[mapIdx] = loc;
            switch(typeIdx){
                case 0: {
                    listLoc0.emplace_back(mapIdx); break;}
                case 1: {
                    listLoc1.emplace_back(mapIdx); break;}
                case 2: {
                    listLoc2.emplace_back(mapIdx); break;}
                }
            }
        return true;
    }
//
    bool MobilityGenerator::generateMan(unsigned int manNum, Config *config_){
        for(unsigned int i = config_->startIdx;i < manNum+config_->startIdx; ++i){
            Man *man = new Man(i, config_);
            idx2Man[i] = man;
        }
        return true;
    }
//
    unsigned short MobilityGenerator::matchMan2Loc(Man* curMan,int typeIdx){
        unsigned short locIdx = config->startIdx;
        int useful_loc_num = int(listLoc0.size() * (1- config->OVERLAP_RATIO));
        if(useful_loc_num == 0) locIdx = config->startIdx;
        else{
            switch(typeIdx){
                case 0:  {locIdx = listLoc0[xorshf96() % int((listLoc0.size() * (1- config->OVERLAP_RATIO)))];break;}
                case 1:  {locIdx = listLoc1[xorshf96() % int((listLoc1.size() * (1- config->OVERLAP_RATIO)))];break;}
                case 2:  {locIdx = listLoc2[xorshf96() % int((listLoc2.size() * (1- config->OVERLAP_RATIO)))];break;}
            }
        }


        return locIdx;
    }
//
//
    bool MobilityGenerator::generateInitRoutine() {
        for (unsigned int manIdx =config->startIdx; manIdx<config->startIdx+ idx2Man.size();++manIdx){
            //home
            Man *curMan = idx2Man[manIdx];
            unsigned short locIdx = MobilityGenerator::matchMan2Loc(curMan,0);
            curMan->dailyRoutine[0] = locIdx;
            // idx2Loc[locIdx]->loc2ManRoutine.emplace_back(manIdx);
            // idx2Loc[locIdx]->loc2ManRoutine.insert(manIdx);
            //work
            locIdx = MobilityGenerator::matchMan2Loc(curMan, 1);
            curMan->dailyRoutine[1] = locIdx;
            // idx2Loc[locIdx]->loc2ManRoutine.emplace_back(manIdx);
            // idx2Loc[locIdx]->loc2ManRoutine.insert(manIdx);
            //mall
            locIdx = MobilityGenerator::matchMan2Loc(curMan,2);
            curMan->dailyRoutine[2] = locIdx;
            // idx2Loc[locIdx]->loc2ManRoutine.emplace_back(manIdx);
            // idx2Loc[locIdx]->loc2ManRoutine.insert(manIdx);
        }

        return true;
    }

    bool MobilityGenerator::generateHourTrajectory(int totalHours) {
        for (int hour =0; hour<totalHours; ++hour) {
            globalStep++;
            unsigned int step_idx = globalStep % (config->stepToTrack);
            //clear the location population
            for (unsigned short locIdx=config->startIdx; locIdx< config->startIdx + idx2Loc.size();locIdx++){
                Location *curLoc = idx2Loc[locIdx];
                curLoc->loc2ManIncre[step_idx].clear();
            }

            for (unsigned int manIdx = config->startIdx; manIdx <config->startIdx+ idx2Man.size(); ++manIdx) {// manIdx for man
                Man *curMan = idx2Man[manIdx];

                //get strategy
                int curStrategy = curMan->getInterveneStrategy();

                //free
                if(curStrategy==0){
                    int placeId = MobilityGenerator::hour2Routine(curMan, step_idx);
                    unsigned short locIdx = curMan->dailyRoutine[placeId];//routine place
                    //find where indeed curMan goes
                    if(curMan->isFollowRoutineDuration && (placeId!=2 || curMan->isFollowMallRoutine)){
                        //non-incremental
                        idx2Loc[locIdx]->loc2ManIncre[step_idx].emplace_back(manIdx);
                        curMan->man2LocIncre[step_idx] = 0; }
                    else{//incremental
                        if (placeId ==2 && !curMan->isFollowMallRoutine){
                            locIdx = curMan->todayMallLoc;
                        }
                        idx2Loc[locIdx]->loc2ManIncre[step_idx].emplace_back(manIdx);
                        curMan->man2LocIncre[step_idx] = locIdx;
                    }
                    continue;
                }

                //not free
                else if(curStrategy == 1){ //confine, stay at home all day
                    if((globalStep+1)%config->hoursPerDay==0){
                        curMan->strategyHours-=14;
                        if(curMan->strategyHours <= 0){//person is back to freedom again
                            //back to no intervention strategy
                            curMan->interveneStrategy[curStrategy] = false;
                            curMan->interveneStrategy[0] = true;
                        }
                    }
                    int placeId = MobilityGenerator::hour2Routine(curMan, step_idx);
                    unsigned short locIdx = curMan->dailyRoutine[0];
                    idx2Loc[locIdx]->loc2ManIncre[step_idx].emplace_back(manIdx);
                    if(placeId==0) curMan->man2LocIncre[step_idx] = 0;
                    else curMan->man2LocIncre[step_idx] = locIdx;
                    continue;
                }

                else if(curStrategy == 2){  //isolated, stay away from other people
                    if((globalStep+1)%config->hoursPerDay==0){
                        curMan->strategyHours -= 14;
                        if(curMan->strategyHours <= 0){//person is back to freedom again
                            //back to no intervention strategy
                            curMan->interveneStrategy[curStrategy] = false;
                            curMan->interveneStrategy[0] = true;
                        }
                    }
                    curMan->man2LocIncre[step_idx] = 1;//isolation house
                    continue;
                }

                else if(curStrategy ==3){
                    if((globalStep+1)%config->hoursPerDay==0){ //hospitalized, take medical care
                        curMan->strategyHours -=14;
                        if(curMan->strategyHours <= 0){//person is recovered; back to freedom again 
                            //back to no intervention strategy
                            curMan->interveneStrategy[curStrategy] = false;
                            curMan->interveneStrategy[0] = true;
                            int curHS = curMan->getHealthStatus();
                            curMan->healthStatus[curHS] = false; //cancel current status
                            curMan->healthStatus[3] = true; //come to recovered
                            daily_recovered_cnt++;
                            //update infected list
                            eraseInfectedIdxList(manIdx);
                        }
                    }
                    curMan->man2LocIncre[step_idx] = 0; //hospital
                    continue;
                }
            }
        }
        return true;
    }

    bool MobilityGenerator::generateDayTrajectory(int totalDays) {
        for(int day =0;day<totalDays; ++day){
            MobilityGenerator::setTodayDestination();
            MobilityGenerator::setTodayDuration();
            MobilityGenerator::generateHourTrajectory(config->hoursPerDay);
        }
        return true;
    }

    int MobilityGenerator::hour2Routine(Man* curMan, unsigned int hour) {
        hour = hour % config->hoursPerDay;
        int placeIdx = 0;
        std::vector<unsigned short>& durations = curMan->dailyDuration;
        if(!curMan->isFollowRoutineDuration){
            durations = curMan->abnormalDuration;
        }

        while(hour >= durations[placeIdx]){
            hour -= durations[placeIdx++];
        }
        return placeIdx;
    }


    bool MobilityGenerator::setTodayDuration() {
        if(std::fabs(1-config->followTimeProb)<0.001){//curMan ->isFollowRoutineDuration = true;
            return true;
        }

        for (unsigned int manIdx =config->startIdx; manIdx<config->startIdx+ idx2Man.size();++manIdx) {
            Man *curMan = idx2Man[manIdx];
            float follow_prob = xorshf96() % (random_N + 1) / float(random_N + 1);
            if (follow_prob > config->followTimeProb) {//do not follow routine
                curMan->isFollowRoutineDuration = false;
                short duration0 = xorshf96() % config->hoursPerDay;
                short duration1 = xorshf96() % (config->hoursPerDay - duration0);
                curMan->abnormalDuration[0] = duration0;
                curMan->abnormalDuration[1] = duration1;
                curMan->abnormalDuration[2] = config->hoursPerDay - duration0 - duration1;
            }else{
                curMan->isFollowRoutineDuration = true;
            }
        }
        return true;
    }

    bool MobilityGenerator::setTodayDestination() {
        if(std::fabs(1-config->followMallProb)<0.001){//curMan->isFollowMallRoutine = true;
            return true;
        }

        for (unsigned int manIdx =config->startIdx; manIdx<1+ idx2Man.size();++manIdx) {
            Man *curMan = idx2Man[manIdx];
            float follow_prob = xorshf96() % (random_N + 1) / float(random_N + 1);
            if (follow_prob > config->followMallProb) {//person does not follow routine
                unsigned short locIdx = MobilityGenerator::matchMan2Loc(curMan, 2);
                curMan->todayMallLoc = locIdx;
                curMan->isFollowMallRoutine = false;
            } else {
                curMan->isFollowMallRoutine =true;
            }
        }
        return true;
    }

    std::vector<std::vector<unsigned int>>
    MobilityGenerator::getLoc2ManHis(Location* curLoc, unsigned int currentStep, unsigned int stepsBack){
        unsigned int actualStepsBack = std::min(stepsBack, currentStep);
        std::vector<std::vector<unsigned int>> tmp;
        tmp.reserve(actualStepsBack);

        for (unsigned int step=currentStep-actualStepsBack; step < currentStep; step++)
        {
            tmp.emplace_back(getHourLoc2Man(curLoc, step));
        }
        return tmp;
    }

    std::vector<unsigned int>
    MobilityGenerator::getHourLoc2Man(Location* curLoc, unsigned int queryStep) {
        unsigned int step_idx = queryStep % (config->stepToTrack);
        return curLoc->loc2ManIncre[step_idx];
    }


    std::unordered_map<unsigned int, Man*>& MobilityGenerator::getIdx2Man() {
        return idx2Man;
    }


    std::unordered_map<unsigned short, Location*>& MobilityGenerator::getIdx2Loc() {
        return idx2Loc;
    }


    bool MobilityGenerator::hour2IsOpen(unsigned int hour, int placeId) {
        switch(placeId){
            case 0:{return (hour>=0 && hour<4);   break;}
            case 1:{return (hour>=4 && hour<12);   break;}
            case 2:{return (hour>=12 && hour<14);   break;}
        }
        return false;
    }

    bool MobilityGenerator::setIntervention(unsigned int manIdx, int interventionType, int contHours) {
        if(interventionType<0 || interventionType>4 || !isValidManIdx(manIdx)) return false;
        Man *curMan = idx2Man[manIdx];
        int curIntType = curMan->getInterveneStrategy();
        if(interventionType > curIntType  ){ //upgrade intervention level if necessary; else remain unchanged
            curMan->interveneStrategy[curIntType] = false;
            curMan->interveneStrategy[interventionType] = true;
            curMan->strategyHours = contHours; //update leaving days
        }
        return true;
    }


    std::vector<unsigned short> MobilityGenerator::getMan2LocHis(Man* curMan, unsigned int currentStep, unsigned int stepsBack){
        unsigned int actualStepsBack = std::min(stepsBack, currentStep);
        std::vector<unsigned short> tmp;
        tmp.reserve(actualStepsBack);

        for (unsigned int step=currentStep-actualStepsBack; step < currentStep; step++)
        {
            tmp.emplace_back(getHourMan2Loc(curMan, step));
        }
        return tmp;
    }

    unsigned short MobilityGenerator::getHourMan2Loc(Man *curMan, unsigned int queryStep) {
        unsigned int step_idx = queryStep % (config->stepToTrack);
        //totally follow routine
        if(std::fabs(1-config->followMallProb)<0.001 && std::fabs(1-config->followTimeProb)<0.001){
            int placeId = hour2Routine(curMan, step_idx);
            return curMan->dailyRoutine[placeId];
        }
        //end

        if (curMan->man2LocIncre[step_idx] != 0)
            return curMan->man2LocIncre[step_idx];

        int placeId = hour2Routine(curMan, queryStep);
        return curMan->dailyRoutine[placeId];
    }

    bool MobilityGenerator::generateAllLocation( Config *config_) {
        generateLocation(config->houseLocNum,0, config);
        generateLocation(config->workLocNum,1, config);
        generateLocation(config->mallLocNum,2, config);
        return true;
    }

    Man *MobilityGenerator::getManIdx2Pointer(unsigned int manIdx) {
        if(manIdx<config->startIdx) return nullptr;
        return idx2Man[manIdx];
    }

    Location *MobilityGenerator::getLocIdx2Pointer(unsigned short locIdx) {
        if(locIdx<config->startIdx) return nullptr;
        return idx2Loc[locIdx];
    }

    bool MobilityGenerator::setInterventionGroup(std::vector<unsigned int> &concernedGroup, int interventionType,
                                                    int contHour) {
        for (auto manIdx: concernedGroup){
            setIntervention(manIdx, interventionType, contHour);
        }
        return true;
    }

    MobilityGenerator::MobilityGenerator()
    :globalStep(-1)
    {
        config = new Simulator::Config();
        build_mobility_class();

        epiConfig = new Simulator::EpiConfig();
        build_epidemic_class();
    }

    unsigned int MobilityGenerator::getManNum() {
        return idx2Man.size();
    }

    unsigned int MobilityGenerator::getLocNum() {
        return idx2Loc.size();
    }

    unsigned short MobilityGenerator::PyGetHourMan2Loc(unsigned int manIdx, unsigned int queryStep) {
        return getHourMan2Loc(idx2Man[manIdx], queryStep) ;
    }

    std::vector<unsigned int> MobilityGenerator::PyGetHourLoc2Man(unsigned short locIdx, unsigned int queryStep) {
        return getHourLoc2Man(idx2Loc[locIdx], queryStep);
    }

    std::vector<unsigned short>
    MobilityGenerator::PyGetMan2LocHis(unsigned int manIdx, unsigned int currentStep, unsigned int stepsBack) {
        return getMan2LocHis(idx2Man[manIdx], currentStep, stepsBack);
    }

    std::vector<std::vector<unsigned int>>
    MobilityGenerator::PyGetLoc2ManHis(unsigned short locIdx, unsigned int currentStep, unsigned int stepsBack) {
        return getLoc2ManHis(idx2Loc[locIdx], currentStep, stepsBack);
    }


    bool MobilityGenerator::build_mobility_class() {
        globalStep =-1;
        generateAllLocation(config);
        generateMan(config->manNum, config);
        generateInitRoutine();
        std::cout<<"Finished generating "<<config->locNum<<" location & "<< config->manNum<<" man\n";
        return true;
    }

    MobilityGenerator::MobilityGenerator(std::unordered_map<std::string, std::string> configMap) {
        config = map2Config(configMap);
        config->locNum = config->houseLocNum + config->workLocNum + config->mallLocNum;
        build_mobility_class();
    }

    bool MobilityGenerator::setInitInfection(std::vector<unsigned int> initInfectedMan) {
        for(auto manIdx:initInfectedMan){
            IncubationIdxList.push_back(manIdx);
            // InfectedIdxList.push_back(manIdx);
            Man* man = idx2Man[manIdx];
            man->healthStatus[0] = false;
            man->healthStatus[1] = true; //incubation
            man->incubationHourLeft = epiConfig->incubationHours - config->hoursPerDay - 1;
        }
        return true;
    }

    bool MobilityGenerator::setDailyInfection() {
        for(auto manIdx : dailyIncubationIdxList){
            Man* man= idx2Man[manIdx];
            man->healthStatus[man->getHealthStatus()] = false;
            man->healthStatus[1] = true; //incubation
            man->isInDailyInfectedList = false;
            man->incubationHourLeft = epiConfig->incubationHours - config->hoursPerDay;
            IncubationIdxList.push_back(manIdx);
        }
        dailyIncubationIdxList.clear();

        return true;
    }

    bool MobilityGenerator::computeInfection(const std::vector<unsigned int> &HourManInLoc) {
        //count health status
        auto stateCnt = countGroupSEIR(HourManInLoc);
        //compute infection
        unsigned int man_num = HourManInLoc.size();
        if(stateCnt[1] + stateCnt[2]==0) {
            return true;  //there's no one infected in this location
        }

        auto s = float(stateCnt[0]);
        auto i = float(stateCnt[1] + stateCnt[2]);
        auto r = float(stateCnt[3]);
        float infectionRate = epiConfig->pInfection * i / (s+i+r);
        int infectionNum = int( s * infectionRate);

        if(infectionNum==0){
            for (unsigned int curManIdx : HourManInLoc){
                if(getManHealthStatus(curManIdx) ==0 && !idx2Man[curManIdx]->isInDailyInfectedList){  //susceptible and not infected yet
                    int ran_num = xorshf96()%int(RD_PRECISION);
                    if(ran_num < infectionRate*RD_PRECISION){
                        idx2Man[curManIdx]->isInDailyInfectedList=true;
                        dailyIncubationIdxList.push_back(curManIdx);
                        cur_infected_cnt++;
                    }
                }
            }
        }
        else{
            //random
            int newly_infected_cnt = 0;
            while(newly_infected_cnt < infectionNum){
                int ran_num = xorshf96()%man_num;
                unsigned int curManIdx = HourManInLoc[ran_num];
                if(getManHealthStatus(curManIdx) ==0 && !idx2Man[curManIdx]->isInDailyInfectedList){
                    idx2Man[curManIdx]->isInDailyInfectedList=true;
                    dailyIncubationIdxList.push_back(curManIdx);
                    cur_infected_cnt++;
                    newly_infected_cnt++;
                }
            }

        }
        return true;
    }

    int MobilityGenerator::getInfectedCount() {
        return InfectedIdxList.size();
    }

    std::vector<unsigned int>& MobilityGenerator::getInfectedIdxList() {
        return InfectedIdxList;
    }

    std::vector<unsigned int>& MobilityGenerator::getIncubationIdxList() {
        return IncubationIdxList;
    }

    bool MobilityGenerator::addInfectedIdxList(unsigned int manIdx) {
        InfectedIdxList.emplace_back(manIdx);
        return true;
    }

    bool MobilityGenerator::eraseInfectedIdxList(int position) {
        InfectedIdxList.erase(InfectedIdxList.begin() + position);
        return true;
    }

    bool MobilityGenerator::eraseIncubationIdxList(int position) {
        IncubationIdxList.erase(IncubationIdxList.begin() + position);
        return true;
    }

    bool MobilityGenerator::minusCurInfectedNum(int recovered_cnt) {
        cur_infected_cnt-=recovered_cnt;
        return true;
    }

    bool MobilityGenerator::setManHealthStatus(unsigned int manIdx, int newStatus) {
        idx2Man[manIdx]->setHealthStatus(newStatus);
        return true;
    }

    int MobilityGenerator::getManIncubationHourLeft(unsigned int manIdx) {
        return idx2Man[manIdx]->incubationHourLeft;
    }

    bool MobilityGenerator::DecreaseManIncubationDayLeft(unsigned int manIdx) {
        idx2Man[manIdx]->incubationHourLeft -= config->hoursPerDay;
        return true;
    }

    int MobilityGenerator::getDailyRecoveredCount() {
        return daily_recovered_cnt;
    }

    bool MobilityGenerator::generateDayInfection(int sim_days) {
        for(int day=0; day<sim_days; day++) {
            daily_recovered_cnt = 0;
            int globalDay = (globalStep+1) / config->hoursPerDay;
            std::cout<<"day:"<<globalDay<<"\t";
            setTodayDestination();
            setTodayDuration();
            //update incubation
            if (globalDay >0) {
                updateIncubationDaily();
            }
            // update acqt group incubation
            for (unsigned int manIdx = config->startIdx; manIdx< config->startIdx + config->manNum; manIdx++){
                generateAcqtInfection(manIdx);
            }
            // update location incubation
            for(int hour=0; hour<config->hoursPerDay; hour++){
                generateHourInfection(1);
                }
            // daily, transform incubation to infected
            setDailyInfection();
        }
        return true;
    }

    bool MobilityGenerator::build_epidemic_class() {
        //initial infections
        std::vector<unsigned int> initInfectedMan;
        for(int manIdx=config->startIdx; manIdx<config->startIdx + epiConfig->init_infected_num; manIdx++){
            initInfectedMan.push_back(manIdx);
        }
        setInitInfection(initInfectedMan); //set initial infected man

        cur_infected_cnt = epiConfig->init_infected_num;
        return true;
    }

    std::vector<unsigned int> MobilityGenerator::countGroupSEIR(const std::vector<unsigned int>& GroupOfPeople) {
        stateCnt = {0,0,0,0};
        for (unsigned int manIdx : GroupOfPeople) {
            int manHS = getManHealthStatus(manIdx);
            stateCnt[manHS]+=1;
        }
        return stateCnt;
    }

    int MobilityGenerator::getManHealthStatus(unsigned int manIdx) {
        return idx2Man[manIdx]->getHealthStatus();
    }

    MobilityGenerator::MobilityGenerator(std::unordered_map<std::string, std::string>& configMap,
                                         std::unordered_map<std::string, std::string>& EpiConfigMap) {
        config = map2Config(configMap);
        config->locNum = config->houseLocNum + config->workLocNum + config->mallLocNum;
        build_mobility_class();

        epiConfig = map2EpiConfig(EpiConfigMap);
        build_epidemic_class();
    }

    bool MobilityGenerator::isValidManIdx(unsigned int manIdx) {
        return (manIdx >=config->startIdx) && (manIdx< config->startIdx + idx2Man.size());
    }

    bool MobilityGenerator::isValidLocIdx(unsigned short locIdx) {
        return (locIdx >=config->startIdx) && (locIdx< config->startIdx + idx2Loc.size());;
    }

    bool MobilityGenerator::setManIncubation(unsigned int manIdx) {
        auto *curMan = idx2Man[manIdx];
        curMan->healthStatus[curMan->getHealthStatus()] = false;
        curMan->healthStatus[1] = true; //incubation
        curMan->incubationHourLeft = epiConfig->incubationHours - config->hoursPerDay;
        IncubationIdxList.emplace_back(manIdx);
        return true;
    }

    int MobilityGenerator::getIncubationCount() {
        return IncubationIdxList.size();
    }

    bool MobilityGenerator::reset() {
        //clear health status
        for (unsigned int manIdx=config->startIdx; manIdx< config->startIdx + idx2Man.size(); ++manIdx){
            setManHealthStatus(manIdx,0);
            idx2Man[manIdx]->incubationHourLeft = 0;
        }
        //clear vectors
        idx2Man.clear();
        idx2Loc.clear();
        std::vector<unsigned short>().swap(listLoc0);
        std::vector<unsigned short>().swap(listLoc1);
        std::vector<unsigned short>().swap(listLoc2);
        std::vector<unsigned int>().swap(InfectedIdxList);
        std::vector<unsigned int>().swap(IncubationIdxList);
        std::vector<unsigned int>().swap(dailyIncubationIdxList);
        stateCnt= { 0, 0, 0, 0 };
        cur_infected_cnt = 0;
        daily_recovered_cnt = 0;
        globalStep = -1;

        //rebuild
        build_mobility_class();
        build_epidemic_class();
        return true;
    }

    bool Simulator::MobilityGenerator::generateHourInfection(int hours) {
        generateHourTrajectory(1);
        // int hourly_recovered = getDailyRecoveredCount();
        // total_recovered += hourly_recovered;
        // minusCurInfectedNum(hourly_recovered);
        for(unsigned short locIdx= config->startIdx; locIdx< config->startIdx + config->locNum; locIdx++) {
            auto hourloc2man = getHourLoc2Man(idx2Loc[locIdx], getGlobalStep());
            if (hourloc2man.size() == 0) continue;
            computeInfection(hourloc2man);
        }
        return true;
    }

    bool MobilityGenerator::updateIncubationDaily() {
        dailyInfectedIdxList.clear();
        for (int position = IncubationIdxList.size() - 1; position >= 0; position--) { //must not use unsigned int index, otherwise get bug
            int manIdx = IncubationIdxList[position];
            if (getManIncubationHourLeft(manIdx) <= 0) { //incubation period end, turn to symptomatic
                eraseIncubationIdxList(position);
                dailyInfectedIdxList.emplace_back(manIdx);
                addInfectedIdxList(manIdx);
                setManHealthStatus(manIdx, 2);
            } else DecreaseManIncubationDayLeft(manIdx);
        }
        return true;
    }

    std::vector<unsigned int>& MobilityGenerator::getAcqtGroup(unsigned int manIdx) {
        return idx2Man[manIdx]->acqtGroup;
    }

    bool MobilityGenerator::generateAcqtInfection(unsigned int manIdx) {
        int hs = getManHealthStatus(manIdx); //health status
        int is = idx2Man[manIdx]->getInterveneStrategy(); //intervention strategy
        if (hs==0  || hs==3 || is>1 ){return false;}

        auto manAcqtGroup = getAcqtGroup(manIdx);
        for (unsigned int acqtManIdx:manAcqtGroup){
            int ran_num = xorshf96()%int(RD_PRECISION);
            if(ran_num < epiConfig->pAcqtInfection * RD_PRECISION && getManHealthStatus(acqtManIdx) ==0 && !idx2Man[acqtManIdx]->isInDailyInfectedList){
                idx2Man[acqtManIdx]->isInDailyInfectedList=true;
                dailyIncubationIdxList.push_back(acqtManIdx);
                cur_infected_cnt++;
            }
        }
        return true;
    }

    MobilityGenerator::MobilityGenerator(Config *config_, EpiConfig *epiConfig_) {
        globalStep = -1;
        config = config_;
        build_mobility_class();

        epiConfig = epiConfig_;
        build_epidemic_class();
    }

    std::vector<unsigned int> &MobilityGenerator::getDailyInfectedIdxList() {
        return dailyInfectedIdxList;
    }

    std::vector<unsigned int> MobilityGenerator::getman2loc2man(unsigned int manIdx,unsigned int currentStep, unsigned int steps_back) {
        std::vector<unsigned int> man2loc2man;
        if (!isValidManIdx(manIdx)) return man2loc2man;
        unsigned int actualStepsBack = std::min(steps_back, currentStep);

        for (unsigned int step=currentStep-actualStepsBack; step < currentStep; step++)
        {
            unsigned short stepLocIdx = getHourMan2Loc(idx2Man[manIdx], step);
            std::vector<unsigned int> stepManIdxList = PyGetHourLoc2Man(stepLocIdx,step);
            if(stepManIdxList.size()==0) continue;
            man2loc2man.insert(man2loc2man.end(),stepManIdxList.begin(), stepManIdxList.end());
        }
        std::sort(man2loc2man.begin(),man2loc2man.end());
        man2loc2man.erase(std::unique(man2loc2man.begin(), man2loc2man.end()),man2loc2man.end());
        return man2loc2man;
    }

    unsigned int Simulator::NetworkGenerator::getGlobalStep() const {
        return globalStep;
    }

    NetworkGenerator::NetworkGenerator():globalStep(-1)
    {
        auto *config_ = new Config();
        config = config_;

        generateMan(config->manNum,config);
        generateRoutineContact();
        std::cout<<"Finished generating man\n";
    }
    NetworkGenerator::NetworkGenerator(Config *config_)
    :globalStep(-1)
     {
        config = config_;
        generateMan(config->manNum,config);
        generateRoutineContact();
        std::cout<<"Finished generating "<<config->manNum<<" man\n";
    }

    std::unordered_map<unsigned int, Man*> NetworkGenerator::getIdx2Man() {
        return idx2Man;
    }

    bool NetworkGenerator::generateMan(unsigned int manNum, Config *config_) {
        unsigned int mapIdx = NetworkGenerator::idx2Man.size()+ config_->startIdx;
        for(unsigned int i = 0;i < manNum; ++i){
            Man *man = new Man(mapIdx, config_);
            NetworkGenerator::idx2Man[mapIdx++] = man;
        }
        return true;
    }

    bool NetworkGenerator::generateHourContact(int totalHours){
        for (int hour =0;hour<totalHours; ++hour){
            globalStep++;
            unsigned int step_idx = globalStep % (config->stepToTrack);
            for (unsigned int manIdx = config->startIdx; manIdx<config->startIdx + idx2Man.size();++manIdx) {/// manIdx for man
                Man *curMan = idx2Man[manIdx];
                std::vector<unsigned int>().swap(curMan->contactHis[step_idx]);

                //cur man has intervention strategy
                //get strategy
                int curStrategy = curMan->getInterveneStrategy();

                //not free
                if(curStrategy !=0){
                    //update intervention strategy for man
                    curMan->strategyHours--;
                    if(curMan->strategyHours <= 0){//person is back to freedom again
                        //back to no intervention strategy
                        curMan->interveneStrategy[curStrategy] = false;
                        curMan->interveneStrategy[0] = true;
                    }
                    if(curStrategy == 4){
                        curMan->contactHis[step_idx].emplace_back(0); // hospital
                        continue;
                    }
                    else if(curStrategy == 2){
                        curMan->contactHis[step_idx].emplace_back(1); // isolate
                        continue;
                    }
                }
                float follow_prob = xorshf96() % (random_N + 1) / float(random_N + 1);

                if(follow_prob > config->followContactProb){
//                    curMan->contactHis[step_idx].reserve(config->numContactPerMan+3);
                    for(int contact_idx=0; contact_idx<config->numContactPerMan; contact_idx++){
                        curMan->contactHis[step_idx].emplace_back(xorshf96()%config->manNum + config->startIdx);
                    }
                }
                else{
                    std::vector<unsigned int>().swap(curMan->contactHis[step_idx]);
                }
            }
        }
        return true;
    }

    bool NetworkGenerator::generateDayContact(int totalDays) {
        for(int day =0;day<totalDays; ++day){
            std::cout<<"Day "<<day<<std::endl;
            NetworkGenerator::generateHourContact(config->hoursPerDay);
        }
        return true;
    }

    std::vector<unsigned int> NetworkGenerator::getHourManContact(Man *curMan,unsigned int step) {
        unsigned int step_idx = step % (config->stepToTrack);
        //if follow routine
        if (curMan->contactHis[step_idx].size()==0)   return curMan->routineContact;

        //if he is intervened, he would not contact anyone
        if(curMan->contactHis[step_idx][0] < config->startIdx) {
            return std::vector<unsigned int>();
        }
        //else pick random contacts generated before
        return curMan->contactHis[step_idx];
    }

    bool NetworkGenerator::generateRoutineContact() {
        for (unsigned int manIdx =config->startIdx; manIdx<config->startIdx+ idx2Man.size();++manIdx) {/// manIdx for man
            Man *curMan = idx2Man[manIdx];
            for(int contact_idx=0;contact_idx<config->numContactPerMan;contact_idx++){
                curMan->routineContact[contact_idx] = (xorshf96()%config->manNum + config->startIdx) ;
            }

        }
        return true;
    }


    bool EpiSim::generateLocation(unsigned short LocNum, int typeIdx, Config *config_) {
        for(unsigned short i = 0;i < LocNum; ++i){
            auto *loc = new Location(i, typeIdx, config);
            unsigned short mapIdx = idx2Loc.size() + config_->startIdx;
            idx2Loc[mapIdx] = loc;
            switch(typeIdx){
            case 0: {
                listLoc0.emplace_back(mapIdx); break;}
            case 1: {
                listLoc1.emplace_back(mapIdx); break;}
            case 2: {
                listLoc2.emplace_back(mapIdx); break;}
            }
        }
        return true;
        }

    bool EpiSim::generateAllLocation(unsigned short LocNum, Config *config_) {
        generateLocation(config->locNum/3,0, config);
        generateLocation(config->locNum/3,1, config);
        generateLocation(config->locNum/3,2, config);
        return true;
    }

    bool EpiSim::generateMan(unsigned int manNum, Config *config_) {
        unsigned int mapIdx = idx2Man.size()+config_->startIdx;
        for(unsigned int i = 0; i < manNum; ++i){
            Man *man = new Man(mapIdx, config_);
            idx2Man[mapIdx++] = man;
        }
        return true;
    }

    unsigned short EpiSim::matchMan2Loc(Man *curMan, int typeIdx) {
        unsigned short locIdx = config->startIdx;
        switch(typeIdx){
            case 0:  {locIdx = listLoc0[xorshf96() % (listLoc0.size())];break;}
            case 1:  {locIdx = listLoc1[xorshf96() % (listLoc1.size())];break;}
            case 2:  {locIdx = listLoc2[xorshf96() % (listLoc2.size())];break;}
        }

        return locIdx;
    }

    int EpiSim::hour2Routine(Man *curMan, int hour) {
        hour = hour % config->hoursPerDay;
        int placeIdx = 0;
        std::vector<unsigned short> durations(curMan->dailyDuration);
        if(!curMan->isFollowRoutineDuration){
            durations = curMan->abnormalDuration;
        }

        while(hour >= durations[placeIdx]){
            hour -= durations[placeIdx++];
        }
        return placeIdx;
    }

    bool EpiSim::generateInitRoutine() {
        for (unsigned int manIdx =config->startIdx; manIdx< config->startIdx+ idx2Man.size();++manIdx){
            //home
            Man *curMan = idx2Man[manIdx];
            unsigned short locIdx = matchMan2Loc(curMan,0);
            curMan->dailyRoutine[0] = locIdx;
            //work
            locIdx = matchMan2Loc(curMan, 1);
            curMan->dailyRoutine[1] = locIdx;
            //mall
            locIdx = matchMan2Loc(curMan,2);
            curMan->dailyRoutine[2] = locIdx;
        }

        return true;
    }

    bool EpiSim::hour2IsOpen(int hour, int placeId) {
        switch(placeId){
            case 0:{return (hour>=0 && hour<4);   break;}
            case 1:{return (hour>=4 && hour<12);   break;}
            case 2:{return (hour>=12 && hour<14);   break;}
        }
        return true;
    }

    EpiSim::EpiSim(Config *config_)
            :globalStep(-1)
    {
        config = config_;
        generateAllLocation(config_->locNum, config_);
        generateMan(config->manNum,config);
        generateInitRoutine();
        std::cout<<"Finished generating "<<config_->locNum<<" location &"<<config->manNum << " man\n";
    }

    unsigned int EpiSim::getGlobalStep() const {
        return globalStep;
    }

    bool EpiSim::generateDayTrajectory(int totalDays) {
        for(int day =0;day<totalDays; ++day){
            setDayRoutine();
            resetRoutineDuration();
            generateHourTrajectory(config->hoursPerDay);
        }
        return true;

    }

    bool EpiSim::generateHourTrajectory(int totalHours) {
        for (int hour =0; hour<totalHours; ++hour) {
            globalStep++;
            unsigned int step_idx = globalStep % (config->stepToTrack);
            //clear the location population
            for (unsigned short locIdx=config->startIdx; locIdx< config->startIdx + idx2Loc.size();locIdx++){
                Location *curLoc = idx2Loc[locIdx];

                curLoc->loc2ManIncre[step_idx].clear();
            }
            for (unsigned int manIdx = config->startIdx; manIdx <config->startIdx+ idx2Man.size(); ++manIdx) {// manIdx for man
                Man *curMan = idx2Man[manIdx];

                //get strategy
                int curStrategy = curMan->getInterveneStrategy();

                //not free
                if(curStrategy !=0){
                    if(curStrategy == 3 || curStrategy == 4){
                        curMan->man2LocIncre[step_idx] = 1; //hospital
                        curMan->strategyHours--;
                        if(curMan->strategyHours <= 0){//person is back to freedom again
                            //back to no intervention strategy
                            curMan->interveneStrategy[curStrategy] = false;
                            curMan->interveneStrategy[0] = true;
                            //update health status
                            int curHS = curMan->getHealthStatus();
                            if(curHS==1){
                                curMan->healthStatus[curHS] = false; //cancel current status
                                curMan->healthStatus[4] = true; //come to recovered
                            }
                        }
                        continue;
                    }
                    if(curStrategy == 2){
                        curMan->man2LocIncre[step_idx] = 2; //isolation place
                        curMan->strategyHours--;
                        if(curMan->strategyHours <= 0){//person is back to freedom again
                            //back to no intervention strategy
                            curMan->interveneStrategy[curStrategy] = false;
                            curMan->interveneStrategy[0] = true;
                        }
                        continue;
                    }
                }

                //free
                int placeId = hour2Routine(curMan, step_idx);
                unsigned short locIdx = matchMan2Loc(curMan,placeId);
                idx2Loc[locIdx]->loc2ManIncre[step_idx].emplace_back(manIdx);
                curMan->man2LocIncre[step_idx] = locIdx;
            }
        }

        return true;
    }

    bool EpiSim::resetRoutineDuration() {
        for (unsigned int manIdx = config->startIdx; manIdx< config->startIdx+ idx2Man.size();++manIdx) {
            if(std::fabs(1-config->followTimeProb)<0.001){//curMan ->isFollowRoutineDuration = true;
                continue;}

            Man *curMan = idx2Man[manIdx];
            float follow_prob = xorshf96() % (random_N + 1) / float(random_N + 1);
            if (follow_prob > config->followTimeProb) {//do not follow routine
                curMan->isFollowRoutineDuration = false;
                short duration0 = xorshf96() % config->hoursPerDay;
                short duration1 = xorshf96() % (config->hoursPerDay - duration0);
                curMan->abnormalDuration[0] = duration0;
                curMan->abnormalDuration[1] = duration1;
                curMan->abnormalDuration[2] = config->hoursPerDay - duration0 - duration1;
            }else{
                curMan->isFollowRoutineDuration = true;
            }
        }
        return true;
    }

    bool EpiSim::setDayRoutine() {
        if(std::fabs(1-config->followMallProb)<0.001){//curMan->isFollowMallRoutine = true;
            return true;
        }

        for (unsigned int manIdx =config->startIdx; manIdx< config->startIdx+ idx2Man.size();++manIdx) {
            Man *curMan = idx2Man[manIdx];
            float follow_prob = xorshf96() % (random_N + 1) / float(random_N + 1);
            if (follow_prob > config->followMallProb) {//person does not follow routine
                unsigned short locIdx = matchMan2Loc(curMan, 2);
                curMan->todayMallLoc = locIdx;
                curMan->isFollowMallRoutine = false;
            } else {
                curMan->isFollowMallRoutine =true;
            }
        }
        return true;
    }

    std::vector<unsigned int> EpiSim::getHourLoc2Man(Location *curLoc, unsigned int queryStep) {
        int placeId = curLoc->getType();
        if(!hour2IsOpen(queryStep % config->hoursPerDay, placeId)) return std::vector<unsigned int>();

        unsigned int step_idx = queryStep % (config->stepToTrack);
        return curLoc->loc2ManIncre[step_idx];

    }

    std::unordered_map<unsigned short, Location *> EpiSim::getIdx2Loc() {
        return idx2Loc;
    }

    std::unordered_map<unsigned int, Man*> EpiSim::getIdx2Man() {
        return idx2Man;
    }

    std::vector<std::vector<unsigned int>>
    NetworkGenerator::getManContactHis(Man *curMan, unsigned int currentStep, unsigned int stepsBack) {
        unsigned int actualStepsBack = std::min(stepsBack, currentStep);
        std::vector<std::vector<unsigned int>> tmp;
        tmp.reserve(actualStepsBack);

        for (unsigned int step=currentStep-actualStepsBack; step < currentStep; step++)
        {
            tmp.emplace_back(getHourManContact(curMan, step));
        }
        return tmp;
    }

    bool NetworkGenerator::setIntervention(unsigned int manIdx, int interventionType, int contDay) {
        if(interventionType<0 || interventionType>5 || manIdx<config->startIdx) return false;
        Man *curMan = idx2Man[manIdx];
        int curIntType = curMan->getInterveneStrategy();
        if(interventionType > curIntType  ){ //upgrade intervention level if necessary
            curMan->interveneStrategy[curIntType] = false;
            curMan->interveneStrategy[interventionType] = true;
            curMan->strategyHours = contDay; //update leaving days
        }
        return true;
    }

    std::vector<unsigned short> EpiSim::getMan2LocHis(Man *curMan, unsigned int currentStep, unsigned int stepsBack) {
        unsigned int actualStepsBack = std::min(stepsBack, currentStep);
        std::vector<unsigned short> tmp;
        tmp.reserve(actualStepsBack);

        for (unsigned int step=currentStep-actualStepsBack; step < currentStep; step++)
        {
            tmp.emplace_back(getHourMan2Loc(curMan, step));
        }
        return tmp;
    }

    unsigned short EpiSim::getHourMan2Loc(Man *curMan, unsigned int queryStep) {
        return curMan->man2LocIncre[queryStep % config->stepToTrack];
    }


    std::vector<std::vector<unsigned int>>
    EpiSim::getLoc2ManHis(Location *curLoc, unsigned int currentStep, unsigned int stepsBack) {
        unsigned int actualStepsBack = std::min(stepsBack, currentStep);
        std::vector<std::vector<unsigned int>> tmp;
        tmp.reserve(actualStepsBack);

        for (unsigned int step=currentStep-actualStepsBack; step < currentStep; step++)
        {
            tmp.emplace_back(getHourLoc2Man(curLoc, step));
        }
        return tmp;
    }

    bool EpiSim::setIntervention(unsigned int manIdx, int interventionType, int contDay) {
        if(interventionType<0 || interventionType>5 || manIdx <config->startIdx) return false;
        Man *curMan = idx2Man[manIdx];
        int curIntType = curMan->getInterveneStrategy();
        if(interventionType > curIntType  ){ //upgrade intervention level if necessary
            curMan->interveneStrategy[curIntType] = false;
            curMan->interveneStrategy[interventionType] = true;
            curMan->strategyHours = contDay; //update leaving days
        }

        return true;
    }

    Man *EpiSim::getManIdx2Pointer(unsigned int manIdx) {
        if(manIdx<config->startIdx) return nullptr;
        return idx2Man[manIdx];
    }


    bool EpiSim::setInterventionGroup(std::vector<unsigned int> &concernedGroup, int interventionType, int contHour) {
        for (auto manIdx: concernedGroup){
            setIntervention(manIdx, interventionType, contHour);
        }
        return true;
    }

    Man *MobilityBase::getManIdx2Pointer(unsigned int manIdx) {
        return nullptr;
    }

    Man *NetworkGenerator::getManIdx2Pointer(unsigned int manIdx) {
        return idx2Man[manIdx];
    }
}

