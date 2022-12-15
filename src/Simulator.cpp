//#include "engine/epidemic.h"
#include "utilities//mobility.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace py = pybind11;
using namespace py::literals;

//PYBIND11_MODULE(humanflow, m) {
//    py::class_<Simulator::NetworkGenerator>(m, "mobility")
//        .def(py::init())
//        .def("get_idx_man", &Simulator::NetworkGenerator::getIdx2Man)
//        .def("get_hour_man_contact", &Simulator::NetworkGenerator::getHourManContact,
//                "man"_a, "step"_a=1)
//        .def("set_random_seed", &Simulator::NetworkGenerator::generateDayContact,
//                "day"_a);

PYBIND11_MODULE(humanflow, m) {
py::class_<Simulator::MobilityGenerator>(m, "mobility")
        .def(py::init())
        .def(py::init<std::unordered_map<std::string,std::string>>())
        .def(py::init<std::unordered_map<std::string,std::string>&, std::unordered_map<std::string,std::string>&>())
        //reset
        .def("reset", &Simulator::MobilityGenerator::reset)

        //query statistics
        .def("get_cur_step", &Simulator::MobilityGenerator::getGlobalStep)
        .def("get_cur_day", &Simulator::MobilityGenerator::getGlobalDay)
        .def("man_num", &Simulator::MobilityGenerator::getManNum)
        .def("loc_num", &Simulator::MobilityGenerator::getLocNum)

        // mobility operators:
        .def("setTodayDestination", &Simulator::MobilityGenerator::setTodayDestination)
        .def("setTodayDuration", &Simulator::MobilityGenerator::setTodayDuration)

        //step (generate mobility)
        .def("stepHour", &Simulator::MobilityGenerator::generateHourTrajectory,
                "hours"_a)
        .def("stepDay", &Simulator::MobilityGenerator::generateDayTrajectory,
                "days"_a)

        //query is valid
        .def("isValidManIdx", &Simulator::MobilityGenerator::isValidManIdx,
                "manIdx"_a)
        .def("isValidLocIdx", &Simulator::MobilityGenerator::isValidLocIdx,
                "locIdx"_a)

        // query man2loc2man: contact tracing
            .def("get_ContactTracing", &Simulator::MobilityGenerator::getman2loc2man,
                 "manIdx"_a, "query_step"_a, "steps_back"_a)

        // query man2loc
        .def("get_hour_man2loc", &Simulator::MobilityGenerator::PyGetHourMan2Loc,
                "manIdx"_a, "query_step"_a=1)
        .def("get_his_man2loc", &Simulator::MobilityGenerator::PyGetMan2LocHis,
                "manIdx"_a, "query_step"_a=1, "steps_back"_a=14)

        // query loc2man
        .def("get_hour_loc2man", &Simulator::MobilityGenerator::PyGetHourLoc2Man,
                "locIdx"_a, "step"_a=1)
        .def("get_his_loc2man", &Simulator::MobilityGenerator::PyGetLoc2ManHis,
                "locIdx"_a, "query_step"_a=1, "steps_back"_a=14)

        //query for man
        .def("getManHealthStatus", &Simulator::MobilityGenerator::getManHealthStatus,
                "manIdx"_a)
        .def("getManInterventionStrategy", &Simulator::MobilityGenerator::getManInterventionStrategy,
                "manIdx"_a)
        .def("getManAcqtGroup", &Simulator::MobilityGenerator::getAcqtGroup,
                "manIdx"_a)

        //query for loc
        .def("getLocType", &Simulator::MobilityGenerator::getLocType,
                "locIdx"_a)

        // query for group
        .def("countGroupSEIR", &Simulator::MobilityGenerator::countGroupSEIR,
                "GroupOfPeople"_a)
        //-------------------------------------------------------- epidemic APIs
        //query epidemic statistics
        .def("getIncubationCount", &Simulator::MobilityGenerator::getIncubationCount)
        .def("getInfectedCount", &Simulator::MobilityGenerator::getInfectedCount)
        .def("getDailyRecoveredCount", &Simulator::MobilityGenerator::getDailyRecoveredCount)
        .def("getInfectedManList", &Simulator::MobilityGenerator::getInfectedIdxList)
        .def("getIncubationManList", &Simulator::MobilityGenerator::getIncubationIdxList)
        .def("getDailyInfectedManList", &Simulator::MobilityGenerator::getDailyInfectedIdxList)

        //compute infection
        .def("setInitInfection", &Simulator::MobilityGenerator::setInitInfection,
                "initInfectedMan"_a)
        .def("setDailyInfection", &Simulator::MobilityGenerator::setDailyInfection)
        
        .def("computeInfection", &Simulator::MobilityGenerator::computeInfection,
                "hourManInLoc"_a)

        // generate infection by hour.day
        .def("generateHourInfection", &Simulator::MobilityGenerator::generateHourInfection)
        .def("generateDayInfection", &Simulator::MobilityGenerator::generateDayInfection)

        //operations
        .def("addInfectedIdxList", &Simulator::MobilityGenerator::addInfectedIdxList,
                "manIdx"_a)
        .def("eraseInfectedIdxList", &Simulator::MobilityGenerator::eraseInfectedIdxList,
                "manIdx"_a)
        .def("eraseIncubationIdxList", &Simulator::MobilityGenerator::eraseIncubationIdxList,
                "manIdx"_a)
        .def("minusCurInfectedNum", &Simulator::MobilityGenerator::minusCurInfectedNum,
                "daily_recovered_cnt"_a)

        //-------------------------------------------------------------intervention
        .def("setIntervention", &Simulator::MobilityGenerator::setIntervention,
                "manIdx"_a, "interventionType"_a, "contHours"_a)
        .def("setInterventionGroup", &Simulator::MobilityGenerator::setInterventionGroup,
                "concernedGroup"_a, "interventionType"_a, "contHours"_a);

#ifdef VERSION
    m.attr("__version__") = VERSION;
#else
    m.attr("__version__") = "dev";
#endif
}




