/* ============================================================
   MainWindow.cpp —— 主窗口实现
   ============================================================ */

#include "MainWindow.h"
#include "FormulaDetailWidget.h"
#include "LoginWidget.h"
#include "ProfileDialog.h"
#include "api/FormulaApi.h"
#include "api/LoginApi.h"
#include "api/HttpClient.h"
#include "models/FormulaModel.h"
#include "utils/Auth.h"
#include "utils/SearchHistory.h"
#include "utils/SearchGather.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QScrollBar>
#include <QTimer>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMenu>
#include <QEvent>
#include <functional>

// 点击事件过滤器
namespace {
class ClickFilter : public QObject {
public:
    using Callback = std::function<void()>;
    ClickFilter(Callback cb, QObject *parent = nullptr)
        : QObject(parent), m_cb(std::move(cb)) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            m_cb();
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
private:
    Callback m_cb;
};
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("恺祁颜色系统"));
    setupUi();
    renderAccount();
    renderHistory();

    // 监听 token 失效
    connect(&HttpClient::instance(), &HttpClient::tokenExpired, this, [this]() {
        auto *login = new LoginWidget;
        login->resize(480, 700);
        login->show();
        this->close();
    });
}

void MainWindow::setupUi()
{
    auto *central = new QWidget;
    setCentralWidget(central);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ===== 顶部导航栏 =====
    auto *topBar = new QWidget;
    topBar->setObjectName(QStringLiteral("topBar"));
    topBar->setFixedHeight(52);
    setupTopBar(topBar);
    mainLayout->addWidget(topBar);

    // ===== 下方区域：左侧导航栏 + 内容区 =====
    auto *bodyWidget = new QWidget;
    auto *bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // 左侧导航栏
    auto *navBar = new QWidget;
    navBar->setObjectName(QStringLiteral("navBar"));
    navBar->setFixedWidth(72);
    setupNavBar(navBar);
    bodyLayout->addWidget(navBar);

    // 内容区（QStackedWidget）
    m_stack = new QStackedWidget;

    // 页面 0：搜索页（当前主页面内容）
    auto *searchPage = new QWidget;
    searchPage->setObjectName(QStringLiteral("searchPage"));
    setupSearchPage(searchPage);
    m_stack->addWidget(searchPage);

    // 页面 1：自动 S&C（占位）
    auto *autoPage = new QWidget;
    setupPlaceholderPage(autoPage, QStringLiteral("自动 S&C"), QStringLiteral("⚙"));
    m_stack->addWidget(autoPage);

    // 页面 2：设置（占位）
    auto *settingsPage = new QWidget;
    setupPlaceholderPage(settingsPage, QStringLiteral("设置"), QStringLiteral("☰"));
    m_stack->addWidget(settingsPage);

    bodyLayout->addWidget(m_stack, 1);
    mainLayout->addWidget(bodyWidget);

    // 默认选中搜索页
    updateNavSelection(0);
}

void MainWindow::setupTopBar(QWidget *topBar)
{
    auto *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(20, 0, 20, 0);

    auto *appName = new QLabel(QStringLiteral("恺祁颜色系统"));
    appName->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 18px; font-weight: 700; letter-spacing: 2px;"));
    layout->addWidget(appName);
    layout->addStretch();

    // 用户区域（头像 + 名称 + 下拉箭头，点击弹出菜单）
    auto *userArea = new QWidget;
    userArea->setCursor(Qt::PointingHandCursor);
    userArea->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *userAreaLayout = new QHBoxLayout(userArea);
    userAreaLayout->setContentsMargins(0, 0, 0, 0);
    userAreaLayout->setSpacing(6);

    m_accountAvatarLabel = new QLabel(QStringLiteral("用"));
    m_accountAvatarLabel->setAlignment(Qt::AlignCenter);
    m_accountAvatarLabel->setFixedSize(30, 30);
    m_accountAvatarLabel->setStyleSheet(QStringLiteral(
        "background: rgba(255,255,255,0.25); color: #ffffff; font-size: 13px; font-weight: bold; "
        "border-radius: 15px;"));
    userAreaLayout->addWidget(m_accountAvatarLabel);

    m_accountNameLabel = new QLabel(QStringLiteral("用户"));
    m_accountNameLabel->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 14px; font-weight: 500;"));
    userAreaLayout->addWidget(m_accountNameLabel);

    auto *arrowLabel = new QLabel(QStringLiteral("▾"));
    arrowLabel->setStyleSheet(QStringLiteral("color: rgba(255,255,255,0.8); font-size: 12px;"));
    userAreaLayout->addWidget(arrowLabel);

    // 下拉菜单
    auto *userMenu = new QMenu(userArea);
    userMenu->setStyleSheet(QStringLiteral(
        "QMenu { background: #ffffff; border: 1px solid #e0e3e8; border-radius: 8px; padding: 4px 0; }"
        "QMenu::item { padding: 8px 24px; font-size: 13px; color: #1a2332; }"
        "QMenu::item:selected { background: #f0f2f5; color: #0d4f8b; }"));
    userMenu->addAction(QStringLiteral("个人资料"), this, [this]() {
        auto *dlg = new ProfileDialog(this);
        dlg->exec();
    });
    userMenu->addSeparator();
    userMenu->addAction(QStringLiteral("退出登录"), this, &MainWindow::handleLogout);

    // 点击用户区域弹出菜单（通过 mouseReleaseEvent）
    auto showMenu = [userArea, userMenu]() {
        userMenu->popup(userArea->mapToGlobal(QPoint(0, userArea->height())));
    };
    userArea->setProperty("clickable", true);
    connect(userArea, &QWidget::customContextMenuRequested, userArea, showMenu);
    userArea->setContextMenuPolicy(Qt::NoContextMenu);
    // 用 mouseRelease 实现点击
    auto *clickFilter = new QObject;
    userArea->installEventFilter(new ClickFilter(showMenu, userArea));

    layout->addWidget(userArea);
}

void MainWindow::setupNavBar(QWidget *navBar)
{
    auto *layout = new QVBoxLayout(navBar);
    layout->setContentsMargins(0, 12, 0, 12);
    layout->setSpacing(4);

    struct NavItem {
        QString icon;
        QString text;
    };
    QVector<NavItem> items = {
        { QStringLiteral("\U0001F50D"), QStringLiteral("搜索") },
        { QStringLiteral("\U0001F52C"), QStringLiteral("自动S&C") },
        { QStringLiteral("\u2699"),     QStringLiteral("设置") },
    };

    for (int i = 0; i < items.size(); ++i) {
        auto *btn = new QPushButton(items[i].icon + QStringLiteral("\n") + items[i].text);
        btn->setObjectName(QStringLiteral("navBtn"));
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(64);
        btn->setProperty("index", i);
        connect(btn, &QPushButton::clicked, this, [this, i]() { switchPage(i); });
        layout->addWidget(btn);
        m_navButtons.append(btn);
    }
    layout->addStretch(1);
}

void MainWindow::setupSearchPage(QWidget *page)
{
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 左侧搜索面板
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(8);
    splitter->setStyleSheet(QStringLiteral(
        "QSplitter::handle { background: #f0f2f5; }"));

    auto *sidebar = new QFrame;
    sidebar->setObjectName(QStringLiteral("sidebarFrame"));
    sidebar->setFrameShape(QFrame::NoFrame);
    sidebar->setStyleSheet(QStringLiteral(
        "#sidebarFrame { background: #ffffff; border: 2px solid #c0c4cc; border-radius: 8px; }"
        "QScrollBar:vertical { background: #f0f2f5; width: 10px; margin: 0; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: #606266; min-height: 30px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: #4a4c50; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"));
    sidebar->setMinimumWidth(340);
    sidebar->setMaximumWidth(480);
    setupSidebar(sidebar);
    splitter->addWidget(sidebar);

    // 右侧详情
    m_detailWidget = new FormulaDetailWidget;
    splitter->addWidget(m_detailWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    layout->addWidget(splitter);
}

void MainWindow::setupPlaceholderPage(QWidget *page, const QString &title, const QString &icon)
{
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    auto *iconLabel = new QLabel(icon);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet(QStringLiteral(
        "font-size: 64px; color: #c0c4cc;"));
    layout->addWidget(iconLabel);

    auto *titleLabel = new QLabel(title);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: 600; color: #909399; margin-top: 16px;"));
    layout->addWidget(titleLabel);

    auto *hintLabel = new QLabel(QStringLiteral("功能开发中…"));
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet(QStringLiteral(
        "font-size: 14px; color: #c0c4cc; margin-top: 8px;"));
    layout->addWidget(hintLabel);
}

void MainWindow::switchPage(int index)
{
    if (m_stack) {
        m_stack->setCurrentIndex(index);
        updateNavSelection(index);
    }
}

void MainWindow::updateNavSelection(int index)
{
    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(i == index);
    }
}

void MainWindow::setupSidebar(QWidget *sidebar)
{
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(12, 12, 12, 0);
    layout->setSpacing(8);

    // 搜索行
    auto *searchRow = new QWidget;
    auto *searchLayout = new QHBoxLayout(searchRow);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(8);

    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText(QStringLiteral("搜索颜色名称、色号(支持多色号)、车型"));
    m_searchInput->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 20px; "
        "padding: 8px 14px; font-size: 14px; min-height: 20px; color: #1a2332; }"
        "QLineEdit:focus { border-color: #0d4f8b; background: #ffffff; }"));
    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::doSearch);
    connect(m_searchInput, &QLineEdit::textChanged, this, [this](const QString &) { onSearchInput(); });
    searchLayout->addWidget(m_searchInput);

    m_searchBtn = new QPushButton(QStringLiteral("搜索"));
    m_searchBtn->setCursor(Qt::PointingHandCursor);
    m_searchBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #0d4f8b; color: #ffffff; font-size: 14px; font-weight: 600; "
        "border: none; border-radius: 20px; padding: 8px 20px; min-width: 60px; }"
        "QPushButton:hover { background: #0a3f70; }"
        "QPushButton:pressed { background: #082e52; }"));
    connect(m_searchBtn, &QPushButton::clicked, this, &MainWindow::doSearch);
    searchLayout->addWidget(m_searchBtn);

    layout->addWidget(searchRow);

    // 联想面板
    m_suggestPanel = new QWidget;
    m_suggestPanel->setObjectName(QStringLiteral("suggestPanel"));
    m_suggestPanel->hide();
    auto *suggestLayout = new QVBoxLayout(m_suggestPanel);
    suggestLayout->setContentsMargins(0, 0, 0, 0);
    m_suggestList = new QListWidget;
    m_suggestList->setObjectName(QStringLiteral("suggestList"));
    m_suggestList->setMaximumHeight(240);
    m_suggestList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_suggestList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        QString text = item->data(Qt::UserRole).toString();
        if (text.isEmpty()) return;
        m_searchInput->setText(SearchGather::mergeSuggestion(m_searchInput->text(), text));
        m_suggestPanel->hide();
        doSearch();
    });
    suggestLayout->addWidget(m_suggestList);
    layout->addWidget(m_suggestPanel);

    // 搜索历史
    m_historyBar = new QWidget;
    auto *histLayout = new QHBoxLayout(m_historyBar);
    histLayout->setContentsMargins(0, 0, 0, 0);
    histLayout->setSpacing(6);
    m_historyTags = new QWidget;
    histLayout->addWidget(new QLabel(QStringLiteral("历史：")));
    histLayout->addWidget(m_historyTags);
    histLayout->addStretch();
    m_historyBar->hide();
    layout->addWidget(m_historyBar);

    // 结果列表
    m_resultList = new QListWidget;
    m_resultList->setStyleSheet(QStringLiteral(
        "QListWidget { border: none; background: transparent; }"
        "QListWidget::item { background: transparent; }"
        "QListWidget::item:selected { background: #f0f2f5; }"
        "QListWidget::item:hover { background: #f5f7fa; }"
        "QScrollBar:vertical { background: #f0f2f5; width: 10px; margin: 0; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: #606266; min-height: 30px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: #4a4c50; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"));
    m_resultList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_resultList, &QListWidget::itemClicked, this, &MainWindow::onResultClicked);
    // 滚动到底部加载下一页
    connect(m_resultList->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        auto *sb = m_resultList->verticalScrollBar();
        if (sb->maximum() - value < 60) {
            fetchList(true);
        }
    });
    layout->addWidget(m_resultList, 1);

    // 底部状态
    m_footerLabel = new QLabel(QStringLiteral("输入关键词开始搜索"));
    m_footerLabel->setObjectName(QStringLiteral("listFooter"));
    m_footerLabel->setAlignment(Qt::AlignCenter);
    m_footerLabel->setFixedHeight(32);
    layout->addWidget(m_footerLabel);

    // 防抖定时器
    m_suggestTimer = new QTimer(this);
    m_suggestTimer->setSingleShot(true);
    connect(m_suggestTimer, &QTimer::timeout, this, [this]() {
        QString token = SearchGather::getSuggestToken(m_searchInput->text());
        if (!token.isEmpty())
            fetchSuggestions(token);
    });
}

void MainWindow::renderAccount()
{
    auto user = Auth::instance().getUser();
    QString nick = user.value(QStringLiteral("nickName")).toString(
                   user.value(QStringLiteral("userName")).toString());
    if (nick.isEmpty()) nick = QStringLiteral("用户");
    m_accountNameLabel->setText(nick);
    m_accountAvatarLabel->setText(nick.left(1).toUpper());
}

void MainWindow::renderHistory()
{
    auto list = SearchHistory::instance().list();
    if (list.isEmpty()) {
        m_historyBar->hide();
        return;
    }
    m_historyBar->show();
    // 清除旧标签
    qDeleteAll(m_historyTags->findChildren<QPushButton *>());
    auto *layout = new QHBoxLayout(m_historyTags);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    for (const auto &h : list) {
        auto *btn = new QPushButton(h);
        btn->setProperty("class", QStringLiteral("history-tag"));
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, h]() {
            m_searchInput->setText(h);
            doSearch();
        });
        layout->addWidget(btn);
    }
}

void MainWindow::doSearch()
{
    m_suggestPanel->hide();
    QString k = m_searchInput->text().trimmed();
    if (k.isEmpty()) {
        showToast(QStringLiteral("请输入搜索关键词"));
        return;
    }
    SearchHistory::instance().push(k);
    m_keyword = k;
    m_searchKeyword = k;
    m_page = 0;
    m_total = 0;
    m_noMore = false;
    m_selectedId = 0;
    renderHistory();
    fetchList(false);
}

void MainWindow::onSearchInput()
{
    m_keyword = m_searchInput->text();

    QString token = SearchGather::getSuggestToken(m_keyword);
    if (token.isEmpty()) {
        m_suggestPanel->hide();
        return;
    }
    m_suggestTimer->start(SUGGEST_DEBOUNCE_MS);
}

void MainWindow::fetchSuggestions(const QString &q)
{
    FormulaApi::instance().suggest(q, 10,
        [this](const QJsonObject &res) {
            m_suggestList->clear();
            QJsonArray arr;
            if (res.contains(QStringLiteral("data")))
                arr = res.value(QStringLiteral("data")).toArray();
            else if (res.contains(QStringLiteral("rows")))
                arr = res.value(QStringLiteral("rows")).toArray();

            if (arr.isEmpty()) {
                m_suggestPanel->hide();
                return;
            }
            m_suggestPanel->show();
            for (const auto &v : arr) {
                QString text;
                QString subText;
                QString type;
                if (v.isString()) {
                    text = v.toString();
                } else {
                    auto obj = v.toObject();
                    text = obj.value(QStringLiteral("text")).toString(
                           obj.value(QStringLiteral("value")).toString());
                    subText = obj.value(QStringLiteral("subText")).toString();
                    type = obj.value(QStringLiteral("type")).toString();
                }
                if (text.isEmpty()) continue;
                auto *item = new QListWidgetItem(text + (subText.isEmpty() ? "" : "  " + subText));
                item->setData(Qt::UserRole, text);
                m_suggestList->addItem(item);
            }
        },
        [this](const QString &, int) {
            m_suggestPanel->hide();
        }
    );
}

void MainWindow::fetchList(bool append)
{
    if (m_loading) return;
    if (append && m_noMore) {
        setFooter(QStringLiteral("— 没有更多了 —"));
        return;
    }

    if (!append) {
        m_resultList->clear();
        m_detailWidget->clear();
        setFooter(QStringLiteral("加载中…"));
    } else {
        setFooter(QStringLiteral("加载中…"));
    }
    m_loading = true;

    int nextPage = append ? m_page + 1 : 1;
    FormulaApi::instance().searchFormula(m_searchKeyword, nextPage, PAGE_SIZE,
        [this, append, nextPage](const QJsonObject &res) {
            auto rows = FormulaModel::pickFormulaRows(res);
            m_page = nextPage;
            m_total = res.value(QStringLiteral("total")).toInt();

            if (!append) m_resultList->clear();

            for (const auto &v : rows) {
                auto row = FormulaModel::normalizeFormulaRow(v.toObject());
                auto *item = new QListWidgetItem;
                // 构建卡片 Widget
                auto *card = new QWidget;
                auto *cardLayout = new QHBoxLayout(card);
                cardLayout->setContentsMargins(8, 8, 8, 8);
                cardLayout->setSpacing(10);

                // 缩略图占位
                auto *thumbLabel = new QLabel;
                thumbLabel->setFixedSize(48, 48);
                thumbLabel->setAlignment(Qt::AlignCenter);
                thumbLabel->setStyleSheet(QStringLiteral(
                    "background: #f0f2f5; border-radius: 8px; font-size: 18px; color: #0d4f8b;"));
                if (!row.cover.isEmpty()) {
                    // 异步加载图片
                    auto *nam = new QNetworkAccessManager(thumbLabel);
                    QNetworkRequest req(QUrl(row.cover));
                    connect(nam, &QNetworkAccessManager::finished, thumbLabel, [thumbLabel, nam](QNetworkReply *reply) {
                        if (reply->error() == QNetworkReply::NoError) {
                            QPixmap pix;
                            pix.loadFromData(reply->readAll());
                            if (!pix.isNull()) {
                                thumbLabel->setPixmap(pix.scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                                thumbLabel->setStyleSheet(QStringLiteral("border-radius: 8px;"));
                            }
                        }
                        nam->deleteLater();
                    });
                    nam->get(req);
                } else {
                    QString ch = row.colorCode.isEmpty() ? QStringLiteral("?") : row.colorCode.left(1).toUpper();
                    thumbLabel->setText(ch);
                }
                cardLayout->addWidget(thumbLabel);

                // 文字信息
                auto *textWidget = new QWidget;
                auto *textLayout = new QVBoxLayout(textWidget);
                textLayout->setContentsMargins(0, 0, 0, 0);
                textLayout->setSpacing(2);

                if (!row.colorCode.isEmpty()) {
                    auto *sampleLabel = new QLabel(
                        QStringLiteral("<span style='color:#909399;font-size:11px;'>色号：</span>"
                                       "<span style='color:#1a2332;font-size:13px;font-weight:600;'>%1</span>")
                            .arg(row.colorCode.toHtmlEscaped()));
                    sampleLabel->setTextFormat(Qt::RichText);
                    textLayout->addWidget(sampleLabel);
                }
                auto *nameLabel = new QLabel(
                    QStringLiteral("<span style='color:#909399;font-size:11px;'>名称：</span>"
                                   "<span style='color:#1a2332;font-size:13px;'>%1</span>")
                        .arg(row.displayName().toHtmlEscaped()));
                nameLabel->setTextFormat(Qt::RichText);
                textLayout->addWidget(nameLabel);

                QString vehicle = row.vehicleInfo();
                if (!vehicle.isEmpty()) {
                    auto *vehicleLabel = new QLabel(
                        QStringLiteral("<span style='color:#909399;font-size:11px;'>车型：</span>"
                                       "<span style='color:#606266;font-size:12px;'>%1</span>")
                            .arg(vehicle.toHtmlEscaped()));
                    vehicleLabel->setTextFormat(Qt::RichText);
                    textLayout->addWidget(vehicleLabel);
                }
                cardLayout->addWidget(textWidget, 1);

                item->setData(Qt::UserRole, row.id);
                item->setSizeHint(card->sizeHint());
                m_resultList->addItem(item);
                m_resultList->setItemWidget(item, card);
            }

            bool noMore = rows.size() < PAGE_SIZE;
            m_noMore = noMore;

            if (rows.isEmpty() && !append && m_total == 0)
                setFooter(QStringLiteral("未找到匹配的配方"));
            else if (noMore)
                setFooter(QStringLiteral("— 没有更多了 —"));
            else
                setFooter(QStringLiteral("滚动加载更多"));
        },
        [this](const QString &errMsg, int) {
            setFooter(errMsg);
        }
    );
    m_loading = false;
}

void MainWindow::onResultClicked(QListWidgetItem *item)
{
    int id = item->data(Qt::UserRole).toInt();
    if (id <= 0) return;
    m_selectedId = id;
    // 高亮选中项
    for (int i = 0; i < m_resultList->count(); ++i) {
        auto *it = m_resultList->item(i);
        it->setSelected(it->data(Qt::UserRole).toInt() == id);
    }
    m_detailWidget->loadDetail(id);
}

void MainWindow::handleLogout()
{
    LoginApi::instance().logout(
        [](const QJsonObject &) {
            Auth::instance().removeToken();
        },
        [](const QString &, int) {
            Auth::instance().removeToken();
        }
    );
    Auth::instance().removeToken();
    auto *login = new LoginWidget;
    login->resize(480, 700);
    login->show();
    this->close();
}

void MainWindow::showToast(const QString &msg)
{
    QMessageBox::information(this, QStringLiteral("提示"), msg);
}

void MainWindow::setFooter(const QString &text)
{
    m_footerLabel->setText(text);
}
