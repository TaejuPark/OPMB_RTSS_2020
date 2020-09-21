#include "sys_model.h"
#include "common.h"
#include <cstdio>

bool MAA(struct sys_model *model)
{
    while(true)
    {
        if(model->nUM==0)
            return true;

        bool schedulable_find = false;
        for(int ch = 1; ch <= model->nCh; ch++)
        {
            for(int idx = 0; idx < model->channels[ch].frames.size(); idx++)
            {
		if(model->channels[ch].frames[idx].priorityAssigned)
		    continue;

                int responseTime = getFrameResponseTime(model, ch, idx);
                if(model->channels[ch].frames[idx].fwtype==NOT_FORWARD)
                {
                    if(responseTime <= model->channels[ch].frames[idx].curDeadline)
                    {
			applyAssignment(model, makeNewAssignment(model->channels[ch].cs.clp, ch, idx));
                        schedulable_find = true;
                        break;
                    }
                }

                if(model->channels[ch].frames[idx].fwtype==FORWARD_SRC)
                {
                    if(responseTime > model->channels[ch].frames[idx].curDeadline)
                        continue;

                    bool crpd_ok = true;
                    for(int crpd = 0; crpd < model->channels[ch].frames[idx].crpdFrames.size(); crpd++)
                    {
                        int crpdCh = model->channels[ch].frames[idx].crpdFrames[crpd]->channel;
                        int crpdfIdx = model->channels[ch].frames[idx].crpdFrames[crpd]->idx;
                        int crpdResponseTime = getFrameResponseTime(model, crpdCh, crpdfIdx);
                        if(responseTime + crpdResponseTime > model->channels[ch].frames[idx].curDeadline)
                        {
                            crpd_ok = false;
                        }
                    }
                    
                    if(!crpd_ok)
                        continue;

		    applyAssignment(model, makeNewAssignment(model->channels[ch].cs.clp, ch, idx));
                    for(int crpd = 0; crpd < model->channels[ch].frames[idx].crpdFrames.size(); crpd++)
                    {
                        int crpdCh = model->channels[ch].frames[idx].crpdFrames[crpd]->channel;
                        int crpdfIdx = model->channels[ch].frames[idx].crpdFrames[crpd]->idx;
		        applyAssignment(model, makeNewAssignment(model->channels[crpdCh].cs.clp, crpdCh, crpdfIdx));
                    }
                    schedulable_find = true;
                    break;
                }
            }
            if(schedulable_find)
                break;
        }

        if(!schedulable_find)
            return false;
    }
}
