#include <cstdio>
#include "sys_model.h"
#include "opmb.h"
#include "common.h"
#include <cmath>
#include <ctime>
#include <unistd.h>

clock_t opmbStartTime;
int schdLevel;
void initOPMB(struct sys_model *model)
{
    opmbStartTime = clock();
    schdLevel = 0;
}

int failureCheck(struct sys_model *model, vector<struct priority_assignment> schd, int nCh)
{
    vector<bool> checker;
    for(int i = 0; i <= nCh; i++)
	checker.push_back(false);

    for(int i = 0; i < schd.size(); i++)
	checker[schd[i].ch] = true;

    for(int i = 1; i <= nCh; i++)
    {
	if(checker[i] == false)
	{
	    for(int j = 0; j < model->channels[i].frames.size(); j++)
	    {
		if(model->channels[i].frames[j].priorityAssigned == false)
		    return i;
	    }
	}
    }

    return 0;
}

bool isCrpdFrameInUMf(struct sys_model *model, struct priority_assignment pa, struct fail_state *fs)
{
    int ch = pa.ch;
    int fIdx = pa.fIdx;

    for(int i = 0; i < model->channels[ch].frames[fIdx].crpdFrames.size(); i++)
    {
	if(model->channels[ch].frames[fIdx].crpdFrames[i]->channel == fs->ch &&
	    fs->cs.UM[model->channels[ch].frames[fIdx].crpdFrames[i]->idx] == true)
	{
	    return true;
	}
    }

    return false;
}

struct fail_state getFailState(struct sys_model *model, int ch)
{
    struct fail_state fs;
    fs.ch = ch;
    for(int i = 0; i < 1000; i++)
    {
	fs.cs.AM[i] = model->channels[ch].cs.AM[i];
	fs.cs.UM[i] = model->channels[ch].cs.UM[i];
    }
    fs.cs.clp = model->channels[ch].cs.clp;

    return fs;
}

struct fail_state OPMB(struct sys_model *model)
{
    //printf("[OPMB] 1\n");
    //fflush(stdout);

    struct fail_state fs;
    fs.ch = -1;

    //printf("[OPMB] 2\n");
    //fflush(stdout);
    if(model->nUM == 0)
    {
        model->isSolutionFind = true;
        return fs;
    }

    //printf("[OPMB] 3\n");
    //fflush(stdout);
    // Timeout: 72000s = 20hr (set to 10s Timeout now)
    if(((double)(clock() - opmbStartTime)/CLOCKS_PER_SEC) > 10000)
    {
        model->isTimeout = true;
	return fs;
    }


    //printf("[OPMB] 4\n");
    //fflush(stdout);
    vector<struct priority_assignment> fix = getFixableAssignment(model);
    if(fix.size() > 0)
    {
	applyFixableAssignment(model, fix);      
	fs = OPMB(model);
	if(model->isSolutionFind || model->isTimeout)
	{
	    //printf("Solution find or Timeout\n");
	    //fflush(stdout);
            return fs;
	}
	revokeFixableAssignment(model, fix.size());
	return fs;
    }

    //printf("[OPMB] 5\n");
    //fflush(stdout);
    vector<struct priority_assignment> schd = getSchedulableAssignment(model);
    int failureCh = failureCheck(model, schd, model->nCh);
    if(failureCh)
    {
	//printf("Channel %d Failure\n", failureCh);
	//fflush(stdout);
	return getFailState(model, failureCh);
    }


    //printf("[OPMB] 6\n");
    //fflush(stdout);
    //printf("[enter] schedulable assignment size = %lu, schdLevel = %d\n", schd.size(), ++schdLevel);
    //fflush(stdout);
    int selIdx = -1;
    for(int i = 0; i < schd.size(); i++)
    {
        if(fs.ch != -1)
        {
            if(schd[selIdx].ch != fs.ch && !isCrpdFrameInUMf(model, schd[selIdx], &fs))
                continue;
        }

	selIdx = i;
	vector<int> storedCrpdFrameDeadline = applySchedulableAssignment(model, schd[i]);
        fs = OPMB(model);
        if(model->isSolutionFind || model->isTimeout)
            return fs;
	revokeSchedulableAssignment(model, storedCrpdFrameDeadline);
    }
    //printf("[exit] schedulable assignment\n");
    //schdLevel--;
    //fflush(stdout);
        
    //printf("[OPMB] 7\n");
    //fflush(stdout);
    return fs;
}

