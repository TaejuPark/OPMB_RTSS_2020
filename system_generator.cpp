#include <cstdlib>
#include <ctime>
#include <unordered_set>
#include <queue>
#include "system_generator.h"
#include "common.h"

vector<struct channel> generate_channels(int nCh)
{
    
    int CANspeed[2] = {250000, 500000};
    int FDspeed[3] = {2000000, 5000000, 8000000};

    vector<struct channel> channels;
    struct channel temp_channel;
    channels.push_back(temp_channel);

    for(int i = 1; i <= nCh; i++)
    {
        struct channel new_channel;
	new_channel.utilization = 0;
	/*
	if(rand()%2==0)
		new_channel.ptype = CAN;
	else
		new_channel.ptype = CANFD;

	if(new_channel.ptype == CAN)
	{
	    //int linkrand = rand()%3;
	    //new_channel.linkspeed = CANspeed[linkrand];
	    new_channel.linkspeed = CANspeed[1];
	}
	else
	{
	    //int linkrand = rand()%3;
	    //new_channel.linkspeed = FDspeed[linkrand];
	    new_channel.linkspeed = FDspeed[0];
	}*/
	new_channel.ptype = CANFD;
	//int linkrand = rand()%3;
	new_channel.linkspeed = CANspeed[1];

	channels.push_back(new_channel);
    }
    return channels;
}

vector<struct ecu> generate_ecus(vector<struct channel> channels)
{
    vector<struct ecu> ecus;
    struct ecu temp_ecu;
    ecus.push_back(temp_ecu);

    int tot_n_ECUs = 0;
    for(int i = 1; i < channels.size(); i++)
    {
	int nECU_for_channel;
	if(channels[i].linkspeed == 250000)
            nECU_for_channel = rand()%2 + 3;
	else if(channels[i].linkspeed == 500000)
            nECU_for_channel = rand()%4 + 4;
	else if(channels[i].linkspeed == 2000000)
	    nECU_for_channel = rand()%4 + 7;
	else if(channels[i].linkspeed == 5000000)
	    nECU_for_channel = rand()%5 + 8;
	else if(channels[i].linkspeed == 8000000)
	    nECU_for_channel = rand()%6 + 10;

	tot_n_ECUs += nECU_for_channel;
	
	for(int j = 1; j <= nECU_for_channel; j++)
        {
	    struct ecu new_ecu;
            new_ecu.channel = i;
            ecus.push_back(new_ecu);
        }
    }
    return ecus;
}

struct routing_table generate_initial_routing_table(struct sys_model *model)
{
    // Initial routing table - assuming all PDU-Direct routing
    struct routing_table init_rt;
    vector<struct pdurt> init_pdurt;
    vector<struct comrt> init_comrt;

    for(int i = 1; i <= model->nECU; i++)
    {
	for(int j = 0; j < model->ecus[i].frames.size(); j++)
	{
	    model->ecus[i].frames[j].id = model->maxFrameID;
	    //model->frames.push_back(model->ecus[i].frames[j]);

	    // Create a temporary item
	    struct pdurt new_pdurt_item;
	    new_pdurt_item.pduid = model->maxFrameID;
	    new_pdurt_item.sbus = model->ecus[i].channel;
	    new_pdurt_item.rtype = PDU_DIRECT;

	    // Destination buses discovery
	    unordered_set<int> dbset; // destination bus set
	    struct frame f = model->ecus[i].frames[j];
	    for(int k = 0; k < f.signals.size(); k++)
	    {
		for(int z = 0; z < f.signals[k].destinations.size(); z++)
		{
		    int dest_ecu = f.signals[k].destinations[z];
		    if(dbset.find(model->ecus[dest_ecu].channel) == dbset.end())
		    {
		    	dbset.insert(model->ecus[dest_ecu].channel);
		    }
		}	
	    }
	    
	    for(auto db : dbset)
	    {
		if(db != new_pdurt_item.sbus)
		{
		    new_pdurt_item.dbus.push_back(db);
		}
	    }
	    
	    // Add new item into PDUR routing table
	    if(new_pdurt_item.dbus.size() != 0)
	    {
		init_pdurt.push_back(new_pdurt_item);
	    }

	    model->maxFrameID++;
	}
    }
    
    init_rt.pdurt = init_pdurt;
    init_rt.comrt = init_comrt;
    return init_rt;
}

void generate_frames_on_src_bus(struct sys_model *model)
{
    //printf("[generate_frames_on_src_bus]\n");
    //fflush(stdout);

    for(int i = 1; i <= model->nECU; i++)
    {
        int ch = model->ecus[i].channel;
        for(int j = 0; j < model->ecus[i].frames.size(); j++)
        {
	    model->frames.push_back(model->ecus[i].frames[j]);
            model->channels[ch].frames.push_back(model->ecus[i].frames[j]);
	    model->nUM++;
        }
    }
}

void generate_frames_on_dest_buses(struct sys_model *model)
{
    //printf("[generate_frames_on_dest_buses]\n");
    //fflush(stdout);

    // Generate PDU Direct routing msgs on dest buses

    for(int i = 0; i < model->rt.pdurt.size(); i++)
    {
	if(model->rt.pdurt[i].rtype == PDU_DIRECT)
	{
	    for(int j = 0; j < model->rt.pdurt[i].dbus.size(); j++)
	    {
		int ch = model->rt.pdurt[i].dbus[j];
		struct frame tframe = model->frames[model->rt.pdurt[i].pduid];
		tframe.channel = ch;
		tframe.fwtype = FORWARD_DEST;
		model->channels[ch].frames.push_back(tframe);
		model->nUM++;
	    }
	}
    }

    // Generate Signal-based routing msgs on dest buse
    // TODO: need to think deadline computation

    for(int i = 0; i < model->rt.comrt.size(); i++)
    {
        if(model->rt.comrt[i].iotype == SEND)
	{
	    struct frame txframe;
	    //txframe.id = rt.comrt[i].pduid;
	    txframe.fwtype = FORWARD_DEST;
	    txframe.id = model->rt.comrt[i].pduid;
	    txframe.size = 0;
	    for(int j = 0; j < model->rt.comrt[i].signals.size(); j++)
	    {
		txframe.signals.push_back(model->rt.comrt[i].signals[j]);
		txframe.size += model->rt.comrt[i].signals[j].size;
		txframe.period = get_frame_period(&txframe);
		txframe.iniDeadline = get_frame_deadline(&txframe);
		txframe.curDeadline = txframe.iniDeadline;
	    }
	    
	    model->frames.push_back(txframe);

	    for(int j = 0; j < model->rt.comrt[i].dbus.size(); j++)
	    {
		int ch = model->rt.comrt[i].dbus[j];
		txframe.channel = ch;
		txframe.curDeadline -= 100000; // 100us for signal-based task 
		model->channels[ch].frames.push_back(txframe);
		model->nUM++;
	    }
	}
    }
}

void signal_period_deadline_reconfigure(struct sys_model *model)
{
    for(int i = 1; i <= model->nECU; i++)
    {
	for(int j = 0; j < model->ecus[i].frames.size(); j++)
	{
	    for(int k = 0; k < model->ecus[i].frames[j].signals.size(); k++)
	    {
		int sid = model->ecus[i].frames[j].signals[k].sid;
	        model->ecus[i].frames[j].signals[k].period = model->ecus[i].frames[j].period;
		model->ecus[i].frames[j].signals[k].deadline = model->ecus[i].frames[j].iniDeadline;
		model->signals[sid].period = model->ecus[i].frames[j].period;
		model->signals[sid].deadline = model->ecus[i].frames[j].iniDeadline;
	    }
	}
    }
}

void compute_chrs_of_generated_frames(struct sys_model *model)
{
    //printf("[compute_chrs_of_generated_frames]\n");
    //fflush(stdout);

    for(int i = 1; i <= model->nCh; i++)
    {
	for(int j = 0; j < model->channels[i].frames.size(); j++)
	{
            model->channels[i].frames[j].channel = i;
	    model->channels[i].frames[j].idx = j;
	    model->channels[i].frames[j].transmission_time = get_frame_transmission_time(model->channels[i].ptype, model->channels[i].frames[j].size, model->channels[i].linkspeed);
	    model->channels[i].frames[j].utilization = ((double) model->channels[i].frames[j].transmission_time / (double) model->channels[i].frames[j].period);
	    //printf("id = %d, ch = %d, trans_time = %d, period = %d, utilization = %.4lf\n", model->channels[i].frames[j].id, i, model->channels[i].frames[j].transmission_time, model->channels[i].frames[j].period, model->channels[i].frames[j].utilization);
	    //printf("frame size = %d, channel linkspeed = %d, signals = ", model->channels[i].frames[j].size, model->channels[i].linkspeed);
	    //for(int k = 0; k < model->channels[i].frames[j].signals.size(); k++)
	//	printf("%d, ", model->channels[i].frames[j].signals[k].sid);
	    //printf("\n");
	    //fflush(stdout);
	    model->channels[i].utilization += model->channels[i].frames[j].utilization;
	}
    }
}

void discover_frame_signal_mapping(struct sys_model *model)
{
    //printf("[discover_frame_signal_mapping]\n");
    //fflush(stdout);

    for(int i = 0; i < model->signals.size(); i++)
    {
        // Discover frames which includes signal i
	vector<struct frame *> frs;
	for(int j = 1; j <= model->nCh; j++)
	{
	    for(int k = 0; k < model->channels[j].frames.size(); k++)
	    {
		for(int z = 0; z < model->channels[j].frames[k].signals.size(); z++)
		{
		    if(model->channels[j].frames[k].signals[z].sid == i)
		    {
			frs.push_back(&(model->channels[j].frames[k]));
		    }
		}
	    }
	}

	for(int j = 0; j < frs.size(); j++)
	{
	    if(frs.size() > 1 && frs[j]->fwtype==NOT_FORWARD)
		frs[j]->fwtype = FORWARD_SRC;
	    else if(frs[j]->fwtype==NOT_FORWARD)
		model->signals[i].srcFrame = frs[j];

	    if(frs[j]->fwtype==FORWARD_SRC)
	    {
		model->signals[i].srcFrame = frs[j];
		//printf("[DFSM] i = %d, signals[i].srcFrame address = %p, frs[j] = %p, model->channels[frs[j]->channel].frames[frs[j]->idx] address = %p, frs[j]->channel = %d, frs[j]->idx = %d\n",
		//		i, model->signals[i].srcFrame, frs[j], &(model->channels[frs[j]->channel].frames[frs[j]->idx]), frs[j]->channel, frs[j]->idx);
		//fflush(stdout);
	    }
	    else if(frs[j]->fwtype==FORWARD_DEST)
		model->signals[i].destFrames.push_back(frs[j]);
	}
    }
}

void discover_chained_crpd_frames(struct sys_model *model)
{
    //printf("[discover_chained_crpd_frames] start\n");
    //fflush(stdout);
    for(int i = 1; i <= model->nCh; i++)
    {
        for(int j = 0; j < model->channels[i].frames.size(); j++)
	{
	    queue<struct frame*> q;
	    unordered_set<string> visitCheckMap;
	    q.push(&(model->channels[i].frames[j]));
	    visitCheckMap.insert(getVisitMapKey(i, j));

	    while(q.size()!=0)
	    {
	        struct frame* fr = q.front();
	        for(int k = 0; k < fr->crpdFrames.size(); k++)
	        {
	            int crpdCh = fr->crpdFrames[k]->channel;
		    int crpdIdx = fr->crpdFrames[k]->idx;
		    string key = getVisitMapKey(crpdCh, crpdIdx);
		    if(visitCheckMap.find(key)==visitCheckMap.end())
		    {
			q.push(&(model->channels[crpdCh].frames[crpdIdx]));
			visitCheckMap.insert(key);
		    }
	        }
		
		if(fr->fwtype==FORWARD_SRC)
		{
		    model->channels[i].frames[j].chainCrpdFrames.push_back(fr);
		}

		q.pop();
	    }
        }
    }
    //printf("[discover_chained_crpd_frames] end\n");
    //fflush(stdout);
}

void discover_crpd_frames(struct sys_model *model)
{
    //printf("[discover_crpd_frames] start\n");
    //fflush(stdout);
    for(int i = 0; i < model->signals.size(); i++)
    {
	for(int j = 0; j < model->signals[i].destFrames.size(); j++)
	{
	    bool flag = true;
	    for(int k = 0; k < model->signals[i].srcFrame->crpdFrames.size(); k++)
	    {
		if(model->signals[i].srcFrame->crpdFrames[k] == model->signals[i].destFrames[j])
		{
		    flag = false;
		    break;
		}
	    }

	    if(flag)
	    {
	        model->signals[i].srcFrame->crpdFrames.push_back(model->signals[i].destFrames[j]);
	        model->signals[i].destFrames[j]->crpdFrames.push_back(model->signals[i].srcFrame);
	    }
	}
    }
    //printf("[discover_crpd_frames] end\n");
    //fflush(stdout);
}

struct sys_model generateInitialSystemModel(int nCh, int sig_per_ch, int intra_sig_prob)
{
    struct sys_model omodel; // Original System Model
    omodel.nUM = 0;
    omodel.isSolutionFind = false;
    omodel.isTimeout = false;
    omodel.logForRe = false;
    omodel.maxFrameID = 0;
    omodel.channels = generate_channels(nCh);
    omodel.ecus = generate_ecus(omodel.channels);
    omodel.nCh = nCh;
    omodel.nECU = omodel.ecus.size()-1;

    // Generate signals
    // sig_per_ch: average number of signals per channel (given as parameter)
    // intra_sig_prob: probability of gatewayed signal (given as parameter)
    for(int i = 0; i < nCh * sig_per_ch; i++) 
    {
        struct signal new_signal = generate_signal(&omodel, intra_sig_prob);
	new_signal.sid = i;
	omodel.signals.push_back(new_signal);
        omodel.ecus[new_signal.source].signals.push_back(new_signal);
    }

    // Packing the generated signals using Algorithm2 in
    // // Prachi Joshi., 'The Multi-Domain Frame Packing Problem for CAN-FD', ECRTS 2017 
    greedy_bw_best_fit(&omodel);
    signal_period_deadline_reconfigure(&omodel);

    omodel.rt = generate_initial_routing_table(&omodel);
    generate_frames_on_src_bus(&omodel);
    generate_frames_on_dest_buses(&omodel);
    compute_chrs_of_generated_frames(&omodel);

    return omodel;
}

void modelUpdateForMerge(struct sys_model *model)
{
     //printf("[modelUpdateForMerge]\n");
     //fflush(stdout);

     // Initialize channel state
     model->nUM = 0;
     initSystemState(model);
     for(int i = 1; i <= model->nCh; i++)
     {
	 model->channels[i].utilization = 0;
	 model->channels[i].frames = vector<struct frame>();
	 model->frames = vector<struct frame>();
     }
     // Generate frames on channels based on current routing table
     generate_frames_on_src_bus(model);
     generate_frames_on_dest_buses(model);

     // Initialize corresponding frames of signals
     for(int i = 0; i < model->signals.size(); i++)
     {
	 model->signals[i].srcFrame = NULL;
	 model->signals[i].destFrames = vector<struct frame*>();
     }

     // Configure properties
     compute_chrs_of_generated_frames(model);
     discover_frame_signal_mapping(model);
     discover_crpd_frames(model);
     discover_chained_crpd_frames(model);
     applyGatewayProcessingDelay(model);
     //reconfiguring_deadline(model);
}

void reconfiguring_deadline(struct sys_model *model)
{
    while(true)
    {
	bool isReconfigure = false;
	for(int i = 1; i <= model->nCh; i++)
	{
	    for(int j = 0; j < model->channels[i].frames.size(); j++)
	    {
		int minDeadline = model->channels[i].frames[j].curDeadline;
		for(int k = 0; k < model->channels[i].frames[j].crpdFrames.size(); k++)
		{
		    if(model->channels[i].frames[j].curDeadline != model->channels[i].frames[j].crpdFrames[k]->curDeadline)
			isReconfigure = true;

		    minDeadline = getMin(minDeadline, model->channels[i].frames[j].crpdFrames[k]->curDeadline);
		}

		model->channels[i].frames[j].curDeadline = minDeadline;

		for(int k = 0; k < model->channels[i].frames[j].crpdFrames.size(); k++)
		{
		    model->channels[i].frames[j].crpdFrames[k]->curDeadline = minDeadline;
		    model->channels[i].frames[j].crpdFrames[k]->iniDeadline = minDeadline;
		}
		    
	    }
	}

	if(isReconfigure == false)
	    break;
    }
}

void mapping_signal_frame_crpdframe(struct sys_model *model)
{
    discover_frame_signal_mapping(model);
    discover_crpd_frames(model);
    discover_chained_crpd_frames(model);
    //reconfiguring_deadline(model);
}

int applyGatewayProcessingDelay(struct sys_model *model)
{
    // Calculate gateway processing delay
    // Jin Ho Kim et al., 'Gateway Framework for in-Vehicle Networks based on CAN, FlexRay and Ethernet', 
    // IEEE Transaction on Vehicular Technology (TVT), Vol.64, Issue 10, Oct, 2015
	
    // Assume gateway have dedicated core for each channel (No waiting time for ISR_rx execution)
    int ISR_rx_waiting_time = 0;
    int ISR_rx_execution_time = 5000; // 5us
    int theta = 1000; //1us
    int T_c = 5000; // 5us
    int task_tx_execution_time = 20000; // 20us
    int num_rt_entry = model->rt.pdurt.size(); // number of routing table entrys
    int routing_table_search_time = theta * num_rt_entry; // 1us * number of routing table entrys

    // gpw: gateway processing delay
    int gpw = ISR_rx_waiting_time + ISR_rx_execution_time + routing_table_search_time + T_c + task_tx_execution_time;
    
    // Apply gateway processing delay to gatewayed messages
    for(int i = 1; i <= model->nCh; i++)
    {
	for(int j = 0; j < model->channels[i].frames.size(); j++)
	{
	    if(model->channels[i].frames[j].fwtype!=NOT_FORWARD)
	    {
		//int multiple;
		//if(model->channels[i].ptype == CAN && model->channels[i].frames[j].size > 8)
		//    multiple = (model->channels[i].frames[j].size / 8) + 1;
		//else
		//    multiple = 1;

		//gpw += (multiple * task_tx_execution_time);
		model->channels[i].frames[j].iniDeadline -= gpw;
		model->channels[i].frames[j].curDeadline = model->channels[i].frames[j].iniDeadline;
	    }
	}
    }

    return gpw;
}
