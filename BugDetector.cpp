#include <sstream>
#include <string>

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>

#include "BugDetector.h"

namespace bugfinder {

namespace {

using DebugInfo_t = std::pair<unsigned, std::string>;

DebugInfo_t getDebugInfo(Instruction *inst) {
    auto debugLoc = inst->getDebugLoc();
    unsigned lineNum = 0;
    std::string fileName;
    if (debugLoc) {
        lineNum = debugLoc.getLine();
        MDNode *scope = debugLoc.getScope();
        if (isa<DISubprogram>(scope)) {
            DISubprogram *disp = cast<DISubprogram>(scope);
            fileName = disp->getFilename().str();
        }
    }
    return std::make_pair(lineNum, fileName);
}

}  // namespace

// implementation of class MessageBoxP

MessageBoxP::MessageBoxP(const rule_t *_rule, const SupportInfo_t &_detail)
    : MessageBox(_rule, _detail) {
    std::stringstream ss;
    ss << "**WARNING**\n"
       << "  Expect an instruction but MISSING: \"" << MD5decoding(rule->getTarget()) << "\"\n"
       << "  This warning is generated because the following instructions are found:\n";
    for (auto [HashVal, cnt] : rule->getCond()->getSet()) {
        // outs() << MD5decoding(HashVal) << detail[HashVal].size() << ' ' << cnt << "\n";
        // assert(detail[HashVal].size() >= cnt);
        for (size_t i = 0; i < cnt; i++) {
            auto [lineNum, fileName] = getDebugInfo(detail[HashVal][i]);
            ss << "    - \"" << MD5decoding(HashVal) << "\" at " << fileName << ":" << lineNum
               << "\n";
        }
    }
    info = ss.str();
}

// implementation of class MessageBoxN

MessageBoxN::MessageBoxN(const rule_t *_rule, const SupportInfo_t &_detail)
    : MessageBox(_rule, _detail) {
    std::stringstream ss;
    auto [tLineNum, tFileName] = getDebugInfo(detail[rule->getTarget()].back());
    ss << "**WARNING**\n"
       << "  Instruction \"" << MD5decoding(rule->getTarget()) << "\" at " << tFileName << ":"
       << tLineNum << " might be REDUNDANT.\n"
       << "  This warning is generated because the following instructions are found:\n";
    for (auto [HashVal, cnt] : rule->getCond()->getSet()) {
        // outs() << MD5decoding(HashVal) << detail[HashVal].size() << ' ' << cnt << "\n";
        // assert(detail[HashVal].size() >= cnt);
        for (size_t i = 0; i < cnt; i++) {
            auto [lineNum, fileName] = getDebugInfo(detail[HashVal][i]);
            ss << "    - \"" << MD5decoding(HashVal) << "\" at " << fileName << ":" << lineNum
               << "\n";
        }
    }
    info = ss.str();
}

using SupportCount::CountSupport;

std::vector<MessageBoxP> check_positive(Module &M, ruleSet *PARs) {
    vector<MessageBoxP> res;
    for (Function &F : M) {
        if (F.empty()) {
            continue;
        }
        for (const rule_t &rule : PARs->getData()) {
            std::pair<int, SupportInfo_t> cond = CountSupport(F, rule.getCond(), true);
            std::pair<int, SupportInfo_t> full = CountSupport(F, rule.getFull());
            if (cond.first && !full.first) {
                // rule.display(outs());
                // outs() << "\n";
                res.emplace_back(&rule, cond.second);
            }
        }
    }
    return res;
}

std::vector<MessageBoxN> check_negative(Module &M, ruleSet *NARs) {
    vector<MessageBoxN> res;
    for (Function &F : M) {
        if (F.empty()) {
            continue;
        }
        for (const rule_t &rule : NARs->getData()) {
            std::pair<int, SupportInfo_t> full = CountSupport(F, rule.getFull(), true);
            if (full.first) {
                res.emplace_back(&rule, full.second);
            }
        }
    }
    return res;
}

}  // namespace bugfinder