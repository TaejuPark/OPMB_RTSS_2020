#ifndef __SYS_MODEL_H__
#define __SYS_MODEL_H__

#include <vector>
#include <unordered_set>
#include "routing_table.h"

using namespace std;

enum protocol_type
{
    CAN,
    CANFD,
};

struct merge
{
    int ch;
    int idx1;
    int idx2;
    vector<struct signal> signals;
    double utilReduction;
};

struct sys_model
{
    bool isSolutionFind;
    bool isTimeout;
    bool logForRe;

    int nECU;
    int nCh;
    int nUM; // total number of priority unassigned frames;
    int maxFrameID;

    vector<struct ecu> ecus;
    vector<struct channel> channels;
    vector<struct signal> signals;
    vector<struct frame> frames;
    vector<struct priority_assignment> pa;
    struct routing_table rt;
};

struct channel_state
{
    bool AM[1000]; // priority assigned frames
    bool UM[1000]; // priority unassigned frames
    int clp; // current lowest priority;
};

struct channel
{
    enum protocol_type ptype;
    int linkspeed;
    double util_limit;
    double utilization;
    vector<struct frame> frames;
    struct channel_state cs;
};

struct ecu
{
    int id;
    int channel;
    vector<struct signal> signals;
    vector<struct frame> frames;
};

struct signal
{
    int sid; // signal ID; same as index of sys_model.signals vector
    int size;
    int period;
    int source;
    int deadline;

    vector<int> destinations;
    bool destChannels[10];
    struct frame* srcFrame;
    vector<struct frame*> destFrames;
};

enum forward_type
{
    NOT_FORWARD,
    FORWARD_SRC,
    FORWARD_DEST,
};

struct frame
{
    int id; // frame id;
    int idx; // index of the frame in the frame list of a channel
    enum forward_type fwtype;
    int size;
    int period;
    int curDeadline;
    int iniDeadline;
    int transmission_time; // in nano seconds
    int responseTime;
    double utilization;
    int channel;
    bool priorityAssigned;
    vector<struct signal> signals;
    vector<struct frame*> crpdFrames;
    vector<struct frame*> chainCrpdFrames;
};

struct priority_assignment
{
    int priority;
    int ch;
    int fIdx;
};

struct fail_state
{
    int ch;
    struct channel_state cs;
};


int get_frame_period(struct frame *fr);
int get_frame_deadline(struct frame *fr);
int get_frame_transmission_time(enum protocol_type ptype, int framesize, int linkspeed);
double get_frame_utilization(struct frame *fr);
struct signal generate_signal(struct sys_model *model, int intra_sig_prob);
void greedy_bw_best_fit(struct sys_model *model);
bool DM(struct sys_model *model);
bool MAA(struct sys_model *model);
bool ZSPA(struct sys_model *model);
int OPMB_solution_verification(struct sys_model *model, bool brute_result, int nSignal);
bool ZSPA_solution_verification(struct sys_model *model);
vector<int> ch_response_time_lower_bound_discovery(struct sys_model *model, int ch);
int get_frame_response_time(struct sys_model *model, int ch, vector<struct frame> frames, int candidate_idx, vector<bool> assigned);

#endif
