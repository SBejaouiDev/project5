#ifndef __CPU_PRED_GSELECT_BP_HH__
#define __CPU_PRED_GSELECT_BP_HH__

#include <vector>

#include "base/sat_counter.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/GSelectBP.hh"
#include "debug/GSDebug.hh"

/**
 * Implements a Gselect branch predictor. BPredUnit is an abstract class for branch predictors in gem5. 
 * It defines common functions that all branch predictors must implement (lookup, update, squash, etc).
 */
class GSelectBP : public BPredUnit 
{ 
  public: 
    /**
     * Default branch predictor constructor.
     * Explanation: const GSelectBPParams &params pulls from the BranchPredictor.py config file setting all the fields.
     * PredictorSize       = Param.Unsigned(1024, "Size of predictor (entries).")
     * PHTCtrBits          = Param.Unsigned(2, "Bits per counter.")
     * globalHistoryBits   = Param.Unsigned(8, "Bits of the global history.") 
     * 
     * Question? Why is Python used to store the parameters? Why not have it as a C++ file??
     */
    GSelectBP(const GSelectBPParams &params);

    /**
     * Define the n bits from global history and m bits of branch address.
     * Calculate the Pattern History Table Index (PHTIdx)
     *  - This is done by masking the lower bits of GH and BA and concatenating them.
     * Modify the history using the thread to determine a prediction.
     * 
     * @param branch_addr The address of the branch to look up
     * @param bp_history  Pointer that will be set to the BPHistory object
     * @return Whether or not the branch prediction is taken
     */
    bool lookup(ThreadID tid, Addr branch_addr, void *&bp_history);

    /**
     * When an instruction is an unconditional branch, it will update the global history to taken.
     *  - Modify the global history using the thread
     *  - Modify the prediction attribute and the global history attribute of the history
     * 
     * @param bp_history Pointer that will be set to the BPHistory object
     */
    void uncondBranch(ThreadID tid, Addr pc, void *&bp_history);

    /**
     * When there is a branch miss in the branch target buffer (BTB), the global history is updated to ensure prediction is not taken.
     */
    void btbUpdate(ThreadID tid, Addr branch_addr, void *&bp_history);

    /**
     * Trains the predictor using the actual result of the branch.
     */
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr &inst, Addr corrTarget);

    /**
     * Restores the global history if the prediction was incorrect.
     */
    void squash(ThreadID tid, void *bp_history);

  private:
    // Store the information regarding whether or not the prediction was taken,
    // the PHT index, and what the global history register was.
    struct BPHistory {

        unsigned ghr; //old global history 
        unsigned index;
        bool prediction;
        unsigned branch_addr;
    };

    unsigned branchAddressBits;
    unsigned branchMask;
    unsigned globalHistoryMask;
    unsigned PHTThreshold;

    unsigned ShiftAmount;
    unsigned PredictorSize;
    unsigned PHTCtrBits;
    unsigned globalHistoryBits;
    
    

    std::vector<SatCounter8> pht;
    std::vector<unsigned> globalHistory;
};

#endif // __CPU_PRED_GSELECT_BP_HH__
