#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QProcess>
#include <QString>
#include "settingdialog.h"

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

private slots:
    void onBtnFileButtonClicked();
    void onBtnSettingButtonClicked();
    void onBtnStartButtonClicked();
    void onChkBoxBugsCheckBoxClicked(bool);
    void procStart();
    void procFinished();
};
#endif // MAINWINDOW_H
