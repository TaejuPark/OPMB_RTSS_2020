#ifndef __SYSTEM_GENERATOR_H__
#define __SYSTEM_GENERATOR_H__

#include "sys_model.h"

vector<struct channel> generate_channels(int nCh);
vector<struct ecu> generate_ecus(vector<struct channel> channels);
struct routing_table generate_initial_routing_table(struct sys_model *model);
void generate_frames_on_src_bus(struct sys_model *model);
void generate_frames_on_dest_buses(struct sys_model *model);
void compute_chrs_of_generated_frames(struct sys_model *model);
void discover_frame_signal_mapping(struct sys_model *model);
void discover_crpd_frames(struct sys_model *model);
void discover_chained_crpd_frames(struct sys_model *model);
struct sys_model generateInitialSystemModel(int nCh, int sig_per_ch, int intra_sig_prob);
void mapping_signal_frame_crpdframe(struct sys_model *model);
int applyGatewayProcessingDelay(struct sys_model *model);
void modelUpdateForMerge(struct sys_model *model);
void reconfiguring_deadline(struct sys_model *model);
#endif
