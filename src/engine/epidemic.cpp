#include "engine/epidemic.h"
#include <random>
#define RD_PRECISION 1e9
namespace Simulator {
    EpiGenerator::EpiGenerator( Config *config_,EpiConfig* epiConfig_)
    {
        epiConfig = epiConfig_;
        config =  config_;
        cur_infected_cnt = epiConfig->init_infected_num;
    }

    bool EpiGenerator::setInitInfection(const std::vector<unsigned int> initInfectedMan){
        for(auto manIdx:initInfectedMan){
            IncubationManIdxList.emplace_back(manIdx);
//            InfectedIdxList.emplace_back(manIdx);
            Man* man = MobilityB->getManIdx2Pointer(manIdx);
            man->healthStatus[man->getHealthStatus()] = false;
            man->healthStatus[1] = true; //incubation
            man->incubationHourLeft = epiConfig->incubationHours - config->hoursPerDay - 1;
        }
        return true;
    }

	bool EpiGenerator::computeInfection(const std::vector<unsigned int>& HourManInLoc){
        //count health status
        std::memset(stateCnt,0,sizeof(stateCnt));
        int man_num = HourManInLoc.size();
        for (unsigned int manIdx : HourManInLoc) {
            Man* man = MobilityB->getManIdx2Pointer(manIdx);
            if (man->healthStatus[0]){  //susceptible
                stateCnt[0]++;
            }
            else if (man->healthStatus[3]){  //recovered
                stateCnt[2]++;
            }
            else stateCnt[1]++;  //infected
        }

        if(stateCnt[1]==0) {
            return true;  //there's no one infected in this location
        }

		auto i = float(stateCnt[1]);
		auto s = float(stateCnt[0]);
		auto r = float(stateCnt[2]);

        float infectionRate = epiConfig->pInfection * i / (s+i+r);
        int infectionNum = int( s * infectionRate);

        if(infectionNum==0){
            for (auto curMan : HourManInLoc){
//                Man* man = idx2Man[curMan];
                Man* man = MobilityB->getManIdx2Pointer(curMan);
                if(man->healthStatus[0]==1 && !man->isInDailyInfectedList){  //susceptible and not infected yet
                    int ran_num = xorshf96()%int(RD_PRECISION);
                    if(ran_num < infectionRate*RD_PRECISION){
                        man->isInDailyInfectedList=true;
                        dailyIncubationManList.emplace_back(curMan);
                        cur_infected_cnt++;
                    }
                }
            }
        }
        else{
            //shuffle
//            std::shuffle(susceptibleManInLoc.begin(), susceptibleManInLoc.end(), std::mt19937(std::random_device()()));
//            for (int inf = 0; inf < infectionNum; inf++) {
//                Man* m = susceptibleManInLoc[inf];
//                m->isInDailyInfectedList = true;
//                InfectedManList.emplace_back(m);
//                cur_infected_cnt++;
//            }
            //random
            int newly_infected_cnt = 0;
            while(newly_infected_cnt < infectionNum){
                int ran_num = xorshf96()%man_num;
                int manIdx = HourManInLoc[ran_num];
                Man* curMan = MobilityB->getManIdx2Pointer(manIdx);
                if(curMan->healthStatus[0] && !curMan ->isInDailyInfectedList){
                    curMan ->isInDailyInfectedList = true;
                    dailyIncubationManList.emplace_back(manIdx);
                    cur_infected_cnt++;
                    newly_infected_cnt++;
                }
            }

        }
        return true;
	}

    bool EpiGenerator::computeEpiFastInfection(Man* man, const std::vector<unsigned int>& contactMan){
        if(man->healthStatus[1] || man->healthStatus[2]){ //infected (incubation or critical)
            for(unsigned int manIdx : contactMan){
                Man* man2Infect= MobilityB->getManIdx2Pointer(manIdx);
                if(man2Infect->healthStatus[0] && !man2Infect->isInDailyInfectedList){  //susceptible and not in the DailyInfectedList
                    int ran_num = xorshf96()%int(RD_PRECISION);
                    if( ran_num < epiConfig->pEpiFastInfection*RD_PRECISION){
                        man2Infect->isInDailyInfectedList=true;
                        dailyIncubationManList.emplace_back(manIdx);
                        cur_infected_cnt++;
                    }
                }
            }
        }
        return true;
    }

    bool EpiGenerator::computeFREDInfection(const std::vector<unsigned int>& HourManInLoc){
        for(auto manIdx:HourManInLoc){
            Man* man= MobilityB->getManIdx2Pointer(manIdx);
            if(man->healthStatus[1] || man->healthStatus[2]){ //infected (incubation or critical)
                for(auto contactManIdx:HourManInLoc){
                    if(contactManIdx != manIdx){
                        Man* man2Infect= MobilityB->getManIdx2Pointer(contactManIdx);
                        if(man2Infect->healthStatus[0] && !man2Infect->isInDailyInfectedList){  //susceptible and not in the DailyInfectedList
                            int ran_num = xorshf96()%int(RD_PRECISION);
                            if( ran_num < epiConfig->pInfection*RD_PRECISION){
                                man2Infect->isInDailyInfectedList=true;
                                dailyIncubationManList.emplace_back(contactManIdx);
                                cur_infected_cnt++;
                            }
                        }
                    }
                }
            }
        }
        return true;
    }

    float EpiGenerator::pKInfection(int n,int k){
        double pk=0;

        if(k==0){
            pk += epiConfig->fastSIRThreshold*std::pow(1-epiConfig->pFastSIRInfection,n)/
                    (1-(1-epiConfig->fastSIRThreshold)*(std::pow(1-epiConfig->pFastSIRInfection,n)));
        }
        else
            pk = n/k*pKInfection(n-1,k-1)-(n-k+1)/k*pKInfection(n,k-1);
        return pk;
    }

    bool EpiGenerator::computeFastSIRInfection(Man* man, const std::vector<unsigned int>& contactMan,float rate, float rate2){
        if(contactMan.size()==0){
            return true;
        }
        int susceptible_num = 0;
        double p_total= 0;
        int infect_num = 0;
        std::vector<Man* > susceptibleManInContact;
        if(man->healthStatus[1] || man->healthStatus[2]){  //infected (incubation or critical)
            //count susceptible man in the current man's contact list
            for(auto& manIdx : contactMan){
//                Man* man2Infect = idx2Man[manIdx];
                Man* man2Infect= MobilityB->getManIdx2Pointer(manIdx);
                if(man2Infect->healthStatus[0]){ //susceptible
                    susceptibleManInContact.emplace_back(man2Infect);
                }
            }
            susceptible_num = susceptibleManInContact.size();
            //calculate the predicted number of men in contact to be infected
            for(int k=0; k<susceptible_num; k++){
                p_total += pKInfection(susceptible_num,k);
                if(p_total>=epiConfig->fastSIRThreshold){
                    infect_num = k;
                    break;
                }
            }

            if(infect_num==0) {
                int ran_num = xorshf96() % int(RD_PRECISION);
                if (ran_num < rate * RD_PRECISION) {
                infect_num = int(rate2 * susceptibleManInContact.size());}
            }

            //randomly pick k men in all susceptible men in contact to be infected
            std::shuffle(susceptibleManInContact.begin(), susceptibleManInContact.end(), std::mt19937(std::random_device()()));
            for(int i=0;i< std::min(infect_num,susceptible_num);i++){
                Man* curMan = susceptibleManInContact[i];
                if(!curMan->isInDailyInfectedList){
                    curMan->isInDailyInfectedList=true;
                    dailyIncubationManList.emplace_back(curMan->getManIdx());
                    cur_infected_cnt++;
                }
            }
        }
        return true;
    }

    bool EpiGenerator::setDailyInfection() {
        for(auto manIdx : dailyIncubationManList){
            Man* man= MobilityB->getManIdx2Pointer(manIdx);
            man->healthStatus[man->getHealthStatus()] = false;
            man->healthStatus[1] = true; //incubation
            man->isInDailyInfectedList = false;
            man->incubationHourLeft = epiConfig->incubationHours;
            IncubationManIdxList.emplace_back(manIdx);
        }
        dailyIncubationManList.clear();

        return true;
    }

    int EpiGenerator::getCurInfectedCount() const{
        return cur_infected_cnt;
    }


    std::vector<unsigned int> EpiGenerator::getInfectedManIdxList(){
        return InfectedManIdxList;
    }

    std::vector<unsigned int> EpiGenerator::getIncubationManIdxList(){
        return IncubationManIdxList;
    }

    bool EpiGenerator::addInfectedManIdxList(int manIdx) {
        InfectedManIdxList.emplace_back(manIdx);
        return true;
    }

    bool EpiGenerator::eraseInfectedManIdxList(int position) {
        InfectedManIdxList.erase(InfectedManIdxList.begin() + position);
        return true;
    }

    bool EpiGenerator::eraseIncubationManIdxList(int position) {
        IncubationManIdxList.erase(IncubationManIdxList.begin() + position);
        return true;
    }

    bool EpiGenerator::minusCurInfectedNum(int recovered_cnt) {
        cur_infected_cnt-=recovered_cnt;
        return true;
    }

    EpiGenerator::EpiGenerator() {
        auto *epiConfig_ = new EpiConfig();
        auto *config_  = new Config();

        epiConfig = epiConfig_;
        config =  config_;
        cur_infected_cnt = epiConfig->init_infected_num;
    }

}
