/* ============================================================
   MainWindow.cpp —— 主窗口实现
   ============================================================ */

#include "MainWindow.h"
#include "FormulaDetailWidget.h"
#include "LoginWidget.h"
#include "ProfileDialog.h"
#include "SettingsPage.h"
#include "BrandSeriesPage.h"
#include "ColorSearchSettingsPage.h"
#include "AutoScPage.h"
#include "UpdateWidget.h"
#include "api/FormulaApi.h"
#include "api/LoginApi.h"
#include "api/HttpClient.h"
#include "models/FormulaModel.h"
#include "utils/Auth.h"
#include "utils/SearchHistory.h"
#include "utils/SearchGather.h"
#include "utils/I18nManager.h"
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
#include <QSettings>
#include <QApplication>
#include <QScreen>
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
    setWindowTitle(T("app_title"));

    // 初始尺寸按屏幕可用区域夹取：150% 缩放的 1080 屏放不下 780 逻辑高，
    // 多出来的部分在屏幕外，左栏底部的搜索按钮怎么滑都看不到，只能靠最大化补救。
    // 再把窗口对齐可用区域左上角：默认居中会把底部推到任务栏底下。
    const QRect avail = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(1200, avail.width()), qMin(780, avail.height()));
    move(avail.x(), avail.y());

    // 在途请求归零后才能安全销毁旧页面
    connect(&HttpClient::instance(), &HttpClient::pendingChanged, this,
            [this](int pending) { if (pending == 0) cleanupRetiredPages(); });

    setupUi();
    renderAccount();
    renderHistory();

    // 监听语言切换
    connect(&I18nManager::instance(), &I18nManager::languageChanged, this, &MainWindow::retranslateUi);

    // 监听 token 失效
    connect(&HttpClient::instance(), &HttpClient::tokenExpired, this, [this]() {
        auto *login = new LoginWidget;
        login->resize(480, 700);
        login->show();
        this->close();
    });
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_retiredPages);
    m_retiredPages.clear();
}

void MainWindow::setupUi()
{
    auto *central = new QWidget;
    setCentralWidget(central);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ===== 顶部导航栏（全宽） =====
    auto *topBar = new QWidget;
    topBar->setObjectName(QStringLiteral("topBar"));
    topBar->setFixedHeight(52);
    topBar->setStyleSheet(QStringLiteral(
        "background: #0d4f8b; border-bottom: 1px solid #0a3f70;"));
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

    // 内容区（QStackedWidget）——页面统一由 rebuildPages() 构建，语言切换时重建
    m_stack = new QStackedWidget;
    rebuildPages();

    bodyLayout->addWidget(m_stack, 1);
    mainLayout->addWidget(bodyWidget, 1);
}

void MainWindow::retireOldPages()
{
    while (m_stack->count() > 0) {
        QWidget *page = m_stack->widget(0);
        m_stack->removeWidget(page);
        page->hide();
        // 立刻 delete 会让仍在途的 HTTP 回调访问已析构的子控件（切换语言时闪退的原因）
        m_retiredPages.append(page);
    }
    cleanupRetiredPages();
}

void MainWindow::cleanupRetiredPages()
{
    if (HttpClient::instance().pendingCount() > 0) return;
    if (m_retiredPages.isEmpty()) return;

    const QList<QWidget*> pages = m_retiredPages;
    m_retiredPages.clear();
    qDeleteAll(pages);
}

void MainWindow::rebuildPages()
{
    const int keepIndex = m_stack->currentIndex();

    retireOldPages();

    // 页面 0：搜索页
    auto *searchPage = new QWidget;
    searchPage->setObjectName(QStringLiteral("searchPage"));
    setupSearchPage(searchPage);
    m_stack->addWidget(searchPage);

    // 页面 1：品牌车型搜索
    m_stack->addWidget(new BrandSeriesPage);

    // 页面 2：自动 S&C
    m_stack->addWidget(new AutoScPage);

    // 页面 3：配方下载设置
    m_stack->addWidget(new SettingsPage);

    // 页面 4：搜色用户配置
    m_stack->addWidget(new ColorSearchSettingsPage);

    m_stack->setCurrentIndex(keepIndex >= 0 ? keepIndex : 0);
    updateNavSelection(m_stack->currentIndex());
}

void MainWindow::setupTopBar(QWidget *topBar)
{
    auto *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(20, 0, 20, 0);

    m_appNameLabel = new QLabel(T("app_title"));
    m_appNameLabel->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 18px; font-weight: 700; letter-spacing: 2px;"));
    layout->addWidget(m_appNameLabel);
    layout->addStretch();

    // 语言切换按钮（图标 + 下拉菜单）
    m_langBtn = new QPushButton;
    m_langBtn->setText(QStringLiteral("🌐"));  // 使用地球图标
    m_langBtn->setCursor(Qt::PointingHandCursor);
    m_langBtn->setFixedSize(36, 36);
    m_langBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: none; font-size: 18px; background: transparent; }"
        "QPushButton:hover { background: rgba(255,255,255,0.15); border-radius: 4px; }"));

    // 语言下拉菜单
    m_langMenu = new QMenu(m_langBtn);
    m_langMenu->setStyleSheet(QStringLiteral(
        "QMenu { background: #ffffff; border: 1px solid #e0e3e8; border-radius: 8px; padding: 4px 0; }"
        "QMenu::item { padding: 8px 24px; font-size: 13px; color: #1a2332; min-width: 80px; }"
        "QMenu::item:selected { background: #f0f2f5; color: #0d4f8b; }"));

    QString currentLang = I18nManager::instance().currentLanguage();

    // 语言列表：显示名 -> 语言代码
    auto langs = I18nManager::instance().supportedLanguages();
    for (auto it = langs.constBegin(); it != langs.constEnd(); ++it) {
        auto *action = m_langMenu->addAction(it.value());
        action->setCheckable(true);
        action->setChecked(currentLang == it.key());
        connect(action, &QAction::triggered, this, [this, action, code = it.key()]() {
            I18nManager::instance().setLanguage(code);
            action->setChecked(true);
            for (auto *a : m_langMenu->actions()) {
                if (a != action) a->setChecked(false);
            }
        });
    }

    // 点击按钮弹出菜单
    connect(m_langBtn, &QPushButton::clicked, this, [this]() {
        m_langMenu->popup(m_langBtn->mapToGlobal(QPoint(0, m_langBtn->height())));
    });

    layout->addWidget(m_langBtn);

    // 用户区域（头像 + 名称 + 下拉箭头，点击弹出菜单）
    auto *userArea = new QWidget;
    userArea->setCursor(Qt::PointingHandCursor);
    userArea->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *userAreaLayout = new QHBoxLayout(userArea);
    userAreaLayout->setContentsMargins(0, 0, 0, 0);
    userAreaLayout->setSpacing(6);

    m_accountAvatarLabel = new QLabel(T("avatar_char"));
    m_accountAvatarLabel->setAlignment(Qt::AlignCenter);
    m_accountAvatarLabel->setFixedSize(30, 30);
    m_accountAvatarLabel->setStyleSheet(QStringLiteral(
        "background: rgba(255,255,255,0.25); color: #ffffff; font-size: 13px; font-weight: bold; "
        "border-radius: 15px;"));
    userAreaLayout->addWidget(m_accountAvatarLabel);

    m_accountNameLabel = new QLabel(T("user"));
    m_accountNameLabel->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 14px; font-weight: 500;"));
    userAreaLayout->addWidget(m_accountNameLabel);

    auto *arrowLabel = new QLabel(QStringLiteral("▾"));
    arrowLabel->setStyleSheet(QStringLiteral("color: rgba(255,255,255,0.8); font-size: 12px;"));
    userAreaLayout->addWidget(arrowLabel);

    // 下拉菜单
    m_userMenu = new QMenu(userArea);
    m_userMenu->setStyleSheet(QStringLiteral(
        "QMenu { background: #ffffff; border: 1px solid #e0e3e8; border-radius: 8px; padding: 4px 0; }"
        "QMenu::item { padding: 8px 24px; font-size: 13px; color: #1a2332; }"
        "QMenu::item:selected { background: #f0f2f5; color: #0d4f8b; }"));
    m_userMenu->addAction(T("profile"), this, [this]() {
        auto *dlg = new ProfileDialog(this);
        dlg->exec();
    });
    m_userMenu->addSeparator();
    m_userMenu->addAction(T("logout"), this, &MainWindow::handleLogout);

    // 点击用户区域弹出菜单（通过 mouseReleaseEvent）
    auto showMenu = [userArea, this]() {
        m_userMenu->popup(userArea->mapToGlobal(QPoint(0, userArea->height())));
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

    // 搜索 & 自动S&C 按钮（页面切换）
    struct NavItem {
        QString icon;
        QString text;
    };
    QVector<NavItem> items = {
        { QStringLiteral("\U0001F50D"), T("nav_search") },
        { QStringLiteral("\U0001F4C5"), T("nav_brand") },
        { QStringLiteral("\U0001F52C"), T("nav_autosc") },
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

    // 设置按钮（下拉菜单）
    m_settingsBtn = new QPushButton(QStringLiteral("\u2699\n") + T("settings"));
    m_settingsBtn->setObjectName(QStringLiteral("navBtn"));
    m_settingsBtn->setCursor(Qt::PointingHandCursor);
    m_settingsBtn->setFixedHeight(64);
    layout->addWidget(m_settingsBtn);

    m_settingsMenu = new QMenu(m_settingsBtn);
    m_settingsMenu->setStyleSheet(QStringLiteral(
        "QMenu { background: #ffffff; border: 1px solid #e0e3e8; border-radius: 8px; padding: 4px 0; }"
        "QMenu::item { padding: 10px 28px; font-size: 13px; color: #1a2332; min-width: 120px; }"
        "QMenu::item:selected { background: #f0f2f5; color: #0d4f8b; }"
        "QMenu::separator { height: 1px; background: #e4e7ed; margin: 4px 12px; }"));
    m_settingsMenu->addAction(T("formula_download_settings"), this, [this]() {
        switchPage(3);
    });
    m_settingsMenu->addSeparator();
    m_settingsMenu->addAction(T("color_search_settings"), this, [this]() {
        switchPage(4);
    });
    m_settingsMenu->addSeparator();
    m_settingsMenu->addAction(T("software_update"), this, [this]() {
        auto *updateWidget = new UpdateWidget;
        updateWidget->setAttribute(Qt::WA_DeleteOnClose);
        updateWidget->show();
    });

    connect(m_settingsBtn, &QPushButton::clicked, this, [this]() {
        m_settingsMenu->exec(m_settingsBtn->mapToGlobal(
            QPoint(m_settingsBtn->width(), 0)));
    });

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

    auto *hintLabel = new QLabel(T("feature_in_development"));
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
    m_searchInput->setPlaceholderText(T("search_placeholder"));
    m_searchInput->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 20px; "
        "padding: 8px 14px; font-size: 14px; min-height: 20px; color: #1a2332; }"
        "QLineEdit:focus { border-color: #0d4f8b; background: #ffffff; }"));
    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::doSearch);
    connect(m_searchInput, &QLineEdit::textChanged, this, [this](const QString &) { onSearchInput(); });
    searchLayout->addWidget(m_searchInput);

    m_searchBtn = new QPushButton(T("search_btn"));
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
    histLayout->addWidget(new QLabel(T("history_label")));
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
    m_footerLabel = new QLabel(T("start_search_hint"));
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
    if (nick.isEmpty()) nick = T("user");
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
        showToast(T("enter_search_keyword"));
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
        setFooter(T("no_more"));
        return;
    }

    if (!append) {
        m_resultList->clear();
        m_detailWidget->clear();
        setFooter(T("loading"));
    } else {
        setFooter(T("loading"));
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
                        QStringLiteral("<span style='color:#909399;font-size:11px;'>") + T("color_code_label") + QStringLiteral("</span>")
                        + QStringLiteral("<span style='color:#1a2332;font-size:13px;font-weight:600;'>%1</span>").arg(row.colorCode.toHtmlEscaped()));
                    sampleLabel->setTextFormat(Qt::RichText);
                    textLayout->addWidget(sampleLabel);
                }
                auto *nameLabel = new QLabel(
                    QStringLiteral("<span style='color:#909399;font-size:11px;'>") + T("color_name_label") + QStringLiteral("</span>")
                    + QStringLiteral("<span style='color:#1a2332;font-size:13px;'>%1</span>").arg(row.displayName().toHtmlEscaped()));
                nameLabel->setTextFormat(Qt::RichText);
                textLayout->addWidget(nameLabel);

                QString vehicle = row.vehicleInfo();
                if (!vehicle.isEmpty()) {
                    auto *vehicleLabel = new QLabel(
                        QStringLiteral("<span style='color:#909399;font-size:11px;'>") + T("car_model_label") + QStringLiteral("</span>")
                        + QStringLiteral("<span style='color:#606266;font-size:12px;'>%1</span>").arg(vehicle.toHtmlEscaped()));
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
                setFooter(T("no_match_found"));
            else if (noMore)
                setFooter(T("no_more"));
            else
                setFooter(T("scroll_load_more"));
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 如果已经在关闭过程中，直接接受关闭
    if (m_isClosing) {
        QMainWindow::closeEvent(event);
        return;
    }
    
    // 关闭窗口时自动退出账号（调用登出 API 并清除 token）
    // 先忽略关闭事件，等待登出 API 完成后再关闭
    event->ignore();
    m_isClosing = true;
    
    qInfo() << "MainWindow::closeEvent - 调用登出 API";
    LoginApi::instance().logout(
        [this](const QJsonObject &) {
            qInfo() << "登出 API 成功";
            Auth::instance().removeToken();
            this->close();  // 登出成功后关闭窗口
        },
        [this](const QString &msg, int) {
            qWarning() << "登出 API 失败:" << msg;
            Auth::instance().removeToken();
            this->close();  // 即使失败也关闭窗口
        }
    );
}

void MainWindow::showToast(const QString &msg)
{
    QMessageBox::information(this, T("info"), msg);
}

void MainWindow::setFooter(const QString &text)
{
    m_footerLabel->setText(text);
}

void MainWindow::retranslateUi()
{
    // 窗口标题
    setWindowTitle(T("app_title"));

    // 顶部栏
    if (m_appNameLabel) m_appNameLabel->setText(T("app_title"));
    if (m_accountAvatarLabel) {
        auto user = Auth::instance().getUser();
        QString nick = user.value(QStringLiteral("nickName")).toString(
                       user.value(QStringLiteral("userName")).toString());
        if (nick.isEmpty()) nick = T("user");
        m_accountAvatarLabel->setText(nick.left(1).toUpper());
    }

    // 导航按钮
    if (m_navButtons.size() >= 3) {
        m_navButtons[0]->setText(QStringLiteral("\U0001F50D\n") + T("nav_search"));
        m_navButtons[1]->setText(QStringLiteral("\U0001F4C5\n") + T("nav_brand"));
        m_navButtons[2]->setText(QStringLiteral("\U0001F52C\n") + T("nav_autosc"));
    }

    // 设置按钮
    if (m_settingsBtn) m_settingsBtn->setText(QStringLiteral("\u2699\n") + T("settings"));

    // 重建用户菜单
    if (m_userMenu) {
        m_userMenu->clear();
        m_userMenu->addAction(T("profile"), this, [this]() {
            auto *dlg = new ProfileDialog(this);
            dlg->exec();
        });
        m_userMenu->addSeparator();
        m_userMenu->addAction(T("logout"), this, &MainWindow::handleLogout);
    }

    // 重建设置菜单
    if (m_settingsMenu) {
        m_settingsMenu->clear();
        m_settingsMenu->addAction(T("formula_download_settings"), this, [this]() { switchPage(3); });
        m_settingsMenu->addSeparator();
        m_settingsMenu->addAction(T("color_search_settings"), this, [this]() { switchPage(4); });
        m_settingsMenu->addSeparator();
        m_settingsMenu->addAction(T("software_update"), this, [this]() {
            auto *updateWidget = new UpdateWidget;
            updateWidget->setAttribute(Qt::WA_DeleteOnClose);
            updateWidget->show();
        });
    }

    // 重建语言菜单勾选状态
    if (m_langMenu) {
        QString currentLang = I18nManager::instance().currentLanguage();
        for (auto *action : m_langMenu->actions()) {
            // 通过检查 action 文本与 supportedLanguages 的映射来确定语言代码
            auto langs = I18nManager::instance().supportedLanguages();
            for (auto it = langs.constBegin(); it != langs.constEnd(); ++it) {
                if (action->text() == it.value()) {
                    action->setChecked(currentLang == it.key());
                    break;
                }
            }
        }
    }

    // 内容页只在构造时读取翻译，按新语言整体重建
    rebuildPages();
}
