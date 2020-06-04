#ifndef DataDig_H_
#define DataDig_H_

#include <set>

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include "CountSupport.h"

namespace datadig {

using namespace llvm;
using namespace SupportCount;
using std::set;

class itemSets {
private:
    set<itemSet *> mSets;

public:
    itemSets();
    itemSets(itemSets *items);
    itemSets(Module &M);
    ~itemSets();
    void merge_Set(itemSets *items);
    void merge_Item(itemSet *item);
    bool isIn(itemSet *item);
    bool empty();
    void prune();
    set<itemSet *> &getSet();
    void print(raw_ostream &os);
#ifdef _LOCAL_DEBUG
    void printHash();
#endif
};

void find_FIS_IIS(Module &M, int mfs, int mis);

int support(Module &M, itemSet *I);

itemSets *getFIS();
itemSets *getIIS();

}  // namespace datadig

#endif