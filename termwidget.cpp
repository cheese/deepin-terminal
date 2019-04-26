#include "termwidget.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopServices>
#include <QDebug>
#include <QMenu>

#include <DDesktopServices>

DWIDGET_USE_NAMESPACE

TermWidget::TermWidget(QWidget *parent)
    : QTermWidget(0, parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    setHistorySize(5000);

    // set shell program
    QString shell { getenv("SHELL") };
    setShellProgram(shell.isEmpty() ? "/bin/bash" : shell);
    setTerminalOpacity(0.75);
    setScrollBarPosition(QTermWidget::ScrollBarRight);

    // config
    qDebug() << availableColorSchemes();
    setColorScheme("Linux");

    setBlinkingCursor(true);

    connect(this, &QTermWidget::urlActivated, this, [](const QUrl & url, bool fromContextMenu){
        if (QApplication::keyboardModifiers() & Qt::ControlModifier || fromContextMenu) {
            QDesktopServices::openUrl(url);
        }
    });

    connect(this, &QWidget::customContextMenuRequested, this, &TermWidget::customContextMenuCall);

    startShellProgram();
}

void TermWidget::customContextMenuCall(const QPoint &pos)
{
    QMenu menu;

    QList<QAction*> termActions = filterActions(pos);
    for (QAction *& action : termActions) {
        menu.addAction(action);
    }

    if (!menu.isEmpty()) {
        menu.addSeparator();
    }

    // add other actions here.
    if (!selectedText().isEmpty()) {
        menu.addAction(QIcon::fromTheme("edit-copy"), tr("Copy &Selection"), this, [this] {
            copyClipboard();
        });
    }

    menu.addAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this, [this] {
        pasteClipboard();
    });

    menu.addAction(QIcon::fromTheme("document-open-folder"), tr("Open Working &Directory"), this, [this] {
        DDesktopServices::showFolder(QUrl::fromLocalFile(workingDirectory()));
    });

    menu.addSeparator();

    menu.addAction(QIcon::fromTheme("view-split-left-right"), tr("Split &Horizontally"), [this] {
        emit termRequestSplit(Qt::Horizontal);
    });

    menu.addAction(QIcon::fromTheme("view-split-top-bottom"), tr("Split &Vertically"), [this] {
        emit termRequestSplit(Qt::Vertical);
    });

    // display the menu.
    menu.exec(mapToGlobal(pos));
}

TermWidgetWrapper::TermWidgetWrapper(QWidget *parent)
    : QWidget(parent)
{
    initUI();

    connect(m_term, &QTermWidget::titleChanged, this, [this] { emit termTitleChanged(m_term->title()); });
    connect(m_term, &QTermWidget::finished, this, &TermWidgetWrapper::termClosed);
    // proxy signal:
    connect(m_term, &TermWidget::termRequestSplit, this, &TermWidgetWrapper::termRequestSplit);
}

void TermWidgetWrapper::initUI()
{
    m_term = new TermWidget(this);
    setFocusProxy(m_term);

    m_layout = new QVBoxLayout;
    setLayout(m_layout);

    m_layout->addWidget(m_term);
    m_layout->setContentsMargins(0, 0, 0, 0);
}