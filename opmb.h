#ifndef __OPMB_H__
#define __OPMB_H__

#include "sys_model.h"

// Function for fixable assignment
vector<struct priority_assignment> getFixableAssignment(struct sys_model *model);
void applyFixableAssignment(struct sys_model *model, vector<struct priority_assignment> fpa);
void revokeFixableAssignment(struct sys_model *model, unsigned int nRevokedPA);

// Function for schedulable assignment
vector<struct priority_assignment> getSchedulableAssignment(struct sys_model *model);
vector<int> applySchedulableAssignment(struct sys_model *model, struct priority_assignment spa);
void revokeSchedulableAssignment(struct sys_model *model, vector<int> storedCrpdFrameDeadline);

// OPMB functions
void initOPMB(struct sys_model *model);
struct fail_state OPMB(struct sys_model *model);
#endif
