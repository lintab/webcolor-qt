/* ============================================================
   RegisterWidget.cpp —— 注册页面实现
   ============================================================ */

#include "RegisterWidget.h"
#include "api/LoginApi.h"
#include "utils/I18nManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QJsonArray>
#include <QMessageBox>
#include <QHash>
#include <QMap>

namespace {

// 下拉框显示文本映射：数据值（currentData）保持中文，
// 因为省份/国家的显隐联动与后端字段都依赖中文值，仅显示文本随语言变化。
QHash<QString, QString> countryDisplayNames()
{
    return {
        {QStringLiteral("中国"), T("register_country_china")},
        {QStringLiteral("美国"), T("register_country_usa")},
        {QStringLiteral("英国"), T("register_country_uk")},
        {QStringLiteral("德国"), T("register_country_germany")},
        {QStringLiteral("法国"), T("register_country_france")},
        {QStringLiteral("日本"), T("register_country_japan")},
        {QStringLiteral("韩国"), T("register_country_korea")},
        {QStringLiteral("澳大利亚"), T("register_country_australia")},
        {QStringLiteral("加拿大"), T("register_country_canada")},
        {QStringLiteral("新加坡"), T("register_country_singapore")},
        {QStringLiteral("马来西亚"), T("register_country_malaysia")},
        {QStringLiteral("泰国"), T("register_country_thailand")},
        {QStringLiteral("印度"), T("register_country_india")},
        {QStringLiteral("俄罗斯"), T("register_country_russia")},
        {QStringLiteral("意大利"), T("register_country_italy")},
        {QStringLiteral("西班牙"), T("register_country_spain")},
        {QStringLiteral("荷兰"), T("register_country_netherlands")},
        {QStringLiteral("瑞士"), T("register_country_switzerland")},
        {QStringLiteral("阿联酋"), T("register_country_uae")},
        {QStringLiteral("沙特阿拉伯"), T("register_country_saudi")},
        {QStringLiteral("土耳其"), T("register_country_turkey")},
        {QStringLiteral("越南"), T("register_country_vietnam")},
        {QStringLiteral("印度尼西亚"), T("register_country_indonesia")},
        {QStringLiteral("菲律宾"), T("register_country_philippines")},
        {QStringLiteral("巴西"), T("register_country_brazil")},
        {QStringLiteral("墨西哥"), T("register_country_mexico")},
        {QStringLiteral("南非"), T("register_country_south_africa")},
        {QStringLiteral("新西兰"), T("register_country_new_zealand")},
        {QStringLiteral("其他"), T("register_country_other")}
    };
}

QHash<QString, QString> provinceDisplayNames()
{
    return {
        {QStringLiteral("北京市"), T("register_province_beijing")},
        {QStringLiteral("天津市"), T("register_province_tianjin")},
        {QStringLiteral("河北省"), T("register_province_hebei")},
        {QStringLiteral("山西省"), T("register_province_shanxi")},
        {QStringLiteral("内蒙古自治区"), T("register_province_inner_mongolia")},
        {QStringLiteral("辽宁省"), T("register_province_liaoning")},
        {QStringLiteral("吉林省"), T("register_province_jilin")},
        {QStringLiteral("黑龙江省"), T("register_province_heilongjiang")},
        {QStringLiteral("上海市"), T("register_province_shanghai")},
        {QStringLiteral("江苏省"), T("register_province_jiangsu")},
        {QStringLiteral("浙江省"), T("register_province_zhejiang")},
        {QStringLiteral("安徽省"), T("register_province_anhui")},
        {QStringLiteral("福建省"), T("register_province_fujian")},
        {QStringLiteral("江西省"), T("register_province_jiangxi")},
        {QStringLiteral("山东省"), T("register_province_shandong")},
        {QStringLiteral("河南省"), T("register_province_henan")},
        {QStringLiteral("湖北省"), T("register_province_hubei")},
        {QStringLiteral("湖南省"), T("register_province_hunan")},
        {QStringLiteral("广东省"), T("register_province_guangdong")},
        {QStringLiteral("广西壮族自治区"), T("register_province_guangxi")},
        {QStringLiteral("海南省"), T("register_province_hainan")},
        {QStringLiteral("重庆市"), T("register_province_chongqing")},
        {QStringLiteral("四川省"), T("register_province_sichuan")},
        {QStringLiteral("贵州省"), T("register_province_guizhou")},
        {QStringLiteral("云南省"), T("register_province_yunnan")},
        {QStringLiteral("西藏自治区"), T("register_province_tibet")},
        {QStringLiteral("陕西省"), T("register_province_shaanxi")},
        {QStringLiteral("甘肃省"), T("register_province_gansu")},
        {QStringLiteral("青海省"), T("register_province_qinghai")},
        {QStringLiteral("宁夏回族自治区"), T("register_province_ningxia")},
        {QStringLiteral("新疆维吾尔自治区"), T("register_province_xinjiang")},
        {QStringLiteral("台湾省"), T("register_province_taiwan")},
        {QStringLiteral("香港特别行政区"), T("register_province_hong_kong")},
        {QStringLiteral("澳门特别行政区"), T("register_province_macao")}
    };
}

} // namespace

const QString RegisterWidget::BRAND_CUSTOM = QStringLiteral("自定义");
const QString RegisterWidget::COUNTRY_CHINA = QStringLiteral("中国");

QString RegisterWidget::countryDisplayName(const QString &value)
{
    return countryDisplayNames().value(value, value);
}

QString RegisterWidget::provinceDisplayName(const QString &value)
{
    return provinceDisplayNames().value(value, value);
}

QString RegisterWidget::brandDisplayName(const QString &value)
{
    // 品牌是后端 brand 字段的固定中文枚举，下拉提交值不变，只有显示文本随语言走
    static const QMap<QString, const char *> labels = {
        {QStringLiteral("猛鹰"),   "register_brand_mengying"},
        {QStringLiteral("雄途"),   "register_brand_xiongtu"},
        {QStringLiteral("恺祁"),   "register_brand_kaiqi"},
        {QStringLiteral("施卡斯"), "register_brand_shikasi"},
        {QStringLiteral("篮彩星"), "register_brand_lancaixing"},
        {QStringLiteral("蓝彩金"), "register_brand_lancaixing"},   // 早期资料里的旧写法
        {QStringLiteral("击剑"),   "register_brand_jijian"},
        {QStringLiteral("速航"),   "register_brand_suhang"},
        {QStringLiteral("帕特美"), "register_brand_patemei"},
        {RegisterWidget::BRAND_CUSTOM, "register_custom_brand"}
    };
    const QString key = value.trimmed();
    return labels.contains(key) ? T(labels.value(key)) : key;
}

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
        QStringLiteral("施卡斯"), QStringLiteral("篮彩星"), QStringLiteral("击剑"),
        QStringLiteral("速航"), QStringLiteral("帕特美"), BRAND_CUSTOM
    };
}

RegisterWidget::RegisterWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(T("register_title"));
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
    auto *titleLabel = new QLabel(T("register_create_account"));
    titleLabel->setStyleSheet(QStringLiteral(
        "font-size: 24px; font-weight: 700; color: #ffffff;"));
    auto *subtitleLabel = new QLabel(T("register_subtitle"));
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

    auto *phoneTab = new QPushButton(T("register_phone_tab"));
    auto *emailTab = new QPushButton(T("register_email_tab"));
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
    m_phoneEdit->setPlaceholderText(T("register_phone_placeholder"));
    m_phoneEdit->setStyleSheet(inputStyle);
    m_phoneEdit->setMaxLength(11);
    formLayout->addWidget(m_phoneEdit);

    // ===== 邮箱输入 =====
    m_emailEdit = new QLineEdit;
    m_emailEdit->setPlaceholderText(T("register_email_placeholder"));
    m_emailEdit->setStyleSheet(inputStyle);
    m_emailEdit->setMaxLength(50);
    m_emailEdit->setVisible(false);
    formLayout->addWidget(m_emailEdit);

    // ===== 验证码输入 + 发送按钮 =====
    auto *codeRow = new QHBoxLayout;
    codeRow->setSpacing(8);
    m_codeEdit = new QLineEdit;
    m_codeEdit->setPlaceholderText(T("register_code_placeholder"));
    m_codeEdit->setStyleSheet(inputStyle);
    m_codeEdit->setMaxLength(6);
    m_sendCodeBtn = new QPushButton(T("register_send_code"));
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
    m_nickNameEdit->setPlaceholderText(T("register_nickname_placeholder"));
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
    m_passwordEdit->setPlaceholderText(T("register_password_placeholder"));
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
    m_confirmPasswordEdit->setPlaceholderText(T("register_confirm_password_placeholder"));
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
    m_countryCombo->addItem(T("register_select_country"), QString());
    for (const auto &c : countryList())
        m_countryCombo->addItem(countryDisplayName(c), c);
    // 默认选中中国
    m_countryCombo->setCurrentIndex(m_countryCombo->findData(COUNTRY_CHINA));
    formLayout->addWidget(m_countryCombo);

    // ===== 省份 =====
    m_provinceCombo = new QComboBox;
    m_provinceCombo->setStyleSheet(comboStyle);
    m_provinceCombo->addItem(T("register_select_province"), QString());
    for (const auto &p : provinceList())
        m_provinceCombo->addItem(provinceDisplayName(p), p);
    formLayout->addWidget(m_provinceCombo);

    // ===== 品牌 =====
    m_brandCombo = new QComboBox;
    m_brandCombo->setStyleSheet(comboStyle);
    m_brandCombo->addItem(T("register_select_brand"), QString());
    for (const auto &b : brandList())
        m_brandCombo->addItem(brandDisplayName(b), b);
    formLayout->addWidget(m_brandCombo);

    // ===== 自定义品牌 =====
    m_customBrandEdit = new QLineEdit;
    m_customBrandEdit->setPlaceholderText(T("register_custom_brand_placeholder"));
    m_customBrandEdit->setStyleSheet(inputStyle);
    m_customBrandEdit->setMaxLength(50);
    m_customBrandEdit->setVisible(false);
    formLayout->addWidget(m_customBrandEdit);

    // ===== 供应商 =====
    m_supplierEdit = new QLineEdit;
    m_supplierEdit->setPlaceholderText(T("register_supplier_placeholder"));
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
    m_registerBtn = new QPushButton(T("register_btn"));
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
    auto *backLabel = new QLabel(T("register_have_account"));
    backLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #606266;"));
    auto *backBtn = new QPushButton(T("register_login_now"));
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
            m_sendCodeBtn->setText(T("register_send_code"));
        } else {
            m_sendCodeBtn->setText(T("register_resend_countdown").arg(m_countdownSec));
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
            showToast(T("register_invalid_phone"), true);
            return;
        }
    } else {
        target = m_emailEdit->text().trimmed();
        if (!target.contains(QStringLiteral("@")) || !target.contains(QStringLiteral("."))) {
            showToast(T("register_invalid_email"), true);
            return;
        }
    }

    m_sendCodeBtn->setEnabled(false);

    auto onSuccess = [this, isPhone](const QJsonObject &) {
        showToast(isPhone ? T("register_sms_sent") : T("register_email_sent"));
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
            showToast(T("register_invalid_phone"), true);
            return;
        }
    } else {
        QString email = m_emailEdit->text().trimmed();
        if (!email.contains(QStringLiteral("@")) || !email.contains(QStringLiteral("."))) {
            showToast(T("register_invalid_email"), true);
            return;
        }
    }

    QString code = m_codeEdit->text().trimmed();
    if (code.isEmpty()) {
        showToast(T("register_code_placeholder"), true);
        return;
    }

    QString nickName = m_nickNameEdit->text().trimmed();
    if (nickName.isEmpty()) {
        showToast(T("register_nickname_placeholder"), true);
        return;
    }

    QString password = m_passwordEdit->text();
    if (password.isEmpty()) {
        showToast(T("register_password_placeholder"), true);
        return;
    }

    QString confirmPassword = m_confirmPasswordEdit->text();
    if (confirmPassword.isEmpty()) {
        showToast(T("register_confirm_password_placeholder"), true);
        return;
    }
    if (password != confirmPassword) {
        showToast(T("register_password_mismatch"), true);
        return;
    }

    QString country = m_countryCombo->currentData().toString();
    if (country.isEmpty()) {
        showToast(T("register_select_country"), true);
        return;
    }

    QString brand = m_brandCombo->currentData().toString();
    if (brand.isEmpty()) {
        showToast(T("register_select_brand"), true);
        return;
    }
    if (brand == BRAND_CUSTOM) {
        brand = m_customBrandEdit->text().trimmed();
        if (brand.isEmpty()) {
            showToast(T("register_custom_brand_placeholder"), true);
            return;
        }
    }

    QString supplier = m_supplierEdit->text().trimmed();
    if (supplier.isEmpty()) {
        showToast(T("register_supplier_placeholder"), true);
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
    m_registerBtn->setText(T("register_submitting"));

    auto onSuccess = [this, isPhone](const QJsonObject &) {
        m_registerBtn->setEnabled(true);
        m_registerBtn->setText(T("register_btn"));
        QString username = isPhone ? m_phoneEdit->text().trimmed() : m_emailEdit->text().trimmed();
        QMessageBox::information(this, T("register_success"),
            T("register_success_detail").arg(username));
        emit backToLogin();
    };
    auto onError = [this](const QString &errMsg, int) {
        m_registerBtn->setEnabled(true);
        m_registerBtn->setText(T("register_btn"));
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
    m_sendCodeBtn->setText(T("register_resend_countdown").arg(60));
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
