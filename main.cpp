#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

using namespace std;

#include "sys_model.h"
#include "opmb.h"
#include "system_generator.h"
#include "common.h"

int main(int argc, char *argv[])
{
    int opt;
    srand(time(NULL));

    int nCh = 4;
    int sig_per_ch = 100;
    int intra_sig_prob = 50;
    int nSignal;
    char *filename;
    FILE *fout;
    int case_cnt = 0;
    clock_t start;
    clock_t end;

    while((opt = getopt(argc, argv, "c:s:p:")) != -1)
    {
        switch(opt)
        {
            case 'c':
                nCh = atoi(optarg);
                break;
            case 's':
                sig_per_ch = atoi(optarg);
                break;
            case 'p':
                intra_sig_prob = atoi(optarg);
                break;
            default:
                break;
        }
    }
    
    nSignal = nCh * sig_per_ch;
    filename = strcat(strcat(strcat(strcat(strcat(strcat(argv[0], argv[1]), argv[2]), argv[3]), argv[4]), argv[5]), argv[6]);
    fout = fopen(filename, "w");
    case_cnt = 0;
    
    fprintf(fout, "Case\tDM\tDMV\tZSPA\tZSPAV\tMAA\tMAAV\tOPMB\tOPMBV\tOPMBT\n");
    fflush(fout);
    
    while(case_cnt<30)
    {
	case_cnt++;
        fprintf(fout, "[#%d]\t", case_cnt);

	struct sys_model omodel = generateInitialSystemModel(nCh, sig_per_ch, intra_sig_prob);
	struct sys_model modelForDm = omodel;
	struct sys_model modelForOpmb = omodel;
	struct sys_model modelForMaa = omodel;
        struct sys_model modelForZspa = omodel;
	struct sys_model modelForOpmbRe = omodel;
	struct sys_model modelForZspaRe = omodel;
	
	mapping_signal_frame_crpdframe(&modelForDm);
	int gpw = applyGatewayProcessingDelay(&modelForDm);
	mapping_signal_frame_crpdframe(&modelForZspa);
	applyGatewayProcessingDelay(&modelForZspa);
	mapping_signal_frame_crpdframe(&modelForMaa);
	applyGatewayProcessingDelay(&modelForMaa);
	mapping_signal_frame_crpdframe(&modelForOpmb);
	applyGatewayProcessingDelay(&modelForOpmb);
	mapping_signal_frame_crpdframe(&modelForOpmbRe);
	applyGatewayProcessingDelay(&modelForOpmbRe);
	mapping_signal_frame_crpdframe(&modelForZspaRe);
	applyGatewayProcessingDelay(&modelForZspaRe);

        initSystemState(&modelForDm);
        initSystemState(&modelForZspa);
        initSystemState(&modelForMaa);
        initSystemState(&modelForOpmb);
        initSystemState(&modelForOpmbRe);
        initSystemState(&modelForZspaRe);
	
	if(getMaximumChannelUtilization(&modelForOpmb) > 1.0)
	{
	    fprintf(fout, "\n");
	    continue;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////// [Start] Schedulability Test ///////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Deadline Monotonic(DM) Schedulability Test
	start = clock();
	if(DM(&modelForDm))
	    fprintf(fout, "Schedulable\t");
	else
	    fprintf(fout, "Unschedulable\t");
	end = clock();

	//Verify DM solution
	bool dmVerificationResult = solutionVerification(&modelForDm, gpw);
	if(dmVerificationResult)
	    fprintf(fout, "Schedulable\t");
	else
	    fprintf(fout, "Unschedulable\t");
        //fprintf(fout, "%.4lf\t", (double)((end-start)/CLOCKS_PER_SEC));
        fflush(fout);


        // ZSPA Schedulability Test
	// H.Yoon et al., 'Guaranteeing end-to-end deadlines for AUTOSAR-based automotive software',
	// International Journal of Automotive Technology, Aug. 2015
	start = clock();
        if(ZSPA(&modelForZspa))
            fprintf(fout, "Schedulable\t");
        else
            fprintf(fout, "Unschedulable\t");
	end = clock();

	// Verify ZSPA solution
	bool zspaVerificationResult = solutionVerification(&modelForZspa, gpw);
        if(zspaVerificationResult)
            fprintf(fout,"Schedulable\t");
        else
            fprintf(fout,"Unschedulable\t");
        //fprintf(fout, "%.4lf\t", (double)((end-start)/CLOCKS_PER_SEC));
        fflush(fout);
	
	
	// MAA Schedulability Test 
	// (Modified Audsley's Algorithm), Algorithm 1 in 
	// Prachi Joshi., 'The Multi-Domain Frame Packing Problem for CAN-FD', ECRTS 2017
	start = clock();
        if(MAA(&modelForMaa))
            fprintf(fout, "Schedulable\t");
        else
            fprintf(fout, "Unschedulable\t");
	end = clock();
	
	//Verify MAA solution
	bool maaVerificationResult = solutionVerification(&modelForMaa, gpw);
	if(maaVerificationResult)
	    fprintf(fout, "Schedulable\t");
	else
	    fprintf(fout, "Unschedulable\t");
        //fprintf(fout, "%.4lf\t", (double)((end-start)/CLOCKS_PER_SEC));
	fflush(fout);

	// OPMB Schedulability Test
	start = clock();
        initOPMB(&modelForOpmb);
	OPMB(&modelForOpmb);
        if(modelForOpmb.isSolutionFind)
            fprintf(fout,"Schedulable\t");
        else
            fprintf(fout,"Unschedulable\t");
        end = clock();
	
	//Verify OPMB solution
	bool opmbVerificationResult = solutionVerification(&modelForOpmb, gpw);
        if(opmbVerificationResult)
            fprintf(fout,"Schedulable\t");
        else
            fprintf(fout,"Unschedulable\t");
	
	double OPMBT = (double)((double)(end-start)/(double)CLOCKS_PER_SEC);
	fprintf(fout, "%.4lf\t", OPMBT);
	
/*	if(zspaVerificationResult && !opmbVerificationResult && OPMBT < 10)
	{
	    modelForOpmbRe.logForRe = true;
	    initOPMB(&modelForOpmbRe);
	    OPMB(&modelForOpmbRe);
	    //ZSPA(&modelForZspaRe);
	}*/
	
	fprintf(fout, "\n");
        fflush(fout);
	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////// [End] Schedulability Test ////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////
    }

    fclose(fout);
    return 0;
}
