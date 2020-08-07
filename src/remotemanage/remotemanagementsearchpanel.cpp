#include "remotemanagementsearchpanel.h"
#include "utils.h"
#include "listview.h"
#include "iconbutton.h"
#include "mainwindow.h"

#include <DApplicationHelper>
#include <DGuiApplicationHelper>

#include <QKeyEvent>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>

RemoteManagementSearchPanel::RemoteManagementSearchPanel(QWidget *parent) : CommonPanel(parent)
{
    initUI();
}

void RemoteManagementSearchPanel::initUI()
{
    this->setBackgroundRole(QPalette::Base);
    this->setAutoFillBackground(true);

    m_rebackButton = new IconButton(this);
    m_rebackButton->setObjectName("RemoteSearchRebackButton");
    m_rebackButton->setIcon(DStyle::StandardPixmap::SP_ArrowLeave);
    m_rebackButton->setFixedSize(QSize(ICONSIZE_36, ICONSIZE_36));
    m_rebackButton->setFocusPolicy(Qt::TabFocus);

    m_listWidget = new ListView(ListType_Remote, this);
    m_label = new DLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    // 字体颜色随主题变化而变化
    DPalette palette = m_label->palette();
    QColor color;
    if (DApplicationHelper::instance()->themeType() == DApplicationHelper::DarkType) {
        color = QColor::fromRgb(192, 198, 212, 102);
    } else {
        color = QColor::fromRgb(85, 85, 85, 102);
    }
    palette.setBrush(QPalette::Text, color);
    m_label->setPalette(palette);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addSpacing(SPACEWIDTH);
    hlayout->addWidget(m_rebackButton);
    hlayout->addWidget(m_label, 0, Qt::AlignCenter);
    hlayout->setSpacing(0);
    hlayout->setMargin(0);

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addSpacing(SPACEHEIGHT);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(m_listWidget);
    vlayout->setMargin(0);
    vlayout->setSpacing(SPACEHEIGHT);
    setLayout(vlayout);

    // 返回键被点击 搜索界面，返回焦点返回搜索框
    connect(m_rebackButton, &DIconButton::clicked, this, &RemoteManagementSearchPanel::rebackPrevPanel);
    connect(m_rebackButton, &IconButton::preFocus, this, &RemoteManagementSearchPanel::rebackPrevPanel);
    connect(m_rebackButton, &IconButton::focusOut, this, &RemoteManagementSearchPanel::onFocusOutList);
    connect(m_listWidget, &ListView::itemClicked, this, &RemoteManagementSearchPanel::onItemClicked);
    connect(m_listWidget, &ListView::groupClicked, this, &RemoteManagementSearchPanel::showGroupPanel);
    connect(ServerConfigManager::instance(), &ServerConfigManager::refreshList, this, &RemoteManagementSearchPanel::onRefreshList);
    connect(m_listWidget, &ListView::focusOut, this, [ = ](Qt::FocusReason type) {
        if (Qt::TabFocusReason == type) {
            // tab 进入 +
            QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_Tab, Qt::MetaModifier);
            QApplication::sendEvent(Utils::getMainWindow(this), &keyPress);
            qDebug() << "search panel focus on '+'";
            m_listWidget->clearIndex();
        } else if (Qt::BacktabFocusReason == type || type == Qt::NoFocusReason) {
            // shift + tab 返回 返回键               // 列表为空，也返回到返回键上
            m_rebackButton->setFocus();
            m_listWidget->clearIndex();
            qDebug() << "search panel type" << type;
        }

    });
    // 字体颜色随主题变化变化
    connect(DApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, m_label, [ = ](DGuiApplicationHelper::ColorType themeType) {
        DPalette palette = m_label->palette();
        QColor color;
        if (themeType == DApplicationHelper::DarkType) {
            color = QColor::fromRgb(192, 198, 212, 102);
        } else {
            color = QColor::fromRgb(85, 85, 85, 102);
        }
        palette.setBrush(QPalette::Text, color);
        m_label->setPalette(palette);
    });
}

void RemoteManagementSearchPanel::refreshDataByGroupAndFilter(const QString &strGroup, const QString &strFilter)
{
    setSearchFilter(strFilter);
    m_isGroupOrNot = true;
    m_strGroupName = strGroup;
    m_strFilter = strFilter;
    m_listWidget->clearData();
    ServerConfigManager::instance()->refreshServerList(ServerConfigManager::PanelType_Search, m_listWidget, strFilter, strGroup);
}

void RemoteManagementSearchPanel::refreshDataByFilter(const QString &strFilter)
{
    setSearchFilter(strFilter);
    m_isGroupOrNot = false;
    m_strFilter = strFilter;
    m_listWidget->clearData();
    ServerConfigManager::instance()->refreshServerList(ServerConfigManager::PanelType_Search, m_listWidget, strFilter);
}

/*******************************************************************************
 1. @函数:    onItemClicked
 2. @作者:    ut000610 戴正文
 3. @日期:    2020-07-22
 4. @说明:    远程项被点击，连接远程
*******************************************************************************/
void RemoteManagementSearchPanel::onItemClicked(const QString &key)
{
    // 获取远程信息
    ServerConfig *remote = ServerConfigManager::instance()->getServerConfig(key);
    if (nullptr != remote) {
        emit doConnectServer(remote);
    } else {
        qDebug() << "can't connect to remote" << key;
    }
}

/*******************************************************************************
 1. @函数:    onFocusOutList
 2. @作者:    ut000610 戴正文
 3. @日期:    2020-08-04
 4. @说明:    焦点从列表中出的事件
*******************************************************************************/
void RemoteManagementSearchPanel::onFocusOutList(Qt::FocusReason type)
{
    // 焦点切出，没值的时候
    if (type == Qt::TabFocusReason && m_listWidget->count() == 0) {
        // tab 进入 +
        QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_Tab, Qt::MetaModifier);
        QApplication::sendEvent(Utils::getMainWindow(this), &keyPress);
        qDebug() << "search panel focus to '+'";
    }
}

/*******************************************************************************
 1. @函数:    onRefrushList
 2. @作者:    ut000610 戴正文
 3. @日期:    2020-08-04
 4. @说明:    处理刷新列表信号
*******************************************************************************/
void RemoteManagementSearchPanel::onRefreshList()
{
    // 判断是否显示
    if (m_isShow) {
        if (m_isGroupOrNot) {
            // 刷新分组下搜索列表
            refreshDataByGroupAndFilter(m_strGroupName, m_strFilter);
        } else {
            // 刷新搜索列表
            refreshDataByFilter(m_strFilter);
        }
    }
}


/*******************************************************************************
 1. @函数:    clearAllFocus
 2. @作者:    ut000610 戴正文
 3. @日期:    2020-07-29
 4. @说明:    清除界面所有的焦点
*******************************************************************************/
void RemoteManagementSearchPanel::clearAllFocus()
{
    m_rebackButton->clearFocus();
    m_listWidget->clearFocus();
    m_label->clearFocus();
}

/*******************************************************************************
 1. @函数:    setFocusBack
 2. @作者:    ut000610 戴正文
 3. @日期:    2020-08-04
 4. @说明:    设置焦点返回，从分组返回
*******************************************************************************/
void RemoteManagementSearchPanel::setFocusBack(const QString &strGroup, bool isFocusOn, int prevIndex)
{
    // 返回前判断之前是否要有焦点
    if (isFocusOn) {
        // 要有焦点
        // 找到分组的新位置
        int index = m_listWidget->indexFromString(strGroup, ItemFuncType_Group);
        if (index < 0) {
            // 小于0代表没找到 获取下一个
            index = m_listWidget->getNextIndex(prevIndex);
        }

        if (index >= 0) {
            // 找得到, 设置焦点
            m_listWidget->setCurrentIndex(index);
        } else {
            // 没找到焦点设置到添加按钮
            m_rebackButton->setFocus();
        }
    }
    // 不要焦点
    else {
        Utils::getMainWindow(this)->focusCurrentPage();
    }
}

/*******************************************************************************
 1. @函数:    getListIndex
 2. @作者:    ut000610 戴正文
 3. @日期:    2020-08-05
 4. @说明:    获取列表当前的焦点
*******************************************************************************/
int RemoteManagementSearchPanel::getListIndex()
{
    qDebug() << __FUNCTION__ << "current index : " << m_listWidget->currentIndex();
    return m_listWidget->currentIndex();
}


void RemoteManagementSearchPanel::setSearchFilter(const QString &filter)
{
    m_strFilter = filter;
    QString showText = filter;
    showText = Utils::getElidedText(m_label->font(), showText, ITEMMAXWIDTH, Qt::ElideMiddle);
    m_label->setText(QString("%1：%2").arg(tr("Search"), showText));
}
