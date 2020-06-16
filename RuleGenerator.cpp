#include <algorithm>
#include <utility>

#include "DataDig.h"
#include "RuleGenerator.h"

namespace ruleGen {

// implementation of class rule_t.

void rule_t::display(raw_ostream &os) const {
    cond->print(os);
    os << " ==> {" << MD5decoding(target) << "}, cond = " << confidence;
}

// implementation of class ruleSet.

ruleSet::~ruleSet() {
    for (rule_t cur : data) {
        delete cur.getCond();
    }
}

void ruleSet::display(raw_ostream &os) {
    for (rule_t cur : data) {
        cur.display(os);
        os << "\n";
    }
}

// implementation of other functions.

using datadig::support;
using std::pair;

void rule_generator(Module &M, itemSets *FIs, itemSets *IIs, double min_conf, ruleSet *PARs,
                    ruleSet *NARs) {
    for (itemSet *I : FIs->getSet()) {
        I->setFormal();
        if (I->getSize() == 1) {
            continue;
        }
        for (pair<hash_t, int> cur : I->getSet()) {
            itemSet *tmp = new itemSet(I);
            if (cur.second == 1) {
                tmp->getSet().erase(cur.first);
            } else {
                tmp->getSet()[cur.first]--;
            }
            double conf = (double)I->getSupportValue() / support(M, tmp);
            if (conf >= min_conf && conf < 1) {
                PARs->addRule(tmp, I, cur.first, conf);
            } else {
                delete tmp;
            }
        }
    }
    PARs->sort();

    for (itemSet *I : IIs->getSet()) {
        I->setFormal();
        if (I->getSize() == 1) {
            continue;
        }
        itemSet *min_support = nullptr;
        hash_t target = -1;
        for (pair<hash_t, int> cur : I->getSet()) {
            itemSet *tmp = new itemSet(I);
            if (cur.second == 1) {
                tmp->getSet().erase(cur.first);
            } else {
                tmp->getSet()[cur.first]--;
            }
            int sp_val = support(M, tmp);
            if (sp_val == 0 || (min_support && sp_val >= min_support->getSupportValue())) {
                delete tmp;
            } else {
                delete min_support;
                min_support = tmp, target = cur.first;
            }
        }
        double conf = 1. - (double)I->getSupportValue() / min_support->getSupportValue();
        if (conf >= min_conf && conf < 1) {
            NARs->addRule(min_support, I, target, conf);
        } else {
            delete min_support;
        }
    }
    NARs->sort();
}

}  // namespace ruleGen
