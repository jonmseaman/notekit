#include "about.h"
#include "ui_about.h"
#include "config.h"

CAboutDialog::CAboutDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CAboutDialog)
{
    ui->setupUi(this);
    ui->versionLabel->setText(QStringLiteral("Version: " NK_VERSION));
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

CAboutDialog::~CAboutDialog()
{
    delete ui;
}
