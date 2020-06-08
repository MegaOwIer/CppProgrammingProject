#ifndef COUNTSUPPORT_H_
#define COUNTSUPPORT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

#include "Hash.h"
#include "SimpleDataDependenceGraph.h"

namespace SupportCount {

using namespace llvm;
using namespace miner;
using std::map;
using std::set;
using std::string;
using std::vector;

class itemSet {
private:
    map<hash_t, int> mItems;
    int SupportValue;
public:
    itemSet();
    itemSet(const itemSet *src);
    itemSet(Instruction *inst);
    ~itemSet();
    map<hash_t, int> &getSet();
    void addItem(hash_t item);
    int getnumItem(hash_t item);
    bool islarger(itemSet *I);
    bool issame(itemSet *I);
    bool isempty();
    int getCommon(itemSet *I);
    int getSize();
    void print(raw_ostream &os = errs());
    int getSupportValue();
    void setSupportValue(int x);
    void setFormal();
#ifdef _LOCAL_DEBUG
    void printHash();
#endif
};

class SCCNode {
private:
    vector<BasicBlock *> blocks;
    set<SCCNode *> mSuccessors;
    set<SCCNode *> mPredecessors;

public:
    SCCNode(scc_iterator<Function *> iter);
    ~SCCNode();
    void addBlock(BasicBlock *block);
    void addSuccessor(SCCNode *Node);
    void addPredecessor(SCCNode *Node);
    set<SCCNode *> &getSuccessors();
    set<SCCNode *> &getPredecessors();
    vector<BasicBlock *> &getBlocks();
    void buildRelation();
    bool dfsNode(SDDG *G, itemSet *I);
};

class SCCGraph {
private:
    vector<SCCNode *> SCCNodes;
    SCCNode *EntryNode;

public:
    SCCGraph(Function &F);
    ~SCCGraph();
    void buildGraph();
    bool dfsGraph(SDDG *G, itemSet *I);
    SCCNode *getEntry();
};

void addBlockSCC(BasicBlock *block, SCCNode *SCC);
SCCNode *getSCC(BasicBlock *block);

void transition(string &normalizedStr, Value *inst);
int CountSupport(Function &F, itemSet *I);
itemSet *merge_itemSet(itemSet *fst, itemSet *snd);
void rbclear();

}  // namespace SupportCount

#endif