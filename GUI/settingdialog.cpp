#include <QDir>
#include <QFileDialog>
#include "settingdialog.h"
#include "ui_settingdialog.h"

SettingDialog::SettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog)
{
    ui->setupUi(this);
    connect(ui->clangPathButton, SIGNAL(clicked()), this, SLOT(onBtnClangPathButtonClicked()));
}

SettingDialog::~SettingDialog()
{
    delete ui;
}

int SettingDialog::getMFS() {
    return ui->mfsSpinBox->value();
}

int SettingDialog::getMIS() {
    return ui->misSpinBox->value();
}

int SettingDialog::getMinConf() {
    return ui->minconfSpinBox->value();
}

QString SettingDialog::getClangPath() {
    return ui->pathLineEdit->text();
}

int SettingDialog::getCstd() {
    if (ui->c11RadioButton->isChecked())
        return 11;
    else
        return 99;
}

void SettingDialog::setMFS(int mfs) {
    ui->mfsSpinBox->setValue(mfs);
}

void SettingDialog::setMIS(int mis) {
    ui->misSpinBox->setValue(mis);
}

void SettingDialog::setMinConf(int min_conf) {
    ui->minconfSpinBox->setValue(min_conf);
}

void SettingDialog::setClangPath(QString &clangPath) {
    ui->pathLineEdit->setText(clangPath);
}

void SettingDialog::setCstd(int cstd) {
    if (cstd == 11)
        ui->c11RadioButton->setChecked(true);
    else
        ui->c99RadioButton->setChecked(true);
}

void SettingDialog::onBtnClangPathButtonClicked() {
    QString startPath = "/usr/bin";
    QString dlgTitle = "Choose clang path";
    QString fileName = QFileDialog::getOpenFileName(this, dlgTitle, startPath);
    if (!fileName.isEmpty()) {
        ui->pathLineEdit->setText(fileName);
    }
}
