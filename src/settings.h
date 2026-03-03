#pragma once
// DEPRECATED GTK settings backend removed — migrated to QSettings
// See contracts/settings-schema.md for key definitions

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QByteArray>

struct AppSettings {
    QString basePath;
    QString activeDocument;
    bool sidebarVisible = true;
    bool presentationMode = false;
    bool syntaxHighlighting = true;
    QByteArray windowGeometry;
    QByteArray windowState;
    int paletteCount = 1;
    QString paletteName = QStringLiteral("Default");
    QStringList paletteColors;
    int paletteActive = 0;
    int activePalette = 0;
};

AppSettings loadSettings();
void saveSettings(const AppSettings &s);
bool needsLegacyMigration();
void importLegacyJson(AppSettings &s);
