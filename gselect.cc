#include "cpu/pred/gselect.hh"
#include "base/intmath.hh"
#include "debug/Mispredict.hh"
#include "base/trace.hh"

GSelectBP::GSelectBP(const GSelectBPParams &params)
      : BPredUnit(params),
      // Size of the predictor
      PredictorSize(1 << ceilLog2(params.PredictorSize)),
      // Bits per counter
      PHTCtrBits(params.PHTCtrBits),
      // Shift amount for branch address
      ShiftAmount(2),
      // Global history bits
      globalHistoryBits(params.globalHistoryBits),
      // Calculate branch address bits
      branchAddressBits(ceilLog2(params.PredictorSize) - params.globalHistoryBits),
      // Create masks
      branchMask((1 << branchAddressBits) - 1),
      globalHistoryMask((1 << globalHistoryBits) - 1),
      // PHT setup
      pht(PredictorSize, SatCounter8(PHTCtrBits)),
      globalHistory(params.numThreads, 0)
{
    // For debugging
    DPRINTF(GSDebug, "Constructor: PHTIndexBits=%u, globalHistoryBits=%u, branchAddressBits=%u\n", 
            ceilLog2(PredictorSize), globalHistoryBits, branchAddressBits);
    DPRINTF(GSDebug, "Constructor: branchMask=%u, globalHistoryMask=%u\n", 
            branchMask, globalHistoryMask);
    
    // PHT threshold calculation
    PHTThreshold = (1 << (PHTCtrBits - 1));
}



bool GSelectBP::lookup(ThreadID tid, Addr branch_addr, void *&bp_history)
{
    
    unsigned oldGlobalHistory = globalHistory[tid];
   
    unsigned shiftVal = branch_addr >> ShiftAmount; //PASS
    
    // Define n bits of global history and m bits of branch address
    unsigned pcBits = shiftVal & branchMask;		   //FAIL branch mask could be calcualted in correctly
    unsigned ght = oldGlobalHistory & globalHistoryMask; //PASS
    
    //Wrong index is being printed
    unsigned index = (ght << branchAddressBits) | pcBits; //Branch address bits could be wrong
    

    DPRINTF(GSDebug, "In lookup. globalHistoryReg: %u, branchAddr: %u, nn: %u, mm: %u\n", oldGlobalHistory, (unsigned)branch_addr, ght, pcBits);
    DPRINTF(GSDebug, "In lookup. instShiftAmt: %u, shifted val: %u, mask: %u\n",ShiftAmount, shiftVal, branchMask);
    DPRINTF(GSDebug, "In lookup. lookup PHTredictor size: %u\n", PredictorSize);
    DPRINTF(GSDebug, "In lookup. lookup PHTIdx: %u\n", index);
		
    assert(index < PredictorSize && "PHT index out of bounds in lookup!");
    bool prediction = pht[index] >= PHTThreshold;
    
    //testing
    DPRINTF(GSDebug, "In lookup. Counter value: %u, PHTThreshold: %u, Prediction: %d\n", index, PHTThreshold, prediction);	

    globalHistory[tid] = ((oldGlobalHistory << 1) | (prediction ? 1 : 0)) & globalHistoryMask;

    //testing
    DPRINTF(GSDebug, "In lookup. Global history register was: %u. Branch address: %u\n", oldGlobalHistory, (unsigned)branch_addr);
    DPRINTF(GSDebug, "In lookup. Global history register is: %u. Branch address: %u\n", globalHistory[tid], (unsigned)branch_addr);


    // Allocate BPHistory
    BPHistory *history = new BPHistory;
    history->ghr = oldGlobalHistory;
    history->index = index;
    history->prediction = prediction;
    history->branch_addr = branch_addr;
    
    // Set the bp_history pointer
    bp_history = static_cast<void *>(history);

    return prediction;
}




void
GSelectBP::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                  bool squashed, const StaticInstPtr &inst, Addr corrTarget)
{
   
    BPHistory *history = static_cast<BPHistory *>(bp_history);
      if (!history) return;
    unsigned index = history->index;

    // Update the PHT counter based on actual outcome
    if(squashed) { 
    	
    	DPRINTF(Mispredict, "In update. Squashed. Global history register was: %u. Branch address: %u\n", history->ghr, (unsigned)branch_addr);
    	
    	globalHistory[tid] = history->ghr;
    
    } else { 
    	 DPRINTF(Mispredict, "In update. Global history register was: %u. Branch address: %u\n",
                history->ghr, (unsigned)branch_addr);
    		
   	 if (taken) {
        	pht[index]++;
        	 DPRINTF(Mispredict, "In update. update PHTIdx: %u.\n", index);
    	} else {
        	pht[index]++;
        	 DPRINTF(Mispredict, "In update. update PHTIdx: %u.\n", index);	
   	}
    }

    delete history;
}

void
GSelectBP::squash(ThreadID tid, void *bp_history)
{
 
     if (!bp_history) return; //solved seg fault issue 162500
    
     BPHistory *history = static_cast<BPHistory *>(bp_history);
    DPRINTF(GSDebug, "In squash. Global history register is: %u\n",history->ghr);
    globalHistory[tid] = history->ghr;
    delete history;
}


void GSelectBP::uncondBranch(ThreadID tid, Addr pc, void *&bp_history)
{
   globalHistory[tid] = ((globalHistory[tid] << 1) | 1) & globalHistoryMask;
   
    DPRINTF(GSDebug, "In uncondBranch. Global history register is: %u. Branch address: %u\n", globalHistory[tid], (unsigned)pc);
    bp_history = NULL;
    
  
   
}

// btbUpdate: Updates global history when there's a branch miss in the branch target buffer
void GSelectBP::btbUpdate(ThreadID tid, Addr branch_addr, void *&bp_history)
{	
     //seg fault occurs here... why????
    DPRINTF(GSDebug, "In btbUpdate. Global history register was: %u. Branch address: %u\n",globalHistory[tid], (unsigned)branch_addr);
    globalHistory[tid] = (globalHistory[tid] << 1) & globalHistoryMask; 
    bp_history = NULL;
}

