#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>
#include "SimpleDataDependenceGraph.h"


using namespace llvm;

namespace {
class MyPass : public ModulePass {
public:
    static char ID;
    MyPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M) {
        for (auto f = M.begin(); f != M.end(); f++) {
            miner::SDDG func(&(*f));
            func.buildSDDG();
            func.flattenSDDG();
            func.dotify();
        }
        return false;
    }
};
} // namespace

char MyPass::ID = 0;


static void registerMyPass(const PassManagerBuilder &PMB, legacy::PassManagerBase &PM) {
   PM.add(new MyPass());
}

// works with "-O0" or no optimization options
static RegisterStandardPasses RegisterMyPass_OPT0(PassManagerBuilder::EP_EnabledOnOptLevel0, registerMyPass);

// works with "-O1", "-O2", ...
static RegisterStandardPasses RegisterMyPass_OPT(PassManagerBuilder::EP_ModuleOptimizerEarly, registerMyPass);
