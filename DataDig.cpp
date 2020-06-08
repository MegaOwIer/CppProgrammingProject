#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/Casting.h>

#include "DataDig.h"

namespace datadig {

itemSets::itemSets() {}
itemSets::itemSets(itemSets *items) {
    for (auto fst : items->mSets)
        for (auto snd : items->mSets) {
            if (fst->getCommon(snd) == fst->getSize() - 1) {
                itemSet *nowSet;
                nowSet = merge_itemSet(fst, snd);
                if (!isIn(nowSet)) {
                    mSets.insert(nowSet);
                } else {
                    delete nowSet;
                }
            } else if (fst->getCommon(snd) == fst->getSize()) {
                for (auto pr : fst->getSet()) {
                    itemSet *nowSet;
                    nowSet = merge_itemSet(fst, snd); 
                    nowSet->addItem(pr.first);
                    if (!isIn(nowSet)) {
                        mSets.insert(nowSet);
                    } else {
                        delete nowSet;
                    }
                }
            }
        }
}

itemSets::~itemSets() {
    for (auto items : mSets) {
        delete items;
    }
}

void itemSets::merge_Set(itemSets *items) {
    for (auto Set : items->getSet()) {
        merge_Item(Set);
    }
}

bool itemSets::isIn(itemSet *item) {
    for (auto Set : mSets)
        if (Set->issame(item)) return true;
    return false;
}

void itemSets::merge_Item(itemSet *item) {
    if (!isIn(item)) {
        itemSet *newSet = new itemSet(item);
        mSets.insert(newSet);
    }
}

static itemSets FIS, IIS;

void itemSets::prune() {
    set<itemSet *> delt;
    for (auto iSet : mSets) {
        for (auto item : iSet->getSet()) {
            if (!--iSet->getSet()[item.first]) {
                iSet->getSet().erase(item.first);
            }
            bool inIIS = IIS.isIn(iSet);
            iSet->getSet()[item.first]++;
            if (inIIS) {
                delt.insert(iSet);
                break;
            }
        }
    }
    for (auto iSet : delt) {
        mSets.erase(iSet);
        delete iSet;
    }
}

bool itemSets::empty() { return mSets.begin() == mSets.end(); }
set<itemSet *> &itemSets::getSet() { return mSets; }

void itemSets::print(raw_ostream &os) {
    for (auto iSet : mSets) {
        iSet->print(os);
        os << "\n";
    }
}

#ifdef _LOCAL_DEBUG
void itemSets::printHash() {
    for (auto iSet : mSets)
        iSet->printHash();
}
#endif

int support(Module &M, itemSet *I) {
    int ans = 0;
    for (auto &F : M) {
        ans += SupportCount::CountSupport(F, I);
    }
    I->setSupportValue(ans);
    return ans;
}

itemSets *getFIS() { return &FIS; }
itemSets *getIIS() { return &IIS; }

void find_FIS_IIS(Module &M, int mfs, int mis) {
    itemSets *L, *N, *S;
    L = N = S = NULL;
    L = new itemSets;
    for (auto &F : M) {
        for (auto &BB : F) {
            for (auto &inst : BB) {
                if (Instruction::Call == inst.getOpcode()) {
                    Function *func = (dyn_cast<CallInst>(&inst))->getCalledFunction();
                    if (func->isIntrinsic()) {
                        if (func->getIntrinsicID() != Intrinsic::memcpy) continue;
                    }
                    itemSet *tmp = new itemSet(&inst);
                    L->merge_Item(tmp);
                    delete tmp;
                }
            }
        }
    }
    for (int k = 2; !L->empty(); k++) {
        delete N;
        delete S;
        S = new itemSets(L);

        S->prune();
        delete L;
        L = new itemSets;
        N = new itemSets;

        for (auto I : S->getSet()) {
            int sup_num = support(M, I);
            if (sup_num >= mfs) {
                L->merge_Item(I);
            } else if (sup_num <= mis && sup_num > 0)
                N->merge_Item(I);
        }

        FIS.merge_Set(L);
        IIS.merge_Set(N);
    }
    for(auto item:IIS.getSet())
        item->setFormal();
    for(auto item:FIS.getSet())
        item->setFormal();
}

}  // namespace datadig
