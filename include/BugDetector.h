#ifndef BUGDETECTOR_H_
#define BUGDETECTOR_H_

#include <map>
#include <string>
#include <vector>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include "Hash.h"
#include "RuleGenerator.h"
#include "CountSupport.h"

namespace bugfinder {

using namespace llvm;
using namespace ruleGen;

using SupportCount::SupportInfo_t;

class MessageBox {
public:
    const rule_t *rule;
    SupportInfo_t detail;
    std::string info;

    MessageBox() = delete;
    MessageBox(const rule_t *_rule, const SupportInfo_t &_detail) : rule(_rule), detail(_detail) {}
    virtual std::string what() const = 0;
};

class MessageBoxP : public MessageBox {
public:
    MessageBoxP() = delete;
    MessageBoxP(const rule_t *_rule, const SupportInfo_t &_detail);

    virtual std::string what() const { return info; }
};

class MessageBoxN : public MessageBox {
public:
    MessageBoxN() = delete;
    MessageBoxN(const rule_t *_rule, const SupportInfo_t &_detail);

    virtual std::string what() const { return info; }
};

std::vector<MessageBoxP> check_positive(Module &M, ruleSet *PARs);

std::vector<MessageBoxN> check_negative(Module &M, ruleSet *NARs);

}  // namespace bugfinder

#endif
