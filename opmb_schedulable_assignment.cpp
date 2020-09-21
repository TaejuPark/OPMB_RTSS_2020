#include "sys_model.h"
#include "routing_table.h"
#include "system_generator.h"
#include "opmb.h"
#include "common.h"

vector<struct priority_assignment> getSchedulableAssignment(struct sys_model *model)
{
    //printf("[getSchedulableAssignment] start\n");
    //fflush(stdout);

    vector<struct priority_assignment> schd;
    vector<struct priority_assignment> retSchd;
    vector<int> schdResponseTime;

    for(int ch = 1; ch <= model->nCh; ch++)
    {
	for(int i = 0; i < model->channels[ch].frames.size(); i++)
	{
	    if(model->channels[ch].frames[i].priorityAssigned)
	        continue;

	    if(model->channels[ch].frames[i].fwtype==NOT_FORWARD)
		continue;

            int responseTime = getFrameResponseTime(model, ch, i);
	    if(responseTime <= model->channels[ch].frames[i].curDeadline)
	    {
		schd.push_back(makeNewAssignment(model->channels[ch].cs.clp, ch, i));
		schdResponseTime.push_back(responseTime);
	    }
	}
    }

    // Further reduction of schedulable assignment (Line 9 --- Line 24 in Algorithm2)
    
    unordered_set<int> eraseIdx;
    
    for(int i = 0; i < schd.size(); i++)
    {
	if(eraseIdx.find(i) != eraseIdx.end())
	    continue;

        for(int j = 0; j < schd.size(); j++)
	{
	    if(schd[i].ch != schd[j].ch || i==j || eraseIdx.find(j) != eraseIdx.end())
	        continue;
	    
	    bool removeFlag = true;
	    for(int k = 1; k <= model->nCh; k++)
	    {
		struct frame* iCrpdFrameOnK = NULL;
		struct frame* jCrpdFrameOnK = NULL;
		
		for(int p = 0; p < model->channels[schd[i].ch].frames[schd[i].fIdx].crpdFrames.size(); p++)
		{
		    if(model->channels[schd[i].ch].frames[schd[i].fIdx].crpdFrames[p]->channel==k)
		    {
			iCrpdFrameOnK = model->channels[schd[i].ch].frames[schd[i].fIdx].crpdFrames[p];
			break;
		    }
		}

		for(int p = 0; p < model->channels[schd[j].ch].frames[schd[j].fIdx].crpdFrames.size(); p++)
		{
		    if(model->channels[schd[j].ch].frames[schd[j].fIdx].crpdFrames[p]->channel==k)
		    {
			jCrpdFrameOnK = model->channels[schd[j].ch].frames[schd[j].fIdx].crpdFrames[p];
			break;
		    }
		}

		if(iCrpdFrameOnK == NULL && jCrpdFrameOnK != NULL)
	    	{
		    removeFlag = false;
		    break;
		}

		if(iCrpdFrameOnK != NULL && jCrpdFrameOnK != NULL)
		{
		    int iCrpdMargin = getMin(iCrpdFrameOnK->curDeadline, iCrpdFrameOnK->iniDeadline - schdResponseTime[i]);
		    int jCrpdMargin = getMin(jCrpdFrameOnK->curDeadline, jCrpdFrameOnK->iniDeadline - schdResponseTime[j]);
		    
		    if(iCrpdMargin < jCrpdMargin)
		    {
			removeFlag = false;
			break;
		    }

		    if(iCrpdMargin == jCrpdMargin && iCrpdFrameOnK->size < jCrpdFrameOnK->size)
		    {
			removeFlag = false;
			break;
		    }
		}
	    }

	    if(removeFlag == true)
	    {
		eraseIdx.insert(j);
		//printf("erase (%d)\n", j);
		//fflush(stdout);
	    }
	}
    }
    //printf("schd size = %lu, eraseIdx size = %lu\n", schd.size(), eraseIdx.size());
    //fflush(stdout);

    vector<int> retSchdMargin;
    for(int i = 0; i < schd.size(); i++)
    {
	if(eraseIdx.find(i) == eraseIdx.end())
	{
	    retSchd.push_back(schd[i]);
	}
    }

    /*
    // sort by deadline
    for(int i = 0; i < retSchd.size()-1; i++)
    {
	for(int j = i+1; j < retSchd.size(); j++)
	{
	    if(retSchdMargin[i] > retSchdMargin[j])
	    {
		int tempMargin;
		tempMargin = retSchdMargin[i];
		retSchdMargin[i] = retSchdMargin[j];
		retSchdMargin[j] = tempMargin;

		int tempPriority;
		tempPriority = retSchd[i].priority;
		retSchd[i].priority = retSchd[j].priority;
		retSchd[j].priority = tempPriority;

		int tempCh;
		tempCh = retSchd[i].ch;
		retSchd[i].ch = retSchd[j].ch;
		retSchd[j].ch = tempCh;

		int tempfIdx;
		tempfIdx = retSchd[i].fIdx;
		retSchd[i].fIdx = retSchd[j].fIdx;
		retSchd[j].fIdx = tempfIdx;
	    }
	}
    }*/
    
    //printf("[getSchedulableAssignment] end\n");
    //fflush(stdout);

    return retSchd;
}

vector<int> applySchedulableAssignment(struct sys_model *model, struct priority_assignment spa)
{
    if(model->logForRe)
    {
        printf("[applySchedulableAssignment]\n");
        fflush(stdout);
    }

    int ch = spa.ch;
    int fIdx = spa.fIdx;
    int responseTime = getFrameResponseTime(model, ch, fIdx);
    int forwardMargin;
    vector<int> storedCrpdFrameDeadline;

    applyAssignment(model, spa);

    // Change corresponding frames' deadlines
    
    //if(model->channels[ch].frames[fIdx].fwtype == FORWARD_SRC)
    //    forwardMargin = model->channels[ch].frames[fIdx].iniDeadline - responseTime;
    //else
    //    forwardMargin = model->channels[ch].frames[fIdx].curDeadline - responseTime;

    forwardMargin = model->channels[ch].frames[fIdx].iniDeadline - responseTime;
    for(int i = 0; i < model->channels[ch].frames[fIdx].crpdFrames.size(); i++)
    {
	struct frame *crpdFrame = model->channels[ch].frames[fIdx].crpdFrames[i];
	storedCrpdFrameDeadline.push_back(crpdFrame->curDeadline);
	if(!crpdFrame->priorityAssigned)
	{
	    crpdFrame->curDeadline = getMin(crpdFrame->curDeadline, forwardMargin);
	}
    }
    
    return storedCrpdFrameDeadline;
}

void revokeSchedulableAssignment(struct sys_model *model, vector<int> storedCrpdFrameDeadline)
{
    //printf("[revokeSchedulableAssignment]\n");
    //fflush(stdout);
    struct priority_assignment spa = model->pa.back();
    int ch = spa.ch;
    int fIdx = spa.fIdx;

    // Revoke change of corresponding frames' deadlines
    for(int i = 0; i < model->channels[ch].frames[fIdx].crpdFrames.size(); i++)
    {
	model->channels[ch].frames[fIdx].crpdFrames[i]->curDeadline = storedCrpdFrameDeadline[i];
    }

    revokeAssignment(model);
}
