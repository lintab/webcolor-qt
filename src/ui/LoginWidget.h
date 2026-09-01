/* ============================================================
   LoginWidget.h —— 登录页面 UI
   - 蓝色渐变背景 + 居中白色卡片
   - 用户名/密码/验证码输入、记住我、用户协议
   ============================================================ */

#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QCheckBox;
class QLabel;
class QFrame;

class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWidget(QWidget *parent = nullptr);

signals:
    void loginSuccess();

private slots:
    void handleLogin();
    void loadCaptcha();

private:
    void setupUi();
    void loadRemembered();
    void saveRemembered();
    bool validate();
    void showToast(const QString &msg);

    // UI 元素
    QFrame    *m_card;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_codeEdit;
    QLabel    *m_captchaLabel;
    QWidget   *m_captchaRow;
    QCheckBox *m_rememberCheck;
    QCheckBox *m_agreeCheck;
    QPushButton *m_loginBtn;
    QLabel    *m_agreeTip;
    QWidget   *m_loadingOverlay;

    // 验证码状态
    bool m_captchaEnabled = true;
    QString m_captchaUuid;
};
