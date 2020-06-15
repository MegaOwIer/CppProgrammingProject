#ifndef CODE_H
#define CODE_H

#include <QWidget>
#include <QFile>
#include <vector>
#include "bughighlight.h"

namespace Ui {
class Code;
}

class Code : public QWidget
{
    Q_OBJECT

public:
    explicit Code(QWidget *parent = nullptr, std::vector<BugHighlight *> *list = nullptr);
    ~Code();
    void addListItem();
    bool openFile(const QString &fileName);

private:
    Ui::Code *ui;
    QFile file;
    std::vector<BugHighlight *> *bugList;

private slots:
    void onListItemChanged();
};

#endif // CODE_H
