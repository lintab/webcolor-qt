/* ============================================================
   ProfileDialog.cpp —— 个人资料弹窗（对齐 App 编辑资料功能）
   ============================================================ */

#include "ProfileDialog.h"
#include "utils/Auth.h"
#include "api/LoginApi.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

const QString ProfileDialog::BRAND_CUSTOM = QStringLiteral("自定义");

QStringList ProfileDialog::provinceList()
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

QStringList ProfileDialog::brandList()
{
    return {
        QStringLiteral("猛鹰"), QStringLiteral("雄途"), QStringLiteral("恺祁"),
        QStringLiteral("施卡斯"), QStringLiteral("蓝彩金"), QStringLiteral("击剑"),
        QStringLiteral("速航"), QStringLiteral("帕特美"), BRAND_CUSTOM
    };
}

ProfileDialog::ProfileDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("个人资料"));
    setFixedSize(460, 600);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 16);
    mainLayout->setSpacing(8);

    setStyleSheet(QStringLiteral(
        "QDialog { background: #ffffff; }"
        "QLabel { color: #1a2332; }"
        "QLineEdit { background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 8px; "
        "padding: 8px 12px; font-size: 14px; color: #1a2332; }"
        "QLineEdit:focus { border-color: #4a9ede; background: #ffffff; }"
        "QLineEdit:disabled { background: #eef2f6; border: none; padding: 8px 0; "
        "color: #909399; font-weight: 500; }"
        "QComboBox { background: #f5f7fa; border: 1px solid #dce0e6; border-radius: 8px; "
        "padding: 6px 28px 6px 10px; font-size: 14px; color: #1a2332; min-height: 22px; }"
        "QComboBox:hover { border-color: #4a9ede; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow { image: none; border: none; "
        "border-left: 4px solid transparent; border-right: 4px solid transparent; "
        "border-top: 5px solid #606266; margin-right: 6px; }"
        "QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #e0e3e8; "
        "border-radius: 6px; selection-background-color: #e8f4fd; selection-color: #0d4f8b; }"
        "QComboBox:disabled { background: #eef2f6; color: #909399; }"));

    // ===== 头像 =====
    auto *avatarLayout = new QHBoxLayout;
    avatarLayout->setAlignment(Qt::AlignCenter);
    auto *avatarLabel = new QLabel;
    avatarLabel->setFixedSize(64, 64);
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(QStringLiteral(
        "background: #e8f4fd; color: #0d4f8b; font-size: 26px; font-weight: 700; "
        "border-radius: 32px; border: 2px solid #b8ddf5;"));

    m_user = Auth::instance().getUser();
    QString nickName = m_user.value(QStringLiteral("nickName")).toString();
    if (nickName.isEmpty())
        nickName = m_user.value(QStringLiteral("userName")).toString();
    if (nickName.isEmpty())
        nickName = QStringLiteral("用");
    avatarLabel->setText(nickName.left(1).toUpper());
    avatarLayout->addWidget(avatarLabel);
    mainLayout->addLayout(avatarLayout);

    // ===== 滚动区域 =====
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral("QScrollArea { border: none; }"));

    auto *scrollWidget = new QWidget;
    auto *formLayout = new QVBoxLayout(scrollWidget);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(6);

    // 辅助函数：创建表单行
    auto addRow = [&](const QString &label, QWidget *editWidget, QLabel **outLabel) {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 2, 0, 2);

        auto *labelW = new QLabel(label);
        labelW->setFixedWidth(70);
        labelW->setStyleSheet(QStringLiteral("font-size: 13px; color: #909399;"));

        // 只读标签
        auto *valueLabel = new QLabel;
        valueLabel->setStyleSheet(QStringLiteral(
            "font-size: 14px; color: #1a2332; font-weight: 500;"));
        valueLabel->setWordWrap(true);

        row->addWidget(labelW);
        row->addWidget(valueLabel, 1);
        row->addWidget(editWidget, 1);
        editWidget->setVisible(false);
        formLayout->addLayout(row);

        if (outLabel) *outLabel = valueLabel;
        return valueLabel;
    };

    // 用户名（只读）
    {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 2, 0, 2);
        auto *labelW = new QLabel(QStringLiteral("用户名"));
        labelW->setFixedWidth(70);
        labelW->setStyleSheet(QStringLiteral("font-size: 13px; color: #909399;"));
        auto *valueW = new QLabel(m_user.value(QStringLiteral("userName")).toString());
        valueW->setStyleSheet(QStringLiteral("font-size: 14px; color: #1a2332; font-weight: 500;"));
        row->addWidget(labelW);
        row->addWidget(valueW, 1);
        formLayout->addLayout(row);
    }

    // 昵称
    m_nickEdit = new QLineEdit;
    addRow(QStringLiteral("昵称"), m_nickEdit, &m_nickLabel);

    // 手机号
    m_phoneEdit = new QLineEdit;
    addRow(QStringLiteral("手机号"), m_phoneEdit, &m_phoneLabel);

    // 邮箱
    m_emailEdit = new QLineEdit;
    addRow(QStringLiteral("邮箱"), m_emailEdit, &m_emailLabel);

    // 性别
    m_sexCombo = new QComboBox;
    m_sexCombo->addItem(QStringLiteral("男"), QStringLiteral("0"));
    m_sexCombo->addItem(QStringLiteral("女"), QStringLiteral("1"));
    addRow(QStringLiteral("性别"), m_sexCombo, &m_sexLabel);

    // 省份
    m_provinceCombo = new QComboBox;
    m_provinceCombo->addItem(QStringLiteral("请选择"), QString());
    for (const auto &p : provinceList())
        m_provinceCombo->addItem(p, p);
    addRow(QStringLiteral("省份"), m_provinceCombo, &m_provinceLabel);

    // 品牌
    m_brandCombo = new QComboBox;
    m_brandCombo->addItem(QStringLiteral("请选择"), QString());
    for (const auto &b : brandList())
        m_brandCombo->addItem(b, b);
    addRow(QStringLiteral("品牌"), m_brandCombo, &m_brandLabel);

    // 自定义品牌
    m_customBrandEdit = new QLineEdit;
    m_customBrandEdit->setPlaceholderText(QStringLiteral("请输入自定义品牌名"));
    addRow(QStringLiteral("自定义品牌"), m_customBrandEdit, &m_customBrandLabel);

    // 供应商
    m_supplierEdit = new QLineEdit;
    m_supplierEdit->setPlaceholderText(QStringLiteral("请输入供应商"));
    addRow(QStringLiteral("供应商"), m_supplierEdit, &m_supplierLabel);

    formLayout->addStretch();
    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea, 1);

    // ===== 提示标签 =====
    auto *toastLabel = new QLabel;
    toastLabel->setAlignment(Qt::AlignCenter);
    toastLabel->setStyleSheet(QStringLiteral("font-size: 13px;"));
    toastLabel->setVisible(false);
    mainLayout->addWidget(toastLabel);

    // ===== 按钮 =====
    auto *btnLayout = new QHBoxLayout;
    btnLayout->setAlignment(Qt::AlignCenter);
    btnLayout->setSpacing(12);

    auto makeBtn = [&](const QString &text, const QString &bg, const QString &color,
                       const QString &border) -> QPushButton * {
        auto *btn = new QPushButton(text);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(100, 36);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background: %1; color: %2; font-size: 14px; font-weight: 600; "
            "border: 1px solid %3; border-radius: 18px; }"
            "QPushButton:hover { opacity: 0.85; }").arg(bg, color, border));
        return btn;
    };

    m_editBtn = makeBtn(QStringLiteral("编辑"), QStringLiteral("#e8f4fd"),
                        QStringLiteral("#0d4f8b"), QStringLiteral("#b8ddf5"));
    m_saveBtn = makeBtn(QStringLiteral("保存"), QStringLiteral("#4a9ede"),
                        QStringLiteral("#ffffff"), QStringLiteral("#4a9ede"));
    m_cancelBtn = makeBtn(QStringLiteral("取消"), QStringLiteral("#f5f7fa"),
                          QStringLiteral("#606266"), QStringLiteral("#dce0e6"));
    m_saveBtn->setVisible(false);
    m_cancelBtn->setVisible(false);

    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    // ===== 信号连接 =====
    connect(m_editBtn, &QPushButton::clicked, this, [this]() {
        setEditMode(true);
    });
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        fillFormFromUser();
        setEditMode(false);
    });
    connect(m_saveBtn, &QPushButton::clicked, this, &ProfileDialog::saveProfile);

    // 品牌选择变化时显示/隐藏自定义品牌
    connect(m_brandCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        bool showCustom = (m_brandCombo->currentData().toString() == BRAND_CUSTOM);
        m_customBrandEdit->setVisible(showCustom && m_saveBtn->isVisible());
        m_customBrandLabel->setVisible(showCustom && !m_saveBtn->isVisible());
    });

    // showToast lambda
    auto doShowToast = [toastLabel](const QString &msg, bool isError) {
        toastLabel->setStyleSheet(QStringLiteral(
            "font-size: 13px; color: %1;").arg(isError ? QStringLiteral("#e74c3c") : QStringLiteral("#27ae60")));
        toastLabel->setText(msg);
        toastLabel->setVisible(true);
        QTimer::singleShot(2500, toastLabel, [toastLabel]() {
            toastLabel->setVisible(false);
        });
    };
    this->setProperty("_toastFn", QVariant::fromValue(std::function<void(const QString&, bool)>(doShowToast)));

    // 初始填充数据
    fillFormFromUser();
    // 从服务器加载最新数据
    loadProfile();
}

void ProfileDialog::fillFormFromUser()
{
    QString phone = m_user.value(QStringLiteral("phonenumber")).toString();
    QString email = m_user.value(QStringLiteral("email")).toString();
    m_hasPhone = !phone.trimmed().isEmpty();
    m_hasEmail = !email.trimmed().isEmpty();

    m_nickLabel->setText(m_user.value(QStringLiteral("nickName")).toString());
    m_nickEdit->setText(m_user.value(QStringLiteral("nickName")).toString());

    m_phoneLabel->setText(m_hasPhone ? phone : QStringLiteral("未绑定"));
    m_phoneEdit->setText(phone);
    m_phoneEdit->setEnabled(m_hasPhone);

    m_emailLabel->setText(m_hasEmail ? email : QStringLiteral("未绑定"));
    m_emailEdit->setText(email);
    m_emailEdit->setEnabled(m_hasEmail);

    // 性别
    QString sex = m_user.value(QStringLiteral("sex")).toString();
    m_sexLabel->setText(sex == QStringLiteral("1") ? QStringLiteral("女") : QStringLiteral("男"));
    m_sexCombo->setCurrentIndex(sex == QStringLiteral("1") ? 1 : 0);

    // 省份
    QString province = m_user.value(QStringLiteral("province")).toString();
    m_provinceLabel->setText(province);
    int pIdx = m_provinceCombo->findData(province);
    m_provinceCombo->setCurrentIndex(pIdx >= 0 ? pIdx : 0);

    // 品牌
    QString brand = m_user.value(QStringLiteral("brand")).toString();
    QString customBrand;
    if (brand == BRAND_CUSTOM) {
        customBrand = brand;
        m_brandLabel->setText(BRAND_CUSTOM);
    } else {
        m_brandLabel->setText(brand);
    }
    int bIdx = m_brandCombo->findData(brand);
    m_brandCombo->setCurrentIndex(bIdx >= 0 ? bIdx : 0);
    m_customBrandEdit->setText(customBrand);
    m_customBrandLabel->setText(customBrand);

    // 供应商
    QString supplier = m_user.value(QStringLiteral("supplier")).toString();
    m_supplierLabel->setText(supplier);
    m_supplierEdit->setText(supplier);
}

void ProfileDialog::loadProfile()
{
    LoginApi::instance().getProfile(
        [this](const QJsonObject &res) {
            QJsonObject data = res.value(QStringLiteral("data")).toObject();
            if (data.isEmpty()) data = res;
            m_user = data;
            // 更新本地缓存
            Auth::instance().setUser(data);
            fillFormFromUser();
        },
        [this](const QString &errMsg, int) {
            showToast(errMsg, true);
        });
}

void ProfileDialog::setEditMode(bool edit)
{
    // 标签
    m_nickLabel->setVisible(!edit);
    m_phoneLabel->setVisible(!edit);
    m_emailLabel->setVisible(!edit);
    m_sexLabel->setVisible(!edit);
    m_provinceLabel->setVisible(!edit);
    m_brandLabel->setVisible(!edit);
    m_supplierLabel->setVisible(!edit);

    // 编辑框
    m_nickEdit->setVisible(edit);
    m_phoneEdit->setVisible(edit);
    m_emailEdit->setVisible(edit);
    m_sexCombo->setVisible(edit);
    m_provinceCombo->setVisible(edit);
    m_brandCombo->setVisible(edit);
    m_supplierEdit->setVisible(edit);

    // 自定义品牌
    bool showCustom = (m_brandCombo->currentData().toString() == BRAND_CUSTOM);
    m_customBrandLabel->setVisible(!edit && showCustom);
    m_customBrandEdit->setVisible(edit && showCustom);

    // 按钮
    m_editBtn->setVisible(!edit);
    m_saveBtn->setVisible(edit);
    m_cancelBtn->setVisible(edit);
}

void ProfileDialog::saveProfile()
{
    QString brand = m_brandCombo->currentData().toString();
    if (brand == BRAND_CUSTOM) {
        brand = m_customBrandEdit->text().trimmed();
    }

    QJsonObject data;
    data[QStringLiteral("nickName")] = m_nickEdit->text().trimmed();
    data[QStringLiteral("sex")] = m_sexCombo->currentData().toString();
    data[QStringLiteral("province")] = m_provinceCombo->currentData().toString();
    data[QStringLiteral("brand")] = brand;
    data[QStringLiteral("supplier")] = m_supplierEdit->text().trimmed();

    if (m_hasPhone)
        data[QStringLiteral("phonenumber")] = m_phoneEdit->text().trimmed();
    if (m_hasEmail)
        data[QStringLiteral("email")] = m_emailEdit->text().trimmed();

    m_saveBtn->setEnabled(false);
    m_saveBtn->setText(QStringLiteral("保存中..."));

    LoginApi::instance().updateProfile(data,
        [this, data](const QJsonObject &) {
            // 更新本地缓存
            QJsonObject user = Auth::instance().getUser();
            user[QStringLiteral("nickName")] = data[QStringLiteral("nickName")];
            user[QStringLiteral("sex")] = data[QStringLiteral("sex")];
            user[QStringLiteral("province")] = data[QStringLiteral("province")];
            user[QStringLiteral("brand")] = data[QStringLiteral("brand")];
            user[QStringLiteral("supplier")] = data[QStringLiteral("supplier")];
            if (m_hasPhone)
                user[QStringLiteral("phonenumber")] = data[QStringLiteral("phonenumber")];
            if (m_hasEmail)
                user[QStringLiteral("email")] = data[QStringLiteral("email")];
            Auth::instance().setUser(user);
            m_user = user;

            fillFormFromUser();
            m_saveBtn->setEnabled(true);
            m_saveBtn->setText(QStringLiteral("保存"));
            setEditMode(false);
            showToast(QStringLiteral("保存成功"));
        },
        [this](const QString &errMsg, int) {
            m_saveBtn->setEnabled(true);
            m_saveBtn->setText(QStringLiteral("保存"));
            showToast(errMsg, true);
        });
}

void ProfileDialog::showToast(const QString &msg, bool isError)
{
    auto fn = property("_toastFn").value<std::function<void(const QString&, bool)>>();
    if (fn) fn(msg, isError);
}
