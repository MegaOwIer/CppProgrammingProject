#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QProcess>
#include <QString>
#include <set>
#include "settingdialog.h"
#include "bughighlight.h"
#include "code.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QString fileName;
    SettingDialog *dlgSettings = nullptr;
    QProcess *proc = nullptr;
    int mfs;
    int mis;
    int min_conf;
    int cstd;
    QString clangPath;
    QTextEdit *allOut, *bugsOut;
    std::vector<BugHighlight *> bugList;

private slots:
    void onBtnFileButtonClicked();
    void onBtnSettingButtonClicked();
    void onBtnStartButtonClicked();
    void onChkBoxBugsCheckBoxClicked(bool);
    void onBtnCodeButtonClicked();
    void procStart();
    void procFinished();
    void formatBugs();
};
#endif // MAINWINDOW_H
