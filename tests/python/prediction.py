import argparse
from tqdm import tqdm
import random
import os
import json
import torch
import numpy as np

import humanflow as hf

import torch.nn as nn
import torch.nn.functional as F
from torch.utils.data import DataLoader,Dataset
import torch.optim as optim
#####################################################################################
# config args
parser = argparse.ArgumentParser()
parser.add_argument('--Phidden_size', default=16, help="hidden units of person and location embedding", type=int)
parser.add_argument('--Lhidden_size', default=16, help="hidden units of person and location embedding", type=int)
parser.add_argument('--steps_back', default=350, help="steps back", type=int)
parser.add_argument('--contact_tracing_steps', default=2, help="contact tracing steps back", type=int)
parser.add_argument('--epochs', default=10, help="steps back", type=int)
parser.add_argument('--seed', default=54321, help="seeds", type=int)
parser.add_argument('--max_density', default=350, help="maximum num of people in location", type=int)
parser.add_argument('--intervene_ratio', default=0.01, help="ratio to intervene in each loc", type=float)
parser.add_argument('--intervene_num_daily', default=10, help="num of daily intervene people", type=int)
parser.add_argument('--mode', default="person", help="free/location/person, intervene by the location/person -based")

args = parser.parse_args()

for arg in vars(args):
    print(arg, getattr(args, arg))

# config file
top_dir = "/cluster/home/it_stu110/cityos/HumanFlow/"  # "/home/hygeng/cityos/HumanFlow/"  #
config_dir = top_dir + "proto/configFile/"
config_path = config_dir + "Config.json"
EpiConfig_path = config_dir + "EpiConfig.json"
InterveneConfig_path = config_dir + "InterveneConfig.json"


# read config file
def get_dict(filepath):
    with open(filepath, 'r') as load_f:
        return_dict = json.load(load_f)
    return return_dict


configDict = get_dict(config_path)
EpiConfigDict = get_dict(EpiConfig_path)
#####################################################################################
engine = hf.mobility(configDict, EpiConfigDict)

# query man and loc num
MAN_NUM = engine.man_num()
LOC_NUM = engine.loc_num()
START_IDX = int(configDict['startIdx'])
STEPS_BACK = min(140, int(configDict['stepToTrack']))  # step back in tracking
HOURS_PER_DAY = int(configDict['startIdx'])
ACQT_NUM = 0#int(configDict['acqtGroupNum'])
args.max_density = int(MAN_NUM/LOC_NUM)
PADDING_IDX = 0
TRAIN_VERBOSE = False

# usage:
manIdx_list = list(np.arange(START_IDX, START_IDX + MAN_NUM))

# settings
SIM_DAYS = 10

# intervene settings
# intervene_day_quota = []
intervene_day_quota = [14,12,10,12,12,11,11,11,12,12]
intervene_type = 2
intervene_steps = 70

# device
device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
print("using device: {}".format(device))
np.random.seed(args.seed)
# ------------------------------ Case Study-------------------------------#
# ###  free
if args.mode == "free":
    engine.generateDayInfection(2)
    for day in range(SIM_DAYS):
        engine.generateDayInfection(1)
        print("day {}: asymptomatic people num: {}, symptomatic people num: {}".format(day, engine.getIncubationCount(),engine.getInfectedCount()))
    assert 0

#     #########  symptotic infection-intervention for all
if args.mode == "all":
    engine.generateDayInfection(2)
    for day in range(SIM_DAYS):
        engine.generateDayInfection(1)
        infected_list = engine.getDailyInfectedManList()
        # print(infected_list)
        if(len(infected_list)>0):
            engine.setInterventionGroup(infected_list, intervene_type, intervene_steps)
        print("day {}: asymptomatic people num: {}, symptomatic people num: {}".format(day, engine.getIncubationCount(),engine.getInfectedCount()))
    assert 0

    #########  symptotic infection-intervention by num x+y
# engine.generateDayInfection(2)
# for day in range(SIM_DAYS):
#     engine.generateDayInfection(1)
#     infected_list = engine.getDailyInfectedManList()
#     if(len(infected_list)>0):
#         P_intervention_group = np.unique(np.vstack((infected_list[:20])))
#         engine.setInterventionGroup(P_intervention_group, intervene_type, intervene_steps)
#     print("day {}: asymptomatic people num: {}, symptomatic people num: {}".format(day, engine.getIncubationCount(),engine.getInfectedCount()))
# assert 0

#     ########## randomly intervene
if args.mode == "random":
    engine.generateDayInfection(2)
    for day in range(SIM_DAYS):
        engine.generateDayInfection(1)
        # np.random.seed(day)
        random_intervene_group = np.random.choice(manIdx_list,intervene_day_quota[day])
        engine.setInterventionGroup(random_intervene_group, intervene_type, intervene_steps)
        print("day {}: asymptomatic people num: {}, symptomatic people num: {}".format(day, engine.getIncubationCount(),engine.getInfectedCount()))
    print("total intervne num is ", np.sum(np.array(intervene_day_quota)) )
    assert 0

#     ##########  intervene all infected and random
if args.mode == "random+":
    engine.generateDayInfection(2)
    for day in range(SIM_DAYS):
        engine.generateDayInfection(1)
        infected_list = engine.getDailyInfectedManList()
        # print(infected_list)
        if(len(infected_list)>0):
            engine.setInterventionGroup(infected_list, intervene_type, intervene_steps)
        # np.random.seed(day)
        random_intervene_group = np.random.choice(manIdx_list,intervene_day_quota[day])
        engine.setInterventionGroup(random_intervene_group, intervene_type, intervene_steps)
        print("day {}: asymptomatic people num: {}, symptomatic people num: {}".format(day, engine.getIncubationCount(),engine.getInfectedCount()))
    assert 0


#     ##########  contact tracing
if args.mode == "tracing":
    engine.generateDayInfection(2)
    for day in range(SIM_DAYS):
        engine.generateDayInfection(1)
        infected_list = engine.getDailyInfectedManList()
        print("infected:",len(infected_list))
        contacts_1st = []
        for infected_idx in infected_list:
            man_contacts_1st = engine.get_ContactTracing(infected_idx, engine.get_cur_step(), args.contact_tracing_steps)
            # print(len(man_contacts_1st))
            contacts_1st.extend(man_contacts_1st)
            contacts_1st = list(np.unique(contacts_1st))
        print("here")
        contacts_1st = list(np.unique(contacts_1st))
        if(len(infected_list)>0):
            engine.setInterventionGroup(infected_list, intervene_type, intervene_steps)
        # np.random.seed(day)
        print(len(contacts_1st))
        print(contacts_1st)
        for contact_manIdx in contacts_1st:
            engine.setIntervention(contact_manIdx, intervene_type, intervene_steps)
        # engine.setInterventionGroup(contacts_1st, intervene_type, intervene_steps)
        print("day {}: asymptomatic people num: {}, symptomatic people num: {}".format(day, engine.getIncubationCount(),engine.getInfectedCount()))
    assert 0

    ########## intervene x infected + y random
# engine.generateDayInfection(2)
# for day in range(SIM_DAYS):
#     engine.generateDayInfection(1)
#     random.seed(day+3)
#     infected_list = engine.getDailyInfectedManList()
#     augmented_P_intervention_group = infected_list[:10]
#     augmented_P_intervention_group.extend(random.sample(manIdx_list,10))
#     engine.setInterventionGroup(augmented_P_intervention_group, intervene_type, intervene_steps)
#     print("day {}: asymptomatic people num: {}, symptomatic people num: {}".format(day, engine.getIncubationCount(),engine.getInfectedCount()))
# assert 0


# engine.reset()

# ------------------------------ individual level -------------------------------#

#####################collect function
# person iteration
def get_person_datapoint(manIdx, query_step):
    person_input = engine.get_his_man2loc(manIdx, query_step, STEPS_BACK) #[T]
    person_label = int(engine.getManHealthStatus(manIdx) == 2)
    # add acqt People historical visited locations
    manAcqtGroup = engine.getManAcqtGroup(manIdx)
    # for AcqtManIdx in manAcqtGroup:
    #     person_input.extend(engine.get_his_man2loc(AcqtManIdx, query_step, STEPS_BACK))
    return person_input,manAcqtGroup, person_label


def collect_person_data(startManIdx, endManIdx, query_step):
    collect_manIdx = []
    collect_p_input = []
    collect_acqt_group = []
    collect_p_label = []
    for manIdx in range(startManIdx, endManIdx):
        if not engine.isValidManIdx(manIdx):
            continue
        person_input,person_AcqtGroup, person_label = get_person_datapoint(manIdx, query_step)
        collect_manIdx.append(manIdx)
        collect_acqt_group.append(person_AcqtGroup)
        collect_p_input.append(person_input)
        collect_p_label.append(person_label)
    return np.array(collect_manIdx), np.array(collect_acqt_group), np.array(collect_p_input), np.array(collect_p_label)


# location iteration
def get_location_datapoint(locIdx, query_step, max_len, padding_idx=PADDING_IDX):
    loc_input = engine.get_hour_loc2man(locIdx, query_step)
    seir = engine.countGroupSEIR(loc_input)
    loc_label = [seir[0], seir[2], seir[3]]
    # padding with 0 and clip
    # cur_len = len(loc_input)
    # if cur_len == 0:
    #     return [], []
    # elif cur_len < max_len:
    #     loc_input.extend([0 for _ in range(max_len-cur_len)])
    # elif cur_len > max_len:
    #     loc_input = random.sample(loc_input, max_len)
    return loc_input, loc_label


def collect_location_data(startLocIdx, endLocIdx, query_step, max_len):
    collect_locIdx = []
    collect_l_input = []
    collect_l_label = []
    for locIdx in range(startLocIdx, endLocIdx):
        if not engine.isValidLocIdx(locIdx):
            continue
        loc_input, loc_label = get_location_datapoint(locIdx, query_step, max_len)
        if len(loc_input) > 0:
            collect_locIdx.append(locIdx)
            collect_l_input.append(loc_input)
            collect_l_label.append(loc_label)
    # print(len(collect_locIdx), len(collect_l_input), len(collect_l_label))
    return collect_locIdx, collect_l_input, collect_l_label



# cur_step = engine.get_cur_step()
# l_input_collect, l_label_collect = collect_location_data(START_IDX, LOC_NUM + START_IDX, cur_step, max_len=400)  # [N, max_len], [N,3]

############## model
P_embed = torch.rand((MAN_NUM+ START_IDX, args.Phidden_size),requires_grad=False).to(device) #nn.Parameter(,requires_grad = False)
L_embed = torch.rand((LOC_NUM+ START_IDX, args.Lhidden_size),requires_grad=False).to(device)#nn.Parameter(,requires_grad = False)

# P_embed = nn.Embedding(MAN_NUM + START_IDX, args.Phidden_size, padding_idx=0)
# L_Embed = nn.Embedding(LOC_NUM + START_IDX, args.Lhidden_size, padding_idx=0)

class PersonRisk(nn.Module):
    def __init__(self,  Phidden_size, Lhidden_size, steps_back):
        super(PersonRisk, self).__init__()
        self.Phidden_size = Phidden_size
        self.Lhidden_size = Lhidden_size
        # self.L_Embed = nn.Embedding(LOC_NUM + START_IDX, args.Lhidden_size, padding_idx=0)
        # embedding setting:
        # embedding update
        self.steps_back = steps_back
        concat_size = (ACQT_NUM+1) * steps_back * Lhidden_size
        print(concat_size)
        self.concat_fc1 = nn.Linear(concat_size, concat_size//4)
        self.concat_fc2 = nn.Linear(concat_size//4, concat_size//8)
        self.concat_fc3 = nn.Linear(concat_size//8,Phidden_size)
        self.dropout = nn.Dropout(0.2)
        # risk prediction
        self.fc1 = nn.Linear(Phidden_size, Phidden_size//4)
        self.fc2 = nn.Linear(Phidden_size//4, Phidden_size//8)
        self.fc3 = nn.Linear(Phidden_size//8, 2)
        # self.L2P = nn.Parameter(torch.rand(args.Lhidden_size, args.Phidden_size),requires_grad = True)

    def update_P_Embed(self, manIdxes, p_acqt, p_input):
        #manIdxes: [B]
        # p_input: [B, T]
        pinput_flatten = torch.flatten(p_input)
        p_lookup = torch.index_select(L_embed,0, pinput_flatten)
        input_hidden = torch.reshape(p_lookup,(-1, self.steps_back * (ACQT_NUM +1), self.Lhidden_size)) # [B,T,H_l]
        input_convert = input_hidden
        # input_convert = torch.matmul(input_hidden, self.L2P) # [B,T,H_l] * [H_l, H_p] = [B,T, H_p]
        # sum pooling
        # put_sumpooling = torch.squeeze(torch.sum(input_convert, axis = 1)) # [B,H_p]
        # concat and FC
        input_concat = torch.flatten(input_convert, start_dim = 1) # [B, T*H_p]
        out_embed = torch.tanh(self.concat_fc1(input_concat))
        out_embed = self.dropout(out_embed)
        out_embed = torch.tanh(self.concat_fc2(out_embed))
        out_embed = self.dropout(out_embed)
        out_embed = torch.squeeze(torch.tanh(self.concat_fc3(out_embed))) # [B, H_p]
        final_embed = out_embed

        # acqt_flatten = torch.flatten(p_acqt)
        # acqt_lookup = torch.index_select(P_embed, 0, acqt_flatten)
        # acqt_embed = torch.reshape(acqt_lookup, (-1,2,self.Phidden_size))

        # out_embed = torch.unsqueeze(out_embed, dim=1)
        # final_embed = torch.cat((out_embed,acqt_embed), dim =1)
        # final_embed = torch.squeeze(torch.mean(final_embed,dim=1))
        # P_embed[manIdxes,:] = final_embed
        return final_embed
    #
    def Ppredict(self, embed):
        x = F.relu(self.fc1(embed))
        x = self.dropout(x)
        x = F.relu(self.fc2(x))
        x = self.dropout(x)
        x = self.fc3(x)
        # Apply sigmoid to x
        output = torch.sigmoid(torch.squeeze(x))  # [B, 2]
        return output
    #
    def forward(self, manIdxes, p_acqt, p_input):
        # p_acqt: [B,2]
        # risk prediction func f: H_p -> [2]
        out_embed = self.update_P_Embed(manIdxes, p_acqt, p_input) # [B, H_p]
        output = self.Ppredict(out_embed)
        return output


class LocationRisk(nn.Module):
    def __init__(self, Phidden_size, Lhidden_size, max_density):
        super(LocationRisk, self).__init__()
        self.Lhidden_size = Lhidden_size
        self.Phidden_size = Phidden_size
        # self.P_embed = nn.Embedding(MAN_NUM + START_IDX, args.Phidden_size, padding_idx=0)
        # convert matrix
        # self.P2L = nn.Parameter(torch.rand(args.Phidden_size, args.Lhidden_size), requires_grad = True)
        # sum pooling
        self.fc1 = nn.Linear(Lhidden_size, Lhidden_size//2)
        self.fc2 = nn.Linear(Lhidden_size//2, Lhidden_size//4)
        self.fc3 = nn.Linear(Lhidden_size//4, 1)
        self.dropout = nn.Dropout(0.2)
    #
    def update_L_embed(self, locIndexes, l_input):
        # locIndexes: [B] ([1])              l_input: [B, density]   ## B=1 here
        l_flatten = torch.flatten(l_input) #[density]
        l_aggre_embed = torch.index_select(P_embed, dim=0, index = l_flatten) # [density, H_p]
        # l_aggre_convert = torch.matmul(l_aggre_embed, self.P2L) # [density, H_l]
        # embed_out = torch.squeeze(torch.matmul(torch.sum(l_aggre_embed, axis = 0), self.P2L) ) # [H_l]
        embed_out = torch.squeeze(torch.sum(l_aggre_embed, axis = 0) ) # [H_l]
        L_embed[locIndexes] = embed_out
        return embed_out
    #
    def Lpredict(self, embed):
        x = F.relu(self.fc1(embed))
        x=  self.dropout(x)
        x = F.relu(self.fc2(x))
        x = self.dropout(x)
        out = torch.squeeze(torch.sigmoid(self.fc3(x)))
        return out
    #
    def forward(self, locIndexes, l_input):
        # Pass data through dropout1
        embed_out = self.update_L_embed(locIndexes, l_input) #[H_l]
        out = self.Lpredict(embed_out)
        return out

############## data preparation

# Create dataset from several tensors with matching first dimension
# Samples will be drawn from the first dimension (rows)


class LocationData(Dataset): 
    def __init__(self, l_locIdx_collect, l_input_collect, l_label_collect, device,batch_size =1):
        super(LocationData, self).__init__()
        self.l_locIdx_collect = l_locIdx_collect
        self.l_input_collect = l_input_collect
        self.l_label_collect = l_label_collect
    
    def __len__(self):
        return len(self.l_label_collect)
    
    def __getitem__(self, index):
        return self.l_locIdx_collect[index], \
            self.l_input_collect[index],  self.l_label_collect[index]


############## training procedure
## model
man_Model = PersonRisk(args.Phidden_size, args.Lhidden_size, STEPS_BACK).to(device)
loc_Model = LocationRisk(args.Phidden_size, args.Lhidden_size, args.max_density).to(device)
man_criterion = nn.CrossEntropyLoss()
loc_criterion = nn.MSELoss()
optimizer1 = optim.SGD(man_Model.parameters(), lr=0.01, momentum=0.9)
optimizer2 = optim.SGD(loc_Model.parameters(), lr=0.01, momentum=0.9)
print("person risk parameters: ")
for name, param in man_Model.named_parameters():
    if param.requires_grad:
        print(name, end = "\t")
print("\nlocation parameters: ")
for name, param in loc_Model.named_parameters():
    if param.requires_grad:
        print(name, end = "\t")
print("\n")
# L_predictions = loc_Model.Lpredict(L_embed)
# print(L_predictions)
# assert 0
## train parameters
engine.generateDayInfection(2)
total_intervene_num = 0
for day in range(SIM_DAYS):  # loop over the dataset multiple times
    # run infection for 1 day
    engine.generateDayInfection(1)
    # # update for 1 hour
    # engine.setTodayDestination();
    # engine.setTodayDuration();
    # collect data
    # for hour in range(HOURS_PER_DAY):
    cur_step = engine.get_cur_step()
    p_manIdx_collect,p_acqt_group_colect, p_input_collect, p_label_collect = collect_person_data(START_IDX, MAN_NUM + START_IDX, cur_step)
    p_dataset = torch.utils.data.TensorDataset(torch.LongTensor(p_manIdx_collect).to(device), torch.LongTensor(p_acqt_group_colect).to(device), torch.LongTensor(p_input_collect).to(device), torch.LongTensor(p_label_collect).to(device))
    # get dataloader
    person_loader = torch.utils.data.DataLoader(p_dataset, batch_size=64, shuffle=True)
    running_loss = 0.0
    # person_iteration
    for epoch in range(args.epochs):
        for step, data_slice in enumerate(person_loader):
            # get the inputs; data is a list of [inputs, labels]
            p_manIdx, p_acqt, p_input, p_label = data_slice
            # zero the parameter gradients
            optimizer1.zero_grad()
            # forward + backward + optimize
            p_output = man_Model(p_manIdx, p_acqt, p_input)
            loss = man_criterion(p_output, p_label)
            loss.backward(retain_graph = True)
            optimizer1.step()
            # print statistics
            running_loss += loss.item()
            if TRAIN_VERBOSE and step % 50 == 49:  # print every 2000 mini-batches
                print('[%d, %5d] loss: %.3f' %
                    (day + 1, step + 1, running_loss / 2000))
                running_loss = 0.0
    # location iteration

    l_locIdx_collect, l_input_collect, l_label_collect = [], [], []
    for train_step in [cur_step]: #[cur_step-10, cur_step-2, cur_step]:
        tmp_l_locIdx_collect, tmp_l_input_collect, tmp_l_label_collect = collect_location_data(START_IDX, LOC_NUM + START_IDX, train_step, args.max_density)
        l_locIdx_collect.extend(tmp_l_locIdx_collect)
        l_input_collect.extend(tmp_l_input_collect)
        l_label_collect.extend(tmp_l_label_collect)
    # get dataloader
    location_loader = LocationData(l_locIdx_collect, l_input_collect, l_label_collect, device = device)#, batch_size =1, shuffle=True) #[31,13,33]
    # print(len(location_loader))
    for epoch in range(args.epochs):
        for step, data_slice in enumerate(location_loader):
            # print(step)
            l_locIdx, l_input, l_label = data_slice
            l_locIdx = torch.LongTensor([l_locIdx])
            l_label = torch.FloatTensor(l_label)
            # zero the parameter gradients
            optimizer2.zero_grad()
            # forward + backward + optimize
            l_output = loc_Model(l_locIdx, torch.LongTensor(l_input).to(device))
            l_label_final = torch.div(l_label[1] , torch.sum(l_label)).to(device)
            loss = loc_criterion(l_output, l_label_final)
            loss.backward(retain_graph = True)
            optimizer2.step()
            # print statistics
            running_loss += loss.item()
            if TRAIN_VERBOSE and step % 20 == 9:  # print every 2000 mini-batches
                print('[%d, %5d] loss: %.3f' %
                    (day + 1, step + 1, running_loss / 2000))
                running_loss = 0.0

    # ----------------------------  location-based intervention  ---------------------- #
    daily_intervene_num = 0
    intervene_history = set()
    if args.mode == "location":
        with torch.no_grad():
            # firstly intervene all infected
            infected_list = engine.getDailyInfectedManList()
            engine.setInterventionGroup(infected_list, intervene_type, intervene_steps)
            # choose riksy locations
            L_predictions = loc_Model.Lpredict(L_embed).detach().cpu().numpy()
            L_risk_rank = np.argsort(-1*L_predictions)
            L_intervene_group = list(L_risk_rank[:1])
            # print("intervene locations", L_intervene_group)
            for intervene_locIdx in L_intervene_group:
                print("-",end = "")
                if intervene_locIdx<START_IDX:
                    continue
                type = engine.getLocType(intervene_locIdx)
                if(type==0):
                    grasp_step = cur_step - 10
                elif type==1:
                    grasp_step = cur_step - 2
                elif type==2:
                    grasp_step = cur_step
                else:
                    assert 0
                Grasp_group= engine.get_hour_loc2man(intervene_locIdx,grasp_step)
                # Grasp_index = np.random.randint(len(Grasp_group))#0
                # Grasp_person = Grasp_group[Grasp_index]
                # engine.setIntervention(Grasp_person, intervene_type, intervene_steps)
                # print(Grasp_group)
                Grasp_people = list(np.random.choice(Grasp_group, int(len(Grasp_group) *args.intervene_ratio), replace=False))
                # print(Grasp_people)
                engine.setInterventionGroup(Grasp_people, intervene_type, intervene_steps)
                daily_intervene_num += len(Grasp_people)
        total_intervene_num += daily_intervene_num
        print('({})'.format(daily_intervene_num),end = " ")
    elif args.mode == "location-only":
        with torch.no_grad():
            # choose riksy locations
            L_predictions = loc_Model.Lpredict(L_embed).detach().cpu().numpy()
            L_risk_rank = np.argsort(-1*L_predictions)
            L_intervene_group = list(L_risk_rank[:1])
            # print("intervene locations", L_intervene_group)
            for intervene_locIdx in L_intervene_group:
                print("-",end = "")
                if intervene_locIdx<START_IDX:
                    continue
                type = engine.getLocType(intervene_locIdx)
                if(type==0):
                    grasp_step = cur_step - 10
                elif type==1:
                    grasp_step = cur_step - 2
                elif type==2:
                    grasp_step = cur_step
                else:
                    assert 0
                Grasp_group= engine.get_hour_loc2man(intervene_locIdx,grasp_step)
                # Grasp_index = np.random.randint(len(Grasp_group))#0
                # Grasp_person = Grasp_group[Grasp_index]
                # engine.setIntervention(Grasp_person, intervene_type, intervene_steps)
                # print(Grasp_group)
                Grasp_people = list(np.random.choice(Grasp_group, int(len(Grasp_group) *args.intervene_ratio), replace=False))
                # print(Grasp_people)
                engine.setInterventionGroup(Grasp_people, intervene_type, intervene_steps)
                daily_intervene_num += len(Grasp_people)
        total_intervene_num += daily_intervene_num
        print('({})'.format(daily_intervene_num),end = " ")
    elif args.mode == "location_person":
        with torch.no_grad():
            L_predictions = loc_Model.Lpredict(L_embed).detach().cpu().numpy()
            L_risk_rank = np.argsort(L_predictions)
            L_intervene_group = list(L_risk_rank[:20])
            # print("here", L_intervene_group)
            for intervene_locIdx in L_intervene_group:
                print("-",end = "")
                if intervene_locIdx<START_IDX:
                    continue
                type = engine.getLocType(intervene_locIdx)
                if(type==0):
                    grasp_step = cur_step - 10
                elif type==1:
                    grasp_step = cur_step - 2
                elif type==2:
                    grasp_step = cur_step
                else:
                    assert 0
                Grasp_group = torch.LongTensor(engine.get_hour_loc2man(intervene_locIdx,grasp_step)).to(device)
                Grasp_prdictions = man_Model.Ppredict(torch.index_select(P_embed,0,Grasp_group)).detach().cpu()
                Grasp_P_risk_rank = torch.argsort(Grasp_prdictions[:,1])
                engine.setIntervention(Grasp_P_risk_rank[0], intervene_type, intervene_steps)
    # ---------------------------- individual-based intervention  ---------------------- #
    elif args.mode == "person":
        with torch.no_grad():
            ####### risk prediction + intervention
            P_predictions = man_Model.Ppredict(P_embed).detach().cpu().numpy()
            # firstly intervene all infected
            infected_list = engine.getDailyInfectedManList()
            engine.setInterventionGroup(infected_list, intervene_type, intervene_steps)
            # intervene_history.update(infected_list)
            # mask infected
            risky_score = P_predictions[:,0] 
            # risky_score[infected_list] = np.inf
            P_risk_rank = np.argsort(risky_score)
            # P_intervention_group = list(P_risk_rank[:args.intervene_num_daily])
            P_intervention_group = list(P_risk_rank[:intervene_day_quota[day]])
            # intervene_history.update(P_intervention_group)
            # print(risky_score[P_intervention_group])
            engine.setInterventionGroup(P_intervention_group, intervene_type, intervene_steps)
            daily_intervene_num =  len(P_intervention_group)
            total_intervene_num += daily_intervene_num
            print('({})'.format(daily_intervene_num),end = " ")
    elif args.mode == "person-only":
        with torch.no_grad():
            ####### risk prediction + intervention
            P_predictions = man_Model.Ppredict(P_embed).detach().cpu().numpy()
            # intervene_history.update(infected_list)
            # mask infected
            risky_score = P_predictions[:,0] 
            # risky_score[infected_list] = np.inf
            P_risk_rank = np.argsort(risky_score)
            # P_intervention_group = list(P_risk_rank[:args.intervene_num_daily])
            P_intervention_group = list(P_risk_rank[:intervene_day_quota[day]])
            # intervene_history.update(P_intervention_group)
            # print(risky_score[P_intervention_group])
            engine.setInterventionGroup(P_intervention_group, intervene_type, intervene_steps)
            daily_intervene_num =  len(P_intervention_group)
            total_intervene_num += daily_intervene_num
            print('({})'.format(daily_intervene_num),end = " ")
        #     ######## symptotic infection-intervention + y predicted risky people
    elif args.mode == "person_random":
        with torch.no_grad():
            ####### risk prediction + intervention
            # random.seed(day)
            P_predictions = man_Model.Ppredict(P_embed).detach().cpu()
            P_risk_rank = torch.argsort(P_predictions[:,1])
            P_intervention_group = list(P_risk_rank[:int(args.intervene_num_daily/2)])
            P_intervention_group.extend(random.sample(manIdx_list,int(args.intervene_num_daily/2)))
            engine.setInterventionGroup(P_intervention_group, intervene_type, intervene_steps)
    # verbose
    print("day {}: asymptomatic people num: {}, symptomatic people num: {}".format(day, engine.getIncubationCount(),engine.getInfectedCount()))
    # print(P_risk_rank)
    # assert 0
print('Finished Training, total_intervene_num is ', total_intervene_num)

