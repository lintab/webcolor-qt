/* ============================================================
   ProfileDialog.h —— 个人资料弹窗（对齐 App 编辑资料功能）
   ============================================================ */

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QJsonObject>

class ProfileDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProfileDialog(QWidget *parent = nullptr);

private:
    void loadProfile();
    void fillFormFromUser();
    void setEditMode(bool edit);
    void saveProfile();
    void showToast(const QString &msg, bool isError = false);

    // 可编辑字段
    QLineEdit *m_nickEdit = nullptr;
    QLineEdit *m_phoneEdit = nullptr;
    QLineEdit *m_emailEdit = nullptr;
    QLineEdit *m_supplierEdit = nullptr;
    QLineEdit *m_customBrandEdit = nullptr;
    QComboBox *m_sexCombo = nullptr;
    QComboBox *m_provinceCombo = nullptr;
    QComboBox *m_brandCombo = nullptr;

    // 只读标签
    QLabel *m_nickLabel = nullptr;
    QLabel *m_phoneLabel = nullptr;
    QLabel *m_emailLabel = nullptr;
    QLabel *m_sexLabel = nullptr;
    QLabel *m_provinceLabel = nullptr;
    QLabel *m_brandLabel = nullptr;
    QLabel *m_supplierLabel = nullptr;
    QLabel *m_customBrandLabel = nullptr;

    // 按钮
    QPushButton *m_editBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;

    QJsonObject m_user;
    bool m_hasPhone = false;
    bool m_hasEmail = false;

    // 省份和品牌选项
    static QStringList provinceList();
    static QStringList brandList();
    static const QString BRAND_CUSTOM;
};
