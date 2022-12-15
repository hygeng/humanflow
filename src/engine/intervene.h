#ifndef SIMULATOR_INTERVENE_H
#define SIMULATOR_INTERVENE_H

#include "utilities/utility.h"
#include "utilities/mobility.h"
#include "engine/epidemic.h"

namespace Simulator {

    class Man;

    class Intervene {
    private:
        Config *config;
        InterveneConfig *interveneConfig;
    public:
        Intervene(Config *config_,InterveneConfig *interveneConfig_);

        std::vector<unsigned int> getManContact(unsigned int concernedManIdx);
        //for location-based
        std::unordered_map<unsigned short, std::vector<std::vector<unsigned int>>> hisLoc;
        std::unordered_map<unsigned int, std::vector<unsigned short>> hisMan;
        //for network-based
        std::unordered_map<unsigned int, std::vector<std::vector<unsigned int>>> hisManNetwork;

        void contactIntervene(const std::vector<unsigned int>& concernedManList, int interveneType,
                        MobilityGenerator* mobilityGenerator);


        void fastContactIntervene(const std::vector<unsigned int>& concernedManList, int interveneType,  
                        MobilityGenerator* mobilityGenerator, unsigned int queryStep, int order, int stepsToTrack);
        //for episim
        void contactInterveneEpiSim(const std::vector<unsigned int>& concernedManList, int interveneType,
                        EpiSim* EpiSim0);
        void fastContactInterveneEpiSim(const std::vector<unsigned int>& concernedManList, int interveneType,  
                        EpiSim* EpiSim0, unsigned int queryStep, int order, int stepsToTrack);

        //for network-based methods
        void ContactInterveneNetwork(std::vector<unsigned int>& concernedManList, int interveneType,  NetworkGenerator* NetworkGenerator,
                        int order, unsigned int stepsToTrack,  std::unordered_map<unsigned int, Man*>& idx2Man);
//                                  const __gnu_cxx::hash_map<unsigned int, Man*>& idx2Man);

        float interven_time = 0;
        float interven_save_time = 0;
        int loc_counter = 0;
        int man_counter = 0;
        float intervene_getManHis = 0.0;

    };
};

#endif //SIMULATOR_INTERVENE_H
