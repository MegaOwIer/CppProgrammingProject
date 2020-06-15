#include "bughighlight.h"

BugHighlight::~BugHighlight() {
    numSet.clear();
}

QString BugHighlight::getInfo() {
    return info;
}

std::set<int> &BugHighlight::getNum() {
    return numSet;
}

void BugHighlight::setInfo(const QString &str) {
    info = str;
}

void BugHighlight::addNum(int num) {
    numSet.insert(num);
}
