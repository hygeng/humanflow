#ifndef SIMULATOR_MOBILITY_H
#define SIMULATOR_MOBILITY_H

#include "location.h"
#include "man.h"
#include "utility.h"
#include "stdlib.h"

namespace Simulator {

class MobilityBase{
public:
//     virtual ~MobilityBase() { };
    virtual Man* getManIdx2Pointer(unsigned int manIdx);
};


class MobilityGenerator: public MobilityBase{
    // HMES mobility class
    friend class EpiGenerator;
    friend class Intervene;
    private:
        Config *config;
        // void readConfigFromJson(std::string jsonPath);
        int globalStep =-1;  //global clock
        std::unordered_map<unsigned int, Man*> idx2Man;
        std::unordered_map<unsigned short, Location*> idx2Loc; //for all the locations

        std::vector<unsigned short> listLoc0; // list of locations for home
        std::vector<unsigned short> listLoc1; // list of locatoins for work
        std::vector<unsigned short> listLoc2; // list of locatoins for mall

        bool generateLocation(unsigned short LocNum, int typeIdx, Config *config_); // create several locations
        bool generateAllLocation(Config *config_); // create several locations
        bool generateMan(unsigned int manNum, Config *config_); // create several men

        unsigned short matchMan2Loc(Man* curMan, int typeIdx); // assign a destination for man of typeidx (0, 1, 2)
        int hour2Routine(Man* curMan, unsigned int hour); // return the placeIdx for man at  hour (0-14) in the day
        bool generateInitRoutine();//run once
        bool hour2IsOpen(unsigned int hour, int placeId);// if the current place is open according to precompute trajectory

    public:
        // constructor
        explicit MobilityGenerator(Config *config);
        explicit MobilityGenerator(); // constructor for API; include epidemic construction for implementation simplicity
        explicit MobilityGenerator(Config *config, EpiConfig *epiConfig);
        //constructor by map (from json file)
        explicit MobilityGenerator(std::unordered_map<std::string, std::string> configMap);
        explicit MobilityGenerator(std::unordered_map<std::string, std::string>& configMap, std::unordered_map<std::string, std::string>& EpiConfigMap);
        //constructor util funcs
        bool build_mobility_class();
        bool build_epidemic_class();

        //reset and clear all cache data, rebuild the mobility class object
        bool reset();

        // query api
        unsigned int getGlobalStep(){return globalStep;}
        unsigned int getGlobalDay(){return (globalStep+1) / config->hoursPerDay;}
        unsigned int getManNum();
        unsigned int getLocNum();
        bool isValidManIdx(unsigned int manIdx);
        bool isValidLocIdx(unsigned short locIdx);
        std::unordered_map<unsigned short, Location*>& getIdx2Loc();
        std::unordered_map<unsigned int, Man*>& getIdx2Man();
        short getLocType(unsigned short locIdx){return idx2Loc[locIdx]->getType();}
        int getManInterventionStrategy(unsigned int manIdx){return idx2Man[manIdx]->getInterveneStrategy();}

        //mobility generation
        bool generateDayTrajectory(int totalDays);
        bool generateHourTrajectory(int totalHours);

        bool setTodayDuration(); // reset everyday
        bool setTodayDestination(); // reset mall for every man every day

        Man* getManIdx2Pointer(unsigned int manIdx);
        Location* getLocIdx2Pointer(unsigned short locIdx);

        //query loc2man
        std::vector<unsigned int> getHourLoc2Man(Location* curLoc, unsigned int queryStep); //get list of people at loc at queryStep
        std::vector<std::vector<unsigned int>> getLoc2ManHis(Location* curLoc, \
                unsigned int currentStep, unsigned int stepsBack); //get list of history people at loc from currentStep in  stepsBack range 

        std::vector<unsigned int> PyGetHourLoc2Man(unsigned short locIdx, unsigned int queryStep);  // python interface function
        std::vector<std::vector<unsigned int>> PyGetLoc2ManHis(unsigned short locIdx, unsigned int currentStep, unsigned int stepsBack);

        //query man2man
        std::vector<unsigned int>& getAcqtGroup(unsigned int manIdx);

        //query man2loc2man
        std::vector<unsigned int> getman2loc2man(unsigned int manIdx, unsigned int currentStep,unsigned int steps_back);

        // query man2loc
        unsigned short getHourMan2Loc(Man* curMan, unsigned int queryStep); //get position of man at queryStep
        std::vector<unsigned short> getMan2LocHis(Man* curMan, unsigned int currentStep, unsigned int stepsBack); //get history positons of man 

        unsigned short PyGetHourMan2Loc(unsigned int manIdx, unsigned int queryStep);
        std::vector<unsigned short> PyGetMan2LocHis(unsigned int manIdx, unsigned int currentStep, unsigned int stepsBack);

        // set intervention
        bool setIntervention(unsigned int manIdx, int interventionType, int contHours);
        bool setInterventionGroup(std::vector<unsigned int>& concernedGroup, int interventionType, int contHour);

        //query statistics
        std::vector<unsigned int> countGroupSEIR(const std::vector<unsigned int>& GroupOfPeople); // return {S,E,I,R} (E for exposed, or in incubation period)

        // API for man
        int getManHealthStatus(unsigned int manIdx);
        bool setManHealthStatus(unsigned int manIdx, int newStatus);
        int getManIncubationHourLeft(unsigned int manIdx);
        bool DecreaseManIncubationDayLeft(unsigned int manIdx);

        /*-----------------------------------epidemic APIs --------------------------------------------*/

        //variables
        EpiConfig *epiConfig;
        int cur_infected_cnt = 0; //incubation and critical
        int daily_recovered_cnt = 0;
        std::vector<unsigned int> InfectedIdxList; //infected people list
        std::vector<unsigned int> IncubationIdxList; //incubation people list
        std::vector<unsigned int> dailyIncubationIdxList; //every-day newly infected people
        std::vector<unsigned int> dailyInfectedIdxList;
        std::vector<unsigned int> stateCnt= { 0, 0, 0, 0 };  //0:susceptible(S) 1:incubation(E) 2:critical infected()I 3:recovered (R) //only for provisional recording

        //set infection
        bool setInitInfection(std::vector<unsigned int> initInfectedMan);
        bool setDailyInfection(); // required for each day. update incubation and health status

        // infection
        bool computeInfection(const std::vector<unsigned int>& HourManInLoc);
        bool generateAcqtInfection(unsigned  int manIdx);
        bool generateHourInfection(int hours);
        bool generateDayInfection(int days);
        bool updateIncubationDaily();
        bool setManIncubation(unsigned int manIdx); // from susceptible to incubation
        bool setManInfected(unsigned int manIdx);
        //query
        int getInfectedCount();
        int getDailyRecoveredCount();
        int getIncubationCount();
        std::vector<unsigned int>& getDailyInfectedIdxList();
        std::vector<unsigned int>& getInfectedIdxList();
        std::vector<unsigned int>& getIncubationIdxList();

        // operators
        bool addInfectedIdxList(unsigned int manIdx);
        bool eraseInfectedIdxList(int position);
        bool eraseIncubationIdxList(int position);
        bool minusCurInfectedNum(int recovered_cnt); //not used
};

class NetworkGenerator:public MobilityBase{
    // network-basd mobility baseline
    friend class EpiGenerator;
    friend class Intervene;
    private:
        Config *config;
        int globalStep;
         std::unordered_map<unsigned int, Man*> idx2Man;
        bool generateMan(unsigned int manNum, Config *config); // create several men
        bool generateRoutineContact(); //generate most frequently contact for each person

public:
        explicit NetworkGenerator();
        explicit NetworkGenerator(Config *config);
        unsigned int getGlobalStep() const;
        bool generateDayContact(int totalDays);
        bool generateHourContact(int totalHours);
        std::unordered_map<unsigned int, Man*>getIdx2Man();

        Man* getManIdx2Pointer(unsigned int manIdx) override;

        std::vector<unsigned int> getHourManContact(Man *curMan, unsigned int step);
        std::vector<std::vector<unsigned int>> getManContactHis(Man* curMan, unsigned int currentStep, unsigned int stepsBack);

        bool setIntervention(unsigned int manIdx, int interventionType, int contDay);
    };


class EpiSim:public MobilityBase{
    // episim baseline
    friend class EpiGenerator;
    friend class Intervene;
    private:
        Config *config;
        int globalStep;
        std::unordered_map<unsigned int, Man*> idx2Man;
        std::unordered_map<unsigned short, Location*> idx2Loc; //for all the locations

        std::vector<unsigned short> listLoc0; // for home
        std::vector<unsigned short> listLoc1; // for work
        std::vector<unsigned short> listLoc2; // for mall
        bool generateLocation(unsigned short LocNum, int typeIdx, Config *config_); // create several locations
        bool generateAllLocation(unsigned short LocNum, Config *config_); // create several locations
        bool generateMan(unsigned int manNum, Config *config); // create several men
        unsigned short matchMan2Loc(Man* curMan, int typeIdx); // assign a destination for man of typeidx(0, 1, 2)
        int hour2Routine(Man* curMan, int hour); // return the placeIdx for man at  hour (0-14) in the day
        bool generateInitRoutine(); //run once at initialize
        bool hour2IsOpen(int hour, int placeId);// if the current place is open according to precompute trajectory

    public:
        explicit EpiSim(Config *config);
        unsigned int getGlobalStep() const;
        bool generateDayTrajectory(int totalDays);
        bool generateHourTrajectory(int totalHours);
        bool resetRoutineDuration(); // reset everyday
        bool setDayRoutine(); // reset mall for every man every day

        std::unordered_map<unsigned short, Location*> getIdx2Loc();
        std::unordered_map<unsigned int, Man*> getIdx2Man();

        Man* getManIdx2Pointer(unsigned int manIdx) override;

        unsigned short getHourMan2Loc(Man* curMan, unsigned int queryStep); //get position of man at queryStep
        std::vector<unsigned short> getMan2LocHis(Man* curMan, unsigned int currentStep, unsigned int stepsBack);

        std::vector<unsigned int> getHourLoc2Man(Location* curLoc, unsigned int queryStep); //get list of people at loc at queryStep
        std::vector<std::vector<unsigned int>> getLoc2ManHis(Location* curLoc, unsigned int currentStep, unsigned int stepsBack);

        bool setIntervention(unsigned int manIdx, int interventionType, int contDay);
        bool setInterventionGroup(std::vector<unsigned int>& concernedGroup, int interventionType, int contHour);


    };



};

#endif //SIMULATOR_MOBILITY_H