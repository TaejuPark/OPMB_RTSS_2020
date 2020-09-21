#include "sys_model.h"
#include "common.h"
#include <cmath>

int getMin(int a, int b)
{
    if(a>b)
	return b;
    else
	return a;
}

int getMax(int a, int b)
{
    if(a>b)
	return a;
    else
	return b;
}

string getVisitMapKey(int ch, int idx)
{
    return to_string(ch) + "-" + to_string(idx);
}

double getMaximumChannelUtilization(struct sys_model *model)
{
    double maxChannelUtilization = 0.0;
    for(int i = 1; i <= model->nCh; i++)
    {
	if(model->channels[i].utilization > maxChannelUtilization)
	    maxChannelUtilization = model->channels[i].utilization;
    }
    return maxChannelUtilization;
}

double getAverageChannelUtilization(struct sys_model *model)
{
    double sumChannelUtilization = 0.0;

    for(int i = 1; i <= model->nCh; i++)
	sumChannelUtilization += model->channels[i].utilization;

    return sumChannelUtilization / (double)model->nCh;
}

double getLBChannelUtilization(struct sys_model *model, int ch)
{
    int periods[9] = {1000000, 2000000, 5000000, 10000000, 20000000, 50000000, 100000000, 200000000, 1000000000};
    double lbUtil = 0.0;
    int linkspeed = model->channels[ch].linkspeed;
    int room = 0;

    unordered_set<int> srcECU;
    for(int i = 0; i < model->signals.size(); i++)
    {
	if(model->signals[i].destChannels[ch] == true)
	{
	    if(srcECU.find(model->signals[i].source) == srcECU.end())
	    {
		srcECU.insert(model->signals[i].source);
	    }
	}
    }

    for(auto src : srcECU)
    {			
	for(int j = 0; j < 9; j++) 
	{
	    int sumSignalSize = 0;
            for(int i = 0; i < model->signals.size(); i++)
            {
	        if(model->signals[i].source == src && model->signals[i].period == periods[j])
	        {
		    sumSignalSize += model->signals[i].size;
	        }
            }

	    if(model->channels[ch].ptype == CANFD)
	    {
	        int lbNumFrame = floor((double)sumSignalSize / (double) 64);
	        int remainByte = sumSignalSize % 64;
	        int totalTransTime = (lbNumFrame * 536) * (1000000000/linkspeed) + (lbNumFrame * 28 * 2000);
	        if(remainByte > room)
	        {
		    totalTransTime += ((remainByte-room) * 8 + 24) * (1000000000/linkspeed) + 28 * 2000;
		    room = (64 + room) - remainByte;
	        }
	        lbUtil += (double)totalTransTime / (double)periods[j];
	    }
	    else
	    {
	        int lbNumFrame = floor((double)sumSignalSize / (double) 8);
	        int remainByte = sumSignalSize % 8;
	        int totalTransTime = (lbNumFrame * 108) * (1000000000/linkspeed);
	        if(remainByte > room)
	        {
		    totalTransTime += ((remainByte-room) * 8 + 47) * (1000000000/linkspeed);
		    room = (8 + room) - remainByte;
	        }
	        lbUtil += (double) totalTransTime / (double)periods[j];
	    }
	}

	//printf("ch = %d, linkspeed = %d, period = %d, sumSize = %d, accumlbUtil = %.4lf\n", ch, linkspeed, periods[j], sumSignalSize, lbUtil);
	//fflush(stdout);
    }

    return lbUtil;
}

double getMaximumLBChannelUtilization(struct sys_model *model)
{
    double maxLBChannelUtilization = 0.0;
    for(int i = 1; i <= model->nCh; i++)
    {
	double lbChUtil = getLBChannelUtilization(model, i);
	if(lbChUtil > maxLBChannelUtilization)
	{
	    maxLBChannelUtilization = lbChUtil;
	}
    }

    return maxLBChannelUtilization;
}

int getMergedFrameIndex(struct sys_model *model, int ch)
{
    for(int i = 0; i < model->channels[ch].frames.size(); i++)
    {
	if(model->channels[ch].frames[i].id == model->maxFrameID -1)
	{
	    return i;
	}
    }

    return -1;
}

void initSystemState(struct sys_model *model)
{
    //printf("[initSystemState]\n");
    //fflush(stdout);

    for(int i = 1; i <= model->nCh; i++)
    {
	model->channels[i].cs.clp = 1;
	for(int j = 0; j < 1000; j++)
	{
	    model->channels[i].cs.AM[j] = false;
	    model->channels[i].cs.UM[j] = true;
	}
    }
    model->pa = vector<struct priority_assignment>();
}

struct priority_assignment makeNewAssignment(int priority, int ch, int fIdx)
{
    struct priority_assignment newAssignment;
    newAssignment.priority = priority;
    newAssignment.ch = ch;
    newAssignment.fIdx = fIdx;

    return newAssignment;
}

void applyAssignment(struct sys_model *model, struct priority_assignment pa)
{
    //printf("[applyAssignment]\n");
    //fflush(stdout);

    int ch = pa.ch;
    int fIdx = pa.fIdx;
    int responseTime = getFrameResponseTime(model, ch, fIdx);

    model->pa.push_back(pa);
    model->nUM--;
    model->channels[ch].cs.clp++;
    model->channels[ch].cs.AM[fIdx] = true;
    model->channels[ch].cs.UM[fIdx] = false;
    model->channels[ch].frames[fIdx].priorityAssigned = true;
    model->channels[ch].frames[fIdx].responseTime = responseTime;

    if(model->logForRe)
    {
        printf("[applyAssignment] id = %d, ch = %d, nUM = %d, responseTime = %d\n", 
                model->channels[ch].frames[fIdx].id, ch, model->nUM, model->channels[ch].frames[fIdx].responseTime);
        fflush(stdout);
    }
}

void revokeAssignment(struct sys_model *model)
{
    //printf("[revokeAssignment]\n");
    //fflush(stdout);

    struct priority_assignment pa = model->pa.back();
    int ch = pa.ch;
    int fIdx = pa.fIdx;

    model->channels[ch].frames[fIdx].responseTime = 0;
    model->channels[ch].frames[fIdx].priorityAssigned = false;
    model->channels[ch].cs.UM[fIdx] = true;
    model->channels[ch].cs.AM[fIdx] = false;
    model->channels[ch].cs.clp--;
    model->nUM++;
    model->pa.pop_back();

    if(model->logForRe)
    {
        printf("[revokeAssignment] id = %d, ch = %d, nUM = %d\n", 
                model->channels[ch].frames[fIdx].id, ch, model->nUM);
        fflush(stdout);
    }
}

int getFrameResponseTimeAtHighestPriority(struct sys_model *model, int ch, int fIdx)
{
    // response_time = release jitter + blocking time + transmission time
    int responseTime = 0;
    int releaseJitter = 0; // assume release jitter is 0;
    int blockTime = 0;
    
    for(int i = 0; i < model->channels[ch].frames.size(); i++)
    {
	if(i==fIdx)
	    continue;

        int frameTransTime = model->channels[ch].frames[i].transmission_time;
        if(frameTransTime > blockTime)
	{
            blockTime = frameTransTime;
        }
    }

    blockTime = getMin(blockTime, get_frame_transmission_time(model->channels[ch].ptype, model->channels[ch].ptype==CAN?8:64, model->channels[ch].linkspeed));

    responseTime += releaseJitter;
    responseTime += blockTime;
    responseTime += model->channels[ch].frames[fIdx].transmission_time;
    //printf("release jitter = %d, blockTime = %d, transmission_time = %d\n", releaseJitter, blockTime, model->channels[ch].frames[fIdx].transmission_time);
    //fflush(stdout);
    return responseTime;
}

int getMaxQ(struct sys_model *model, int ch, int fIdx)
{
    long long int longestBusyPeriod = 0;
    long long int releaseJitter = 0; // assume release jitter is 0;
    long long int blockTime = 0;
    for(int i = 0; i < model->channels[ch].frames.size(); i++)
    {
	if(i==fIdx)
	    continue;

        if(model->channels[ch].cs.AM[i] == true) // if frames[i] is priority assigned (lower priority than fIdx)
        {
            int frameTransTime = model->channels[ch].frames[i].transmission_time;
            if(frameTransTime > blockTime)
	    {
                blockTime = frameTransTime;
            }
        }
    }

    long long int interference = 0;
    long long int init_q_delay = blockTime;
    long long int n_q_delay = 0;
    long long int tau_bit = 2000;
    while(true)
    {
        n_q_delay = init_q_delay + interference;
        if(n_q_delay > model->channels[ch].frames[fIdx].curDeadline)
        {
            break;
        }
        long long int inter_temp = 0;
        for(int i = 0; i < model->channels[ch].frames.size(); i++)
        {
            if(model->channels[ch].cs.UM[i] == true) // if frames[i] is not priority assigned (higher priority than fIdx or fIdx itself)
            {
                long long int frameTransTime = model->channels[ch].frames[i].transmission_time;
                inter_temp += (ceil((double)(n_q_delay + tau_bit) / (double) model->channels[ch].frames[i].period) * frameTransTime);
            }
        }

        interference = inter_temp;
        if(n_q_delay == init_q_delay + interference)
        {
            break;
        }
    }
    longestBusyPeriod += n_q_delay;

    int maxQ = ceil((double)(longestBusyPeriod + releaseJitter) / (double) model->channels[ch].frames[fIdx].period) - 1;
    if(maxQ < 0)
    {
	printf("Wrong q calculation\n");
	fflush(stdout);
    }
    return maxQ;
}

long long int getFrameResponseTime(struct sys_model *model, int ch, int fIdx)
{
    int maxQ = getMaxQ(model, ch, fIdx);
    long long int maxResponseTime = 0;

    for(int q = 0; q <= maxQ; q++)
    {
 	long long int responseTimeForQthInstance = getFrameResponseTimeForQthInstance(model, ch, fIdx, q);
	if(maxResponseTime <= responseTimeForQthInstance)
	    maxResponseTime = responseTimeForQthInstance;

	if(maxResponseTime > model->channels[ch].frames[fIdx].curDeadline)
	    break;
    }

    return maxResponseTime;
}

long long int getFrameResponseTimeForQthInstance(struct sys_model *model, int ch, int fIdx, int q)
{	
    long long int responseTime = 0;
    long long int releaseJitter = 0; // assume release jitter is 0;

    // queuing delay = blocking time + q*transmission time + interference
    long long int blockTime = 0;
    
    for(int i = 0; i < model->channels[ch].frames.size(); i++)
    {
	if(i==fIdx)
	    continue;

        if(model->channels[ch].cs.AM[i] == true) // if frames[i] is priority assigned (lower priority than fIdx)
        {
            int frameTransTime = model->channels[ch].frames[i].transmission_time;
            if(frameTransTime > blockTime)
	    {
                blockTime = frameTransTime;
            }
        }
    }

    // queuing delay = blocking time + q*transmission time + interference
    long long int interference = 0;
    long long int init_q_delay = blockTime + (q * model->channels[ch].frames[fIdx].transmission_time);
    long long int n_q_delay = 0;
    long long int tau_bit = 2000;
    while(true)
    {
        n_q_delay = init_q_delay + interference;
        if(n_q_delay > model->channels[ch].frames[fIdx].curDeadline)
        {
            break;
        }
        long long int inter_temp = 0;
        for(int i = 0; i < model->channels[ch].frames.size(); i++)
        {
	    if(i==fIdx)
		continue;

            if(model->channels[ch].cs.UM[i] == true) // if frames[i] is not priority assigned (higher priority than fIdx)
            {
                long long int frameTransTime = model->channels[ch].frames[i].transmission_time;
                inter_temp += (ceil((double)(n_q_delay + tau_bit) / (double) model->channels[ch].frames[i].period) * frameTransTime);
            }
        }

        interference = inter_temp;
        if(n_q_delay == init_q_delay + interference)
        {
            break;
        }
    }

    // response_time = release jitter + queuing delay - q*period + transmission time
    responseTime += releaseJitter;  
    responseTime += n_q_delay;
    responseTime -= (q * model->channels[ch].frames[fIdx].period);
    responseTime += model->channels[ch].frames[fIdx].transmission_time;
    fflush(stdout);

    return responseTime;
}

bool solutionVerification(struct sys_model *model, int gpw)
{
    for(int i = 0; i < model->signals.size(); i++)
    {
	struct frame *srcFrame = model->signals[i].srcFrame;
	if(srcFrame->responseTime==0)
	{
	    //printf("[verify] srcFrame->responseTime = 0, srcID = %d, ch = %d\n", srcFrame->id, srcFrame->channel);
	    //fflush(stdout);
	    return false;
	}

	if(srcFrame->responseTime > (model->signals[i].deadline))
	{
	    //printf("[verify] srcFrame->responseTime = %d, signalDeadline = %d\n",
	    //          srcFrame->responseTime, model->signals[i].deadline);
	    return false;
	}

	for(int j = 0; j < model->signals[i].destFrames.size(); j++)
	{
	    struct frame *destFrame = model->signals[i].destFrames[j];

	    if(destFrame->responseTime==0)
	    {
	        //printf("[verify] destFrame->responseTime = 0, destID = %d, ch = %d\n", destFrame->id, destFrame->channel);
	        //fflush(stdout);
		return false;
	    }

	    if(srcFrame->responseTime + destFrame->responseTime > (model->signals[i].deadline - gpw))
	    {
	        //printf("[verify] srcFrame->responseTime = %d, destFrame->responseTime = %d, signalDeadline = %d\n", 
		//	    srcFrame->responseTime, destFrame->responseTime, model->signals[i].deadline - gpw);
	        //fflush(stdout);
		return false;
	    }
	}
    }
    return true;
}
