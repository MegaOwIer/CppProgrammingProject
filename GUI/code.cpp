#include <QString>
#include <QTextStream>
#include <QTextBlock>
#include "code.h"
#include "ui_code.h"

Code::Code(QWidget *parent, std::vector<BugHighlight *> *list) :
    QWidget(parent),
    ui(new Ui::Code)
{
    ui->setupUi(this);

    bugList = list;
    connect(ui->listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onListItemChanged()));
}

Code::~Code()
{
    file.close();
    delete ui;
}

void Code::onListItemChanged() {
    int itemNum = ui->listWidget->currentRow();
    BugHighlight *bugs = (*bugList)[itemNum];

    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor(Qt::red));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = ui->plainTextEdit->textCursor();

    for (auto iter : bugs->getNum()) {
        selection.cursor.setPosition(0);
        selection.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, iter - 1);
        extraSelections.append(selection);
    }
    ui->plainTextEdit->setExtraSelections(extraSelections);
    int position = ui->plainTextEdit->document()->findBlockByNumber(*(bugs->getNum().begin()) - 1).position();
    selection.cursor.setPosition(position);
    ui->plainTextEdit->setTextCursor(selection.cursor);
}

void Code::addListItem() {
    for (auto iter : *bugList) {
        ui->listWidget->addItem(iter->getInfo());
    }
}

bool Code::openFile(const QString &fileName) {
    file.setFileName(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream textStream(&file);
        int line = 0;
        while (!textStream.atEnd()) {
            line++;
            QString tmpLine = QString("%1.  ").arg(line, 4);
            ui->plainTextEdit->appendPlainText(tmpLine + textStream.readLine());
        }
        return true;
    }
    else {
        return false;
    }
}
