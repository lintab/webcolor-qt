/* ============================================================
   MainWindow.h —— 主窗口（顶部导航 + 左侧导航栏 + 内容区）
   - 顶部导航：账号信息 + 退出按钮
   - 左侧导航栏：搜索 / 自动S&C / 设置
   - 内容区：QStackedWidget 切换不同功能页面
   ============================================================ */

#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QString>
#include <QCloseEvent>

class QLineEdit;
class QPushButton;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QTimer;
class QWidget;
class QStackedWidget;
class QMenu;
class FormulaDetailWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void doSearch();
    void onSearchInput();
    void onResultClicked(QListWidgetItem *item);
    void handleLogout();
    void switchPage(int index);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void rebuildPages();  // 构建/重建内容页（语言切换后按新语言重新构造）
    void retireOldPages();       // 旧页面先隐藏保活，等待在途 HTTP 回调结束
    void cleanupRetiredPages();  // 在途请求归零后再真正销毁旧页面
    void setupTopBar(QWidget *topBar);
    void setupNavBar(QWidget *navBar);
    void setupSearchPage(QWidget *page);
    void setupSidebar(QWidget *sidebar);
    void setupPlaceholderPage(QWidget *page, const QString &title, const QString &icon);
    void renderAccount();
    void renderHistory();
    void fetchList(bool append);
    void fetchSuggestions(const QString &q);
    void showToast(const QString &msg);
    void setFooter(const QString &text);
    void updateNavSelection(int index);
    void retranslateUi();  // 语言切换时刷新 UI 文本

    // 顶部
    QLabel *m_appNameLabel = nullptr;
    QLabel *m_accountNameLabel;
    QLabel *m_accountAvatarLabel;
    QPushButton *m_langBtn = nullptr;  // 语言切换按钮
    QMenu *m_langMenu = nullptr;       // 语言下拉菜单
    QMenu *m_userMenu = nullptr;       // 用户下拉菜单

    // 左侧导航栏
    QList<QPushButton*> m_navButtons;
    QPushButton *m_settingsBtn = nullptr;
    QMenu *m_settingsMenu = nullptr;

    // 内容区
    QStackedWidget *m_stack;
    QList<QWidget*> m_retiredPages;  // 语言切换换下、仍在等待在途请求的旧页面

    // 搜索
    QLineEdit *m_searchInput;
    QPushButton *m_searchBtn;
    QWidget   *m_suggestPanel;
    QListWidget *m_suggestList;
    QWidget   *m_historyBar;
    QWidget   *m_historyTags;

    // 结果列表
    QListWidget *m_resultList;
    QLabel *m_footerLabel;

    // 详情
    FormulaDetailWidget *m_detailWidget;

    // 状态
    QString m_keyword;
    QString m_searchKeyword;
    int m_page = 0;
    int m_total = 0;
    bool m_loading = false;
    bool m_noMore = false;
    int m_selectedId = 0;
    QTimer *m_suggestTimer;
    bool m_isClosing = false;  // 防止 closeEvent 重复调用

    static constexpr int PAGE_SIZE = 10;
    static constexpr int SUGGEST_DEBOUNCE_MS = 300;
};
