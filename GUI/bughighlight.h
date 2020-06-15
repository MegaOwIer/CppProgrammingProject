#ifndef BUGHIGHLIGHT_H
#define BUGHIGHLIGHT_H

#include <QString>
#include <set>

class BugHighlight {
private:
    QString info;
    std::set<int> numSet;

public:
    ~BugHighlight();
    QString getInfo();
    std::set<int> &getNum();
    void setInfo(const QString &str);
    void addNum(int num);
};
#endif // BUGHIGHLIGHT_H
