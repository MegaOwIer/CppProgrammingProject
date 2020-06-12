#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class SettingDialog;
}

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingDialog(QWidget *parent = nullptr);
    ~SettingDialog();
    int getMFS();
    int getMIS();
    int getMinConf();
    QString getClangPath();
    int getCstd();
    void setMFS(int);
    void setMIS(int);
    void setMinConf(int);
    void setClangPath(QString &);
    void setCstd(int);

private:
    Ui::SettingDialog *ui;

private slots:
    void onBtnClangPathButtonClicked();
};

#endif // SETTINGDIALOG_H
