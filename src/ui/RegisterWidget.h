/* ============================================================
   RegisterWidget.h —— 注册页面
   ============================================================ */

#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
#include <QJsonObject>

class RegisterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterWidget(QWidget *parent = nullptr);

signals:
    void backToLogin();

private:
    void switchMode(const QString &mode);
    void sendCode();
    void doRegister();
    void showToast(const QString &msg, bool isError = false);
    void startCountdown();

    // 模式切换
    QString m_mode = QStringLiteral("phone"); // phone / email

    // 输入框
    QLineEdit *m_phoneEdit = nullptr;
    QLineEdit *m_emailEdit = nullptr;
    QLineEdit *m_codeEdit = nullptr;
    QLineEdit *m_nickNameEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLineEdit *m_confirmPasswordEdit = nullptr;
    QLineEdit *m_supplierEdit = nullptr;
    QLineEdit *m_customBrandEdit = nullptr;

    // 下拉框
    QComboBox *m_countryCombo = nullptr;
    QComboBox *m_provinceCombo = nullptr;
    QComboBox *m_brandCombo = nullptr;

    // 按钮
    QPushButton *m_sendCodeBtn = nullptr;
    QPushButton *m_registerBtn = nullptr;
    QPushButton *m_pwdToggleBtn = nullptr;
    QPushButton *m_confirmPwdToggleBtn = nullptr;

    // 标签
    QLabel *m_phoneLabel = nullptr;
    QLabel *m_emailLabel = nullptr;
    QLabel *m_provinceLabel = nullptr;
    QLabel *m_customBrandLabel = nullptr;

    // 倒计时
    QTimer *m_countdownTimer = nullptr;
    int m_countdownSec = 0;

    // 提示
    QLabel *m_toastLabel = nullptr;

    // 省份和品牌选项
    static QStringList countryList();
    static QStringList provinceList();
    static QStringList brandList();
    static const QString BRAND_CUSTOM;
    static const QString COUNTRY_CHINA;
};
