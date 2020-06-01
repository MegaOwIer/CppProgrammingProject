#ifndef COUNTSUPPORT_H_
#define COUNTSUPPORT_H_

#include <set>
#include <string>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "Hash.h"

namespace support{
using namespace llvm;
using std::pair;
using std::set;
using std::string;
using std::vector;
using std::map;
using namespace miner;
using hash_t = __uint128_t;

class itemSet{
private:
    map<hash_t, int> mItems;
public:
    itemSet();
    ~itemSet();
    void addItem(hash_t item);
    int getnumItem(hash_t item);
    bool islarger(itemSet* I);
};

class SCCNode{
private:
    vector<BasicBlock *> blocks;
    set<SCCNode *> mSuccessors;
    set<SCCNode *> mPredecessors;

public:
    SCCNode(scc_iterator<Function *> iter);
    ~SCCNode();
    void addBlock(BasicBlock* block);
    void addSuccessor(SCCNode* Node);
    void addPredecessor(SCCNode* Node);
    set<SCCNode *>& getSuccessors();
    set<SCCNode *>& getPredecessors();
    vector<BasicBlock *>& getBlocks();
    void buildRelation();
    bool dfsNode(SDDG *G, itemSet *I);
};

class SCCGraph{
private:
    vector<SCCNode*> SCCNodes;
    SCCNode* EntryNode;
public:
    SCCGraph(Function &F);
    ~SCCGraph();
    void buildGraph();
    bool dfsGraph(SDDG* G,itemSet* I);
    SCCNode* getEntry();
};

map<BasicBlock*,SCCNode*> BBSCC;
void addBlockSCC(BasicBlock* block, SCCNode* SCC);
SCCNode* getSCC(BasicBlock* block);

// class SupportNode{
// private:
//     string normalizedStr;
// 	hash_t HashValue;
// 	vector<SupportNode *> mSuccessors;
//     vector<SupportNode *> mPredecessors;
// public:
// 	SupportNode(Value *inst);
//     ~SupportNode();
//     void addSuccessor(SupportNode *dst);
//     void addPredecessor(SupportNode *dst);
//     inline Instruction *getInst();
//     vector<SupportNode *> &getSuccessors();
//     vector<SupportNode *> &getPredecessors();
// };

// class BlockNodeSet{
// private:
//     map<BasicBlock*,set<SDDGNode*>* > mTrans;
//     map<SDDGNode*, BasicBlock*> rTrans;
// public:
//     BlockNodeSet();
//     ~BlockNodeSet();
//     void addNode(BasicBlock* block, SDDGNode* inst);
//     set<SDDGNode*>* getSDDGNodes(BasicBlock* block);
//     BasicBlock* getBlock(SDDGNode* inst);
// }BNmap;

extern void transition(string &normalizedStr, Value *inst);
} // namespace support
#endif