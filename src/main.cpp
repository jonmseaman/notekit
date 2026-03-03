#include "mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("NoteKit"));
    app.setOrganizationDomain(QStringLiteral("github.com/blackhole89/notekit"));
    app.setApplicationName(QStringLiteral("NoteKit"));
    app.setApplicationVersion(QStringLiteral("0.3.0"));

    // Load stylesheet from Qt resource system
    QFile stylesheet(QStringLiteral(":/notekit/stylesheet.qss"));
    if (stylesheet.open(QIODevice::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(stylesheet.readAll()));

    CMainWindow w;
    w.show();

    return app.exec();
}
