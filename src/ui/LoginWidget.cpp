/* ============================================================
   LoginWidget.cpp —— 登录页面实现
   ============================================================ */

#include "LoginWidget.h"
#include "MainWindow.h"
#include "RegisterWidget.h"
#include "api/LoginApi.h"
#include "utils/Auth.h"
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

static const QString REMEMBER_KEY = QStringLiteral("webcolor_remember");



LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("恺祁颜色系统 - 登录"));
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

    auto *logoWrap = new QFrame;
    logoWrap->setFixedSize(84, 84);
    logoWrap->setStyleSheet(
        QStringLiteral("background: #ffffff; "
                       "border: 1px solid #d0dff0; "
                       "border-radius: 24px;"));
    auto *logoLayout = new QVBoxLayout(logoWrap);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoLayout->setAlignment(Qt::AlignCenter);
    auto *logoLabel = new QLabel;
    logoLabel->setAlignment(Qt::AlignCenter);
    auto logoPixmap = QPixmap(QStringLiteral(":/logo.png"));
    logoLabel->setPixmap(logoPixmap.scaled(56, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLayout->addWidget(logoLabel);

    auto *appNameLabel = new QLabel(QStringLiteral("恺祁颜色系统"));
    appNameLabel->setAlignment(Qt::AlignCenter);
    appNameLabel->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: 700; color: #1a2332; letter-spacing: 2px;"));

    auto *sloganLabel = new QLabel(QStringLiteral("专业配色查询系统"));
    sloganLabel->setAlignment(Qt::AlignCenter);
    sloganLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #606266;"));

    brandLayout->addWidget(logoWrap, 0, Qt::AlignCenter);
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

    auto *titleLabel = new QLabel(QStringLiteral("账号登录"));
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
    m_usernameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
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
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
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
    m_codeEdit->setPlaceholderText(QStringLiteral("验证码"));
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
    m_rememberCheck = new QCheckBox(QStringLiteral("记住我"));
    rememberLayout->addWidget(m_rememberCheck);
    rememberLayout->addStretch();
    cardLayout->addWidget(rememberRow);
    cardLayout->addSpacing(8);

    // 协议提示
    m_agreeTip = new QLabel(QStringLiteral("请先勾选同意用户协议与隐私政策"));
    m_agreeTip->setStyleSheet(QStringLiteral("color: #e74c3c; font-size: 12px;"));
    m_agreeTip->hide();
    cardLayout->addWidget(m_agreeTip);

    // 登录按钮
    m_loginBtn = new QPushButton(QStringLiteral("登 录"));
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #0d4f8b; color: #ffffff; font-size: 16px; font-weight: 600; "
        "letter-spacing: 6px; border: none; border-radius: 24px; min-height: 44px; }"
        "QPushButton:hover { background: #0a3f70; }"
        "QPushButton:pressed { background: #082e52; }"
        "QPushButton:disabled { background: #a0b8d0; }"));
    cardLayout->addWidget(m_loginBtn);
    cardLayout->addSpacing(12);

    // 协议勾选
    m_agreeCheck = new QCheckBox;
    auto *agreeLabel = new QLabel(QStringLiteral("已阅读并同意《隐私政策》与《用户协议》"));
    agreeLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #909399;"));
    agreeLabel->setWordWrap(true);
    auto *agreeLayout = new QHBoxLayout;
    agreeLayout->setContentsMargins(0, 0, 0, 0);
    agreeLayout->addWidget(m_agreeCheck);
    agreeLayout->addWidget(agreeLabel);
    auto *agreeWidget = new QWidget;
    agreeWidget->setLayout(agreeLayout);
    cardLayout->addWidget(agreeWidget);
    cardLayout->addSpacing(10);

    // 注册入口
    auto *regLabel = new QLabel(QStringLiteral("<span style='color:#909399;font-size:13px;'>还没有账号？</span>"
                                                "<a href='#' style='font-size:13px;'>立即注册</a>"));
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
    m_loginBtn->setText(QStringLiteral("登录中…"));

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
            m_loginBtn->setText(QStringLiteral("登 录"));

            // 获取用户信息后再切换到主窗口
            LoginApi::instance().getInfo(
                [this](const QJsonObject &infoRes) {
                    auto user = infoRes.value(QStringLiteral("user")).toObject();
                    Auth::instance().setUser(user);

                    auto *mw = new MainWindow;
                    mw->resize(1200, 780);
                    mw->show();
                    this->close();
                },
                [this](const QString &, int) {
                    // getInfo 失败也进入主窗口
                    auto *mw = new MainWindow;
                    mw->resize(1200, 780);
                    mw->show();
                    this->close();
                }
            );
        },
        [this](const QString &errMsg, int) {
            m_loginBtn->setEnabled(true);
            m_loginBtn->setText(QStringLiteral("登 录"));
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
        showToast(QStringLiteral("请输入用户名"));
        return false;
    }
    if (m_passwordEdit->text().isEmpty()) {
        showToast(QStringLiteral("请输入密码"));
        return false;
    }
    if (!m_agreeCheck->isChecked()) {
        m_agreeTip->show();
        return false;
    }
    m_agreeTip->hide();
    return true;
}

void LoginWidget::showToast(const QString &msg)
{
    QMessageBox::information(this, QStringLiteral("提示"), msg);
}
