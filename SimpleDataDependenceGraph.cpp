#include <fstream>
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
#include <utility>

#include "Hash.h"
#include "SimpleDataDependenceGraph.h"

namespace miner {

using std::endl;
using std::ofstream;
using std::pair;

// TRUE for successful insertion; FALSE indicates an existing pair.
bool SDDG::share(Instruction *fst, Instruction *snd) {
    pair<Instruction *, Instruction *> p1(fst, snd);
    pair<Instruction *, Instruction *> p2(snd, fst);
    if (mShares.find(p1) == mShares.end() && mShares.find(p2) == mShares.end()) {
        mShares.insert(p1);
        return true;
    }
    return false;
}

void SDDGNode::addSuccessor(SDDGNode *dst) { mSuccessors.push_back(dst); }

void SDDGNode::addPredecessor(SDDGNode *dst) { mPredecessors.push_back(dst); } 

vector<SDDGNode *> &SDDGNode::getSuccessors() { return mSuccessors; }

vector<SDDGNode *> &SDDGNode::getPredecessors() { return mPredecessors; }


inline Instruction *SDDGNode::getInst() { return mInst; }

SDDGNode::~SDDGNode() {}

SDDG::~SDDG() {
    for (auto iter : mNodes) {
        if (iter.second != nullptr) delete iter.second;
        iter.second = nullptr;
    }
    for (auto iter : mInterestingNodes) {
        if (iter.second != nullptr) delete iter.second;
        iter.second = nullptr;
    }
    mNodes.clear();
    mInterestingNodes.clear();
    mShares.clear();
}
DenseMap<Instruction *, SDDGNode *>& SDDG::getInterestingNodes(){return mInterestingNodes;}

namespace {

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

void dotifyToFile(DenseMap<Instruction *, SDDGNode *> &nodes,
                  set<pair<Instruction *, Instruction *>> &shares, string &file,
                  bool showShareRelations, bool showHashedValue) {
    ofstream fos;
    fos.open(file);
    fos << "digraph {\n" << endl;
    for (auto iter = nodes.begin(); iter != nodes.end(); iter++) {
        Instruction *inst = iter->first;
        fos << "Inst" << (void *)inst << "[align = left, shape = box, label = \"";
        string label;
        raw_string_ostream rso(label);
        if (!showHashedValue) {
            inst->print(rso);
            string::size_type pos(0);
            while ((pos = label.find('"', pos)) != string::npos) {
                label.replace(pos, 1, "'");
            }
        } else {
            transition(label, inst);
            hash_t hashValue = MD5encoding(label.c_str());
            label = to_string(hashValue);
        }
        fos << label << "\"];" << endl;
    }
    fos << endl;
    for (auto iter = nodes.begin(); iter != nodes.end(); iter++) {
        Instruction *src = iter->first;
        SDDGNode *node = iter->second;
        vector<SDDGNode *> successors = node->getSuccessors();
        for (SDDGNode *succ : successors) {
            Instruction *dst = succ->getInst();
            fos << "Inst" << (void *)src << " -> Inst" << (void *)dst << " [dir=back];" << endl;
        }
    }
    fos << endl;
    if (showShareRelations || nodes.empty()) {
        if (nodes.empty()) {
            for (pair<Instruction *, Instruction *> pp : shares) {
                Instruction *inst1[2] = {pp.first, pp.second};
                Instruction *inst;
                for (int idx = 0; idx < 2; idx++) {
                    inst = inst1[idx];
                    fos << "Inst" << (void *)inst << "[align = left, shape = box, label = \"";
                    string label;
                    raw_string_ostream rso(label);
                    if (!showHashedValue) {
                        inst->print(rso);
                        string::size_type pos(0);
                        while ((pos = label.find('"', pos)) != string::npos) {
                            label.replace(pos, 1, "'");
                        }
                    } else {
                        transition(label, inst);
                        hash_t hashValue = MD5encoding(label.c_str());
                        label = to_string(hashValue);
                    }
                    fos << label << "\"];" << endl;
                }
            }
            fos << endl;
        }
        for (pair<Instruction *, Instruction *> pp : shares) {
            Instruction *inst1 = pp.first;
            Instruction *inst2 = pp.second;
            fos << "Inst" << (void *)inst1 << " -> Inst" << (void *)inst2
                << " [dir=none, color=red, style=dashed];" << endl;
        }
    }
    fos << "}" << endl;
    fos.close();
}

}  // namespace

void SDDG::dotify(bool showShareRelations) {
    std::string file = mFunc->getName().str() + ".dot";
    dotifyToFile(mNodes, mShares, file, showShareRelations, false);
    if (!mInterestingNodes.empty()) {
        file = mFunc->getName().str() + ".flat.dot";
        dotifyToFile(mInterestingNodes, mShares, file, showShareRelations, false);
        file = mFunc->getName().str() + ".transaction.dot";
        dotifyToFile(mInterestingNodes, mShares, file, showShareRelations, true);
    }
}

namespace dfa {

class Definition {
    DenseMap<Value *, Instruction *> mDef;
    
public:
    Definition() = default;
    ~Definition() { mDef.clear(); }
    void define(Value *var, Instruction *inst) { mDef[var] = inst; }
    DenseMap<Value *, Instruction *> &getDef() { return mDef; }
    Instruction *getDef(Value *var) {
        auto iter = mDef.find(var);
        if (iter != mDef.end()) {
            return iter->second;
        }
        return nullptr;
    }
    // 输出当前基本块的Definition信息，主要用于测试、检查代码正确性
    void dump() {
        errs() << "Definitions: \n";
        for (auto iter = mDef.begin(); iter != mDef.end(); iter++) {
            errs() << *(iter->first) << "  <-  " << *(iter->second) << "\n";
        }
        errs() << "\n";
    }
};

class Use {
    DenseMap<Value *, set<Instruction *> *> mUse;

public:
    Use() = default;
    ~Use() {
        for (auto iter = mUse.begin(); iter != mUse.end(); iter++) {
            set<Instruction *> *uses = iter->second;
            if (uses) delete uses;
        }
        mUse.clear();
    }

    DenseMap<Value *, set<Instruction *> *> &getUse() { return mUse; }
    set<Instruction *> *getUse(Value *var) {
        auto iter = mUse.find(var);
        if (iter != mUse.end()) return iter->second;
        return nullptr;
    }
    bool use(Value *var, Instruction *inst) {  // true 代表改变了
        if (mUse.find(var) == mUse.end()) mUse[var] = new set<Instruction *>;
        if (mUse[var]->find(inst) != mUse[var]->end()) return false;
        mUse[var]->insert(inst);
        return true;
    }
    void dump() {
        errs() << "Use:\n";
        for (auto iter = mUse.begin(); iter != mUse.end(); iter++) {
            Value *key = iter->first;
            set<Instruction *> *val = iter->second;
            errs() << " Use: " << *key << "  at \n";
            for (Instruction *vv : *val) {
                errs() << "  ++ " << *vv << "\n";
            }
        }
    }
};

// 以下两个以“Share”开始的类仅用于计算数据共享关系
class ShareDefinition {
    DenseMap<Value *, set<Value *> *> mShareDef;

public:
    ShareDefinition() = default;
    ~ShareDefinition() {
        for (auto iter = mShareDef.begin(); iter != mShareDef.end(); iter++) {
            set<Value *> *sdefs = iter->second;
            if (sdefs) delete sdefs;
        }
        mShareDef.clear();
    }
    DenseMap<Value *, set<Value *> *> &getShareDefs() { return mShareDef; }
    set<Value *> *getShareDef(Value *var) {
        auto iter = mShareDef.find(var);
        if (iter != mShareDef.end()) {
            return iter->second;
        }
        return nullptr;
    }
    void shareDefine(Value *var, Value *def) {
        auto iter = mShareDef.find(var);
        set<Value *> *vSet;
        set<Value *> *dSet = getShareDef(def);
        if (iter != mShareDef.end()) {
            vSet = iter->second;
            vSet->clear();  // re-definition overrides all old ones.
        } else {
            vSet = new set<Value *>;
            mShareDef[var] = vSet;
        }
        if (dSet) {
            for (Value *ds : *dSet) {
                vSet->insert(ds);
            }
        } else {
            vSet->insert(def);
        }
    }
    void dump() {
        errs() << "ShareDefinition:\n";
        for (auto iter = mShareDef.begin(); iter != mShareDef.end(); iter++) {
            Value *key = iter->first;
            set<Value *> *val = iter->second;
            errs() << " Define: " << *key << "  as \n";
            for (Value *vv : *val) {
                errs() << "  ++ " << *vv << "\n";
            }
        }
    }
};

class ShareUse {
    DenseMap<Value *, set<Instruction *> *> mShareUse;
    set<Instruction *> *findOrCreateSharedUse(Value *var) {
        auto iter = mShareUse.find(var);
        if (iter != mShareUse.end()) return iter->second;
        set<Instruction *> *sSet = new set<Instruction *>;
        mShareUse[var] = sSet;
        return sSet;
    }

public:
    ShareUse() = default;
    ~ShareUse() {
        for (auto iter = mShareUse.begin(); iter != mShareUse.end(); iter++) {
            set<Instruction *> *suses = iter->second;
            if (suses) delete suses;
        }
        mShareUse.clear();
    }
    DenseMap<Value *, set<Instruction *> *> &getShareUses() { return mShareUse; }
    set<Instruction *> *getShareUse(Value *var) {
        auto iter = mShareUse.find(var);
        if (iter != mShareUse.end()) return iter->second;
        return nullptr;
    }
    void shareUse(Value *var, Instruction *inst, ShareDefinition *sdef) {
        set<Value *> *vDef = sdef->getShareDef(var);
        if (vDef) {
            for (Value *vd : *vDef) {
                set<Instruction *> *vdUse = findOrCreateSharedUse(vd);
                vdUse->insert(inst);
            }
        }
        // keep var:USE(inst) as well for a simple merge in DFA.
        set<Instruction *> *vUse = findOrCreateSharedUse(var);
        vUse->insert(inst);
    }
    void dump() {
        errs() << "ShareUse:\n";
        for (auto iter = mShareUse.begin(); iter != mShareUse.end(); iter++) {
            Value *key = iter->first;
            set<Instruction *> *val = iter->second;
            errs() << " Use: " << *key << "  at \n";
            for (Instruction *vv : *val) {
                errs() << "  ++ " << *vv << "\n";
            }
        }
    }
};

template <typename TK, typename TV>
TV *findOrCreate(DenseMap<TK *, TV *> &ian, TK *bb) {
    auto iter = ian.find(bb);
    TV *agr = nullptr;
    if (iter == ian.end()) {
        agr = new TV;
        ian[bb] = agr;
    } else {
        agr = iter->second;
    }
    return agr;
}

/* Merge two maps.
 * Returns TRUE for changes in 'to' and FALSE for no changes in 'to'.
 */
template <typename TSE>
bool mergeTwoMaps(DenseMap<Value *, set<TSE *> *> &to,
                  const DenseMap<Value *, set<TSE *> *> &from) {
    bool changed = false;
    for (auto iter : from) {
        Value *var = iter.first;
        set<TSE *> *fromSet = iter.second;
        if (to.find(var) == to.end()) {
            to[var] = new set<TSE *>;
            changed = true;
        }
        set<TSE *> *toSet = to[var];
        for (auto setIter : *fromSet) {
            if (toSet->find(setIter) != toSet->end()) continue;
            toSet->insert(setIter);
            changed = true;
        }
    }
    return changed;
}

}  // namespace dfa

/* Clean a map in order to prevent memory leak.
 */
template <typename T>
inline void mapCleaner(DenseMap<BasicBlock *, T *> &mp) {
    for (auto cur : mp) {
        if (cur.second != nullptr) {
            delete cur.second;
        }
    }
    mp.clear();
}

void SDDG::buildSDDG() {
    DenseMap<BasicBlock *, dfa::Definition *> dfaDepDefs;
    DenseMap<BasicBlock *, dfa::Use *> dfaDepUses;
    // 1. 初始化每个基本块的gen和kill、definition和use，并建立基本块内部的数据依赖关系
    // 初始化 Definition 和 Use 并建立基本块内依赖关系
    for (auto bbIter = mFunc->begin(); bbIter != mFunc->end(); bbIter++) {
        BasicBlock &bb = *bbIter;
        dfa::Definition *bbDef = dfa::findOrCreate(dfaDepDefs, &bb);
        dfa::Use *bbUse = dfa::findOrCreate(dfaDepUses, &bb);
        for (auto instIter = bb.begin(); instIter != bb.end(); instIter++) {
            Instruction *inst = dyn_cast<Instruction>(instIter);

#ifdef _LOCAL_DEBUG
            errs() << *inst << '\n';
#endif

            if (Instruction::Alloca == inst->getOpcode()) {
                continue;
            } else if (Instruction::Store == inst->getOpcode()) {
                // store fstOp, sndOp ==> sndOp = fstOp
                mNodes[inst] = new SDDGNode(inst);
                Value *fstOp = inst->getOperand(0);
                Value *sndOp = inst->getOperand(1);

                // 对于 b 来说此处为一个 definition
                bbDef->define(sndOp, inst);

                // 如果 a 未在块内被定义则放入 Use 集合，否则建立块内依赖关系
                if (bbDef->getDef().find(fstOp) != bbDef->getDef().end()) {
                    mNodes[bbDef->getDef()[fstOp]]->addSuccessor(mNodes[inst]);
                    mNodes[inst]->addPredecessor(mNodes[bbDef->getDef()[fstOp]]);
                } else {
                    bbUse->use(fstOp, inst);
                }
            } else if (Instruction::Load == inst->getOpcode()) {
                // lValue = load op ==> lValue = op
                mNodes[inst] = new SDDGNode(inst);
                Value *lValue = dyn_cast<Value>(inst);
                Value *op = inst->getOperand(0);
                bbDef->define(lValue, inst);

                if (bbDef->getDef().find(op) != bbDef->getDef().end()) {
                    mNodes[bbDef->getDef()[op]]->addSuccessor(mNodes[inst]);
                    mNodes[inst]->addPredecessor(mNodes[bbDef->getDef()[op]]);
                } else {
                    bbUse->use(op, inst);
                }
            } else if (Instruction::Call == inst->getOpcode()) {
                // 函数调用
                Function *func = (dyn_cast<CallInst>(inst))->getCalledFunction();
                // 检查 intrinsic 函数，只留下 memcpy
                if (func->isIntrinsic()) {
                    if (func->getIntrinsicID() != Intrinsic::memcpy) continue;
                }

                mNodes[inst] = new SDDGNode(inst);

                // 不是 void 类型的函数则将返回值记为一个 definition
                if (!inst->getType()->isVoidTy()) {
                    Value *lValue = dyn_cast<Value>(inst);
                    bbDef->define(lValue, inst);
                }

                // 最后一个 Operand 似乎是函数声明
                unsigned int nOprands = inst->getNumOperands() - 1;

                // 参数计入 Use 集合
                for (unsigned int idxo = 0; idxo < nOprands; idxo++) {
                    Value *var = inst->getOperand(idxo);
                    if (bbDef->getDef().find(var) != bbDef->getDef().end()) {
                        mNodes[bbDef->getDef()[var]]->addSuccessor(mNodes[inst]);
                        mNodes[inst]->addPredecessor(mNodes[bbDef->getDef()[var]]);
                    } else {
                        bbUse->use(var, inst);
                    }
                }
            } else if (Instruction::Ret == inst->getOpcode() ||
                       Instruction::Br == inst->getOpcode()) {
                // 返回和分支跳转
                unsigned int nOprands = inst->getNumOperands();
                bool interesting = false;
                for (unsigned int idxo = 0; idxo < nOprands; idxo++) {
                    Value *var = inst->getOperand(idxo);
                    // 检查是否有用到有实际意义的参数
                    if (isa<Argument>(var) || isa<Instruction>(var)) {
                        // 如果语句包含有意义的参数则在图中分配一个 node
                        if (!interesting) {
                            mNodes[inst] = new SDDGNode(inst);
                            interesting = true;
                        }
                        if (bbDef->getDef().find(var) != bbDef->getDef().end()) {
                            mNodes[bbDef->getDef()[var]]->addSuccessor(mNodes[inst]);
                            mNodes[inst]->addPredecessor(mNodes[bbDef->getDef()[var]]);
                        } else {
                            bbUse->use(var, inst);
                        }
                    }
                }
            } else {
                // 其它指令参考下方 share 代码实现
                bool interesting = false;
                // 检查这个指令是否被使用过 -> 指令是否是一个定义？
                if (!inst->use_empty()) {
                    Value *lValue = dyn_cast<Value>(inst);
                    bbDef->define(lValue, inst);
                    mNodes[inst] = new SDDGNode(inst);
                    interesting = true;
                }
                unsigned int nOprands = inst->getNumOperands();
                for (unsigned int idxo = 0; idxo < nOprands; idxo++) {
                    Value *var = inst->getOperand(idxo);
                    if (isa<Argument>(var) || isa<Instruction>(var)) {
                        if (!interesting) {
                            mNodes[inst] = new SDDGNode(inst);
                            interesting = true;
                        }
                        if (bbDef->getDef().find(var) != bbDef->getDef().end()) {
                            mNodes[bbDef->getDef()[var]]->addSuccessor(mNodes[inst]);
                            mNodes[inst]->addPredecessor(mNodes[bbDef->getDef()[var]]);
                        } else {
                            bbUse->use(var, inst);
                        }
                    }
                }
                if (interesting) mNodes[inst] = new SDDGNode(inst);
            }
        }
    }
    // 初始化 gen-kill
    DenseMap<BasicBlock *, dfa::Use *> kill;
    for (auto bbIter = mFunc->begin(); bbIter != mFunc->end(); bbIter++) {
        BasicBlock &bb = *bbIter;

        // 一个基本块的 gen 是它生成的变量语句，也就是被它定义过了
        dfa::Definition *gen = dfaDepDefs[&bb];
        dfa::Use *bbKill = dfa::findOrCreate(kill, &bb);

        // 找到一个该基本块 gen 的变量
        for (auto genIter = gen->getDef().begin(); genIter != gen->getDef().end(); genIter++) {
            Value *var = genIter->first;
            // 遍历前驱
            for (auto iter : predecessors(&bb)) {
                dfa::Definition *pre = dfaDepDefs[iter];
                if (pre->getDef().find(var) == pre->getDef().end()) continue;
                // 前驱中定义了该变量则该块中的定义 kill 了前驱中的定义
                Instruction *inst = pre->getDef()[var];
                bbKill->use(var, inst);
            }
        }
    }

    // 2. 根据迭代数据流算法，计算IN和OUT
    DenseMap<BasicBlock *, dfa::Use *> IN, OUT;
    for (auto bbIter = mFunc->begin(); bbIter != mFunc->end(); bbIter++) {
        BasicBlock &bb = *bbIter;
        // 初始化 OUT[B] := gen[B]
        dfa::Definition *gen = dfa::findOrCreate(dfaDepDefs, &bb);
        dfa::Use *bbOUT = dfa::findOrCreate(OUT, &bb);
        for (auto iter = gen->getDef().begin(); iter != gen->getDef().end(); iter++) {
            bbOUT->use(iter->first, iter->second);
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto bbIter = mFunc->begin(); bbIter != mFunc->end(); bbIter++) {
            BasicBlock &bb = *bbIter;
            dfa::Use *bbIN = dfa::findOrCreate(IN, &bb);
            for (auto iter : predecessors(&bb)) {
                // IN[B] := \/ OUT[P]
                dfa::Use *preOUT = dfa::findOrCreate(OUT, iter);
                if (dfa::mergeTwoMaps(bbIN->getUse(), preOUT->getUse())) {
                    changed = true;
                }
            }
#ifdef _LOCAL_DEBUG
            errs() << changed << '\n';
#endif

            // 用 IN[B] 更新 kill[B]
            dfa::Use *bbKill = dfa::findOrCreate(kill, &bb);
            dfa::Definition *gen = dfaDepDefs[&bb];
            for (auto inIter : bbIN->getUse()) {
                Value *var = inIter.first;
                set<Instruction *> *inSet = inIter.second;
                if (gen->getDef().find(var) ==
                    gen->getDef().end())  // 基本块内没有 var 变量的定义就不会被 kill
                    continue;
                for (auto inst : *inSet) {
                    changed |= bbKill->use(var, inst);
                }
            }
            // OUT[B] = gen[B] \/ (IN[B] - kill[B]) ==> out[B] 改变意味着 IN[B] / kill[B] 改变
            if (changed) {
                dfa::Use *bbOUT = dfa::findOrCreate(OUT, &bb);
                for (auto iter = bbOUT->getUse().begin(); iter != bbOUT->getUse().end(); iter++) {
                    set<Instruction *> *uses = iter->second;
                    if (uses) delete uses;
                }
                bbOUT->getUse().clear();                             // 清空 OUT[B]
                dfa::mergeTwoMaps(bbOUT->getUse(), bbIN->getUse());  // OUT[B] = IN[B]
                dfa::Use *bbKill = dfa::findOrCreate(kill, &bb);

                // OUT[B] -= kill[B]
                for (auto iter = bbKill->getUse().begin(); iter != bbKill->getUse().end(); iter++) {
                    Value *fir = iter->first;  // 取出 kill[B] 中的一个元素
                    set<Instruction *> *snd = iter->second;
                    if (bbOUT->getUse().find(fir) == bbOUT->getUse().end())  // 不在 OUT[B] 中就跳过
                        continue;
                    set<Instruction *> *bbSnd =
                        bbOUT->getUse()[fir];  // 取出 OUT[B] 中变量的 Instruction 集合
                    for (auto iter = snd->begin(); iter != snd->end(); iter++) {  // 遍历 kill[B]
                        // 在 OUT[B] 中找到了 kill[B] 的 Instruction 就删掉
                        if (bbSnd->find(*iter) != bbSnd->end()) {
                            bbSnd->erase(*iter);
                        }
                    }
                    if (bbSnd->empty()) {
                        delete bbSnd;
                        bbOUT->getUse().erase(fir);
                    }
                }
                dfa::Definition *gen = dfa::findOrCreate(dfaDepDefs, &bb);  // OUT[B] \/= gen[B];
                for (auto iter = gen->getDef().begin(); iter != gen->getDef().end(); iter++) {
                    bbOUT->use(iter->first, iter->second);
                }
            }
        }
    }

    // 3. 根据每个基本块的IN和use信息，更新数据依赖图
    for (auto bbIter = mFunc->begin(); bbIter != mFunc->end();
         bbIter++) {  // 处理基本块之间的依赖关系
        BasicBlock &bb = *bbIter;
        dfa::Use *bbUse = dfaDepUses[&bb];
        dfa::Use *bbIN = IN[&bb];

#ifdef _LOCAL_DEBUG
        bbUse->dump();
        bbIN->dump();
        errs() << '\n';
#endif

        // 取出 Use 集合中的元素，即在基本块中使用了却没有定义的元素
        for (auto iter = bbUse->getUse().begin(); iter != bbUse->getUse().end(); iter++) {
            Value *fir = iter->first;
            set<Instruction *> *snd = iter->second;
            // 遍历使用了该变量的指令集合
            for (auto instIter : *snd) {
                // IN[B] 是前驱输出状态的并集，因此在 IN[B] 中查找该变量的定义
                // 找到之后则使用指令与定义指令之间存在依赖关系
                if (bbIN->getUse().find(fir) != bbIN->getUse().end()) {
                    set<Instruction *> *instSet = bbIN->getUse()[fir];
                    for (auto tmpIter : *instSet) {
                        mNodes[tmpIter]->addSuccessor(mNodes[instIter]);
                        mNodes[instIter]->addPredecessor(mNodes[tmpIter]);
                    }
                }
            }
        }
    }

    // Below we compute the DataShare relations using the following forwarding DFA algorithm:
    // IN(ShareDef : B) = \/[P of B's predecessor](OUT(ShareDef : P))
    // OUT(ShareDef : B) = (IN(ShareDef : B) - Def(B)) \/ ShareDef(B)
    // IN(ShareUse : B) = \/[P of B's predecessor](OUT(ShareUse : P))
    // OUT(ShareUse : B) =  (IN(ShareUse : B) - Def(B)) \/ ShareUse(B)
    // Be careful that "\/" may not a simple merge, but an update with particular operations.
    DenseMap<BasicBlock *, dfa::ShareDefinition *> sDefIN, sDefOUT;
    DenseMap<BasicBlock *, dfa::ShareUse *> sUseIN, sUseOUT;
    DenseMap<BasicBlock *, dfa::ShareDefinition *> sDfaShareDefs;
    DenseMap<BasicBlock *, dfa::ShareUse *> sDfaShareUses;
    // 1. scan for initializing ShareDef and ShareUse of each BB
    for (auto bbIter = mFunc->begin(); bbIter != mFunc->end(); bbIter++) {
        BasicBlock &bb = *bbIter;
        dfa::ShareDefinition *bbSDef = dfa::findOrCreate(sDfaShareDefs, &bb);
        dfa::ShareUse *bbSUse = dfa::findOrCreate(sDfaShareUses, &bb);
        for (auto instIter = bb.begin(); instIter != bb.end(); instIter++) {
            Instruction *inst = dyn_cast<Instruction>(instIter);
#ifdef _LOCAL_DEBUG
            errs() << "Analyze Inst: " << *inst << "  **#OP: " << inst->getNumOperands() << "\n";
#endif
            if (Instruction::Alloca == inst->getOpcode()) {
                continue;
            } else if (Instruction::Store == inst->getOpcode()) {
                Value *fstOp = inst->getOperand(0);
                Value *sndOp = inst->getOperand(1);
                if (isa<Argument>(fstOp) || isa<Instruction>(fstOp)) {
                    // e.g., fstOp is "x = call f()"
                    bbSDef->shareDefine(sndOp, fstOp);
                } else {
                    // should we consider usages of the same constant as a DataShare?
                    bbSDef->shareDefine(sndOp, inst);
                }
                // no need to build ShareUse as we only care uses in interesting nodes (call, ret)
            } else if (Instruction::Call == inst->getOpcode() ||
                       Instruction::Ret == inst->getOpcode()) {
                unsigned int nOprands = inst->getNumOperands();
                // 处理 Intrinsic 函数
                if (Instruction::Call == inst->getOpcode()) {
                    Function *func = (dyn_cast<CallInst>(inst))->getCalledFunction();
                    if (func->isIntrinsic()) {
                        if (func->getIntrinsicID() != Intrinsic::memcpy) continue;
                    }
                }
                if (Instruction::Call == inst->getOpcode() && !inst->getType()->isVoidTy()) {
                    bbSDef->shareDefine(inst, inst);
                    --nOprands;
                }
                // build DataShare relations for all incoming uses that can be seen within this BB
                for (unsigned int idxo = 0; idxo < nOprands; idxo++) {
                    Value *op = inst->getOperand(idxo);
                    set<Instruction *> *opSUses = bbSUse->getShareUse(op);
                    if (opSUses) {
                        for (Instruction *opUse : *opSUses) {
                            share(inst, opUse);
                        }
                    }
                    // as we do not obtain the ShareDef's uses in getShareUse(), we need a further
                    // step here.
                    set<Value *> *opSDefs = bbSDef->getShareDef(op);
                    if (opSDefs) {
                        for (Value *opDef : *opSDefs) {
                            opSUses = bbSUse->getShareUse(opDef);
                            if (opSUses) {
                                for (Instruction *opUse : *opSUses) {
                                    share(inst, opUse);
                                }
                            }
                        }
                    }
                }
                // update ShareUse
                for (unsigned int idxo = 0; idxo < nOprands; idxo++) {
                    Value *op = inst->getOperand(idxo);
                    if (isa<Argument>(op) || isa<Instruction>(op)) {
                        bbSUse->shareUse(op, inst, bbSDef);
                    }
                }
            } else if (Instruction::Br == inst->getOpcode()) {
                // skip. do nothing for 'br'
                continue;
            } else {
                // for other instructions, 'operand's exist only at RHS??
                // ShareUse does nothing, but ShareDef may be updated
                if (inst->use_empty()) {  // not a definition
                    continue;
                }
                for (unsigned int idxo = 0; idxo < inst->getNumOperands(); idxo++) {
                    Value *op = inst->getOperand(idxo);
                    if (isa<Argument>(op) || isa<Instruction>(op)) {
                        bbSDef->shareDefine(inst, op);
                    }
                }
            }
        }
#ifdef _LOCAL_DEBUG
        errs() << "\n#### BB" << &bb << "\n";
        bbSDef->dump();
        bbSUse->dump();
#endif
    }
    // 2. iteratively compute IN/OUT for ShareDef and ShareUse
    changed = true;
    int round = 1;
    while (changed) {
        round++;
        changed = false;
        for (auto bbIter = mFunc->begin(); bbIter != mFunc->end(); bbIter++) {
            BasicBlock &bb = *bbIter;
            dfa::ShareDefinition *bbSDefIN = dfa::findOrCreate(sDefIN, &bb);
            dfa::ShareDefinition *bbSDefOUT = dfa::findOrCreate(sDefOUT, &bb);
            dfa::ShareUse *bbSUseIN = dfa::findOrCreate(sUseIN, &bb);
            dfa::ShareUse *bbSUseOUT = dfa::findOrCreate(sUseOUT, &bb);
            dfa::ShareDefinition tmpSDefIN, tmpSDefOUT;
            dfa::ShareUse tmpSUseIN, tmpSUseOUT;
            // 2.1 merge OUT to IN
            for (auto predIter = bb.user_begin(); predIter != bb.user_end(); predIter++) {
                User *user = *predIter;
                Instruction *terminator = dyn_cast<Instruction>(user);
                BasicBlock *predBB = terminator->getParent();
                // 2.1.1 merge OUT(ShareDef) to tmpIN
                dfa::ShareDefinition *predSDef = dfa::findOrCreate(sDefOUT, predBB);
                dfa::mergeTwoMaps(tmpSDefIN.getShareDefs(), predSDef->getShareDefs());
                // 2.1.2 merge OUT(ShareUse) to tmpIN
                dfa::ShareUse *predSUse = dfa::findOrCreate(sUseOUT, predBB);
                dfa::mergeTwoMaps(tmpSUseIN.getShareUses(), predSUse->getShareUses());
            }
            // 2.1.3 merge OUTs (in tmpIN) to IN(ShareDef)
            changed =
                dfa::mergeTwoMaps(bbSDefIN->getShareDefs(), tmpSDefIN.getShareDefs()) | changed;
            // 2.1.4 merge OUTs (in tmpIN) to IN(ShareUse)
            changed =
                dfa::mergeTwoMaps(bbSUseIN->getShareUses(), tmpSUseIN.getShareUses()) | changed;

#ifdef _LOCAL_DEBUG
            errs() << " After merging OUT for BB" << &bb << "\n";
            errs() << " ~~~~ IN : ShareDef ~~~~\n";
            bbSDefIN->dump();
            errs() << "\n ~~~~ IN : ShareUse ~~~~\n";
            bbSUseIN->dump();
#endif
            // 2.2 compute OUT
            // 2.2.1 ShareDef: tmpOUT = IN(ShareDef : B) - Def(B) [rm all Defs in IN if re-defined]
            dfa::Definition *bbDef = dfaDepDefs[&bb];
            DenseMap<Value *, Instruction *> &bbDefs = bbDef->getDef();
            DenseMap<Value *, set<Value *> *> &bbSDefIns = bbSDefIN->getShareDefs();
            DenseMap<Value *, set<Value *> *> &bbSDefTmpOUT = tmpSDefOUT.getShareDefs();
            // bbDef->dump();
            for (auto iter = bbSDefIns.begin(); iter != bbSDefIns.end(); iter++) {
                Value *key = iter->first;
                // copy from IN to tmpOUT only when not defined in current BB
                // errs() << *key << '\n';
                if (bbDefs.find(key) == bbDefs.end()) {
                    set<Value *> *alreadyInIN = iter->second;
                    set<Value *> *newTmpSet = new set<Value *>;
                    for (Value *vv : *alreadyInIN) {
                        newTmpSet->insert(vv);
                    }
                    bbSDefTmpOUT[key] = newTmpSet;
                }
            }
            // 2.2.2 ShareDef: tmpOUT = tmpOUT \/ ShareDef(B)
            dfa::ShareDefinition *bbShareDef = sDfaShareDefs[&bb];
            dfa::mergeTwoMaps(tmpSDefOUT.getShareDefs(), bbShareDef->getShareDefs());
            // 2.2.3 ShareDef: merge tmpOUT to OUT(ShareDef)
            changed =
                dfa::mergeTwoMaps(bbSDefOUT->getShareDefs(), tmpSDefOUT.getShareDefs()) | changed;
            // 2.2.4 ShareUse: tmpOUT = IN(ShareUse : B) - Def(B)
            DenseMap<Value *, set<Instruction *> *> &bbSUseIns = bbSUseIN->getShareUses();
            DenseMap<Value *, set<Instruction *> *> &bbSUseTmpOUT = tmpSUseOUT.getShareUses();
            for (auto iter = bbSUseIns.begin(); iter != bbSUseIns.end(); iter++) {
                Value *key = iter->first;
                auto miter = bbDefs.find(key);
                // copy from IN to tmpOUT only when not defined in current BB
                if (miter == bbDefs.end()) {
                    set<Instruction *> *alreadyInIN = iter->second;
                    set<Instruction *> *newTmpSet = new set<Instruction *>;
                    for (Instruction *vv : *alreadyInIN) {
                        newTmpSet->insert(vv);
                    }
                    bbSUseTmpOUT[key] = newTmpSet;
                }
            }
            // 2.2.5 ShareUse: tmpOUT = tmpOUT \/ ShareUse(B)
            // // if we allow a:USE and b:USE (a depends on b) in ShareUse, we can simply merge
            // them.
            dfa::mergeTwoMaps(bbSUseTmpOUT, sDfaShareUses[&bb]->getShareUses());
            // 2.2.6 SahreUse: merge tmpOUT to OUT(ShareUse)
            changed = dfa::mergeTwoMaps(bbSUseOUT->getShareUses(), bbSUseTmpOUT) | changed;
            // errs() << "\n After computing OUT for BB" << &bb << "\n";
            // errs() << " ~~~~ OUT : ShareDef ~~~~\n";
            // bbSDefOUT->dump();
            // errs() << "\n ~~~~ OUT : ShareUse ~~~~\n";
            // bbSUseOUT->dump();
        }
    }
    // 3. update DataShare with IN(ShareUse : B) and ShareUse(B)
    for (auto bbIter = mFunc->begin(); bbIter != mFunc->end(); bbIter++) {
        BasicBlock &bb = *bbIter;
        dfa::ShareDefinition *bbSDefIN = dfa::findOrCreate(sDefIN, &bb);
        dfa::ShareUse *bbSUseIN = dfa::findOrCreate(sUseIN, &bb);
        dfa::ShareUse *bbSUse = sDfaShareUses[&bb];
        DenseMap<Value *, set<Instruction *> *> &bbSuses = bbSUse->getShareUses();
        for (auto iter = bbSuses.begin(); iter != bbSuses.end(); iter++) {
            Value *key = iter->first;
            set<Instruction *> *suseSet = iter->second;
            set<Value *> *sdefSet = bbSDefIN->getShareDef(key);
            if (sdefSet) {
                for (Value *dv : *sdefSet) {
                    set<Instruction *> *duseSet = bbSUseIN->getShareUse(dv);
                    if (duseSet) {
                        for (Instruction *di : *duseSet) {
                            for (Instruction *ds : *suseSet) {
                                share(ds, di);
                            }
                        }
                    }
                }
            }
        }
    }

    // clear maps.
    mapCleaner(sDefIN), mapCleaner(sDefOUT);
    mapCleaner(sUseIN), mapCleaner(sUseOUT);
    mapCleaner(sDfaShareDefs), mapCleaner(sDfaShareUses);
    mapCleaner(IN), mapCleaner(OUT);
    mapCleaner(dfaDepDefs), mapCleaner(dfaDepUses);
    mapCleaner(kill);
}

void SDDG::flattenDFS(SDDGNode *self, Instruction *inst, set<Instruction *> &visited) {
    SDDGNode *instNode = mNodes[inst];

    for (auto iter : instNode->getPredecessors()) {
        Instruction *nowInst = iter->getInst();
        if (visited.find(nowInst) != visited.end()) continue;
        visited.insert(nowInst);
        if (Instruction::Call != nowInst->getOpcode() && Instruction::Ret != nowInst->getOpcode()) {
            // 不是 call 和 ret 就继续遍历
            flattenDFS(self, nowInst, visited);
        } else {
            // 是 call 或 ret 就连边返回
            mInterestingNodes[nowInst]->addSuccessor(self);
            self->addPredecessor(mInterestingNodes[nowInst]);
        }
    }
}

void SDDG::flattenSDDG() {
    // 取出所有的 call 和 ret 语句
    for (auto iter : mNodes) {
        Instruction *inst = iter.first;
        if (Instruction::Call == inst->getOpcode() || Instruction::Ret == inst->getOpcode())
            mInterestingNodes[inst] = new SDDGNode(inst);
    }
    set<Instruction *> visited;
    for (auto iter : mInterestingNodes) {
        Instruction *inst = iter.first;
        visited.insert(inst);
        // 从一个 call 或 ret 开始遍历整个图
        flattenDFS(mInterestingNodes[inst], inst, visited);
        visited.clear();
    }
}

}  // namespace miner