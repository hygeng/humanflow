#include "engine/intervene.h"

namespace Simulator{
    Intervene::Intervene(Config *config_,InterveneConfig *interveneConfig_){
        config = config_;
        interveneConfig = interveneConfig_;
    };


    std::vector<unsigned int> Intervene::getManContact(unsigned int concernedManIdx)
    {
        std::vector<unsigned int> cur;
        std::unordered_map<unsigned int, unsigned int> meetCnt;
        for (unsigned int manIdx=config->startIdx; manIdx<config->startIdx + hisMan.size(); manIdx++){
            meetCnt[manIdx] = 0;
        }

        auto curManHis = hisMan[concernedManIdx];
        for (unsigned int step_idx=0; step_idx < curManHis.size(); step_idx++){
            auto locIdx = curManHis[step_idx];
            if (locIdx < config->startIdx) continue; // if x is in hospital or isolated for that time
            else {
                for (auto w: hisLoc[locIdx][step_idx]){
                    meetCnt[w]++;
                }
            }
        }

        for (unsigned int manIdx=config->startIdx; manIdx<config->startIdx + hisMan.size(); manIdx++){
            if (meetCnt[manIdx] > 0) cur.emplace_back(manIdx);
        }
        return cur;
    }

    void Intervene::contactIntervene(const std::vector<unsigned int>& concernedManList, int interveneType,  MobilityGenerator* mobilityGenerator){
        std::vector<unsigned int> contactManList;

        //get contact man list
        for (auto manIdx: concernedManList){
            auto contactList = getManContact(manIdx);
            for(auto contactIdx:contactList) contactManList.push_back(contactIdx);
        }

        //set intervention
        switch(interveneType){
            case 1:
                for (auto concernedManIdx: concernedManList){
                    mobilityGenerator->setIntervention(concernedManIdx, 1, interveneConfig->confineHours);
                }
                for (auto contactManidx: contactManList) {
                    mobilityGenerator->setIntervention(contactManidx, 1, interveneConfig->confineHours);
                }
            case 2:
                for (auto concernedManIdx: concernedManList){
                    mobilityGenerator->setIntervention(concernedManIdx, 2, interveneConfig->isolateHours);
                }
                for (auto contactManidx: contactManList) {
                    mobilityGenerator->setIntervention(contactManidx, 2, interveneConfig->isolateHours);
                }
            case 3:
                for (auto concernedManIdx: concernedManList){
                    mobilityGenerator->setIntervention(concernedManIdx, 3, interveneConfig->treatHours);
                }
                for (auto contactManidx: contactManList) {
                    mobilityGenerator->setIntervention(contactManidx, 2, interveneConfig->isolateHours);
                }
        }

    }

    void Intervene::contactInterveneEpiSim(const std::vector<unsigned int>& concernedManList, int interveneType,  EpiSim* EpiSim0){
        std::vector<unsigned int> contactManList;

        //get contact man list
        for (auto manIdx: concernedManList){
            auto contactList = getManContact(manIdx);
            for(auto contactIdx:contactList) contactManList.push_back(contactIdx);
        }

        //set intervention
        switch(interveneType){
            case 1:
                for (auto concernedManIdx: concernedManList){
                    EpiSim0->setIntervention(concernedManIdx, 1, interveneConfig->confineHours);
                }
                for (auto contactManidx: contactManList) {
                    EpiSim0->setIntervention(contactManidx, 1, interveneConfig->confineHours);
                }
            case 2:
                for (auto concernedManIdx: concernedManList){
                    EpiSim0->setIntervention(concernedManIdx, 2, interveneConfig->isolateHours);
                }
                for (auto contactManidx: contactManList) {
                    EpiSim0->setIntervention(contactManidx, 2, interveneConfig->isolateHours);
                }
            case 3:
                for (auto concernedManIdx: concernedManList){
                    EpiSim0->setIntervention(concernedManIdx, 3, interveneConfig->treatHours);
                }
                for (auto contactManidx: contactManList) {
                    EpiSim0->setIntervention(contactManidx, 2, interveneConfig->isolateHours);
                }
        }

    }

    void Intervene::fastContactIntervene(const std::vector<unsigned int>& concernedManList, int interveneType,  MobilityGenerator* MobilityGenerator0, unsigned int queryStep,
                                         int order, int stepsToTrack){
        loc_counter = 0;
        man_counter = 0;

        int idx2LocSize = MobilityGenerator0->getIdx2Loc().size();
        unsigned int idx2ManSize = MobilityGenerator0->getIdx2Man().size();
        int timeSlot = stepsToTrack;

        std::vector<std::vector<short>> locTimeVisit(idx2LocSize +config->startIdx, std::vector<short>(timeSlot,0));

        std::vector<int> sourceMan(config->startIdx + idx2ManSize,-1);
        for (auto manIdx: concernedManList) sourceMan[manIdx] = 0;

        for (auto indOrder=0; indOrder < order; indOrder++){

            for(unsigned int manIdx = config->startIdx; manIdx< config->startIdx + idx2ManSize; ++manIdx){
                if(sourceMan[manIdx]<0) continue; //not suspicious

                hisMan[manIdx] = MobilityGenerator0->getMan2LocHis(MobilityGenerator0->getManIdx2Pointer(manIdx), queryStep, stepsToTrack);

                for (unsigned int tIdx=0; tIdx < hisMan[manIdx].size(); tIdx++) {
                    auto locIdx = hisMan[manIdx][tIdx];
                    if (locIdx <config->startIdx) continue; // todo - check if people is in hospital
                    else if (locTimeVisit[locIdx][tIdx] > 0) continue; // this place has already been marked
                    else{
                        loc_counter++;
                        locTimeVisit[locIdx][tIdx] = indOrder+1;
                    }
                }
            }

            for (unsigned short locIdx =config->startIdx; locIdx< config->startIdx + idx2LocSize ; ++locIdx){
                auto locVisit = locTimeVisit[locIdx];
                for (unsigned int tIdx=0; tIdx < locVisit.size(); tIdx++){
                    if (locVisit[tIdx] == indOrder + 1){
                        man_counter++;
                        for (auto manIdx: hisLoc[locIdx][tIdx]){ //MobilityGenerator0->getHourLoc2Man(,queryStep)
                            if (sourceMan[manIdx] == -1) sourceMan[manIdx] = indOrder+1;
                        }
                    }
                }
            }
        }

        std::vector<unsigned int> contactManList;

        for(unsigned int manIdx = config->startIdx; manIdx< config->startIdx + idx2ManSize; ++manIdx){
            if (sourceMan[manIdx] > 0) contactManList.emplace_back(manIdx);
        }


        switch(interveneType){
            case 1:
                for (auto concernedManIdx: concernedManList){
                    MobilityGenerator0->setIntervention(concernedManIdx, 1, interveneConfig->confineHours);
                }
                for (auto contactManidx: contactManList) {
                    MobilityGenerator0->setIntervention(contactManidx, 1, interveneConfig->confineHours);
                }
                break;
            case 2:
                for (auto concernedManIdx: concernedManList){
                    MobilityGenerator0->setIntervention(concernedManIdx, 2, interveneConfig->isolateHours);
                }
                for (auto contactManidx: contactManList) {
                    MobilityGenerator0->setIntervention(contactManidx, 2, interveneConfig->isolateHours);
                }
                break;
            case 3:
                for (auto concernedManIdx: concernedManList){
                    MobilityGenerator0->setIntervention(concernedManIdx, 3, interveneConfig->treatHours);
                }
                for (auto contactManidx: contactManList) {
                    MobilityGenerator0->setIntervention(contactManidx, 2, interveneConfig->isolateHours);
                }
                break;
            default:
                break;
        }

    }

    void Intervene::fastContactInterveneEpiSim(const std::vector<unsigned int>& concernedManList, int interveneType,  EpiSim* EpiSim0, unsigned int queryStep,
                                         int order, int stepsToTrack){

        // init loc x time visitation grid
        // locTimeVisit[locIdx][timeIdx] = infectedOrder
        loc_counter = 0;
        man_counter = 0;
        auto endTime1 = std::chrono::high_resolution_clock::now();


        int idx2LocSize = EpiSim0->getIdx2Loc().size();
        unsigned int idx2ManSize = EpiSim0->getIdx2Man().size();
        int timeSlot = stepsToTrack;
        std::vector<std::vector<short>> locTimeVisit(idx2LocSize +config->startIdx, std::vector<short>(timeSlot,0));

        // init infection source
        std::vector<int> sourceMan(config->startIdx + idx2ManSize,-1);
        for (auto manIdx: concernedManList) sourceMan[manIdx] = 0;

        // init man x time cells

        for (auto indOrder=0; indOrder < order; indOrder++){
            // mark the loc x time cell if this cell is visited by one man in sourceMan
            for(unsigned int manIdx = config->startIdx; manIdx< config->startIdx + idx2ManSize; ++manIdx){
                if(sourceMan[manIdx]<0) continue; //not suspicious
                auto time2 = std::chrono::high_resolution_clock::now();
                hisMan[manIdx] = EpiSim0->getMan2LocHis(EpiSim0->getManIdx2Pointer(manIdx), queryStep, stepsToTrack);

                auto time3 = std::chrono::high_resolution_clock::now();
                intervene_getManHis += ((float) (time3 - time2).count()) / 1000000.0F;

                for (unsigned int tIdx=0; tIdx < hisMan[manIdx].size(); tIdx++) {
                    auto locIdx = hisMan[manIdx][tIdx];
                    if (locIdx <config->startIdx) continue; // todo - check if people is in hospital
                    else if (locTimeVisit[locIdx][tIdx] > 0) continue; // this place has already been marked
                    else{
                        loc_counter++;
                        locTimeVisit[locIdx][tIdx] = indOrder+1;
//                        std::cout<<locIdx<<", ";
                    }
                }
            }

            auto endTime2 = std::chrono::high_resolution_clock::now();
            // mark the man x time cell if this cell is visited by one man in sourceMan
            for (unsigned short locIdx =config->startIdx; locIdx< config->startIdx + idx2LocSize ; ++locIdx){
                auto locVisit = locTimeVisit[locIdx];
                for (unsigned int tIdx=0; tIdx < locVisit.size(); tIdx++){
                    if (locVisit[tIdx] == indOrder + 1){
                        man_counter++;
                        for (auto manIdx: hisLoc[locIdx][tIdx]){ //MobilityGenerator0->getHourLoc2Man(,queryStep)
                            if (sourceMan[manIdx] == -1) sourceMan[manIdx] = indOrder+1;
//                            if (manEarliestInfectionTime[manIdx] != -1 && manEarliestInfectionTime[manIdx]> tIdx)
//                                manEarliestInfectionTime[manIdx] = tIdx;
                        }
                    }
                }
            }
            auto endTime3 = std::chrono::high_resolution_clock::now();
            interven_save_time += ((float) (endTime3 - endTime2).count()) / 1000000.0F;

        }

        std::vector<unsigned int> quarantineManList;
        std::vector<unsigned int> criticalManList;

//        for (auto man: sourceMan){
        for(unsigned int manIdx = config->startIdx; manIdx< config->startIdx + idx2ManSize; ++manIdx){
            if (sourceMan[manIdx] > 0) quarantineManList.emplace_back(manIdx);
            else if(sourceMan[manIdx] == 0) criticalManList.emplace_back(manIdx);
        }


        EpiSim0->setInterventionGroup(criticalManList,4,interveneConfig->treatHours);
        EpiSim0->setInterventionGroup(quarantineManList, interveneType,interveneConfig->quarantineHours);
        auto endTime4 = std::chrono::high_resolution_clock::now();
        interven_time += ((float) (endTime4 - endTime1).count()) / 1000000.0F;


    }

    void Intervene::ContactInterveneNetwork(std::vector<unsigned int>& concernedManList, int interveneType, NetworkGenerator *NetworkGenerator,
                                                 int order,unsigned int stepsToTrack, std::unordered_map<unsigned int, Man*>& idx2Man) {
//                                                 const __gnu_cxx::hash_map<unsigned int, Man *>& idx2Man) {

        std::vector<unsigned int> contactManList;

        //get contact man list
        for (unsigned int manIdx: concernedManList){
            auto contactList = NetworkGenerator->getManContactHis(NetworkGenerator->idx2Man[manIdx],NetworkGenerator->getGlobalStep(),stepsToTrack);
            for (unsigned int trackStep = 0; trackStep<stepsToTrack;++trackStep){
                for(auto contactIdx: contactList[trackStep]) contactManList.push_back(contactIdx);
            }
        }

        //set intervention
        switch(interveneType){
            case 1:
                for (unsigned int concernedManIdx: concernedManList){
                    NetworkGenerator->setIntervention(concernedManIdx, 1, interveneConfig->confineHours);
                }
                for (auto contactManidx: contactManList) {
                    NetworkGenerator->setIntervention(contactManidx, 1, interveneConfig->confineHours);
                }
            case 2:
                for (unsigned int concernedManIdx: concernedManList){
                    NetworkGenerator->setIntervention(concernedManIdx, 2, interveneConfig->isolateHours);
                }
                for (auto contactManidx: contactManList) {
                    NetworkGenerator->setIntervention(contactManidx, 2, interveneConfig->isolateHours);
                }
            case 3:
                for (unsigned int concernedManIdx: concernedManList){
                    NetworkGenerator->setIntervention(concernedManIdx, 3, interveneConfig->treatHours);
                }
                for (auto contactManidx: contactManList) {
                    NetworkGenerator->setIntervention(contactManidx, 2, interveneConfig->isolateHours);
                }
        }


    }
}
