#include <algorithm>
#include <utility>

#include "Datadig.h"
#include "RuleGenerator.h"

namespace ruleGen {

// implementation of class rule_t.

rule_t::rule_t(itemSet *_cond, itemSet *_full, hash_t _tar, double _p)
    : cond(_cond), full(_full), target(_tar), confidence(_p) {}

bool rule_t::operator<(const rule_t &u) { return confidence > u.confidence; }

itemSet *rule_t::getCond() { return cond; }
itemSet *rule_t::getFull() { return full; }
hash_t rule_t::getTarget() { return target; }
double rule_t::getConfidence() { return confidence; }

void rule_t::display(raw_ostream &os) {
    cond->print();
    os << " ==> {" << MD5decoding(target) << "}, cond = " << confidence;
}

// implementation of class ruleSet.

ruleSet::~ruleSet() {
    for (rule_t cur : data) {
        delete cur.getCond();
    }
}

void ruleSet::addRule(itemSet *cond, itemSet *full, hash_t target, double confidence) {
    data.emplace_back(cond, full, target, confidence);
}

void ruleSet::sort() { std::sort(data.begin(), data.end()); }

void ruleSet::display(raw_ostream &os) {
    for (rule_t cur : data) {
        cur.display();
        os << "\n";
    }
}

// implementation of other functions.

using datadig::support;
using std::pair;

void rule_generator(Module &M, itemSets *FIs, itemSets *IIs, double min_conf, ruleSet *PARs,
                    ruleSet *NARs) {
    for (itemSet *I : FIs->getSet()) {
        for (pair<hash_t, int> cur : I->getSet()) {
            itemSet *tmp = new itemSet(I);
            if (cur.second == 1) {
                tmp->getSet().erase(cur.first);
            } else {
                tmp->getSet()[cur.first]--;
            }
            double conf = (double)I->getSupportValue() / support(M, tmp);
            if (conf >= min_conf) {
                PARs->addRule(tmp, I, cur.first, conf);
            } else {
                delete tmp;
            }
        }
    }
    PARs->sort();

    for (itemSet *I : IIs->getSet()) {
        itemSet *min_support = nullptr;
        hash_t target = -1;
        for (pair<hash_t, int> cur : I->getSet()) {
            itemSet *tmp = new itemSet(I);
            if (cur.second <= 0) {
                continue;
            } else if (cur.second == 1) {
                tmp->getSet().erase(cur.first);
            } else {
                tmp->getSet()[cur.first]--;
            }
            if (min_support == nullptr) {
                min_support = tmp, target = cur.first;
                support(M, min_support);
            } else if (min_support->getSupportValue() > support(M, tmp)) {
                delete min_support;
                min_support = tmp, target = cur.first;
            } else {
                delete tmp;
            }
        }
        double conf = 1. - (double)I->getSupportValue() / min_support->getSupportValue();
        if (conf >= min_conf) {
            NARs->addRule(min_support, I, target, conf);
        } else {
            delete min_support;
        }
    }
    NARs->sort();
}

}  // namespace ruleGen
