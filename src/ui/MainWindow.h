/* ============================================================
   MainWindow.h —— 主窗口（顶部导航 + 左右分栏）
   - 顶部导航：账号信息 + 退出按钮
   - 左栏：搜索输入（联想）+ 结果列表（分页加载）
   - 右栏：配方详情
   ============================================================ */

#pragma once

#include <QMainWindow>
#include <QListWidget>

class QLineEdit;
class QPushButton;
class QLabel;
class QListWidget;
class QTimer;
class QWidget;
class FormulaDetailWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void doSearch();
    void onSearchInput();
    void onResultClicked(QListWidgetItem *item);
    void handleLogout();

private:
    void setupUi();
    void setupTopBar(QWidget *topBar);
    void setupSidebar(QWidget *sidebar);
    void renderAccount();
    void renderHistory();
    void fetchList(bool append);
    void fetchSuggestions(const QString &q);
    void showToast(const QString &msg);
    void setFooter(const QString &text);

    // 顶部
    QLabel *m_accountNameLabel;
    QLabel *m_accountAvatarLabel;

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

    static constexpr int PAGE_SIZE = 10;
    static constexpr int SUGGEST_DEBOUNCE_MS = 300;
};
