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

namespace SupportCount {

using std::make_pair;
using std::max;
using std::pair;
using std::queue;
using std::vector;

static map<BasicBlock *, SCCNode *> BBSCC;
static SupportInfo_t instSet;

void transition(string &normalizedStr, Value *inst) {
    raw_string_ostream rso(normalizedStr);
    if (isa<ReturnInst>(inst)) {
        rso << "return ";
        Type *rType = inst->getType();
        rType->print(rso);
    } else {
        CallInst *cinst = cast<CallInst>(inst);
        Function *cfunc = cinst->getCalledFunction();
        FunctionType *ftype = cinst->getFunctionType();
        Type *rtype = ftype->getReturnType();
        if (!rtype->isVoidTy()) {
            ftype->getReturnType()->print(rso);
            rso << " = ";
        }
        if (cfunc->hasName()) {
            rso << cfunc->getName();
        }
        rso << "(";
        for (auto iter = ftype->param_begin(); iter != ftype->param_end(); iter++) {
            if (iter != ftype->param_begin()) {
                rso << ", ";
            }
            Type *ptype = *iter;
            ptype->print(rso);
        }
        if (ftype->isVarArg()) {
            if (ftype->getNumParams()) rso << ", ";
            rso << "...";
        }
        rso << ")";
    }
    rso.flush();
    return;
}

static set<BasicBlock *> UsefulBlocks;

bool NodeUseful(SDDGNode *Node, itemSet *I) {
    BasicBlock *BB = Node->getInst()->getParent();
    if (UsefulBlocks.find(BB) == UsefulBlocks.end()) return false;
    string label;
    transition(label, Node->getInst());
    hash_t hashValue = MD5encoding(label.c_str());
    return I->getnumItem(hashValue);
}

vector<SDDGNode *> list;

void dfsSDDG(SDDGNode *Node, itemSet *I, itemSet *nowSet, set<SDDGNode *> &visited, bool genSet) {
#ifdef _LOCAL_DEBUG
    errs() << "gotin " << Node->getInst() << "\n";
#endif
    if (!NodeUseful(Node, I) || visited.find(Node) != visited.end()) {
        return;
    }
    visited.insert(Node);
    if (genSet) list.push_back(Node);
    string label;
    transition(label, Node->getInst());
#ifdef _LOCAL_DEBUG
    errs() << (void *)Node << " " << label << "\n";
#endif
    hash_t hashValue = MD5encoding(label.c_str());
    nowSet->addItem(hashValue);
    for (auto toNode : Node->getSuccessors()) {
        dfsSDDG(toNode, I, nowSet, visited, genSet);
    }
    for (auto toNode : Node->getPredecessors()) {
        dfsSDDG(toNode, I, nowSet, visited, genSet);
    }
}

bool check(SDDG *Graph, itemSet *I, bool genS) {
    set<SDDGNode *> visited;
    for (auto Node : Graph->getInterestingNodes())
        if (visited.find(Node.second) == visited.end()) {
            itemSet nowSet;
            list.clear();
            dfsSDDG(Node.second, I, &nowSet, visited, genS);
            if (nowSet.islarger(I)) {
                if (genS) {
                    for (auto Node : list) {
                        string label;
                        transition(label, Node->getInst());
                        hash_t HashValue = MD5encoding(label.c_str());
                        instSet[HashValue].push_back(Node->getInst());
                    }
                }
                return true;
            }
        }
    return false;
}

bool itemSet::islarger(itemSet *I) {
    for (auto P : I->mItems) {
        if (P.second > getnumItem(P.first)) {
            return false;
        }
    }
    return true;
}

itemSet::itemSet() {}

itemSet::itemSet(const itemSet *src) {
    SupportValue = src->SupportValue;
    mItems = src->mItems;
}

itemSet::itemSet(Instruction *inst) {
    string label;
    transition(label, inst);
    addItem(MD5encoding(label.c_str()));
}

int itemSet::getCommon(itemSet *I) {
    int ans = 0;
    for (auto item : I->mItems) {
        ans += std::min(mItems[item.first], item.second);
    }
    return ans;
}

void itemSet::print(raw_ostream &os) {
    int cnt = 0, siz = getSize();
    os << "{";
    for (auto pr : mItems) {
        for (int i = 1; i <= pr.second; i++) {
            ++cnt;
            os << MD5decoding(pr.first).c_str();
            if (cnt != siz) os << ",";
        }
    }
    os << "}";
}

#ifdef _LOCAL_DEBUG
void itemSet::printHash() {
    errs() << "{";
    for (auto pr : mItems) {
        for (int i = 1; i <= pr.second; i++) {
            errs() << (int)(pr.first & 2047) << ",";
        }
    }
    errs() << "}";
}
#endif

void addBlockSCC(BasicBlock *block, SCCNode *SCC) { BBSCC[block] = SCC; }

SCCNode *getSCC(BasicBlock *block) { return BBSCC[block]; }

SCCNode::SCCNode(scc_iterator<Function *> iter) {
    for (auto elemt : *iter)
        blocks.push_back(elemt);
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

bool SCCNode::dfsNode(SDDG *G, itemSet *I, bool genSet,set<SCCNode*> *visited) {
    if(visited->find(this)!=visited->end())
        return false;
    visited->insert(this);
    for (auto BB : blocks)
        UsefulBlocks.insert(BB);
    for (auto toNode : getSuccessors())
        if (toNode->dfsNode(G, I, genSet, visited)) {
            for (auto BB : blocks)
                UsefulBlocks.erase(BB);
            return true;
        }
    visited->erase(this);
    if (getSuccessors().begin() == getSuccessors().end()) {
        return check(G, I, genSet);
    }
    for (auto BB : blocks)
        UsefulBlocks.erase(BB);
    return false;
}

SCCGraph::SCCGraph(Function &F) {
    scc_iterator<Function *> I = scc_begin(&F), IE = scc_end(&F);
    for (; I != IE;) {
        SCCNodes.push_back(new SCCNode(I));
        ++I;
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

bool SCCGraph::dfsGraph(SDDG *G, itemSet *I, bool genSet) {
    SCCNode *now = getEntry();
    set<SCCNode*> visited;
    return now->dfsNode(G, I, genSet, &visited);
}

SCCNode *SCCGraph::getEntry() { return EntryNode; }

void addShare(SDDG *G) {
    for (auto fst : G->getInterestingNodes())
        for (auto snd : G->getInterestingNodes())
            if (G->inShare(fst.first, snd.first)) {
                // errs()<<"OK"<<"\n";
                if(fst.second==snd.second)
                    continue;
                fst.second->addSuccessor(snd.second);
                fst.second->addPredecessor(snd.second);
                snd.second->addSuccessor(fst.second);
                snd.second->addPredecessor(fst.second);
            }
}

itemSet *merge_itemSet(itemSet *fst, itemSet *snd) {
    itemSet *newItems = new itemSet;
    for (auto item : fst->getSet()) {
        newItems->getSet()[item.first] =
            max(fst->getnumItem(item.first), snd->getnumItem(item.first));
    }
    for (auto item : snd->getSet()) {
        newItems->getSet()[item.first] =
            max(fst->getnumItem(item.first), snd->getnumItem(item.first));
    }
    return newItems;
}

int itemSet::getSize() {
    int ans = 0;
    for (auto item : mItems) {
        ans += item.second;
    }
    return ans;
}

void itemSet::setFormal() {
    set<hash_t> delt;
    for (auto item : getSet()) {
        if (getSet()[item.first] == 0) {
            delt.insert(item.first);
        }
    }
    for (auto item : delt) {
        getSet().erase(item);
    }
}

pair<int, SupportInfo_t> CountSupport(Function &F, itemSet *I, bool genSet) {
    if (F.empty()) {
#ifdef _LOCAL_DEBUG
        // errs() << F.getName() << " is empty\n";
#endif
        return make_pair(0, instSet);
    }
    instSet.clear();
    miner::SDDG SDDGF(&F);
    SCCGraph SCCF(F);
    SCCF.buildGraph();
    SDDGF.buildSDDG();
    SDDGF.flattenSDDG();
    addShare(&SDDGF);
#ifdef _LOCAL_DEBUG
    // SDDGF.dotify(1);
    // errs() << "pre flattensddg\n";
    // errs() << "\n";
    // for (auto Node : SDDGF.getInterestingNodes()) {
    //     string label;
    //     transition(label, Node.first);
    //     errs() << "call:" << label << "\n";
    //     errs() << (void *)Node.first << " " << label << "\n";
    // }
    // errs() << "\n";
    // I->printHash();
#endif
    pair<int, SupportInfo_t> temp = make_pair(SCCF.dfsGraph(&SDDGF, I, genSet), instSet);
    // if(temp.first)
    // {
    //     I->print();
    //     errs()<<"dont support "<< F.getName()<<"\n";
    // }
    return temp;
}

}  // namespace SupportCount
