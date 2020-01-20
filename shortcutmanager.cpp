#include "shortcutmanager.h"
#include "termwidgetpage.h"
#include "mainwindow.h"
#include "settings.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>

// this class only provided for convenience, do not do anything in the construct function,
// let the caller decided when to create the shortcuts.
ShortcutManager *ShortcutManager::m_instance = nullptr;

ShortcutManager::ShortcutManager(QObject *parent) : QObject(parent)
{
    // Q_UNUSED(parent);
    // make sure it is NOT a nullptr since we'll use it all the time.
    // Q_CHECK_PTR(parent);
}

ShortcutManager *ShortcutManager::instance()
{
    if (nullptr == m_instance) {
        m_instance = new ShortcutManager();
    }
    return m_instance;
}

void ShortcutManager::initShortcuts()
{
    m_customCommandList = createCustomCommandsFromConfig();
    m_builtinShortcuts = createBuiltinShortcutsFromConfig();  // use QAction or QShortcut ?

    m_mainWindow->addActions(m_customCommandList);
    m_mainWindow->addActions(m_builtinShortcuts);
}

QList<QAction *> ShortcutManager::createCustomCommandsFromConfig()
{
    QList<QAction *> actionList;

    QDir customCommandBasePath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!customCommandBasePath.exists()) {
        return actionList;
    }

    QString customCommandConfigFilePath(customCommandBasePath.filePath("command-config.conf"));
    if (!QFile::exists(customCommandConfigFilePath)) {
        return actionList;
    }

    QSettings commandsSettings(customCommandConfigFilePath, QSettings::IniFormat);
    QStringList commandGroups = commandsSettings.childGroups();
    // qDebug() << commandGroups.size() << endl;
    for (const QString &commandName : commandGroups) {
        commandsSettings.beginGroup(commandName);
        if (!commandsSettings.contains("Command"))
            continue;
        QAction *action = new QAction(commandName, this);
        action->setData(commandsSettings.value("Command").toString());  // make sure it is a QString
        if (commandsSettings.contains("Shortcut")) {
            QVariant shortcutVariant = commandsSettings.value("Shortcut");
            if (shortcutVariant.type() == QVariant::KeySequence) {
                action->setShortcut(shortcutVariant.convert(QMetaType::QKeySequence));
            } else if (shortcutVariant.type() == QVariant::String) {
                // to make it compatible to deepin-terminal config file.
                QString shortcutStr = shortcutVariant.toString().remove(QChar(' '));
                action->setShortcut(QKeySequence(shortcutStr));
            }
        }
        connect(action, &QAction::triggered, m_mainWindow, [this, action]() {
            QString command = action->data().toString();
            if (!command.endsWith('\n')) {
                command.append('\n');
            }
            m_mainWindow->currentTab()->sendTextToCurrentTerm(command);
        });
        commandsSettings.endGroup();
        actionList.append(action);
    }

    return actionList;
}

QList<QAction *> ShortcutManager::createBuiltinShortcutsFromConfig()
{
    QList<QAction *> actionList;

    // TODO.

    //    QAction *action = nullptr;

    //    action = new QAction("__builtin_focus_nav_up", this);
    //    // blumia: in Qt 5.7.1 (from Debian stretch) if we pass something like "Ctrl+Shift+Q" to QKeySequence then it
    //    won't works.
    //    //         in Qt 5.11.3 (from Debian buster) it works fine.
    //    action->setShortcut(QKeySequence("Alt+k"));

    //    connect(action, &QAction::triggered, m_mainWindow, [this](){
    //        TermWidgetPage *page = m_mainWindow->currentTab();
    //        if (page) page->focusNavigation(Up);
    //    });
    //    actionList.append(action);

    return actionList;
}

QList<QAction *> &ShortcutManager::getCustomCommands()
{
    qDebug() << m_customCommandList.size() << endl;
    return m_customCommandList;
}

void ShortcutManager::setMainWindow(MainWindow *curMainWindow)
{
    m_mainWindow = curMainWindow;
}

QAction *ShortcutManager::addCustomCommand(QAction &action)
{

    QAction *addAction = new QAction(action.text(), this);
    addAction->setData(action.data());
    addAction->setShortcut(action.shortcut());
    m_customCommandList.append(addAction);
    connect(addAction, &QAction::triggered, m_mainWindow, [this, addAction]() {
        QString command = addAction->data().toString();
        if (!command.endsWith('\n')) {
            command.append('\n');
        }
        m_mainWindow->currentTab()->sendTextToCurrentTerm(command);
    });
    saveCustomCommandToConfig(addAction);
    return addAction;
}

QAction *ShortcutManager::checkActionIsExist(QAction &action)
{
    QString strNewName = action.text();
    for (int i = 0; i < m_customCommandList.size(); i++) {
        QAction *curAct = m_customCommandList[i];
        if (strNewName == curAct->text()) {
            return curAct;
        }
    }
    return nullptr;
}

void ShortcutManager::delCustomCommand(QAction *action)
{
    delCUstomCommandToConfig(action);
    m_customCommandList.removeOne(action);
    delete action;
    action = nullptr;
}

void ShortcutManager::saveCustomCommandToConfig(QAction *action)
{
    QDir customCommandBasePath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!customCommandBasePath.exists()) {
        customCommandBasePath.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }

    QString customCommandConfigFilePath(customCommandBasePath.filePath("command-config.conf"));
    QSettings commandsSettings(customCommandConfigFilePath, QSettings::IniFormat);
    commandsSettings.beginGroup(action->text());
    commandsSettings.setValue("Command", action->data());
    QString tmp = action->shortcut().toString();
    commandsSettings.setValue("Shortcut", action->shortcut().toString());
    commandsSettings.endGroup();
}

void ShortcutManager::delCUstomCommandToConfig(QAction *action)
{
    QDir customCommandBasePath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!customCommandBasePath.exists()) {
        customCommandBasePath.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }

    QString customCommandConfigFilePath(customCommandBasePath.filePath("command-config.conf"));
    QSettings commandsSettings(customCommandConfigFilePath, QSettings::IniFormat);
    commandsSettings.remove(action->text());
}