#include "sys_model.h"
#include "routing_table.h"
#include "system_generator.h"
#include "opmb.h"
#include "common.h"
#include <unordered_set>
#include <queue>

vector<struct priority_assignment> getFixableAssignment(struct sys_model *model)
{
    //printf("[getFixableAssignment] start\n");
    //fflush(stdout);

    vector<struct priority_assignment> fix;

    for(int ch = 1; ch <= model->nCh; ch++)
    { 
	for(int i = 0; i < model->channels[ch].frames.size(); i++)
	{
            if(model->channels[ch].frames[i].priorityAssigned)
		continue;
	    
	    int responseTime = getFrameResponseTime(model, ch, i);
	    if(responseTime > model->channels[ch].frames[i].curDeadline)
		continue;

            if(model->channels[ch].frames[i].fwtype==NOT_FORWARD)
	    {
		// FA1: CLP_j to m_ij when c_ij==NOT_FORWARD and delay_ij <= d_ij
		fix.push_back(makeNewAssignment(model->channels[ch].cs.clp, ch, i));
		return fix;
	    }
	    else if(model->channels[ch].frames[i].fwtype==FORWARD_DEST)
	    {
		// FA2: CLP_j to m_ij when c_ij==FORWARD_DEST
		// and m_i,src \in AM_src, and delay_ij <= d_ij

		if(model->channels[ch].frames[i].crpdFrames[0]->priorityAssigned)
		{
		    fix.push_back(makeNewAssignment(model->channels[ch].cs.clp, ch, i));
		    return fix;
		}
            }
	    else if(model->channels[ch].frames[i].fwtype==FORWARD_SRC)
	    {
		// FA3: CLP_j to m_ij and CLP_k to m_ik when c_ij==FORWARD_SRC, c_ik==FORWARD_DEST
		// \forall delay_ij + delay_ik < d_i - delay_cgw   
		
		bool FA3Test = true;
		for(int k = 0; k < model->channels[ch].frames[i].crpdFrames.size(); k++)
		{
		    int crpdCh = model->channels[ch].frames[i].crpdFrames[k]->channel;
		    int crpdIdx = model->channels[ch].frames[i].crpdFrames[k]->idx;

		    if(model->channels[crpdCh].frames[crpdIdx].priorityAssigned)
		        continue;

		    int crpdResponseTime = getFrameResponseTime(model, crpdCh, crpdIdx);
		    if(responseTime + crpdResponseTime <= model->channels[ch].frames[i].iniDeadline)
			fix.push_back(makeNewAssignment(model->channels[crpdCh].cs.clp, crpdCh, crpdIdx));
		    else
		    {
			fix = vector<struct priority_assignment>();
			FA3Test = false;
			break;
		    }
		}
		if(FA3Test)
		{
		    fix.push_back(makeNewAssignment(model->channels[ch].cs.clp, ch, i));
		    return fix;
		}
	    }
	}
    }
    //printf("[getFixableAssignment] end\n");
    //fflush(stdout);
    return fix;
}

void applyFixableAssignment(struct sys_model *model, vector<struct priority_assignment> fpa)
{
    if(model->logForRe)
    {
        printf("[applyFixableAssignment] start\n");
        fflush(stdout);
    }

    for(int i = 0; i < fpa.size(); i++)
    {
	applyAssignment(model, fpa[i]);
    }
    //printf("[applyFixableAssignment] end\n");
    //fflush(stdout);
}

void revokeFixableAssignment(struct sys_model *model, unsigned int nRevokedPA)
{
    //printf("[revokeFixableAssignment] start\n");
    //fflush(stdout);

    for(int i = 0; i < nRevokedPA; i++)
    {
	revokeAssignment(model);
    }
    //printf("[revokeFixableAssignment] end\n");
    //fflush(stdout);
}

