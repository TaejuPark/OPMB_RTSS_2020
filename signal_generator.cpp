#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "sys_model.h"

bool isfd(int source, struct sys_model *model)
{
    if(model->channels[model->ecus[source].channel].ptype==CANFD)
    {
        return true;
    }
    else
    {
        return false;
    }
}

struct signal generate_signal(struct sys_model *model, int intra_sig_prob)
{
    struct signal new_signal;
    for(int i = 0; i < 10; i++)
	new_signal.destChannels[i] = false;
    int size;
    int period;
    vector<int> destinations;
    vector<int> deadlines;

    int intra_only_percentage = intra_sig_prob;
    //printf("intra_only_percentage = %d\n", intra_only_percentage);

    int source = rand()%(model->nECU) + 1;
    int nDestinations = rand()%4 + 1;
    int r = rand()%100;
    
    if(r < 4)
    {
        period = 1000000;
    }
    else if(r < 7)
    {
        period = 2000000;
    }
    else if(r < 10)
    {
        period = 5000000;
    }
    else if(r < 41)
    {
        period = 10000000;
    }
    else if(r < 72)
    {
        period = 20000000;
    }
    else if(r < 75)
    {
        period = 50000000;
    }
    else if(r < 95)
    {
        period = 100000000;
    }
    else if(r < 96)
    {
        period = 200000000;
    }
    else
    {
        period = 1000000000;
    }

    //if(period < 5000000)
    //{
    //    period = 5000000;
    //}

    if(!(rand()%100 < intra_only_percentage))
    {
        for(int i = 0; i < nDestinations; i++)
        {
            int dest_ecu;
            while(true)
            {
                dest_ecu = rand()%(model->nECU) + 1;
                if(model->ecus[dest_ecu].channel != model->ecus[source].channel)
                {
                    break;
                }
            }
            destinations.push_back(dest_ecu);
	    new_signal.destChannels[model->ecus[dest_ecu].channel] = true;
            deadlines.push_back(period);
        /*r = rand()%100;
        if(r < 4)
        {
            deadlines.push_back(1000000);
        }
        else if(r < 7)
        {
            deadlines.push_back(2000000);
        }
        else if(r < 10)
        {
            deadlines.push_back(5000000);
        }
        else if(r < 41)
        {
            deadlines.push_back(10000000);
        }
        else if(r < 72)
        {
            deadlines.push_back(20000000);
        }
        else if(r < 75)
        {
            deadlines.push_back(50000000);
        }
        else if(r < 95)
        {
            deadlines.push_back(100000000);
        }
        else if(r < 96)
        {
            deadlines.push_back(200000000);
        }
        else
        {
            deadlines.push_back(1000000000);
        }
        
        if(deadlines[deadlines.size()-1] < period)
        {
            deadlines[deadlines.size()-1] = period;
        }*/
        }
    }
    else
    {
        destinations.push_back(source);
	new_signal.destChannels[model->ecus[source].channel] = true;
        deadlines.push_back(period);
    }

    if(!isfd(source, model)) // can signal
    {
        r = rand()%100;
        if(r < 35)
        {
            size = 1;
        }
        else if(r < 84)
        {
            size = 2;
        }
        else if(r < 97)
        {
            size = 4;
        }
        else
        {
            size = rand()%4 + 5;
        }   
    }
    else // can_fd_signal
    {
        r = rand()%1000;
        if(r < 350)
        {
            size = 1;
        }
        else if(r < 840)
        {
            size = 2;
        }
        else if(r < 970)
        {
            size = 4;
        }
        else if(r < 978)
        {
            size = rand()%4 + 5;
        }
        else if(r < 991)
        {
            size = rand()%8 + 9;
        }
        else if(r < 996)
        {
            size = rand()%16 + 17;
        }
        else
        {
            size = rand()%32 + 33;
        }
    }
    new_signal.size = size;
    new_signal.period = period;
    new_signal.source = source;
    new_signal.destinations = destinations;
    new_signal.deadline = period;
    return new_signal;
}
