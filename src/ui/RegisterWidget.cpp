/* ============================================================
   RegisterWidget.cpp —— 注册页面实现
   ============================================================ */

#include "RegisterWidget.h"
#include "api/LoginApi.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QJsonArray>
#include <QMessageBox>

const QString RegisterWidget::BRAND_CUSTOM = QStringLiteral("自定义");
const QString RegisterWidget::COUNTRY_CHINA = QStringLiteral("中国");

QStringList RegisterWidget::countryList()
{
    return {
        QStringLiteral("中国"), QStringLiteral("美国"), QStringLiteral("英国"),
        QStringLiteral("德国"), QStringLiteral("法国"), QStringLiteral("日本"),
        QStringLiteral("韩国"), QStringLiteral("澳大利亚"), QStringLiteral("加拿大"),
        QStringLiteral("新加坡"), QStringLiteral("马来西亚"), QStringLiteral("泰国"),
        QStringLiteral("印度"), QStringLiteral("俄罗斯"), QStringLiteral("意大利"),
        QStringLiteral("西班牙"), QStringLiteral("荷兰"), QStringLiteral("瑞士"),
        QStringLiteral("阿联酋"), QStringLiteral("沙特阿拉伯"), QStringLiteral("土耳其"),
        QStringLiteral("越南"), QStringLiteral("印度尼西亚"), QStringLiteral("菲律宾"),
        QStringLiteral("巴西"), QStringLiteral("墨西哥"), QStringLiteral("南非"),
        QStringLiteral("新西兰"), QStringLiteral("其他")
    };
}

QStringList RegisterWidget::provinceList()
{
    return {
        QStringLiteral("北京市"), QStringLiteral("天津市"), QStringLiteral("河北省"),
        QStringLiteral("山西省"), QStringLiteral("内蒙古自治区"), QStringLiteral("辽宁省"),
        QStringLiteral("吉林省"), QStringLiteral("黑龙江省"), QStringLiteral("上海市"),
        QStringLiteral("江苏省"), QStringLiteral("浙江省"), QStringLiteral("安徽省"),
        QStringLiteral("福建省"), QStringLiteral("江西省"), QStringLiteral("山东省"),
        QStringLiteral("河南省"), QStringLiteral("湖北省"), QStringLiteral("湖南省"),
        QStringLiteral("广东省"), QStringLiteral("广西壮族自治区"), QStringLiteral("海南省"),
        QStringLiteral("重庆市"), QStringLiteral("四川省"), QStringLiteral("贵州省"),
        QStringLiteral("云南省"), QStringLiteral("西藏自治区"), QStringLiteral("陕西省"),
        QStringLiteral("甘肃省"), QStringLiteral("青海省"), QStringLiteral("宁夏回族自治区"),
        QStringLiteral("新疆维吾尔自治区"), QStringLiteral("台湾省"),
        QStringLiteral("香港特别行政区"), QStringLiteral("澳门特别行政区")
    };
}

QStringList RegisterWidget::brandList()
{
    return {
        QStringLiteral("猛鹰"), QStringLiteral("雄途"), QStringLiteral("恺祁"),
        QStringLiteral("施卡斯"), QStringLiteral("蓝彩金"), QStringLiteral("击剑"),
        QStringLiteral("速航"), QStringLiteral("帕特美"), BRAND_CUSTOM
    };
}

RegisterWidget::RegisterWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("账户注册"));
    resize(480, 700);
    setStyleSheet(QStringLiteral("RegisterWidget { background: #f5f7fa; }"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ===== 顶部品牌区 =====
    auto *brandWidget = new QWidget;
    brandWidget->setFixedHeight(120);
    brandWidget->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #052c4d, stop:0.5 #0d4f8b, stop:1 #1a6ec9);"));
    auto *brandLayout = new QVBoxLayout(brandWidget);
    brandLayout->setAlignment(Qt::AlignCenter);
    auto *titleLabel = new QLabel(QStringLiteral("创建账户"));
    titleLabel->setStyleSheet(QStringLiteral(
        "font-size: 24px; font-weight: 700; color: #ffffff;"));
    auto *subtitleLabel = new QLabel(QStringLiteral("加入恺祁颜色系统"));
    subtitleLabel->setStyleSheet(QStringLiteral(
        "font-size: 14px; color: rgba(255,255,255,0.8);"));
    brandLayout->addWidget(titleLabel);
    brandLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(brandWidget);

    // ===== 滚动区域 =====
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { border: none; background: #ffffff; }"));

    auto *scrollWidget = new QWidget;
    scrollWidget->setStyleSheet(QStringLiteral("background: #ffffff;"));
    auto *formLayout = new QVBoxLayout(scrollWidget);
    formLayout->setContentsMargins(32, 24, 32, 24);
    formLayout->setSpacing(12);

    // ===== 模式切换标签 =====
    auto *modeWidget = new QWidget;
    modeWidget->setStyleSheet(QStringLiteral(
        "background: #f5f7fa; border-radius: 10px; border: 1px solid #e0e3e8;"));
    auto *modeLayout = new QHBoxLayout(modeWidget);
    modeLayout->setContentsMargins(4, 4, 4, 4);
    modeLayout->setSpacing(4);

    auto *phoneTab = new QPushButton(QStringLiteral("手机号注册"));
    auto *emailTab = new QPushButton(QStringLiteral("邮箱注册"));
    auto tabStyle = QStringLiteral(
        "QPushButton { background: transparent; border: none; border-radius: 8px; "
        "padding: 10px 16px; font-size: 14px; color: #606266; }"
        "QPushButton:hover { background: #e8ecf0; }");
    auto tabActiveStyle = QStringLiteral(
        "QPushButton { background: #ffffff; border: none; border-radius: 8px; "
        "padding: 10px 16px; font-size: 14px; color: #0d4f8b; font-weight: 600; }");
    phoneTab->setStyleSheet(tabActiveStyle);
    emailTab->setStyleSheet(tabStyle);
    phoneTab->setCursor(Qt::PointingHandCursor);
    emailTab->setCursor(Qt::PointingHandCursor);

    connect(phoneTab, &QPushButton::clicked, this, [this, phoneTab, emailTab, tabStyle, tabActiveStyle]() {
        if (m_mode == QStringLiteral("phone")) return;
        switchMode(QStringLiteral("phone"));
        phoneTab->setStyleSheet(tabActiveStyle);
        emailTab->setStyleSheet(tabStyle);
    });
    connect(emailTab, &QPushButton::clicked, this, [this, phoneTab, emailTab, tabStyle, tabActiveStyle]() {
        if (m_mode == QStringLiteral("email")) return;
        switchMode(QStringLiteral("email"));
        emailTab->setStyleSheet(tabActiveStyle);
        phoneTab->setStyleSheet(tabStyle);
    });

    modeLayout->addWidget(phoneTab);
    modeLayout->addWidget(emailTab);
    formLayout->addWidget(modeWidget);

    // ===== 输入框样式 =====
    auto inputStyle = QStringLiteral(
        "QLineEdit { background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 10px; "
        "padding: 12px 14px; font-size: 14px; color: #1a2332; }"
        "QLineEdit:focus { border-color: #0d4f8b; background: #ffffff; }");
    auto comboStyle = QStringLiteral(
        "QComboBox { background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 10px; "
        "padding: 10px 30px 10px 14px; font-size: 14px; color: #1a2332; min-height: 22px; }"
        "QComboBox:hover { border-color: #0d4f8b; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow { image: none; border: none; "
        "border-left: 4px solid transparent; border-right: 4px solid transparent; "
        "border-top: 5px solid #606266; margin-right: 8px; }"
        "QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #e0e3e8; "
        "border-radius: 6px; selection-background-color: #e8f4fd; selection-color: #0d4f8b; }");

    // ===== 手机号输入 =====
    m_phoneEdit = new QLineEdit;
    m_phoneEdit->setPlaceholderText(QStringLiteral("请输入手机号"));
    m_phoneEdit->setStyleSheet(inputStyle);
    m_phoneEdit->setMaxLength(11);
    formLayout->addWidget(m_phoneEdit);

    // ===== 邮箱输入 =====
    m_emailEdit = new QLineEdit;
    m_emailEdit->setPlaceholderText(QStringLiteral("请输入邮箱"));
    m_emailEdit->setStyleSheet(inputStyle);
    m_emailEdit->setMaxLength(50);
    m_emailEdit->setVisible(false);
    formLayout->addWidget(m_emailEdit);

    // ===== 验证码输入 + 发送按钮 =====
    auto *codeRow = new QHBoxLayout;
    codeRow->setSpacing(8);
    m_codeEdit = new QLineEdit;
    m_codeEdit->setPlaceholderText(QStringLiteral("请输入验证码"));
    m_codeEdit->setStyleSheet(inputStyle);
    m_codeEdit->setMaxLength(6);
    m_sendCodeBtn = new QPushButton(QStringLiteral("发送验证码"));
    m_sendCodeBtn->setCursor(Qt::PointingHandCursor);
    m_sendCodeBtn->setFixedSize(110, 44);
    m_sendCodeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #0d4f8b; color: #ffffff; font-size: 13px; font-weight: 600; "
        "border: none; border-radius: 10px; }"
        "QPushButton:hover { background: #0a3f70; }"
        "QPushButton:disabled { background: #94a3b8; }"));
    connect(m_sendCodeBtn, &QPushButton::clicked, this, &RegisterWidget::sendCode);
    codeRow->addWidget(m_codeEdit, 1);
    codeRow->addWidget(m_sendCodeBtn);
    formLayout->addLayout(codeRow);

    // ===== 昵称 =====
    m_nickNameEdit = new QLineEdit;
    m_nickNameEdit->setPlaceholderText(QStringLiteral("请输入昵称"));
    m_nickNameEdit->setStyleSheet(inputStyle);
    m_nickNameEdit->setMaxLength(30);
    formLayout->addWidget(m_nickNameEdit);

    // ===== 密码 =====
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
    m_passwordEdit->setMaxLength(20);
    m_passwordEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background: transparent; border: none; font-size: 14px; color: #1a2332; }"
        "QLineEdit:focus { background: transparent; }"));
    m_pwdToggleBtn = new QPushButton;
    m_pwdToggleBtn->setFixedSize(36, 36);
    m_pwdToggleBtn->setCursor(Qt::PointingHandCursor);
    m_pwdToggleBtn->setText(QStringLiteral("\u25CF"));
    m_pwdToggleBtn->setProperty("visible", false);
    m_pwdToggleBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; font-size: 13px; color: #909399; }"
        "QPushButton:hover { color: #0d4f8b; }"));
    // 使用图标文字切换
    m_pwdToggleBtn->setText(QStringLiteral("○"));
    connect(m_pwdToggleBtn, &QPushButton::clicked, this, [this, pwdWrapper]() {
        bool isPassword = (m_passwordEdit->echoMode() == QLineEdit::Password);
        m_passwordEdit->setEchoMode(isPassword ? QLineEdit::Normal : QLineEdit::Password);
        m_pwdToggleBtn->setText(isPassword ? QStringLiteral("●") : QStringLiteral("○"));
        m_pwdToggleBtn->setStyleSheet(isPassword
            ? QStringLiteral("QPushButton { background: transparent; border: none; font-size: 16px; color: #0d4f8b; }")
            : QStringLiteral("QPushButton { background: transparent; border: none; font-size: 13px; color: #909399; }"));
    });
    pwdInner->addWidget(m_passwordEdit, 1);
    pwdInner->addWidget(m_pwdToggleBtn);
    formLayout->addWidget(pwdWrapper);

    // ===== 确认密码 =====
    auto *confirmPwdWrapper = new QWidget;
    confirmPwdWrapper->setFixedHeight(44);
    confirmPwdWrapper->setStyleSheet(QStringLiteral(
        "background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 10px;"));
    auto *confirmPwdInner = new QHBoxLayout(confirmPwdWrapper);
    confirmPwdInner->setContentsMargins(14, 0, 4, 0);
    confirmPwdInner->setSpacing(0);
    m_confirmPasswordEdit = new QLineEdit;
    m_confirmPasswordEdit->setPlaceholderText(QStringLiteral("请再次输入密码"));
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setMaxLength(20);
    m_confirmPasswordEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background: transparent; border: none; font-size: 14px; color: #1a2332; }"
        "QLineEdit:focus { background: transparent; }"));
    m_confirmPwdToggleBtn = new QPushButton;
    m_confirmPwdToggleBtn->setFixedSize(36, 36);
    m_confirmPwdToggleBtn->setCursor(Qt::PointingHandCursor);
    m_confirmPwdToggleBtn->setText(QStringLiteral("○"));
    m_confirmPwdToggleBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; font-size: 13px; color: #909399; }"
        "QPushButton:hover { color: #0d4f8b; }"));
    connect(m_confirmPwdToggleBtn, &QPushButton::clicked, this, [this, confirmPwdWrapper]() {
        bool isPassword = (m_confirmPasswordEdit->echoMode() == QLineEdit::Password);
        m_confirmPasswordEdit->setEchoMode(isPassword ? QLineEdit::Normal : QLineEdit::Password);
        m_confirmPwdToggleBtn->setText(isPassword ? QStringLiteral("●") : QStringLiteral("○"));
        m_confirmPwdToggleBtn->setStyleSheet(isPassword
            ? QStringLiteral("QPushButton { background: transparent; border: none; font-size: 16px; color: #0d4f8b; }")
            : QStringLiteral("QPushButton { background: transparent; border: none; font-size: 13px; color: #909399; }"));
    });
    confirmPwdInner->addWidget(m_confirmPasswordEdit, 1);
    confirmPwdInner->addWidget(m_confirmPwdToggleBtn);
    formLayout->addWidget(confirmPwdWrapper);

    // ===== 国家 =====
    m_countryCombo = new QComboBox;
    m_countryCombo->setStyleSheet(comboStyle);
    m_countryCombo->addItem(QStringLiteral("请选择国家/地区"), QString());
    for (const auto &c : countryList())
        m_countryCombo->addItem(c, c);
    // 默认选中中国
    m_countryCombo->setCurrentIndex(m_countryCombo->findData(COUNTRY_CHINA));
    formLayout->addWidget(m_countryCombo);

    // ===== 省份 =====
    m_provinceCombo = new QComboBox;
    m_provinceCombo->setStyleSheet(comboStyle);
    m_provinceCombo->addItem(QStringLiteral("请选择省份"), QString());
    for (const auto &p : provinceList())
        m_provinceCombo->addItem(p, p);
    formLayout->addWidget(m_provinceCombo);

    // ===== 品牌 =====
    m_brandCombo = new QComboBox;
    m_brandCombo->setStyleSheet(comboStyle);
    m_brandCombo->addItem(QStringLiteral("请选择品牌"), QString());
    for (const auto &b : brandList())
        m_brandCombo->addItem(b, b);
    formLayout->addWidget(m_brandCombo);

    // ===== 自定义品牌 =====
    m_customBrandEdit = new QLineEdit;
    m_customBrandEdit->setPlaceholderText(QStringLiteral("请输入自定义品牌名"));
    m_customBrandEdit->setStyleSheet(inputStyle);
    m_customBrandEdit->setMaxLength(50);
    m_customBrandEdit->setVisible(false);
    formLayout->addWidget(m_customBrandEdit);

    // ===== 供应商 =====
    m_supplierEdit = new QLineEdit;
    m_supplierEdit->setPlaceholderText(QStringLiteral("请输入供应商"));
    m_supplierEdit->setStyleSheet(inputStyle);
    m_supplierEdit->setMaxLength(100);
    formLayout->addWidget(m_supplierEdit);

    // ===== 提示标签 =====
    m_toastLabel = new QLabel;
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->setStyleSheet(QStringLiteral("font-size: 13px;"));
    m_toastLabel->setVisible(false);
    formLayout->addWidget(m_toastLabel);

    // ===== 注册按钮 =====
    m_registerBtn = new QPushButton(QStringLiteral("立即注册"));
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setFixedHeight(48);
    m_registerBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 #0d4f8b, stop:1 #1a6ec9); color: #ffffff; font-size: 16px; font-weight: 600; "
        "border: none; border-radius: 24px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 #0a3f70, stop:1 #155ea8); }"
        "QPushButton:pressed { background: #082e52; }"
        "QPushButton:disabled { background: #94a3b8; }"));
    connect(m_registerBtn, &QPushButton::clicked, this, &RegisterWidget::doRegister);
    formLayout->addWidget(m_registerBtn);

    // ===== 返回登录 =====
    auto *footerLayout = new QHBoxLayout;
    footerLayout->setAlignment(Qt::AlignCenter);
    auto *backLabel = new QLabel(QStringLiteral("已有账号？"));
    backLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #606266;"));
    auto *backBtn = new QPushButton(QStringLiteral("立即登录"));
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; color: #0d4f8b; "
        "font-size: 14px; font-weight: 600; }"
        "QPushButton:hover { color: #1a6ec9; }"));
    connect(backBtn, &QPushButton::clicked, this, &RegisterWidget::backToLogin);
    footerLayout->addWidget(backLabel);
    footerLayout->addWidget(backBtn);
    formLayout->addLayout(footerLayout);

    formLayout->addStretch();
    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    // ===== 倒计时定时器 =====
    m_countdownTimer = new QTimer(this);
    connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
        m_countdownSec--;
        if (m_countdownSec <= 0) {
            m_countdownTimer->stop();
            m_sendCodeBtn->setEnabled(true);
            m_sendCodeBtn->setText(QStringLiteral("发送验证码"));
        } else {
            m_sendCodeBtn->setText(QStringLiteral("重新发送(%1s)").arg(m_countdownSec));
        }
    });

    // ===== 联动：国家选择 =====
    connect(m_countryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        bool isChina = (m_countryCombo->currentData().toString() == COUNTRY_CHINA);
        m_provinceCombo->setVisible(isChina);
    });
    // 初始显示省份
    m_provinceCombo->setVisible(true);

    // ===== 联动：品牌选择 =====
    connect(m_brandCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        bool showCustom = (m_brandCombo->currentData().toString() == BRAND_CUSTOM);
        m_customBrandEdit->setVisible(showCustom);
    });
}

void RegisterWidget::switchMode(const QString &mode)
{
    m_mode = mode;
    bool isPhone = (mode == QStringLiteral("phone"));
    m_phoneEdit->setVisible(isPhone);
    m_emailEdit->setVisible(!isPhone);
    m_codeEdit->clear();
}

void RegisterWidget::sendCode()
{
    QString target;
    bool isPhone = (m_mode == QStringLiteral("phone"));

    if (isPhone) {
        target = m_phoneEdit->text().trimmed();
        if (target.length() != 11 || !target.startsWith(QStringLiteral("1"))) {
            showToast(QStringLiteral("请输入正确的手机号"), true);
            return;
        }
    } else {
        target = m_emailEdit->text().trimmed();
        if (!target.contains(QStringLiteral("@")) || !target.contains(QStringLiteral("."))) {
            showToast(QStringLiteral("请输入正确的邮箱"), true);
            return;
        }
    }

    m_sendCodeBtn->setEnabled(false);

    auto onSuccess = [this, isPhone](const QJsonObject &) {
        showToast(isPhone ? QStringLiteral("短信已发送") : QStringLiteral("邮件已发送"));
        startCountdown();
    };
    auto onError = [this](const QString &errMsg, int) {
        showToast(errMsg, true);
        m_sendCodeBtn->setEnabled(true);
    };

    if (isPhone) {
        LoginApi::instance().sendSmsCode(target, QStringLiteral("REGISTER"), onSuccess, onError);
    } else {
        LoginApi::instance().sendEmailCode(target, QStringLiteral("REGISTER"), onSuccess, onError);
    }
}

void RegisterWidget::doRegister()
{
    // 验证表单
    bool isPhone = (m_mode == QStringLiteral("phone"));

    if (isPhone) {
        QString phone = m_phoneEdit->text().trimmed();
        if (phone.length() != 11 || !phone.startsWith(QStringLiteral("1"))) {
            showToast(QStringLiteral("请输入正确的手机号"), true);
            return;
        }
    } else {
        QString email = m_emailEdit->text().trimmed();
        if (!email.contains(QStringLiteral("@")) || !email.contains(QStringLiteral("."))) {
            showToast(QStringLiteral("请输入正确的邮箱"), true);
            return;
        }
    }

    QString code = m_codeEdit->text().trimmed();
    if (code.isEmpty()) {
        showToast(QStringLiteral("请输入验证码"), true);
        return;
    }

    QString nickName = m_nickNameEdit->text().trimmed();
    if (nickName.isEmpty()) {
        showToast(QStringLiteral("请输入昵称"), true);
        return;
    }

    QString password = m_passwordEdit->text();
    if (password.isEmpty()) {
        showToast(QStringLiteral("请输入密码"), true);
        return;
    }

    QString confirmPassword = m_confirmPasswordEdit->text();
    if (confirmPassword.isEmpty()) {
        showToast(QStringLiteral("请再次输入密码"), true);
        return;
    }
    if (password != confirmPassword) {
        showToast(QStringLiteral("两次密码输入不一致"), true);
        return;
    }

    QString country = m_countryCombo->currentData().toString();
    if (country.isEmpty()) {
        showToast(QStringLiteral("请选择国家/地区"), true);
        return;
    }

    QString brand = m_brandCombo->currentData().toString();
    if (brand.isEmpty()) {
        showToast(QStringLiteral("请选择品牌"), true);
        return;
    }
    if (brand == BRAND_CUSTOM) {
        brand = m_customBrandEdit->text().trimmed();
        if (brand.isEmpty()) {
            showToast(QStringLiteral("请输入自定义品牌名"), true);
            return;
        }
    }

    QString supplier = m_supplierEdit->text().trimmed();
    if (supplier.isEmpty()) {
        showToast(QStringLiteral("请输入供应商"), true);
        return;
    }

    // 构建请求数据
    QJsonObject data;
    data[QStringLiteral("nickName")] = nickName;
    data[QStringLiteral("password")] = password;
    data[QStringLiteral("country")] = country;
    data[QStringLiteral("brand")] = brand;
    data[QStringLiteral("supplier")] = supplier;
    data[QStringLiteral("code")] = code;

    if (country == COUNTRY_CHINA) {
        QString province = m_provinceCombo->currentData().toString();
        if (!province.isEmpty()) {
            data[QStringLiteral("province")] = province;
        }
    }

    m_registerBtn->setEnabled(false);
    m_registerBtn->setText(QStringLiteral("注册中..."));

    auto onSuccess = [this, isPhone](const QJsonObject &) {
        m_registerBtn->setEnabled(true);
        m_registerBtn->setText(QStringLiteral("立即注册"));
        QString username = isPhone ? m_phoneEdit->text().trimmed() : m_emailEdit->text().trimmed();
        QMessageBox::information(this, QStringLiteral("注册成功"),
            QStringLiteral("注册成功！账号：%1\n请使用新账号登录。").arg(username));
        emit backToLogin();
    };
    auto onError = [this](const QString &errMsg, int) {
        m_registerBtn->setEnabled(true);
        m_registerBtn->setText(QStringLiteral("立即注册"));
        showToast(errMsg, true);
    };

    if (isPhone) {
        QString phone = m_phoneEdit->text().trimmed();
        data[QStringLiteral("phone")] = phone;
        data[QStringLiteral("username")] = phone;
        LoginApi::instance().registerByPhone(data, onSuccess, onError);
    } else {
        QString email = m_emailEdit->text().trimmed();
        data[QStringLiteral("email")] = email;
        LoginApi::instance().registerByEmail(data, onSuccess, onError);
    }
}

void RegisterWidget::startCountdown()
{
    m_countdownSec = 60;
    m_sendCodeBtn->setEnabled(false);
    m_sendCodeBtn->setText(QStringLiteral("重新发送(60s)"));
    m_countdownTimer->start(1000);
}

void RegisterWidget::showToast(const QString &msg, bool isError)
{
    m_toastLabel->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: %1;").arg(isError ? QStringLiteral("#e74c3c") : QStringLiteral("#27ae60")));
    m_toastLabel->setText(msg);
    m_toastLabel->setVisible(true);
    QTimer::singleShot(3000, m_toastLabel, [this]() {
        m_toastLabel->setVisible(false);
    });
}
