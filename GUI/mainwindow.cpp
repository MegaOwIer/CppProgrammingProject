#include <QDir>
#include <QFileDialog>
#include <QTextBlock>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->fileButton, SIGNAL(clicked()), this, SLOT(onBtnFileButtonClicked()));
    connect(ui->settingButton, SIGNAL(clicked()), this, SLOT(onBtnSettingButtonClicked()));
    connect(ui->startButton, SIGNAL(clicked()), this, SLOT(onBtnStartButtonClicked()));
    connect(ui->bugsCheckBox, SIGNAL(clicked(bool)), this, SLOT(onChkBoxBugsCheckBoxClicked(bool)));
    mfs = 10, mis = 5, min_conf = 85, cstd = 99;
    clangPath = "/usr/bin/clang";
    fileName = "";
    allOut = new QTextEdit(), bugsOut = new QTextEdit();
    allOut->setReadOnly(true), bugsOut->setReadOnly(true);
    ui->scrollArea->setWidget(allOut);
    ui->scrollArea->setWidgetResizable(true);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete dlgSettings;
}

void MainWindow::onBtnFileButtonClicked() {
    QString curPath = QDir::currentPath();
    QString dlgTitle = "Choose a file";
    QString typeFilter = "C source file(*.c)";
    fileName = QFileDialog::getOpenFileName(this, dlgTitle, curPath, typeFilter);
    if (!fileName.isEmpty()) {
        ui->startButton->setEnabled(true);
        ui->fileLabel->setText(fileName);
    }
    else {
        ui->startButton->setEnabled(false);
        ui->fileLabel->setText("Please choose a file first.");
    }
}

void MainWindow::onBtnSettingButtonClicked() {
    if (dlgSettings == nullptr)
        dlgSettings = new SettingDialog(this);

    dlgSettings->setMFS(mfs);
    dlgSettings->setMIS(mis);
    dlgSettings->setMinConf(min_conf);
    dlgSettings->setClangPath(clangPath);
    dlgSettings->setCstd(cstd);
    int ret = dlgSettings->exec();

    if (ret == QDialog::Accepted) {
        mfs = dlgSettings->getMFS();
        mis = dlgSettings->getMIS();
        min_conf = dlgSettings->getMinConf();
        clangPath = dlgSettings->getClangPath();
        cstd = dlgSettings->getCstd();
    }
}

void MainWindow::onBtnStartButtonClicked() {
    ui->fileButton->setEnabled(false);
    ui->settingButton->setEnabled(false);
    ui->bugsCheckBox->setEnabled(false);
    allOut->clear(), bugsOut->clear();

    proc = new QProcess(this);
    connect(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(procStart()));
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(procFinished()));
    QString std;
    if (cstd == 11)
        std = "-std=c11";
    else
        std = "-std=c99";
    QStringList args;
    args << "-mllvm" << "-mfs=" + QString::number(mfs)
         << "-mllvm" << "-mis=" + QString::number(mis)
         << "-mllvm" << "-min_conf=" + QString::number(min_conf)
         << "-Xclang" << "-load" << "-Xclang" << QDir::currentPath() + "/MyPass.so"
         << "-g" << std << "-c" << fileName;
    proc->setProcessChannelMode(QProcess::SeparateChannels);
    proc->start(clangPath, args);
}

void MainWindow::onChkBoxBugsCheckBoxClicked(bool checked) {
    if (checked) {
        ui->scrollArea->takeWidget();
        ui->scrollArea->setWidget(bugsOut);
        ui->scrollArea->setWidgetResizable(true);
    }
    else {
        ui->scrollArea->takeWidget();
        ui->scrollArea->setWidget(allOut);
        ui->scrollArea->setWidgetResizable(true);
    }
}

void MainWindow::procStart() {
    allOut->setText("Analysis start.\n\n");
    QString result = QString::fromLocal8Bit(proc->readAllStandardOutput());
    allOut->append(result);
}

void MainWindow::procFinished() {
    allOut->append("\nAnalysis completed.\n");
    ui->fileButton->setEnabled(true);
    ui->settingButton->setEnabled(true);
    ui->bugsCheckBox->setEnabled(true);

    QTextDocument *doc = allOut->document();
    int blockCount = doc->blockCount();
    bool bug = false;
    for (int i = 0; i < blockCount; i++) {
        QTextBlock textBlock = doc->findBlockByNumber(i);
        QString text = textBlock.text();

        if (text == "++BUGs") {
            bug = true;
            continue;
        }
        if (text == "--BUGs") {
            bug = false;
            continue;
        }

        if (bug)
            bugsOut->append(text);
    }
}
