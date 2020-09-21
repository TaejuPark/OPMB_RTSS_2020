#include "sys_model.h"
#include "common.h"
#include <cstdio>
#include <unordered_set>
#include <stack>
// frame with shortest deadline has the highest priority (global priority)

bool DM(struct sys_model *model)
{
    unordered_set<string> visitMap;
    stack<int> chStack;
    stack<int> idxStack;

    int selFrameCh;
    int selFrameIdx;

    // Accumulate frames into stack (from highest to lowest priority)
    while(true)
    {
	int minDeadline = 1000000000;
	bool flag = false;
	for(int ch = 1; ch <= model->nCh; ch++)
	{
	    for(int i = 0; i < model->channels[ch].frames.size(); i++)
	    {
		string srcKey = getVisitMapKey(ch, i);
		if(model->channels[ch].frames[i].fwtype!=FORWARD_DEST 
		    && model->channels[ch].frames[i].curDeadline <= minDeadline
		    && visitMap.find(srcKey) == visitMap.end())
		{
		    minDeadline = model->channels[ch].frames[i].curDeadline;
		    selFrameCh = ch;
		    selFrameIdx = i;
		    flag = true;
		}
	    }
	}

	if(flag)
	{
	    chStack.push(selFrameCh);
	    idxStack.push(selFrameIdx);
	    visitMap.insert(getVisitMapKey(selFrameCh, selFrameIdx));
	}
	else
	    break;
    }

    // Apply assignment from lowest to highest
    while(chStack.size() != 0)
    {
	int ch = chStack.top();
	int idx = idxStack.top();
	
	int responseTime = getFrameResponseTime(model, ch, idx);
	if(model->channels[ch].frames[idx].fwtype==NOT_FORWARD)
	{
	    if(responseTime > model->channels[ch].frames[idx].curDeadline)
		return false;
	    
	    applyAssignment(model, makeNewAssignment(model->channels[ch].cs.clp, ch, idx));
	}
	else if(model->channels[ch].frames[idx].fwtype==FORWARD_SRC)
	{
	    for(int k = 0; k < model->channels[ch].frames[idx].crpdFrames.size(); k++)
	    {
		int crpdCh = model->channels[ch].frames[idx].crpdFrames[k]->channel;
		int crpdIdx = model->channels[ch].frames[idx].crpdFrames[k]->idx;
		int crpdResponseTime = getFrameResponseTime(model, crpdCh, crpdIdx);

		if(responseTime + crpdResponseTime > model->channels[ch].frames[idx].curDeadline)
		    return false;

	        applyAssignment(model, makeNewAssignment(model->channels[crpdCh].cs.clp, crpdCh, crpdIdx));
	    }

	    applyAssignment(model, makeNewAssignment(model->channels[ch].cs.clp, ch, idx));
	}
	else
	{
	    printf("Error: deadline monotonic assignment error\n");
	    fflush(stdout);
	}

	chStack.pop();
	idxStack.pop();
    }

    return true;
}
