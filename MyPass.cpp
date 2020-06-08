#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SCCIterator.h>
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
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "DataDig.h"
#include "RuleGenerator.h"
#include "SimpleDataDependenceGraph.h"

using namespace llvm;

namespace {

using namespace datadig;
using namespace ruleGen;

class MyPass : public ModulePass {
public:
    static char ID;
    static const int mfs = 10;
    static const int mis = 5;
    MyPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M) {
        for (Function &f : M) {
            if (f.empty()) continue;
            miner::SDDG func(&f);
            func.buildSDDG();
            func.flattenSDDG();
        }
        find_FIS_IIS(M, mfs, mis);
        itemSets *FIS = getFIS(), *IIS = getIIS();
        ruleSet *PARs = new ruleSet, *NARs = new ruleSet;
        rule_generator(M, FIS, IIS, 0.85, PARs, NARs);
        errs() << "PARs" << "\n";
        PARs->display();
        errs() << "\n";
        errs() << "NARs" << "\n";
        NARs->display();
        errs() << "\n";
        return false;
    }
};

}  // namespace

char MyPass::ID = 0;

static void registerMyPass(const PassManagerBuilder &PMB, legacy::PassManagerBase &PM) {
    PM.add(new MyPass());
}

// works with "-O0" or no optimization options
static RegisterStandardPasses RegisterMyPass_OPT0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                                                  registerMyPass);

// works with "-O1", "-O2", ...
static RegisterStandardPasses RegisterMyPass_OPT(PassManagerBuilder::EP_ModuleOptimizerEarly,
                                                 registerMyPass);
