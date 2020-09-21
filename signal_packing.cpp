#include <cmath>
#include <cstdio>
#include <algorithm>
#include "sys_model.h"

bool compare_signal_utilization(struct signal s1, struct signal s2)
{
    double s1_utilization = (double)s1.size / (double)s1.period;
    double s2_utilization = (double)s2.size / (double)s2.period;

    if(s1_utilization < s2_utilization)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool compare_signal_period(struct signal s1, struct signal s2)
{
    if(s1.period < s2.period)
	return true;
    else
	return false;
}

int get_frame_transmission_time(enum protocol_type ptype, int framesize, int linkspeed)
{
    if(ptype==CANFD)
    {
	int trans_time = 0;
	int temp_frame_size = framesize;

	while(temp_frame_size > 64)
	{
	    trans_time += 32 * 2000 + (28 + 5 * ceil((double)(64 - 16) / (double)64) + 10 * 64) * (1000000000/linkspeed);
	    temp_frame_size -= 64;
	}

	if(temp_frame_size >= 1)
	{
	    int adj_framesize = temp_frame_size;
	
	    if(temp_frame_size > 8 && temp_frame_size <= 12)
	        adj_framesize = 12;
	    else if(temp_frame_size > 12 && temp_frame_size <= 16)
	        adj_framesize = 16;
	    else if(temp_frame_size > 16 && temp_frame_size <= 20)
	        adj_framesize = 20;
	    else if(temp_frame_size > 20 && temp_frame_size <= 24)
	        adj_framesize = 24;
	    else if(temp_frame_size > 24 && temp_frame_size <= 32)
	        adj_framesize = 32;
	    else if(temp_frame_size > 32 && temp_frame_size <= 48)
	        adj_framesize = 48;
	    else if(temp_frame_size > 48 && temp_frame_size <= 64)
	        adj_framesize = 64;

            trans_time += 32 * 2000 + (28 + 5 * ceil((double)(adj_framesize - 16) / (double)64) + 10 * adj_framesize) * (1000000000/linkspeed);
	}

	return trans_time;
    }
    else
    {
        int trans_time = 0;
        int temp_frame_size = framesize;
	
        while(temp_frame_size > 8)
        {
            trans_time += (55 + 10*8) * (1000000000/linkspeed);
            temp_frame_size -= 8;
        }
        
        if(temp_frame_size >= 1)
            trans_time += (55 + 10*temp_frame_size) * (1000000000/linkspeed);

        return trans_time;
    }
}

int get_frame_period(struct frame *fr)
{
    int period;
    
    if(fr->signals.size()==1)
    {
        return fr->signals[0].period;
    }
    else
    {
        period = fr->signals[0].period;
    }

    for(int i = 1; i < fr->signals.size(); i++)
    {
        period = __gcd(period, fr->signals[i].period);
    }

    return period;
}

int get_frame_deadline(struct frame *fr)
{
    int deadline = 2000000000;

    for(int i = 0; i < fr->signals.size(); i++)
    {
        if(fr->signals[i].deadline < deadline)
        {
            deadline = fr->signals[i].deadline;
        }
    }

    return deadline;
}

double get_frame_utilization(struct frame *fr)
{
    //printf("trans_time = %d, period = %d\n", fr->transmission_time, fr->period);
    return (double)fr->transmission_time / (double)fr->period;
}

void greedy_bw_best_fit(struct sys_model *model) // Algorithm2 in "The Multi-Domain Frame Packing Problem for CAN-FD", ECRTS 2017
{
    double current_ch_util[11];
    for(int i = 1; i <= model->nCh; i++)
    {
	current_ch_util[i] = 0.0;
    }

    for(int i = 1; i < model->ecus.size(); i++)
    {
        // Sort(S(psi_i))
        sort(model->ecus[i].signals.begin(), model->ecus[i].signals.end(), compare_signal_period);

        // list of frames = null
        vector<struct frame> frames;

        for(int k = 0; k < model->ecus[i].signals.size(); k++)
        {
	    double added_util[300][11];
	    for(int p = 0; p < 300; p++)
	    {
		for(int q = 0; q < 11; q++)
		{
		    added_util[p][q] = 0.0;
		}
	    }

            // Create a new frame F_np1 containing only sigma_k
            struct frame new_frame;
            new_frame.signals.push_back(model->ecus[i].signals[k]);
	    new_frame.fwtype = NOT_FORWARD;
            new_frame.size = model->ecus[i].signals[k].size;
            new_frame.period = model->ecus[i].signals[k].period;
            new_frame.iniDeadline = model->ecus[i].signals[k].deadline;
	    new_frame.curDeadline = new_frame.iniDeadline;
            new_frame.transmission_time = get_frame_transmission_time(model->channels[model->ecus[i].channel].ptype, new_frame.size, model->channels[model->ecus[i].channel].linkspeed);
            new_frame.utilization = get_frame_utilization(&new_frame);
            
            // [Start] Compute the total BW utilization u_np1 of frames F_1, ..., F_n, F_np1
            double u_np1 = 0.0;           
	    bool sig_k_related_ch[11];

            for(int z = 1; z <= model->nCh; z++)
            {
                u_np1 += current_ch_util[z];
		sig_k_related_ch[z] = false;
            }
	    
	    // discover signal_k related channel
	    sig_k_related_ch[model->ecus[i].channel] = true;
	    for(int z = 0; z < model->ecus[i].signals[k].destinations.size(); z++)
	    {
	        sig_k_related_ch[model->ecus[model->ecus[i].signals[k].destinations[z]].channel] = true;
	    }

	    for(int z = 1; z <= model->nCh; z++)
	    {
		if(sig_k_related_ch[z])
		{
		    int added_transtime_at_ch = get_frame_transmission_time(model->channels[z].ptype, model->ecus[i].signals[k].size, model->channels[z].linkspeed);
		    double added_util_at_ch = (double)added_transtime_at_ch / (double) new_frame.period;
		    u_np1 += added_util_at_ch;
		    added_util[frames.size()][z] = added_util_at_ch;
		}
	    }

            // [End] Compute the total BW utilization u_np1 of frames F_1, ..., F_n, F_np1
            vector<double> u_j;
            for(int j = 0; j < frames.size(); j++)
            {
                if((model->channels[model->ecus[i].channel].ptype == CANFD && frames[j].size + model->ecus[i].signals[k].size <= 64)
                   || (model->channels[model->ecus[i].channel].ptype == CAN && frames[j].size + model->ecus[i].signals[k].size <= 8))
                {
                    // Add sigma_k to F_j
                    frames[j].signals.push_back(model->ecus[i].signals[k]);
                    frames[j].size += model->ecus[i].signals[k].size;
                    frames[j].period = get_frame_period(&frames[j]);
                    frames[j].iniDeadline = get_frame_deadline(&frames[j]);
		    frames[j].curDeadline = frames[j].iniDeadline;
                    frames[j].transmission_time = get_frame_transmission_time(model->channels[model->ecus[i].channel].ptype, frames[j].size, model->channels[model->ecus[i].channel].linkspeed);
                    frames[j].utilization = get_frame_utilization(&frames[j]);

                    // [Start] Compute the total BW utilization u_j of frames F_1, ..., F_n
		    bool frame_j_related_ch[11];
                    u_j.push_back(0);
		    for(int z = 1; z <= model->nCh; z++)
		    {
			frame_j_related_ch[z] = false;
                        u_j[j] += current_ch_util[z];
		    }

		    // discover frame_j related channel
		    for(int p = 0; p < frames[j].signals.size(); p++)
		    {
			for(int q = 0; q < frames[j].signals[p].destinations.size(); q++)
			{
			    frame_j_related_ch[model->ecus[frames[j].signals[p].destinations[q]].channel]= true;
			}
		    }

		    for(int z = 1; z <= model->nCh; z++)
		    {
			int added_trans_time_at_ch = 0;
			double added_util_at_ch = 0.0;
			
			if(frame_j_related_ch[z] && sig_k_related_ch[z])
			{
			    added_trans_time_at_ch += get_frame_transmission_time(model->channels[z].ptype, frames[j].size, model->channels[z].linkspeed);
			    added_trans_time_at_ch -= get_frame_transmission_time(model->channels[z].ptype, (frames[j].size - model->ecus[i].signals[k].size), model->channels[z].linkspeed);
			}
			else if(sig_k_related_ch[z])
			{
			    added_trans_time_at_ch += get_frame_transmission_time(model->channels[z].ptype, frames[j].size, model->channels[z].linkspeed);
			}

		        added_util_at_ch = (double)added_trans_time_at_ch / (double)frames[j].period;
			u_j[j] += added_util_at_ch;
			added_util[j][z] = added_util_at_ch;
		    }
                    // [End] Compute the total BW utilization u_j of frames F_1, ..., F_n

                    // Remove sigma_k from F_j
                    frames[j].signals.pop_back();
                    frames[j].size -= model->ecus[i].signals[k].size;
                    frames[j].period = get_frame_period(&frames[j]);
                    frames[j].iniDeadline = get_frame_deadline(&frames[j]);
		    frames[j].curDeadline = frames[j].iniDeadline;
                    frames[j].transmission_time = get_frame_transmission_time(model->channels[model->ecus[i].channel].ptype, frames[j].size, model->channels[model->ecus[i].channel].linkspeed);
                    frames[j].utilization = get_frame_utilization(&frames[j]);
                }
                else
                {
                    // Set u_j to infinity
                    u_j.push_back(999999999.0);
                }
            }

            // Find the smallest u_j among u_1, ..., u_np1 and pack sigma_k in F_j
            if(frames.size()==0)
            {
                frames.push_back(new_frame);
                continue;
            }
            
            int min_u_j_idx = distance(u_j.begin(), min_element(u_j.begin(), u_j.end()));

            if(u_j[min_u_j_idx] > u_np1)
            {
                // pack sigma_k in F_np1
		for(int z = 1; z <= model->nCh; z++)
		{
		    current_ch_util[z] += added_util[frames.size()][z];
		}
                frames.push_back(new_frame);
            }
            else
            {
                // pack sigma_k in F_j where u_j is smallest value
                frames[min_u_j_idx].signals.push_back(model->ecus[i].signals[k]);
                frames[min_u_j_idx].size += model->ecus[i].signals[k].size;
                frames[min_u_j_idx].period = get_frame_period(&frames[min_u_j_idx]);
                frames[min_u_j_idx].iniDeadline = get_frame_deadline(&frames[min_u_j_idx]);
		frames[min_u_j_idx].curDeadline = frames[min_u_j_idx].iniDeadline;
                frames[min_u_j_idx].transmission_time = get_frame_transmission_time(model->channels[model->ecus[i].channel].ptype, frames[min_u_j_idx].size, model->channels[model->ecus[i].channel].linkspeed);
                frames[min_u_j_idx].utilization = get_frame_utilization(&frames[min_u_j_idx]);

		for(int z = 1; z <= model->nCh; z++)
		{
		    current_ch_util[z] += added_util[min_u_j_idx][z];
		}
            }
        }
        model->ecus[i].frames = frames;
    }
}

void handling_infeasibility() // Section 6.2 in "The Multi-Domain Frame Packing Problem for CAN-FD", ECRTS 2017
{
}
