#ifndef __COMMON_H__
#define __COMMON_H__

#include "sys_model.h"

// utility functions for priority assignment algorithms
int getMin(int a, int b);
int getMax(int a, int b);
string getVisitMapKey(int ch, int idx);
double getMaximumChannelUtilization(struct sys_model *model);
double getAverageChannelUtilization(struct sys_model *model);
double getLBChannelUtilization(struct sys_model *model, int ch);
double getMaximumLBChannelUtilization(struct sys_model *model);
int getMergedFrameIndex(struct sys_model *model, int ch);
void initSystemState(struct sys_model *model);

double getAverageChannelUtilization(struct sys_model *model);
struct priority_assignment makeNewAssignment(int priority, int ch, int fIdx);
void applyAssignment(struct sys_model *model, struct priority_assignment pa);
void revokeAssignment(struct sys_model *model);
int getFrameResponseTimeAtHighestPriority(struct sys_model *model, int ch, int fIdx);
int getMaxQ(struct sys_model *model, int ch, int fIdx);
long long int getFrameResponseTime(struct sys_model *model, int ch, int fIdx);
long long int getFrameResponseTimeForQthInstance(struct sys_model *model, int ch, int fIdx, int q);
bool solutionVerification(struct sys_model *model, int gpw);

#endif
