
#####################################################################################
# read configs from file
import humanflow as hf
import json
top_dir = "./"

# config file
config_dir = top_dir + "proto/configFile/"
config_path = config_dir + "Config.json"
EpiConfig_path = config_dir + "EpiConfig.json"
# InterveneConfig_path = config_dir + "InterveneConfig.json"

# read config file
def get_dict(filepath):
    with open(filepath, 'r') as load_f:
        return_dict = json.load(load_f)
    return return_dict


configDict = get_dict(config_path)
EpiConfigDict = get_dict(EpiConfig_path)
# InterveneConfigDict = get_dict(InterveneConfig_path)

####################################     mobility module  ############################################
####### initiliate the mobility (from 3 ways)
# by default settings
env = hf.mobility()
# by customizing mobility configs
env = hf.mobility(configDict)
# by customizing mobility and epidemic configs
env = hf.mobility(configDict, EpiConfigDict)


# reset 
env.reset()

##### mobility  simulation 
# before epidemic simulation, mobility simulation is a necessary, including two API:
# conduct simulation for hours (1 step = 1 hour)
env.stepHour(1)
# conduct simulation for days (1 day  = 14 steps)
env.stepDay(2)


# query global clock (in step/hour, and in day)
cur_step = env.get_cur_step()
cur_day = env.get_cur_day()

# currently, the man index and location index reserves first few indexes 
# that users could not use. here are api to check if they are valid:
is_valid = env.isValidManIdx(manIdx = 5)
is_valid = env.isValidLocIdx(locIdx = 5)


####### Two APIs to check history locations people have been to:
# get position of man at queryStep
for manIdx in range(3, 10):
    res = env.get_hour_man2loc(manIdx, cur_step)
    print(res)

# get history positions of man at queryStep and past period
for manIdx in range(3, 10):
    res = env.get_his_man2loc(manIdx, cur_step, steps_back = 5)
    print(res)


####### Two APIs to check history group of people for some location:
# get group of people at some place at queryStep
for locIdx in range(3, 10):
    res = env.get_hour_loc2man(locIdx, cur_step)
    print(res)

# get at queryStep and past period
for locIdx in range(3, 10):
    res = env.get_his_loc2man(locIdx, cur_step, steps_back = 5)
    print(res)


#### APIs for basic mobility statistics:

# query man and location total number
man_num = env.man_num()
loc_num = env.loc_num()

# get health status for person  0:susceptible(S) 1:incubation(E) 2:critical infected(I) 3:recovered (R)
hs = env.getManHealthStatus(manIdx = 10)
# get acquantance group for person
group = env.getManAcqtGroup(manIdx = 10)

# get location type: 0: home  1: work  2: entertainment
locIdx = env.getLocType(locIdx = 10)

# count number of S, E, I, R for a list of person IDs
seir = env.countGroupSEIR(GroupOfPeople = [3,4,5,6,7])

################################    epidemic module          ####################################
# epidemic propagation

# initial infected man
initInfectedMan = [idx for idx in range(3, 10)]
env.setInitInfection(initInfectedMan)


#### two equivelant way to conduct a 10day simulation
# 1. detailed customizaton
SIM_DAY = 10
for day in range(SIM_DAY):
    #### conduct simulations by hour
    for hour in range(len(configDict['hoursPerDay'])):
        env.generateDayInfection(1)
    if day != 0:
        ### in each day, requiring conuduct daily operations, in two equivalent ways(just use one):
        # (1) a simple function call:
        env.setDailyInfection()
        # (2) detailed operations:
        # ### Not supported yet
        # incubationManList = env.getIncubationManList()
        # for position in range(len(incubationManList)-1, 0, -1):
        #     manIdx = incubationManList[position]
        #     # remove the incubation towards infected
        #     if env.getManIncubationHourLeft(manIdx) == 0:
        #         env.eraseIncubationManIdx(position)
        #         env.addCriticalManIdx(manIdx)
        #         env.setManHealthStatus(manIdx, 2)
        #     else: # if not yet, update incubation hours left
        #         env.ecreaseManIncubationDayLeft(manIdx)
        #     # end
# 2. by a simple function call, where all the above are included
env.generateDayInfection(10)

# query current infected number
cur_infected_num = env.getInfectedCount()


##### epidemic statistics:

# get incubation count and people list
count = env.getIncubationCount()
incu_manlist = env.getIncubationManList()

# get infection count and people list
count = env.getInfectedCount()
infected_manlist = env.getInfectedManList()
daily_infected_manlist = env.getDailyInfectedManList()

# get recovered count
# not supported
# count = env.getRecoveredCount()


################################    intervention & control          ####################################
# conduct intervention for person:  0: no intervention 1:confine 2:isolate 3: hospitalize
env.setIntervention( manIdx= 100, interventionType =1, contHours = 14) #contHours: continuing hours for the intervention
# conduct intervention for groups 
env.setInterventionGroup([3,4,5,6], interventionType =1, contHours=14)


