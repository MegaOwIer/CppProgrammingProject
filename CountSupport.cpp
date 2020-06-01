#include <fstream>
#include <queue>
#include <utility>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include "CountSupport.h"
#include "SimpleDataDependenceGraph.h"

namespace support {

using std::queue;

// void SupportNode::addSuccessor(SupportNode *dst) { mSuccessors.push_back(dst); }

// void SupportNode::addPredecessor(SupportNode *dst) { mPredecessors.push_back(dst); }

// vector<SupportNode *> &SupportNode::getSuccessors() { return mSuccessors; }

// vector<SupportNode *> &SupportNode::getPredecessors() { return mPredecessors; }

// inline Instruction *SupportNode::getInst() { return mInst; }

// SupportNode::SupportNode(Value *inst) {
// 	transition(normalizedStr, inst);
// 	HashValue = MD5encoding(normalizedStr.c_str());
// }

// SupportNode::~SupportNode() {
// 	mSuccessors.clear();
// 	mPredecessors.clear();
// 	delete &normalizedStr;
// }

// BlockNodeSet::BlockNodeSet() {}

// BlockNodeSet::~BlockNodeSet() {
// 	for(auto iter : mTrans)
// 		iter->second->clear();
// 	mTrans->clear();
// }

// BlockNodeSet::map<BasicBlock*,set<SDDGNode*>* > &getTrans(){
// 	return mTrans;
// }

// BlockNodeSet::void addNode(BasicBlock* block, SDDGNode* inst){
// 	if(mTrans.find(block)==mTrans.end())
// 		mTrans[block] = new set<*SDDGNode>;
// 	mTrans[block]->insert(inst);
// 	rTrans[inst] = block;
// }

// BlockNodeSet::set<SDDGNode*>* getSDDGNodes(BasicBlock* block){
// 	return mTrans[block];
// }

// BlockNodeSet::BasicBlock* getBlock(SDDGNode* inst){
// 	return rTrans[inst];
// }

set<BasicBlock *> UsefulBlocks;

bool NodeUseful(SDDGNode *Node, itemSet *I) {
    BasicBlock *BB = Node->getInst()->getParent();
    if (UsefulBlocks.find(BB) == UsefulBlocks.end()) return false;
    string label;
    transition(label, Node->getInst());
    hash_t hashValue = MD5encoding(label.c_str());
    return I->getnumItem(hashValue);
}

void dfsSDDG(SDDGNode *Node, itemSet *I, itemSet *nowSet, set<SDDGNode *> &visited) {
    if (!NodeUseful(Node, I) || visited.find(Node) != visited.end()) return;
    visited.insert(Node);
    string label;
    transition(label, Node->getInst());
    hash_t hashValue = MD5encoding(label.c_str());
    nowSet->addItem(hashValue);
    for (auto toNode : Node->getSuccessors())
        dfsSDDG(toNode, I, nowSet, visited);
    for (auto toNode : Node->getPredecessors())
        dfsSDDG(toNode, I, nowSet, visited);
}

bool check(SDDG *Graph, itemSet *I) {
    set<SDDGNode *> visited;
    for (auto Node : Graph->getInterestingNodes())
        if (visited.find(Node.second) == visited.end()) {
            itemSet nowSet;
            dfsSDDG(Node.second, I, &nowSet, visited);
            if (nowSet.islarger(I)) return true;
        }
    return false;
}

bool itemSet::islarger(itemSet *I) {
    map<hash_t, int>::iterator S1 = mItems.begin(), S2 = I->mItems.begin();
    map<hash_t, int>::iterator E1 = mItems.end(), E2 = I->mItems.end();
    while (1) {
        if (S1 == E1 && S2 == E2) return true;
        if (S1 == E1 || S2 == E2) return false;
        if ((*S1).first != (*S2).first) return false;
        if ((*S1).second < (*S2).second) return false;
        ++S1;
        ++S2;
    }
}

itemSet::itemSet() {}

itemSet::~itemSet() { mItems.clear(); }

void itemSet::addItem(hash_t item) { mItems[item]++; }

int itemSet::getnumItem(hash_t item) { return mItems[item]; }

void addBlockSCC(BasicBlock *block, SCCNode *SCC) { BBSCC[block] = SCC; }

SCCNode *getSCC(BasicBlock *block) { return BBSCC[block]; }

SCCNode::SCCNode(scc_iterator<Function *> iter) {
    blocks = *iter;
    for (auto block : blocks) {
        addBlockSCC(block, this);
    }
}

SCCNode::~SCCNode() {}

void SCCNode::addBlock(BasicBlock *block) { blocks.push_back(block); }

void SCCNode::addSuccessor(SCCNode *Node) { mSuccessors.insert(Node); }

void SCCNode::addPredecessor(SCCNode *Node) { mPredecessors.insert(Node); }

set<SCCNode *> &SCCNode::getSuccessors() { return mSuccessors; }

set<SCCNode *> &SCCNode::getPredecessors() { return mPredecessors; }

void SCCNode::buildRelation() {
    for (auto block : blocks) {
        succ_iterator SB = succ_begin(block), EB = succ_end(block);
        for (; SB != EB; SB++) {
            addSuccessor(getSCC(*SB));
            getSCC(*SB)->addPredecessor(this);
        }
    }
}

bool SCCNode::dfsNode(SDDG *G, itemSet *I) {
    for (auto BB : blocks)
        UsefulBlocks.insert(BB);
    for (auto toNode : getSuccessors())
        if (toNode->dfsNode(G, I))
            ;
    {
        for (auto BB : blocks)
            UsefulBlocks.erase(BB);
        return true;
    }
    if (getSuccessors().begin() == getSuccessors().end()) return check(G, I);
    for (auto BB : blocks)
        UsefulBlocks.erase(BB);
    return false;
}

SCCGraph::SCCGraph(Function &F) {
    for (scc_iterator<Function *> I = scc_begin(&F), IE = scc_end(&F); I != IE; ++I) {
        SCCNodes.push_back(new SCCNode(I));
    }
    EntryNode = getSCC(&F.getEntryBlock());
}

SCCGraph::~SCCGraph() {
    for (auto SCC : SCCNodes) {
        delete SCC;
    }
}

void SCCGraph::buildGraph() {
    set<SCCNode *> visited;
    queue<SCCNode *> SCCqueue;
    SCCqueue.push(EntryNode);
    visited.insert(EntryNode);
    while (!SCCqueue.empty()) {
        SCCNode *nowNode = SCCqueue.front();
        SCCqueue.pop();
        nowNode->buildRelation();
        for (auto toNode : nowNode->getSuccessors())
            if (visited.find(toNode) == visited.end()) {
                SCCqueue.push(toNode);
                visited.insert(toNode);
            }
    }
}

bool SCCGraph::dfsGraph(SDDG *G, itemSet *I) { return getEntry()->dfsNode(G, I); }

SCCNode *SCCGraph::getEntry() { return EntryNode; }

int CountSupport(Function &F, itemSet *I) {
    miner::SDDG SDDGF(&F);
    SCCGraph SCCF(F);
    return SCCF.dfsGraph(&SDDGF, I);
}

}  // namespace support