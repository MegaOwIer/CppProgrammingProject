#ifndef BUGDETECTOR_H_
#define BUGDETECTOR_H_

#include <string>

#include <llvm/IR/Instruction.h>
#include <llvm/Support/raw_ostream.h>

#include "RuleGenerator.h"

namespace bugfinder {

using namespace llvm;
using ruleGen::rule_t;

class MessageBox {
public:
    rule_t *rule;
    Instruction *inst;

    MessageBox() = delete;
    MessageBox(rule_t *_rule, Instruction *_inst) : rule(_rule), inst(_inst) {}
    virtual void display(raw_ostream &os = errs()) = 0;
};

class MessageBoxP : public MessageBox {
    std::string info;

public:
    MessageBoxP() = delete;
    MessageBoxP(rule_t *_rule, Instruction *_inst) : MessageBox(_rule, _inst) {}
};

}  // namespace bugfinder

#endif
