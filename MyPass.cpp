#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "BugDetector.h"
#include "DataDig.h"
#include "RuleGenerator.h"
#include "SimpleDataDependenceGraph.h"

using namespace llvm;

namespace {

using namespace datadig;
using namespace ruleGen;

class MyPass : public ModulePass {
    int mfs, mis, mcf;

public:
    static char ID;
    MyPass(int MFS, int MIS, int MCF) : ModulePass(ID), mfs(MFS), mis(MIS), mcf(MCF) {}

    virtual bool runOnModule(Module &M) {
        find_FIS_IIS(M, mfs, mis);
        itemSets *FIS = getFIS(), *IIS = getIIS();
        ruleSet *PARs = new ruleSet, *NARs = new ruleSet;
        rule_generator(M, FIS, IIS, mcf / 100.0, PARs, NARs);

        outs()<<"++IIS\n";
        IIS->print(outs());
        outs()<<"--IIS\n\n";
        outs()<<"++FIS\n";
        FIS->print(outs());
        outs()<<"--FIS\n\n";
        outs()<<"++PAR\n";
        PARs->display(outs());
        outs()<<"--PAR\n\n";
        outs()<<"++NAR\n";
        NARs->display(outs());
        outs()<<"--NAR\n\n";
        outs()<<"++BUGs\n";
        for(auto qwq : bugfinder::check_positive(M, PARs)) {
            outs() << qwq.what() << "\n";
        }
        for(auto qwq : bugfinder::check_negative(M, NARs)) {
            outs() << qwq.what() << "\n";
        }
        outs()<<"--BUGs\n";
        return false;
    }
};

}  // namespace

char MyPass::ID = 0;
static cl::opt<int> MFS("mfs", cl::init(10), cl::desc("Minimum Frequent-itemset Support"));
static cl::opt<int> MIS("mis", cl::init(5), cl::desc("Maximum Infrequent-itemset Support"));
static cl::opt<int> MCF("min_conf", cl::init(85), cl::desc("Maximum Infrequent-itemset Support"));

static void registerMyPass(const PassManagerBuilder &PMB, legacy::PassManagerBase &PM) {
    PM.add(new MyPass(MFS, MIS, MCF));
}

// works with "-O0" or no optimization options
static RegisterStandardPasses RegisterMyPass_OPT0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                                                  registerMyPass);

// works with "-O1", "-O2", ...
static RegisterStandardPasses RegisterMyPass_OPT(PassManagerBuilder::EP_ModuleOptimizerEarly,
                                                 registerMyPass);
