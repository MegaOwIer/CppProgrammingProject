#ifndef SIMPLESIMPLEDATADEPENDENCE_H_
#define SIMPLESIMPLEDATADEPENDENCE_H_

#include <set>
#include <string>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

namespace miner {

using namespace llvm;
using std::pair;
using std::set;
using std::string;
using std::vector;

class SDDGNode {
private:
    Instruction *mInst;
    vector<SDDGNode *> mSuccessors;
    vector<SDDGNode *> mPredecessors;

public:
    SDDGNode(Instruction *inst) : mInst(inst) {}
    ~SDDGNode();
    void addSuccessor(SDDGNode *dst);
    void addPredecessor(SDDGNode *dst);
    Instruction *getInst();
    vector<SDDGNode *> &getSuccessors();
    vector<SDDGNode *> &getPredecessors();
};

class SDDG {
private:
    Function *mFunc;
    DenseMap<Instruction *, SDDGNode *> mNodes;
    DenseMap<Instruction *, SDDGNode *> mInterestingNodes;
    set<pair<Instruction *, Instruction *>> mShares;
    bool share(Instruction *fst, Instruction *snd);

public:
    SDDG(Function *func) : mFunc(func) {}
    ~SDDG();
    // 创建数据依赖图及数据共享关系
    void buildSDDG();
    // 将数据依赖图中的无关元素去除，仅保留所关注的元素
    void flattenDFS(SDDGNode *, Instruction *, set<Instruction *> &);
    void flattenSDDG();
    /* 提供将数据依赖图转化为dot文件的方法，对于将字符串映射为整数之后，如何转化，请自行设计实现。
     * 参数指示是否将“数据共享关系”输出到dot文件中。
     * 获得dot文件后（加上名为Test_func.dot，执行命令：dot -Tpng -o a.png Test_func.dot可生成相应
     * 的图形文件（可修改输出格式，生成其他格式如jpg、svg等的图形文件）【需安装Graphviz】。
     */
    void dotify(bool showShareRelations = true);
    DenseMap<Instruction *, SDDGNode *>& getInterestingNodes();
};

}  // namespace miner

#endif