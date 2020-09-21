#include "sys_model.h"
#include "common.h"
#include <cmath>
#include <cstdio>

void EQF_deadline_assignment(struct sys_model *model)
{
    for(int i = 1; i <= model->nCh; i++)
    {
        for(int j = 0; j < model->channels[i].frames.size(); j++)
        {
            model->channels[i].frames[j].curDeadline = model->channels[i].frames[j].curDeadline/2;
        }
    }
}

bool ZSPA(struct sys_model *model)
{
    EQF_deadline_assignment(model);

    while(true)
    {
        int sCh = 0;
        int sfIdx = 0;
        int src_delta[1000] = {0,};
        int src_ch[1000];
        int src_idx[1000];
        int dest_delta[1000] = {0, };
        int dest_ch[1000];
        int dest_idx[1000];

        for(int i = 0; i < 1000; i++)
        {
            src_delta[i] = -1;
            src_ch[i] = -1;
            src_idx[i] = -1;
            dest_delta[i] = 2000000001;
            dest_ch[i] = -1;
            dest_idx[i] = -1;
        }

        // [Start] Find a task with the largest and positive slack
        int max_delta = -1;
        int cases = -1;
        for(int i = 1; i <= model->nCh; i++)
        {
            for(int j = 0; j < model->channels[i].frames.size(); j++)
            {
		int frameID = model->channels[i].frames[j].id;

                if(model->channels[i].frames[j].priorityAssigned == false)
                {
                    int responseTime = getFrameResponseTime(model, i, j);
                    int delta = model->channels[i].frames[j].curDeadline - responseTime;
                    
                    if(model->channels[i].frames[j].fwtype != FORWARD_DEST && delta >= 0)
                    {
                        src_delta[frameID] = delta;
                        src_ch[frameID] = i;
                        src_idx[frameID] = j;
                    }
                    else if(model->channels[i].frames[j].fwtype == FORWARD_DEST)
                    {
			struct frame *crpdFrame = model->channels[i].frames[j].crpdFrames[0];

                        if(crpdFrame->priorityAssigned == false  && delta >= 0 && delta < dest_delta[frameID])
                        {
                            dest_delta[frameID] = delta;
                            dest_ch[frameID] = i;
                            dest_idx[frameID] = j;
                        }

                        if(crpdFrame->priorityAssigned == true && delta >= 0 && delta > max_delta)
                        {
                            max_delta = delta;
                            sCh = i;
                            sfIdx = j;
                            cases = 2;
                        }
                    }
                }
            }
        }

        int case3_id = -1;
        for(int i = 0; i < 1000; i++)
        {
            if(src_delta[i] > max_delta && src_delta[i] >= 0)
            {
                max_delta = src_delta[i];
                sCh = src_ch[i];
                sfIdx = src_idx[i];
                cases = 1;
            }
            if(dest_delta[i]!=2000000001 && dest_delta[i] > max_delta && dest_delta[i] >= 0)
            {
                max_delta = dest_delta[i];
                sCh = dest_ch[i];
                sfIdx = dest_idx[i];
                case3_id = i;
                cases = 3;
            }
        }
        // [End] Find a task with the largest and positive slack
        
        if(sCh==0)
        {
            return false;
        }

        int responseTime = getFrameResponseTime(model, sCh, sfIdx);
	struct priority_assignment pa = makeNewAssignment(model->channels[sCh].cs.clp, sCh, sfIdx);
        if(cases==1)
        {
            model->channels[sCh].frames[sfIdx].curDeadline -= max_delta;
            for(int i = 0; i < model->channels[sCh].frames[sfIdx].crpdFrames.size(); i++)
            {
                model->channels[sCh].frames[sfIdx].crpdFrames[i]->curDeadline += max_delta;
            }
	    applyAssignment(model, pa);
        }
        else if(cases==2)
        {
	    applyAssignment(model, pa);
        }
        else if(cases==3 && case3_id!=-1)
        {
            bool flag = false;
            for(int i = 1; i <= model->nCh; i++)
            {
                for(int j = 0; j < model->channels[i].frames.size(); j++)
                {
                    if(model->channels[i].frames[j].id == case3_id && model->channels[i].frames[j].priorityAssigned == true)
                    {
                        flag = true;
                    }
                }
            }

            if(!flag)
            {
                for(int i = 1; i <= model->nCh; i++)
                {
                    for(int j = 0; j < model->channels[i].frames.size(); j++)
                    {
                        if(model->channels[i].frames[j].id == case3_id)
                        {
                            if(model->channels[i].frames[j].fwtype != FORWARD_DEST)
                                model->channels[i].frames[j].curDeadline += max_delta;
                            else
                                model->channels[i].frames[j].curDeadline -= max_delta;
                        }
                    }
                }
            }
	    applyAssignment(model, pa);
        }
        else
        {
            printf("Something wrong in implementation\n");
            return false;
        }

        if(model->nUM == 0)
            return true;
    }

    return false;
}
