#ifndef RULE_GENERATOR_H_
#define RULE_GENERATOR_H_

#include <llvm/Support/raw_ostream.h>

#include "CountSupport.h"
#include "DataDig.h"
#include "Hash.h"

namespace ruleGen {

using namespace datadig;
using namespace SupportCount;

class rule_t {
    itemSet *cond, *full;
    hash_t target;
    double confidence;

public:
    rule_t() = delete;
    rule_t(itemSet *_cond, itemSet *_full, hash_t _tar, double _p);
    bool operator<(const rule_t &u);
    itemSet *getCond();
    itemSet *getFull();
    hash_t getTarget();
    double getConfidence();
    void display(raw_ostream &os = errs());
};

class ruleSet {
    vector<rule_t> data;

public:
    ruleSet() = default;
    ~ruleSet();
    void addRule(itemSet *cond, itemSet *full, hash_t target, double confidence);
    void sort();
    void display(raw_ostream &os = errs());
};

void rule_generator(Module &M, itemSets *FIs, itemSets *IIs, double min_conf, ruleSet *PARs,
                    ruleSet *NARs);

}  // namespace ruleGen

#endif