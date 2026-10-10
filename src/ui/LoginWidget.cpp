/* ============================================================
   LoginWidget.cpp —— 登录页面实现
   ============================================================ */

#include "LoginWidget.h"
#include "MainWindow.h"
#include "RegisterWidget.h"
#include "api/LoginApi.h"
#include "utils/Auth.h"
#include "utils/I18nManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QLinearGradient>
#include <QMessageBox>
#include <QDialog>
#include <QTextBrowser>
#include <QFile>
#include <QApplication>
#include <QScreen>

static const QString REMEMBER_KEY = QStringLiteral("webcolor_remember");



LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(T("app_title") + QStringLiteral(" - ") + T("login_btn"));
    setMinimumSize(420, 600);
    setupUi();
    loadRemembered();
    loadCaptcha();
}

void LoginWidget::setupUi()
{
    auto *bg = new QWidget(this);
    bg->setStyleSheet(QStringLiteral("background: #f5f7fa;"));
    auto *outerLayout = new QVBoxLayout(bg);
    outerLayout->setContentsMargins(24, 40, 24, 40);
    outerLayout->setAlignment(Qt::AlignCenter);

    // 品牌区
    auto *brandWidget = new QWidget;
    auto *brandLayout = new QVBoxLayout(brandWidget);
    brandLayout->setAlignment(Qt::AlignCenter);

    // logo 自身带圆形外廓，不再套一层边框底板
    auto *logoLabel = new QLabel;
    logoLabel->setAlignment(Qt::AlignCenter);
    auto logoPixmap = QPixmap(QStringLiteral(":/logo.png"));
    logoLabel->setPixmap(logoPixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto *appNameLabel = new QLabel(T("app_title"));
    appNameLabel->setAlignment(Qt::AlignCenter);
    appNameLabel->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: 700; color: #1a2332; letter-spacing: 2px;"));

    auto *sloganLabel = new QLabel(T("login_subtitle"));
    sloganLabel->setAlignment(Qt::AlignCenter);
    sloganLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #606266;"));

    brandLayout->addWidget(logoLabel, 0, Qt::AlignCenter);
    brandLayout->addSpacing(12);
    brandLayout->addWidget(appNameLabel);
    brandLayout->addWidget(sloganLabel);

    // 登录卡片
    m_card = new QFrame;
    m_card->setObjectName(QStringLiteral("loginCard"));
    m_card->setMaximumWidth(400);
    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(32, 32, 32, 28);
    cardLayout->setSpacing(0);

    auto *titleLabel = new QLabel(T("login_account_title"));
    titleLabel->setObjectName(QStringLiteral("loginTitle"));
    cardLayout->addWidget(titleLabel);
    cardLayout->addSpacing(8);

    // 通用输入框样式
    const QString inputStyle = QStringLiteral(
        "QLineEdit { background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 10px; "
        "padding: 10px 14px; font-size: 14px; min-height: 20px; color: #1a2332; }"
        "QLineEdit:focus { border-color: #0d4f8b; background: #ffffff; }");

    // 用户名
    m_usernameEdit = new QLineEdit;
    m_usernameEdit->setPlaceholderText(T("login_account_placeholder"));
    m_usernameEdit->setStyleSheet(inputStyle);
    cardLayout->addWidget(m_usernameEdit);
    cardLayout->addSpacing(12);

    // 密码（带显示/隐藏切换）
    auto *pwdWrapper = new QWidget;
    pwdWrapper->setFixedHeight(44);
    pwdWrapper->setStyleSheet(QStringLiteral(
        "background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 10px;"));
    auto *pwdInner = new QHBoxLayout(pwdWrapper);
    pwdInner->setContentsMargins(14, 0, 4, 0);
    pwdInner->setSpacing(0);
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setPlaceholderText(T("login_password_placeholder"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background: transparent; border: none; font-size: 14px; color: #1a2332; }"
        "QLineEdit:focus { background: transparent; }"));
    auto *pwdToggleBtn = new QPushButton;
    pwdToggleBtn->setFixedSize(36, 36);
    pwdToggleBtn->setCursor(Qt::PointingHandCursor);
    pwdToggleBtn->setText(QStringLiteral("○"));
    pwdToggleBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; font-size: 13px; color: #909399; }"
        "QPushButton:hover { color: #0d4f8b; }"));
    connect(pwdToggleBtn, &QPushButton::clicked, this, [this, pwdToggleBtn]() {
        bool isPassword = (m_passwordEdit->echoMode() == QLineEdit::Password);
        m_passwordEdit->setEchoMode(isPassword ? QLineEdit::Normal : QLineEdit::Password);
        pwdToggleBtn->setText(isPassword ? QStringLiteral("●") : QStringLiteral("○"));
        pwdToggleBtn->setStyleSheet(isPassword
            ? QStringLiteral("QPushButton { background: transparent; border: none; font-size: 16px; color: #0d4f8b; }")
            : QStringLiteral("QPushButton { background: transparent; border: none; font-size: 13px; color: #909399; }"));
    });
    pwdInner->addWidget(m_passwordEdit, 1);
    pwdInner->addWidget(pwdToggleBtn);
    cardLayout->addWidget(pwdWrapper);
    cardLayout->addSpacing(12);

    // 验证码行
    m_captchaRow = new QWidget;
    auto *captchaLayout = new QHBoxLayout(m_captchaRow);
    captchaLayout->setContentsMargins(0, 0, 0, 0);
    m_codeEdit = new QLineEdit;
    m_codeEdit->setPlaceholderText(T("captcha"));
    m_codeEdit->setStyleSheet(inputStyle);
    m_captchaLabel = new QLabel;
    m_captchaLabel->setFixedHeight(36);
    m_captchaLabel->setCursor(Qt::PointingHandCursor);
    m_captchaLabel->setStyleSheet(QStringLiteral("border-radius: 8px;"));
    captchaLayout->addWidget(m_codeEdit);
    captchaLayout->addWidget(m_captchaLabel);
    cardLayout->addWidget(m_captchaRow);

    // 记住我
    auto *rememberRow = new QWidget;
    auto *rememberLayout = new QHBoxLayout(rememberRow);
    rememberLayout->setContentsMargins(0, 0, 0, 0);
    m_rememberCheck = new QCheckBox(T("remember_me"));
    rememberLayout->addWidget(m_rememberCheck);
    rememberLayout->addStretch();
    cardLayout->addWidget(rememberRow);
    cardLayout->addSpacing(8);

    // 登录按钮
    m_loginBtn = new QPushButton(T("login_btn_text"));
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    // 阿拉伯语有字距会打断字母连写，这里单独关掉
    const QString loginSpacing = I18nManager::instance().currentLanguage() == QStringLiteral("ar")
        ? QStringLiteral("0px") : QStringLiteral("6px");
    m_loginBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #0d4f8b; color: #ffffff; font-size: 16px; font-weight: 600; "
        "letter-spacing: %1; border: none; border-radius: 24px; min-height: 44px; }"
        "QPushButton:hover { background: #0a3f70; }"
        "QPushButton:pressed { background: #082e52; }"
        "QPushButton:disabled { background: #a0b8d0; }").arg(loginSpacing));
    cardLayout->addWidget(m_loginBtn);
    cardLayout->addSpacing(12);

    // 登录即同意条款：不勾选，两处协议名仍是可点击链接
    auto *agreeLabel = new QLabel;
    agreeLabel->setTextFormat(Qt::RichText);
    agreeLabel->setAlignment(Qt::AlignCenter);
    agreeLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    agreeLabel->setStyleSheet(QStringLiteral(
        "font-size: 12px; color: #909399; a { color: #0d4f8b; text-decoration: none; }"));
    agreeLabel->setWordWrap(true);
    agreeLabel->setText(T("agree_text").arg(
        QStringLiteral("<a href=\"privacy_policy\">%1</a>").arg(T("agree_privacy_policy")),
        QStringLiteral("<a href=\"user_agreement\">%1</a>").arg(T("agree_user_agreement"))));
    connect(agreeLabel, &QLabel::linkActivated, this, &LoginWidget::showAgreement);
    cardLayout->addWidget(agreeLabel);
    cardLayout->addSpacing(10);

    // 注册入口
    auto *regLabel = new QLabel(T("register_link_text"));
    regLabel->setTextFormat(Qt::RichText);
    regLabel->setAlignment(Qt::AlignCenter);
    regLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(regLabel, &QLabel::linkActivated, this, [this]() {
        auto *regWidget = new RegisterWidget;
        connect(regWidget, &RegisterWidget::backToLogin, regWidget, &QWidget::close);
        regWidget->show();
    });
    cardLayout->addWidget(regLabel);

    outerLayout->addWidget(brandWidget);
    outerLayout->addSpacing(20);
    outerLayout->addWidget(m_card);

    // 主布局
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(bg);

    // 事件绑定
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWidget::handleLogin);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWidget::handleLogin);
    connect(m_codeEdit, &QLineEdit::returnPressed, this, &LoginWidget::handleLogin);
    connect(m_captchaLabel, &QLabel::linkActivated, this, &LoginWidget::loadCaptcha);
}

void LoginWidget::handleLogin()
{
    if (m_loginBtn->isEnabled() == false) return;
    if (!validate()) return;

    m_loginBtn->setEnabled(false);
    m_loginBtn->setText(T("logging_in"));

    LoginApi::instance().login(
        m_usernameEdit->text().trimmed(),
        m_passwordEdit->text(),
        m_codeEdit->text().trimmed(),
        m_captchaUuid,
        [this](const QJsonObject &res) {
            // 保存 token
            Auth::instance().setToken(res.value(QStringLiteral("token")).toString());

            saveRemembered();
            m_loginBtn->setEnabled(true);
            m_loginBtn->setText(T("login_btn_text"));

            // 获取用户信息后再切换到主窗口
            LoginApi::instance().getInfo(
                [this](const QJsonObject &infoRes) {
                    auto user = infoRes.value(QStringLiteral("user")).toObject();
                    Auth::instance().setUser(user);

                    auto *mw = new MainWindow;
                    mw->show();
                    this->close();
                },
                [this](const QString &, int) {
                    // getInfo 失败也进入主窗口
                    auto *mw = new MainWindow;
                    mw->show();
                    this->close();
                }
            );
        },
        [this](const QString &errMsg, int) {
            m_loginBtn->setEnabled(true);
            m_loginBtn->setText(T("login_btn_text"));
            showToast(errMsg);
            if (m_captchaEnabled) loadCaptcha();
        }
    );
}

void LoginWidget::loadCaptcha()
{
    LoginApi::instance().getCaptcha(
        [this](const QJsonObject &res) {
            m_captchaEnabled = res.value(QStringLiteral("captchaEnabled")).toBool(true);
            if (!m_captchaEnabled) {
                m_captchaRow->hide();
                return;
            }
            m_captchaRow->show();
            m_captchaUuid = res.value(QStringLiteral("uuid")).toString();
            // 验证码图片 base64
            QString imgData = res.value(QStringLiteral("img")).toString();
            if (!imgData.isEmpty()) {
                // 去除 data:image/xxx;base64, 前缀
                int commaIdx = imgData.indexOf(QLatin1Char(','));
                QString base64 = commaIdx >= 0 ? imgData.mid(commaIdx + 1) : imgData;
                QByteArray bytes = QByteArray::fromBase64(base64.toLatin1());
                QPixmap pix;
                pix.loadFromData(bytes);
                m_captchaLabel->setPixmap(pix.scaledToHeight(36));
            }
        },
        [this](const QString &, int) {
            m_captchaRow->hide();
        }
    );
}

void LoginWidget::showAgreement(const QString &doc)
{
    const bool isPrivacy = (doc == QLatin1String("privacy_policy"));
    if (!isPrivacy && doc != QLatin1String("user_agreement")) return;

    // 标题用带书名号的词条，窗口标题里不需要
    QString title = isPrivacy ? T("agree_privacy_policy") : T("agree_user_agreement");
    title.remove(QStringLiteral("《")).remove(QStringLiteral("》"));

    // 正文随 exe 打包；当前语言没有对应正文时显示中文版
    auto readDoc = [&doc](const QString &lang) {
        QFile f(QStringLiteral(":/agreement/%1.%2.html").arg(doc, lang));
        if (!f.open(QIODevice::ReadOnly)) return QString();
        return QString::fromUtf8(f.readAll());
    };
    const QString body = [this, &readDoc] {
        const QString own = readDoc(I18nManager::instance().currentLanguage());
        return own.isEmpty() ? readDoc(QStringLiteral("zh")) : own;
    }();

    QDialog dlg(this);
    dlg.setWindowTitle(title);
    // 弹窗尺寸夹到屏幕可用区域，150% 缩放下按钮才不会被推到屏幕外
    const QRect avail = QApplication::primaryScreen()->availableGeometry();
    dlg.resize(qMin(620, avail.width() - 40), qMin(640, avail.height() - 40));

    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(12);

    auto *browser = new QTextBrowser(&dlg);
    browser->setOpenExternalLinks(false);
    browser->setHtml(body);
    lay->addWidget(browser, 1);

    auto *closeBtn = new QPushButton(T("close_btn"), &dlg);
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    lay->addWidget(closeBtn);

    dlg.exec();
}

void LoginWidget::loadRemembered()
{
    QSettings settings;
    QString raw = settings.value(REMEMBER_KEY).toString();
    if (raw.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    auto obj = doc.object();
    if (obj.value(QStringLiteral("rememberMe")).toBool()) {
        m_usernameEdit->setText(obj.value(QStringLiteral("username")).toString());
        m_passwordEdit->setText(obj.value(QStringLiteral("password")).toString());
        m_rememberCheck->setChecked(true);
    }
}

void LoginWidget::saveRemembered()
{
    QSettings settings;
    if (m_rememberCheck->isChecked()) {
        QJsonObject obj;
        obj[QStringLiteral("rememberMe")] = true;
        obj[QStringLiteral("username")]   = m_usernameEdit->text().trimmed();
        obj[QStringLiteral("password")]   = m_passwordEdit->text();
        QJsonDocument doc(obj);
        settings.setValue(REMEMBER_KEY, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    } else {
        settings.remove(REMEMBER_KEY);
    }
}

bool LoginWidget::validate()
{
    if (m_usernameEdit->text().trimmed().isEmpty()) {
        showToast(T("login_account_placeholder"));
        return false;
    }
    if (m_passwordEdit->text().isEmpty()) {
        showToast(T("login_password_placeholder"));
        return false;
    }
    return true;
}

void LoginWidget::showToast(const QString &msg)
{
    QMessageBox::information(this, T("info"), msg);
}
