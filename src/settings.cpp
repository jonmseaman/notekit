#include "settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <sstream>
#include <json/json.h>

static const QStringList DEFAULT_COLORS = {
    QStringLiteral("#000000"),
    QStringLiteral("#555555"),
    QStringLiteral("#888888"),
    QStringLiteral("#AAAAAA"),
    QStringLiteral("#E53935"),
    QStringLiteral("#43A047"),
    QStringLiteral("#1E88E5"),
    QStringLiteral("#FB8C00")
};

AppSettings loadSettings()
{
    QSettings settings;
    AppSettings s;

    QString defaultBase = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                          + QStringLiteral("/Notes");

    s.basePath         = settings.value(QStringLiteral("base-path"), defaultBase).toString();
    s.activeDocument   = settings.value(QStringLiteral("active-document"), QString()).toString();
    s.sidebarVisible   = settings.value(QStringLiteral("sidebar/visible"), true).toBool();
    s.presentationMode = settings.value(QStringLiteral("presentation-mode"), false).toBool();
    s.syntaxHighlighting = settings.value(QStringLiteral("syntax-highlighting"), true).toBool();
    s.windowGeometry   = settings.value(QStringLiteral("window/geometry")).toByteArray();
    s.windowState      = settings.value(QStringLiteral("window/state")).toByteArray();
    s.paletteCount     = settings.value(QStringLiteral("palette/count"), 1).toInt();
    s.paletteName      = settings.value(QStringLiteral("palette/0/name"),
                                        QStringLiteral("Default")).toString();
    s.paletteColors    = settings.value(QStringLiteral("palette/0/colors"),
                                        DEFAULT_COLORS).toStringList();
    if (s.paletteColors.isEmpty())
        s.paletteColors = DEFAULT_COLORS;
    s.paletteActive    = settings.value(QStringLiteral("palette/0/active"), 0).toInt();
    s.activePalette    = settings.value(QStringLiteral("active-palette"), 0).toInt();

    return s;
}

void saveSettings(const AppSettings &s)
{
    QSettings settings;
    settings.setValue(QStringLiteral("base-path"),         s.basePath);
    settings.setValue(QStringLiteral("active-document"),   s.activeDocument);
    settings.setValue(QStringLiteral("sidebar/visible"),   s.sidebarVisible);
    settings.setValue(QStringLiteral("presentation-mode"), s.presentationMode);
    settings.setValue(QStringLiteral("syntax-highlighting"), s.syntaxHighlighting);
    settings.setValue(QStringLiteral("window/geometry"),   s.windowGeometry);
    settings.setValue(QStringLiteral("window/state"),      s.windowState);
    settings.setValue(QStringLiteral("palette/count"),     s.paletteCount);
    settings.setValue(QStringLiteral("palette/0/name"),    s.paletteName);
    settings.setValue(QStringLiteral("palette/0/colors"),  s.paletteColors);
    settings.setValue(QStringLiteral("palette/0/active"),  s.paletteActive);
    settings.setValue(QStringLiteral("active-palette"),    s.activePalette);
}

static QString legacyConfigPath()
{
    // Platform-specific: Qt does not abstract per-OS XDG/AppData/Library paths
    // because the legacy GTK3 config.json location was OS-specific before QSettings.
    // This block is the only justified platform #if in the codebase (T038 audit).
#if defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/../notekit/config.json");
#elif defined(Q_OS_MACOS)
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + QStringLiteral("/Library/Application Support/notekit/config.json");
#else
    QString configHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (configHome.isEmpty())
        configHome = QDir::homePath() + QStringLiteral("/.config");
    return configHome + QStringLiteral("/notekit/config.json");
#endif
}

bool needsLegacyMigration()
{
    QSettings settings;
    if (settings.value(QStringLiteral("migration/legacy-json-imported"), false).toBool())
        return false;
    return QFile::exists(legacyConfigPath());
}

void importLegacyJson(AppSettings &s)
{
    QString path = legacyConfigPath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "importLegacyJson: cannot open" << path;
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream is(data.toStdString());
    if (!Json::parseFromStream(builder, is, &root, &errs)) {
        qDebug() << "importLegacyJson: parse error:" << errs.c_str();
        return;
    }

    // Map legacy JSON keys → QSettings keys (see contracts/settings-schema.md §Migration)
    if (root.isMember("base-path") && root["base-path"].isString())
        s.basePath = QString::fromStdString(root["base-path"].asString());

    if (root.isMember("active-document") && root["active-document"].isString())
        s.activeDocument = QString::fromStdString(root["active-document"].asString());

    if (root.isMember("sidebar") && root["sidebar"].isBool())
        s.sidebarVisible = root["sidebar"].asBool();

    if (root.isMember("presentation-mode") && root["presentation-mode"].isBool())
        s.presentationMode = root["presentation-mode"].asBool();

    if (root.isMember("syntax-highlighting") && root["syntax-highlighting"].isBool())
        s.syntaxHighlighting = root["syntax-highlighting"].asBool();

    if (root.isMember("colors") && root["colors"].isArray()) {
        s.paletteColors.clear();
        for (const auto &c : root["colors"]) {
            if (c.isString())
                s.paletteColors.append(QString::fromStdString(c.asString()));
        }
    }

    // Keys dropped (not applicable to Qt): csd, gschemas

    // Mark migration complete — legacy JSON file is NOT modified
    QSettings settings;
    settings.setValue(QStringLiteral("migration/legacy-json-imported"), true);

    qDebug() << "importLegacyJson: imported base-path=" << s.basePath
             << "palette colors=" << s.paletteColors.size();
}
