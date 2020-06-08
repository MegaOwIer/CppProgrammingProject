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

    rule_t(itemSet *_cond, itemSet *_full, hash_t _tar, double _p)
        : cond(_cond), full(_full), target(_tar), confidence(_p) {}

    bool operator<(const rule_t &u) { return confidence > u.confidence; }

    itemSet *getCond() const { return cond; }

    itemSet *getFull() const { return full; }

    hash_t getTarget() const { return target; }

    double getConfidence() { return confidence; }

    void display(raw_ostream &os = errs()) const;
};

class ruleSet {
    vector<rule_t> data;

public:
    ruleSet() = default;

    ~ruleSet();

    void addRule(itemSet *cond, itemSet *full, hash_t target, double confidence) {
        data.emplace_back(cond, full, target, confidence);
    }

    void sort() { std::sort(data.begin(), data.end()); }

    const vector<rule_t> &getData() { return data; }

    void display(raw_ostream &os = errs());
};

void rule_generator(Module &M, itemSets *FIs, itemSets *IIs, double min_conf, ruleSet *PARs,
                    ruleSet *NARs);

}  // namespace ruleGen

#endif