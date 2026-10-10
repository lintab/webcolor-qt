/* ============================================================
   I18nManager.cpp —— 国际化翻译管理器实现
   - 内嵌所有语言的翻译数据，无需外部 JSON 文件
   ============================================================ */

#include "I18nManager.h"
#include <QSettings>
#include <QCoreApplication>

I18nManager::I18nManager(QObject *parent)
    : QObject(parent)
{
    m_currentLang = QSettings().value(QStringLiteral("language"), QStringLiteral("zh")).toString();
    buildInMemoryTranslations();
    loadLanguage(m_currentLang);
}

I18nManager &I18nManager::instance()
{
    static I18nManager inst;
    return inst;
}

QString I18nManager::tr(const QString &key, const QString &fallback) const
{
    auto it = m_translations.constFind(key);
    if (it != m_translations.constEnd())
        return it.value();
    auto zhIt = m_fallbackZh.constFind(key);
    if (zhIt != m_fallbackZh.constEnd())
        return zhIt.value();
    return fallback.isEmpty() ? key : fallback;
}

QString I18nManager::currentLanguage() const
{
    return m_currentLang;
}

void I18nManager::setLanguage(const QString &langCode)
{
    if (m_currentLang == langCode) return;
    m_currentLang = langCode;
    QSettings().setValue(QStringLiteral("language"), langCode);
    loadLanguage(langCode);
    emit languageChanged();
}

QMap<QString, QString> I18nManager::supportedLanguages() const
{
    return {
        {QStringLiteral("zh"), QStringLiteral("中文")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("ru"), QStringLiteral("Русский")},
        {QStringLiteral("fr"), QStringLiteral("Français")},
        {QStringLiteral("ar"), QStringLiteral("العربية")},
        {QStringLiteral("vi"), QStringLiteral("Tiếng Việt")},
    };
}

void I18nManager::loadLanguage(const QString &langCode)
{
    m_translations.clear();
    // m_allTranslations: langCode -> { key -> value }
    static QMap<QString, QMap<QString, QString>> allTranslations;
    if (allTranslations.isEmpty()) {
        buildInMemoryTranslations();
        // 这里不需要再调用，因为 buildInMemoryTranslations 已经填充了 allTranslations
    }
    // 实际上我们在 buildInMemoryTranslations 中已经为每种语言构建好了
    // 这里直接从预设数据加载
    // 为了简化，我们在构造函数中为每种语言构建翻译表
    // 存储在静态变量中
    static QMap<QString, QMap<QString, QString>> langMap;
    if (langMap.isEmpty()) {
        // ============ 中文 zh（界面原文，作为默认语言与回退）============
        auto &zh = langMap[QStringLiteral("zh")];
        // -- MainWindow --
        zh[QStringLiteral("app_title")] = QStringLiteral("恺祁颜色系统");
        zh[QStringLiteral("user")] = QStringLiteral("用户");
        zh[QStringLiteral("avatar_char")] = QStringLiteral("用");
        zh[QStringLiteral("profile")] = QStringLiteral("个人资料");
        zh[QStringLiteral("logout")] = QStringLiteral("退出登录");
        zh[QStringLiteral("nav_search")] = QStringLiteral("搜索");
        zh[QStringLiteral("nav_brand")] = QStringLiteral("品牌车型");
        zh[QStringLiteral("nav_autosc")] = QStringLiteral("自动S&C");
        zh[QStringLiteral("settings")] = QStringLiteral("设置");
        zh[QStringLiteral("formula_download_settings")] = QStringLiteral("配方下载");
        zh[QStringLiteral("color_search_settings")] = QStringLiteral("颜色搜索");
        zh[QStringLiteral("software_update")] = QStringLiteral("软件更新");
        zh[QStringLiteral("feature_in_development")] = QStringLiteral("功能开发中…");
        zh[QStringLiteral("logout_confirm_title")] = QStringLiteral("确认退出");
        zh[QStringLiteral("logout_confirm_text")] = QStringLiteral("确定要退出登录吗？");
        zh[QStringLiteral("confirm")] = QStringLiteral("确定");
        zh[QStringLiteral("cancel")] = QStringLiteral("取消");

        // -- LoginWidget --
        zh[QStringLiteral("login_title")] = QStringLiteral("恺祁颜色系统");
        zh[QStringLiteral("login_subtitle")] = QStringLiteral("专业汽车颜色管理平台");
        zh[QStringLiteral("login_account_placeholder")] = QStringLiteral("请输入账号");
        zh[QStringLiteral("login_password_placeholder")] = QStringLiteral("请输入密码");
        zh[QStringLiteral("login_remember_account")] = QStringLiteral("记住账号");
        zh[QStringLiteral("login_btn")] = QStringLiteral("登录");
        zh[QStringLiteral("login_register_link")] = QStringLiteral("还没有账号？立即注册");
        zh[QStringLiteral("login_success")] = QStringLiteral("登录成功");
        zh[QStringLiteral("login_account_title")] = QStringLiteral("账号登录");
        zh[QStringLiteral("captcha")] = QStringLiteral("验证码");
        zh[QStringLiteral("remember_me")] = QStringLiteral("记住我");
        zh[QStringLiteral("login_btn_text")] = QStringLiteral("登 录");
        zh[QStringLiteral("agree_text")] = QStringLiteral("登录即表示已阅读并同意 %1 和 %2");
        zh[QStringLiteral("agree_privacy_policy")] = QStringLiteral("《隐私政策》");
        zh[QStringLiteral("agree_user_agreement")] = QStringLiteral("《用户协议》");
        zh[QStringLiteral("logging_in")] = QStringLiteral("登录中…");

        // -- RegisterWidget --
        zh[QStringLiteral("register_title")] = QStringLiteral("注册账号");
        zh[QStringLiteral("register_username_placeholder")] = QStringLiteral("请输入用户名");
        zh[QStringLiteral("register_phone_placeholder")] = QStringLiteral("请输入手机号");
        zh[QStringLiteral("register_password_placeholder")] = QStringLiteral("请输入密码（至少 6 位）");
        zh[QStringLiteral("register_confirm_password_placeholder")] = QStringLiteral("请再次输入密码");
        zh[QStringLiteral("register_company_placeholder")] = QStringLiteral("请输入公司名称");
        zh[QStringLiteral("register_btn")] = QStringLiteral("注 册");
        zh[QStringLiteral("register_login_link")] = QStringLiteral("已有账号？返回登录");
        zh[QStringLiteral("register_success")] = QStringLiteral("注册成功，请登录");
        zh[QStringLiteral("register_password_mismatch")] = QStringLiteral("两次输入的密码不一致");
        zh[QStringLiteral("register_password_short")] = QStringLiteral("密码长度不能少于 6 位");

        // -- AutoScPage --
        zh[QStringLiteral("sc_job_definition")] = QStringLiteral("作业定义");
        zh[QStringLiteral("sc_target_job")] = QStringLiteral("目标作业");
        zh[QStringLiteral("sc_select_target_job")] = QStringLiteral("请选择目标作业");
        zh[QStringLiteral("sc_measurement_job")] = QStringLiteral("测量作业");
        zh[QStringLiteral("sc_select_measurement_job")] = QStringLiteral("请选择测量数据");
        zh[QStringLiteral("sc_color_data")] = QStringLiteral("颜色数据");
        zh[QStringLiteral("sc_spectral_data")] = QStringLiteral("光谱数据");
        zh[QStringLiteral("sc_spectral_curve")] = QStringLiteral("光谱曲线图");
        zh[QStringLiteral("sc_color_block")] = QStringLiteral("色块");
        zh[QStringLiteral("sc_visualization_panel")] = QStringLiteral("可视化面板");
        zh[QStringLiteral("sc_recipe_search")] = QStringLiteral("配方搜索");
        zh[QStringLiteral("sc_search_placeholder")] = QStringLiteral("输入颜色名称 / 配方号搜索");
        zh[QStringLiteral("sc_search_btn")] = QStringLiteral("搜索");
        zh[QStringLiteral("sc_no_data")] = QStringLiteral("暂无数据");
        zh[QStringLiteral("sc_loading")] = QStringLiteral("加载中…");
        zh[QStringLiteral("sc_geometry")] = QStringLiteral("几何角度");
        zh[QStringLiteral("sc_illuminant")] = QStringLiteral("光源");
        zh[QStringLiteral("sc_color_source")] = QStringLiteral("颜色来源");
        zh[QStringLiteral("sc_formula_id")] = QStringLiteral("配方号");
        zh[QStringLiteral("sc_color_name")] = QStringLiteral("颜色名称");
        zh[QStringLiteral("sc_save_recipe")] = QStringLiteral("保存配方");
        zh[QStringLiteral("sc_load_recipe")] = QStringLiteral("载入配方");
        zh[QStringLiteral("sc_delete")] = QStringLiteral("删除");
        zh[QStringLiteral("sc_export")] = QStringLiteral("导出");
        zh[QStringLiteral("sc_import")] = QStringLiteral("导入");
        zh[QStringLiteral("sc_refresh")] = QStringLiteral("刷新");
        zh[QStringLiteral("sc_total")] = QStringLiteral("共");
        zh[QStringLiteral("sc_records")] = QStringLiteral("条");
        zh[QStringLiteral("sc_page")] = QStringLiteral("第");
        zh[QStringLiteral("sc_prev_page")] = QStringLiteral("上一页");
        zh[QStringLiteral("sc_next_page")] = QStringLiteral("下一页");
        zh[QStringLiteral("sc_correction_detail")] = QStringLiteral("修正详情");
        zh[QStringLiteral("sc_initial_color")] = QStringLiteral("原始颜色");
        zh[QStringLiteral("sc_standard_color")] = QStringLiteral("标准颜色");
        zh[QStringLiteral("sc_target_color")] = QStringLiteral("目标颜色");
        zh[QStringLiteral("sc_repaired_color")] = QStringLiteral("修复颜色");
        zh[QStringLiteral("sc_difference")] = QStringLiteral("差值");
        zh[QStringLiteral("sc_tolerance")] = QStringLiteral("容差");
        zh[QStringLiteral("sc_pass")] = QStringLiteral("合格");
        zh[QStringLiteral("sc_fail")] = QStringLiteral("不合格");
        zh[QStringLiteral("sc_type")] = QStringLiteral("类型");
        zh[QStringLiteral("sc_name")] = QStringLiteral("名称");
        zh[QStringLiteral("sc_value")] = QStringLiteral("数值");
        zh[QStringLiteral("sc_action")] = QStringLiteral("操作");
        zh[QStringLiteral("sc_status")] = QStringLiteral("状态");
        zh[QStringLiteral("sc_remark")] = QStringLiteral("备注");
        zh[QStringLiteral("sc_description")] = QStringLiteral("描述");
        zh[QStringLiteral("sc_create_time")] = QStringLiteral("创建时间");
        zh[QStringLiteral("sc_update_time")] = QStringLiteral("更新时间");
        zh[QStringLiteral("sc_operator")] = QStringLiteral("操作人");

        // -- SettingsPage --
        zh[QStringLiteral("settings_save_path")] = QStringLiteral("保存路径");
        zh[QStringLiteral("settings_browse")] = QStringLiteral("浏览");
        zh[QStringLiteral("settings_save")] = QStringLiteral("保存");
        zh[QStringLiteral("settings_saved")] = QStringLiteral("设置已保存");
        zh[QStringLiteral("settings_invalid_path")] = QStringLiteral("路径无效");

        // -- ProfileDialog --
        zh[QStringLiteral("profile_title")] = QStringLiteral("个人资料");
        zh[QStringLiteral("profile_username")] = QStringLiteral("用户名");
        zh[QStringLiteral("profile_phone")] = QStringLiteral("手机号");
        zh[QStringLiteral("profile_company")] = QStringLiteral("公司");
        zh[QStringLiteral("profile_email")] = QStringLiteral("邮箱");
        zh[QStringLiteral("profile_save")] = QStringLiteral("保存");
        zh[QStringLiteral("profile_close")] = QStringLiteral("关闭");
        zh[QStringLiteral("profile_update_success")] = QStringLiteral("资料已更新");

        // -- FormulaDetailWidget --
        zh[QStringLiteral("formula_detail")] = QStringLiteral("配方详情");
        zh[QStringLiteral("formula_id")] = QStringLiteral("配方号");
        zh[QStringLiteral("formula_color_name")] = QStringLiteral("颜色名称");
        zh[QStringLiteral("formula_layers")] = QStringLiteral("层");
        zh[QStringLiteral("formula_total")] = QStringLiteral("合计");
        zh[QStringLiteral("formula_add")] = QStringLiteral("添加层");
        zh[QStringLiteral("formula_calculate")] = QStringLiteral("计算");
        zh[QStringLiteral("formula_save")] = QStringLiteral("保存");
        zh[QStringLiteral("formula_cancel")] = QStringLiteral("取消");

        // -- UpdateWidget --
        zh[QStringLiteral("update_title")] = QStringLiteral("软件更新");
        zh[QStringLiteral("update_checking")] = QStringLiteral("正在检查更新…");
        zh[QStringLiteral("update_available")] = QStringLiteral("发现新版本");
        zh[QStringLiteral("update_latest")] = QStringLiteral("已是最新版本");
        zh[QStringLiteral("update_download")] = QStringLiteral("下载");
        zh[QStringLiteral("update_install")] = QStringLiteral("安装");
        zh[QStringLiteral("update_later")] = QStringLiteral("稍后");
        zh[QStringLiteral("update_version")] = QStringLiteral("版本");
        zh[QStringLiteral("update_size")] = QStringLiteral("大小");

        // -- ColorSearchSettingsPage --
        zh[QStringLiteral("color_search_settings_title")] = QStringLiteral("颜色搜索设置");
        zh[QStringLiteral("color_search_range")] = QStringLiteral("搜索范围");
        zh[QStringLiteral("color_search_accuracy")] = QStringLiteral("搜索精度");

        // -- BrandSeriesPage --
        zh[QStringLiteral("brand_series_title")] = QStringLiteral("品牌车型");
        zh[QStringLiteral("brand_select")] = QStringLiteral("选择品牌");
        zh[QStringLiteral("series_select")] = QStringLiteral("选择车系");
        zh[QStringLiteral("year_select")] = QStringLiteral("选择年款");
        zh[QStringLiteral("color_select")] = QStringLiteral("选择颜色");

        // -- Common --
        zh[QStringLiteral("ok")] = QStringLiteral("确定");
        zh[QStringLiteral("yes")] = QStringLiteral("是");
        zh[QStringLiteral("no")] = QStringLiteral("否");
        zh[QStringLiteral("error")] = QStringLiteral("错误");
        zh[QStringLiteral("warning")] = QStringLiteral("警告");
        zh[QStringLiteral("info")] = QStringLiteral("提示");
        zh[QStringLiteral("success")] = QStringLiteral("成功");
        zh[QStringLiteral("loading")] = QStringLiteral("加载中…");
        zh[QStringLiteral("no_data")] = QStringLiteral("暂无数据");
        zh[QStringLiteral("network_error")] = QStringLiteral("网络异常，请重试");
        zh[QStringLiteral("request_failed")] = QStringLiteral("请求失败");
        zh[QStringLiteral("solid")] = QStringLiteral("素色");
        zh[QStringLiteral("metallic")] = QStringLiteral("金属色");
        zh[QStringLiteral("pearl")] = QStringLiteral("珍珠色");
        zh[QStringLiteral("mixed")] = QStringLiteral("杂色");
        zh[QStringLiteral("tri_coat_pearl")] = QStringLiteral("三层珍珠");
        zh[QStringLiteral("candy")] = QStringLiteral("糖果色");
        zh[QStringLiteral("spectral_data")] = QStringLiteral("光谱数据");
        zh[QStringLiteral("spectral_curve")] = QStringLiteral("光谱曲线图");
        zh[QStringLiteral("color_data")] = QStringLiteral("颜色数据");
        zh[QStringLiteral("color_block")] = QStringLiteral("色块");
        zh[QStringLiteral("wavelength")] = QStringLiteral("波长（nm）");
        zh[QStringLiteral("reflectance")] = QStringLiteral("反射率（%）");

        // -- Search page --
        zh[QStringLiteral("search_placeholder")] = QStringLiteral("搜索颜色名称、色号(支持多色号)、车型");
        zh[QStringLiteral("search_btn")] = QStringLiteral("搜索");
        zh[QStringLiteral("history_label")] = QStringLiteral("历史：");
        zh[QStringLiteral("start_search_hint")] = QStringLiteral("点击左侧配方查看详情");
        zh[QStringLiteral("enter_search_keyword")] = QStringLiteral("请输入搜索关键词");
        zh[QStringLiteral("no_more")] = QStringLiteral("— 没有更多了 —");
        zh[QStringLiteral("color_code_label")] = QStringLiteral("色号：");
        zh[QStringLiteral("color_name_label")] = QStringLiteral("颜色：");
        zh[QStringLiteral("car_model_label")] = QStringLiteral("车型：");
        zh[QStringLiteral("no_match_found")] = QStringLiteral("未找到匹配的配方");
        zh[QStringLiteral("scroll_load_more")] = QStringLiteral("滚动加载更多");
        zh[QStringLiteral("register_link_text")] = QStringLiteral("还没有账号？<a href='#' style='font-size:13px;'>立即注册</a>");

        // -- Settings page --
        zh[QStringLiteral("formula_save_path_title")] = QStringLiteral("配方保存路径");
        zh[QStringLiteral("save_path_desc")] = QStringLiteral("配方文件将保存至：");
        zh[QStringLiteral("select_path_placeholder")] = QStringLiteral("请选择保存路径…");
        zh[QStringLiteral("browse_dots")] = QStringLiteral("浏览…");
        zh[QStringLiteral("save_settings_btn")] = QStringLiteral("保存设置");
        zh[QStringLiteral("select_path_dialog")] = QStringLiteral("选择保存路径");
        zh[QStringLiteral("select_path_warn")] = QStringLiteral("请选择保存路径");
        zh[QStringLiteral("path_not_exist")] = QStringLiteral("路径不存在，请重新选择");
        zh[QStringLiteral("settings_saved_check")] = QStringLiteral("✓ 设置已保存");

        // -- Update widget --
        zh[QStringLiteral("current_version")] = QStringLiteral("当前版本：");
        zh[QStringLiteral("check_update_status")] = QStringLiteral("点击检查更新");
        zh[QStringLiteral("check_update_btn")] = QStringLiteral("检查更新");
        zh[QStringLiteral("update_now_btn")] = QStringLiteral("立即更新");
        zh[QStringLiteral("close_btn")] = QStringLiteral("关闭");
        zh[QStringLiteral("checking_update")] = QStringLiteral("正在检查更新…");
        zh[QStringLiteral("latest_version_msg")] = QStringLiteral("已是最新版本");

        // ============ 英语 en ============
        auto &en = langMap[QStringLiteral("en")];
        // -- MainWindow --
        en[QStringLiteral("app_title")] = QStringLiteral("Kaiqi Color System");
        en[QStringLiteral("user")] = QStringLiteral("User");
        en[QStringLiteral("avatar_char")] = QStringLiteral("U");
        en[QStringLiteral("profile")] = QStringLiteral("Profile");
        en[QStringLiteral("logout")] = QStringLiteral("Logout");
        en[QStringLiteral("nav_search")] = QStringLiteral("Search");
        en[QStringLiteral("nav_brand")] = QStringLiteral("Brand & Model");
        en[QStringLiteral("nav_autosc")] = QStringLiteral("Auto S&C");
        en[QStringLiteral("settings")] = QStringLiteral("Settings");
        en[QStringLiteral("formula_download_settings")] = QStringLiteral("Formula Download");
        en[QStringLiteral("color_search_settings")] = QStringLiteral("Color Search");
        en[QStringLiteral("software_update")] = QStringLiteral("Software Update");
        en[QStringLiteral("feature_in_development")] = QStringLiteral("Feature in development...");
        en[QStringLiteral("logout_confirm_title")] = QStringLiteral("Confirm Logout");
        en[QStringLiteral("logout_confirm_text")] = QStringLiteral("Are you sure you want to logout?");
        en[QStringLiteral("confirm")] = QStringLiteral("Confirm");
        en[QStringLiteral("cancel")] = QStringLiteral("Cancel");

        // -- LoginWidget --
        en[QStringLiteral("login_title")] = QStringLiteral("Kaiqi Color System");
        en[QStringLiteral("login_subtitle")] = QStringLiteral("Professional automotive color management platform");
        en[QStringLiteral("login_account_placeholder")] = QStringLiteral("Please enter account");
        en[QStringLiteral("login_password_placeholder")] = QStringLiteral("Please enter password");
        en[QStringLiteral("login_remember_account")] = QStringLiteral("Remember account");
        en[QStringLiteral("login_btn")] = QStringLiteral("Login");
        en[QStringLiteral("login_register_link")] = QStringLiteral("No account? Register now");
        en[QStringLiteral("login_success")] = QStringLiteral("Login successful");
        en[QStringLiteral("login_account_title")] = QStringLiteral("Account Login");
        en[QStringLiteral("captcha")] = QStringLiteral("Captcha");
        en[QStringLiteral("remember_me")] = QStringLiteral("Remember me");
        en[QStringLiteral("login_btn_text")] = QStringLiteral("Login");
        en[QStringLiteral("agree_text")] = QStringLiteral("By signing in you confirm that you have read and accept the %1 and %2");
        en[QStringLiteral("agree_privacy_policy")] = QStringLiteral("Privacy Policy");
        en[QStringLiteral("agree_user_agreement")] = QStringLiteral("User Agreement");
        en[QStringLiteral("logging_in")] = QStringLiteral("Logging in...");

        // -- RegisterWidget --
        en[QStringLiteral("register_title")] = QStringLiteral("Register Account");
        en[QStringLiteral("register_username_placeholder")] = QStringLiteral("Please enter username");
        en[QStringLiteral("register_phone_placeholder")] = QStringLiteral("Please enter phone number");
        en[QStringLiteral("register_password_placeholder")] = QStringLiteral("Please enter password (min 6 characters)");
        en[QStringLiteral("register_confirm_password_placeholder")] = QStringLiteral("Please confirm password");
        en[QStringLiteral("register_company_placeholder")] = QStringLiteral("Please enter company name");
        en[QStringLiteral("register_btn")] = QStringLiteral("Register");
        en[QStringLiteral("register_login_link")] = QStringLiteral("Already have an account? Login");
        en[QStringLiteral("register_success")] = QStringLiteral("Registration successful, please login");
        en[QStringLiteral("register_password_mismatch")] = QStringLiteral("Passwords do not match");
        en[QStringLiteral("register_password_short")] = QStringLiteral("Password must be at least 6 characters");

        // -- AutoScPage --
        en[QStringLiteral("sc_job_definition")] = QStringLiteral("Job Definition");
        en[QStringLiteral("sc_target_job")] = QStringLiteral("Target Job");
        en[QStringLiteral("sc_select_target_job")] = QStringLiteral("Please select a target job");
        en[QStringLiteral("sc_measurement_job")] = QStringLiteral("Measurement Job");
        en[QStringLiteral("sc_select_measurement_job")] = QStringLiteral("Please select a measurement job");
        en[QStringLiteral("sc_color_data")] = QStringLiteral("Color Data");
        en[QStringLiteral("sc_spectral_data")] = QStringLiteral("Spectral Data");
        en[QStringLiteral("sc_spectral_curve")] = QStringLiteral("Spectral Curve");
        en[QStringLiteral("sc_color_block")] = QStringLiteral("Color Block");
        en[QStringLiteral("sc_visualization_panel")] = QStringLiteral("Visualization Panel");
        en[QStringLiteral("sc_recipe_search")] = QStringLiteral("Recipe Search");
        en[QStringLiteral("sc_search_placeholder")] = QStringLiteral("Enter color name / formula ID to search");
        en[QStringLiteral("sc_search_btn")] = QStringLiteral("Search");
        en[QStringLiteral("sc_no_data")] = QStringLiteral("No data");
        en[QStringLiteral("sc_loading")] = QStringLiteral("Loading...");
        en[QStringLiteral("sc_geometry")] = QStringLiteral("Geometry");
        en[QStringLiteral("sc_illuminant")] = QStringLiteral("Illuminant");
        en[QStringLiteral("sc_color_source")] = QStringLiteral("Color Source");
        en[QStringLiteral("sc_formula_id")] = QStringLiteral("Formula ID");
        en[QStringLiteral("sc_color_name")] = QStringLiteral("Color Name");
        en[QStringLiteral("sc_save_recipe")] = QStringLiteral("Save Recipe");
        en[QStringLiteral("sc_load_recipe")] = QStringLiteral("Load Recipe");
        en[QStringLiteral("sc_delete")] = QStringLiteral("Delete");
        en[QStringLiteral("sc_export")] = QStringLiteral("Export");
        en[QStringLiteral("sc_import")] = QStringLiteral("Import");
        en[QStringLiteral("sc_refresh")] = QStringLiteral("Refresh");
        en[QStringLiteral("sc_total")] = QStringLiteral("Total");
        en[QStringLiteral("sc_records")] = QStringLiteral("records");
        en[QStringLiteral("sc_page")] = QStringLiteral("Page");
        en[QStringLiteral("sc_prev_page")] = QStringLiteral("Prev");
        en[QStringLiteral("sc_next_page")] = QStringLiteral("Next");
        en[QStringLiteral("sc_correction_detail")] = QStringLiteral("Correction Detail");
        en[QStringLiteral("sc_initial_color")] = QStringLiteral("Initial Color");
        en[QStringLiteral("sc_standard_color")] = QStringLiteral("Standard Color");
        en[QStringLiteral("sc_target_color")] = QStringLiteral("Target Color");
        en[QStringLiteral("sc_repaired_color")] = QStringLiteral("Repaired Color");
        en[QStringLiteral("sc_difference")] = QStringLiteral("Difference");
        en[QStringLiteral("sc_tolerance")] = QStringLiteral("Tolerance");
        en[QStringLiteral("sc_pass")] = QStringLiteral("Pass");
        en[QStringLiteral("sc_fail")] = QStringLiteral("Fail");
        en[QStringLiteral("sc_type")] = QStringLiteral("Type");
        en[QStringLiteral("sc_name")] = QStringLiteral("Name");
        en[QStringLiteral("sc_value")] = QStringLiteral("Value");
        en[QStringLiteral("sc_action")] = QStringLiteral("Action");
        en[QStringLiteral("sc_status")] = QStringLiteral("Status");
        en[QStringLiteral("sc_remark")] = QStringLiteral("Remark");
        en[QStringLiteral("sc_description")] = QStringLiteral("Description");
        en[QStringLiteral("sc_create_time")] = QStringLiteral("Create Time");
        en[QStringLiteral("sc_update_time")] = QStringLiteral("Update Time");
        en[QStringLiteral("sc_operator")] = QStringLiteral("Operator");

        // -- SettingsPage --
        en[QStringLiteral("settings_save_path")] = QStringLiteral("Save Path");
        en[QStringLiteral("settings_browse")] = QStringLiteral("Browse");
        en[QStringLiteral("settings_save")] = QStringLiteral("Save");
        en[QStringLiteral("settings_saved")] = QStringLiteral("Settings saved");
        en[QStringLiteral("settings_invalid_path")] = QStringLiteral("Invalid path");

        // -- ProfileDialog --
        en[QStringLiteral("profile_title")] = QStringLiteral("Profile");
        en[QStringLiteral("profile_username")] = QStringLiteral("Username");
        en[QStringLiteral("profile_phone")] = QStringLiteral("Phone");
        en[QStringLiteral("profile_company")] = QStringLiteral("Company");
        en[QStringLiteral("profile_email")] = QStringLiteral("Email");
        en[QStringLiteral("profile_save")] = QStringLiteral("Save");
        en[QStringLiteral("profile_close")] = QStringLiteral("Close");
        en[QStringLiteral("profile_update_success")] = QStringLiteral("Profile updated");

        // -- FormulaDetailWidget --
        en[QStringLiteral("formula_detail")] = QStringLiteral("Formula Detail");
        en[QStringLiteral("formula_id")] = QStringLiteral("Formula ID");
        en[QStringLiteral("formula_color_name")] = QStringLiteral("Color Name");
        en[QStringLiteral("formula_layers")] = QStringLiteral("Layers");
        en[QStringLiteral("formula_total")] = QStringLiteral("Total");
        en[QStringLiteral("formula_add")] = QStringLiteral("Add Layer");
        en[QStringLiteral("formula_calculate")] = QStringLiteral("Calculate");
        en[QStringLiteral("formula_save")] = QStringLiteral("Save");
        en[QStringLiteral("formula_cancel")] = QStringLiteral("Cancel");

        // -- UpdateWidget --
        en[QStringLiteral("update_title")] = QStringLiteral("Software Update");
        en[QStringLiteral("update_checking")] = QStringLiteral("Checking for updates...");
        en[QStringLiteral("update_available")] = QStringLiteral("New version available");
        en[QStringLiteral("update_latest")] = QStringLiteral("Already up to date");
        en[QStringLiteral("update_download")] = QStringLiteral("Download");
        en[QStringLiteral("update_install")] = QStringLiteral("Install");
        en[QStringLiteral("update_later")] = QStringLiteral("Later");
        en[QStringLiteral("update_version")] = QStringLiteral("Version");
        en[QStringLiteral("update_size")] = QStringLiteral("Size");

        // -- ColorSearchSettingsPage --
        en[QStringLiteral("color_search_settings_title")] = QStringLiteral("Color Search Settings");
        en[QStringLiteral("color_search_range")] = QStringLiteral("Search Range");
        en[QStringLiteral("color_search_accuracy")] = QStringLiteral("Search Accuracy");

        // -- BrandSeriesPage --
        en[QStringLiteral("brand_series_title")] = QStringLiteral("Brand & Model");
        en[QStringLiteral("brand_select")] = QStringLiteral("Select Brand");
        en[QStringLiteral("series_select")] = QStringLiteral("Select Series");
        en[QStringLiteral("year_select")] = QStringLiteral("Select Year");
        en[QStringLiteral("color_select")] = QStringLiteral("Select Color");

        // -- Common --
        en[QStringLiteral("ok")] = QStringLiteral("OK");
        en[QStringLiteral("yes")] = QStringLiteral("Yes");
        en[QStringLiteral("no")] = QStringLiteral("No");
        en[QStringLiteral("error")] = QStringLiteral("Error");
        en[QStringLiteral("warning")] = QStringLiteral("Warning");
        en[QStringLiteral("info")] = QStringLiteral("Information");
        en[QStringLiteral("success")] = QStringLiteral("Success");
        en[QStringLiteral("loading")] = QStringLiteral("Loading...");
        en[QStringLiteral("no_data")] = QStringLiteral("No data");
        en[QStringLiteral("network_error")] = QStringLiteral("Network error, please try again");
        en[QStringLiteral("request_failed")] = QStringLiteral("Request failed");
        en[QStringLiteral("solid")] = QStringLiteral("Solid");
        en[QStringLiteral("metallic")] = QStringLiteral("Metallic");
        en[QStringLiteral("pearl")] = QStringLiteral("Pearl");
        en[QStringLiteral("mixed")] = QStringLiteral("Mixed");
        en[QStringLiteral("tri_coat_pearl")] = QStringLiteral("Tri-coat Pearl");
        en[QStringLiteral("candy")] = QStringLiteral("Candy");
        en[QStringLiteral("spectral_data")] = QStringLiteral("Spectral Data");
        en[QStringLiteral("spectral_curve")] = QStringLiteral("Spectral Curve");
        en[QStringLiteral("color_data")] = QStringLiteral("Color Data");
        en[QStringLiteral("color_block")] = QStringLiteral("Color Block");
        en[QStringLiteral("wavelength")] = QStringLiteral("Wavelength (nm)");
        en[QStringLiteral("reflectance")] = QStringLiteral("Reflectance (%)");

        // -- Search page --
        en[QStringLiteral("search_placeholder")] = QStringLiteral("Search color name, code, car model");
        en[QStringLiteral("search_btn")] = QStringLiteral("Search");
        en[QStringLiteral("history_label")] = QStringLiteral("History:");
        en[QStringLiteral("start_search_hint")] = QStringLiteral("Enter keywords to start searching");
        en[QStringLiteral("enter_search_keyword")] = QStringLiteral("Please enter search keywords");
        en[QStringLiteral("no_more")] = QStringLiteral("— No more —");
        en[QStringLiteral("color_code_label")] = QStringLiteral("Code:");
        en[QStringLiteral("color_name_label")] = QStringLiteral("Name:");
        en[QStringLiteral("car_model_label")] = QStringLiteral("Model:");
        en[QStringLiteral("no_match_found")] = QStringLiteral("No matching formulas found");
        en[QStringLiteral("scroll_load_more")] = QStringLiteral("Scroll to load more");
        en[QStringLiteral("register_link_text")] = QStringLiteral("No account? <a href='#' style='font-size:13px;'>Register now</a>");

        // -- Settings page --
        en[QStringLiteral("formula_save_path_title")] = QStringLiteral("Formula Save Path");
        en[QStringLiteral("save_path_desc")] = QStringLiteral("Formula files will be saved to:");
        en[QStringLiteral("select_path_placeholder")] = QStringLiteral("Select save path...");
        en[QStringLiteral("browse_dots")] = QStringLiteral("Browse...");
        en[QStringLiteral("save_settings_btn")] = QStringLiteral("Save Settings");
        en[QStringLiteral("select_path_dialog")] = QStringLiteral("Select Save Path");
        en[QStringLiteral("select_path_warn")] = QStringLiteral("Please select a save path");
        en[QStringLiteral("path_not_exist")] = QStringLiteral("Path does not exist, please reselect");
        en[QStringLiteral("settings_saved_check")] = QStringLiteral("✓ Settings saved");

        // -- Update widget --
        en[QStringLiteral("current_version")] = QStringLiteral("Current version: ");
        en[QStringLiteral("check_update_status")] = QStringLiteral("Click to check for updates");
        en[QStringLiteral("check_update_btn")] = QStringLiteral("Check Update");
        en[QStringLiteral("update_now_btn")] = QStringLiteral("Update Now");
        en[QStringLiteral("close_btn")] = QStringLiteral("Close");
        en[QStringLiteral("checking_update")] = QStringLiteral("Checking for updates...");
        en[QStringLiteral("latest_version_msg")] = QStringLiteral("Already up to date");

        // ============ 俄语 ru ============
        auto &ru = langMap[QStringLiteral("ru")];
        ru[QStringLiteral("app_title")] = QStringLiteral("Система цветов Kaiqi");
        ru[QStringLiteral("user")] = QStringLiteral("Пользователь");
        ru[QStringLiteral("avatar_char")] = QStringLiteral("П");
        ru[QStringLiteral("profile")] = QStringLiteral("Профиль");
        ru[QStringLiteral("logout")] = QStringLiteral("Выход");
        ru[QStringLiteral("nav_search")] = QStringLiteral("Поиск");
        ru[QStringLiteral("nav_brand")] = QStringLiteral("Марка и модель");
        ru[QStringLiteral("nav_autosc")] = QStringLiteral("Авто S&C");
        ru[QStringLiteral("settings")] = QStringLiteral("Настройки");
        ru[QStringLiteral("formula_download_settings")] = QStringLiteral("Загрузка формул");
        ru[QStringLiteral("color_search_settings")] = QStringLiteral("Поиск цвета");
        ru[QStringLiteral("software_update")] = QStringLiteral("Обновление");
        ru[QStringLiteral("feature_in_development")] = QStringLiteral("Функция в разработке...");
        ru[QStringLiteral("logout_confirm_title")] = QStringLiteral("Подтвердите выход");
        ru[QStringLiteral("logout_confirm_text")] = QStringLiteral("Вы уверены, что хотите выйти?");
        ru[QStringLiteral("confirm")] = QStringLiteral("Подтвердить");
        ru[QStringLiteral("cancel")] = QStringLiteral("Отмена");
        ru[QStringLiteral("login_title")] = QStringLiteral("Система цветов Kaiqi");
        ru[QStringLiteral("login_subtitle")] = QStringLiteral("Профессиональная платформа управления цветами");
        ru[QStringLiteral("login_account_placeholder")] = QStringLiteral("Введите аккаунт");
        ru[QStringLiteral("login_password_placeholder")] = QStringLiteral("Введите пароль");
        ru[QStringLiteral("login_remember_account")] = QStringLiteral("Запомнить аккаунт");
        ru[QStringLiteral("login_btn")] = QStringLiteral("Войти");
        ru[QStringLiteral("login_register_link")] = QStringLiteral("Нет аккаунта? Зарегистрироваться");
        ru[QStringLiteral("login_success")] = QStringLiteral("Вход выполнен");
        ru[QStringLiteral("login_account_title")] = QStringLiteral("Вход в аккаунт");
        ru[QStringLiteral("captcha")] = QStringLiteral("Капча");
        ru[QStringLiteral("remember_me")] = QStringLiteral("Запомнить меня");
        ru[QStringLiteral("login_btn_text")] = QStringLiteral("Войти");
        ru[QStringLiteral("agree_text")] = QStringLiteral("Входя в систему, вы принимаете «%1» и «%2»");
        ru[QStringLiteral("agree_privacy_policy")] = QStringLiteral("Политикой конфиденциальности");
        ru[QStringLiteral("agree_user_agreement")] = QStringLiteral("Пользовательским соглашением");
        ru[QStringLiteral("logging_in")] = QStringLiteral("Вход...");
        ru[QStringLiteral("register_title")] = QStringLiteral("Регистрация");
        ru[QStringLiteral("register_username_placeholder")] = QStringLiteral("Введите имя пользователя");
        ru[QStringLiteral("register_phone_placeholder")] = QStringLiteral("Введите номер телефона");
        ru[QStringLiteral("register_password_placeholder")] = QStringLiteral("Введите пароль (мин. 6 символов)");
        ru[QStringLiteral("register_confirm_password_placeholder")] = QStringLiteral("Подтвердите пароль");
        ru[QStringLiteral("register_company_placeholder")] = QStringLiteral("Введите название компании");
        ru[QStringLiteral("register_btn")] = QStringLiteral("Зарегистрироваться");
        ru[QStringLiteral("register_login_link")] = QStringLiteral("Уже есть аккаунт? Войти");
        ru[QStringLiteral("register_success")] = QStringLiteral("Регистрация успешна");
        ru[QStringLiteral("register_password_mismatch")] = QStringLiteral("Пароли не совпадают");
        ru[QStringLiteral("register_password_short")] = QStringLiteral("Пароль должен быть не менее 6 символов");
        ru[QStringLiteral("sc_job_definition")] = QStringLiteral("Определение задания");
        ru[QStringLiteral("sc_target_job")] = QStringLiteral("Целевое задание");
        ru[QStringLiteral("sc_select_target_job")] = QStringLiteral("Выберите целевое задание");
        ru[QStringLiteral("sc_measurement_job")] = QStringLiteral("Задание измерения");
        ru[QStringLiteral("sc_select_measurement_job")] = QStringLiteral("Выберите задание измерения");
        ru[QStringLiteral("sc_color_data")] = QStringLiteral("Данные цвета");
        ru[QStringLiteral("sc_spectral_data")] = QStringLiteral("Спектральные данные");
        ru[QStringLiteral("sc_spectral_curve")] = QStringLiteral("Спектральная кривая");
        ru[QStringLiteral("sc_color_block")] = QStringLiteral("Цветовой блок");
        ru[QStringLiteral("sc_visualization_panel")] = QStringLiteral("Панель визуализации");
        ru[QStringLiteral("sc_recipe_search")] = QStringLiteral("Поиск рецепта");
        ru[QStringLiteral("sc_search_placeholder")] = QStringLiteral("Введите название цвета / ID формулы");
        ru[QStringLiteral("sc_search_btn")] = QStringLiteral("Поиск");
        ru[QStringLiteral("sc_no_data")] = QStringLiteral("Нет данных");
        ru[QStringLiteral("sc_loading")] = QStringLiteral("Загрузка...");
        ru[QStringLiteral("sc_save_recipe")] = QStringLiteral("Сохранить рецепт");
        ru[QStringLiteral("sc_load_recipe")] = QStringLiteral("Загрузить рецепт");
        ru[QStringLiteral("sc_delete")] = QStringLiteral("Удалить");
        ru[QStringLiteral("sc_export")] = QStringLiteral("Экспорт");
        ru[QStringLiteral("sc_import")] = QStringLiteral("Импорт");
        ru[QStringLiteral("sc_refresh")] = QStringLiteral("Обновить");
        ru[QStringLiteral("sc_total")] = QStringLiteral("Всего");
        ru[QStringLiteral("sc_records")] = QStringLiteral("записей");
        ru[QStringLiteral("settings_save_path")] = QStringLiteral("Путь сохранения");
        ru[QStringLiteral("settings_browse")] = QStringLiteral("Обзор");
        ru[QStringLiteral("settings_save")] = QStringLiteral("Сохранить");
        ru[QStringLiteral("settings_saved")] = QStringLiteral("Настройки сохранены");
        ru[QStringLiteral("settings_invalid_path")] = QStringLiteral("Неверный путь");
        ru[QStringLiteral("profile_title")] = QStringLiteral("Профиль");
        ru[QStringLiteral("profile_username")] = QStringLiteral("Имя пользователя");
        ru[QStringLiteral("profile_phone")] = QStringLiteral("Телефон");
        ru[QStringLiteral("profile_company")] = QStringLiteral("Компания");
        ru[QStringLiteral("profile_email")] = QStringLiteral("Эл. почта");
        ru[QStringLiteral("profile_save")] = QStringLiteral("Сохранить");
        ru[QStringLiteral("profile_close")] = QStringLiteral("Закрыть");
        ru[QStringLiteral("profile_update_success")] = QStringLiteral("Профиль обновлён");
        ru[QStringLiteral("formula_detail")] = QStringLiteral("Детали формулы");
        ru[QStringLiteral("formula_layers")] = QStringLiteral("Слои");
        ru[QStringLiteral("formula_total")] = QStringLiteral("Всего");
        ru[QStringLiteral("formula_calculate")] = QStringLiteral("Вычислить");
        ru[QStringLiteral("update_title")] = QStringLiteral("Обновление");
        ru[QStringLiteral("update_checking")] = QStringLiteral("Проверка обновлений...");
        ru[QStringLiteral("update_available")] = QStringLiteral("Доступна новая версия");
        ru[QStringLiteral("update_latest")] = QStringLiteral("Актуальная версия");
        ru[QStringLiteral("update_download")] = QStringLiteral("Скачать");
        ru[QStringLiteral("update_install")] = QStringLiteral("Установить");
        ru[QStringLiteral("update_later")] = QStringLiteral("Позже");
        ru[QStringLiteral("ok")] = QStringLiteral("ОК");
        ru[QStringLiteral("yes")] = QStringLiteral("Да");
        ru[QStringLiteral("no")] = QStringLiteral("Нет");
        ru[QStringLiteral("error")] = QStringLiteral("Ошибка");
        ru[QStringLiteral("warning")] = QStringLiteral("Предупреждение");
        ru[QStringLiteral("info")] = QStringLiteral("Информация");
        ru[QStringLiteral("success")] = QStringLiteral("Успешно");
        ru[QStringLiteral("loading")] = QStringLiteral("Загрузка...");
        ru[QStringLiteral("no_data")] = QStringLiteral("Нет данных");
        ru[QStringLiteral("network_error")] = QStringLiteral("Ошибка сети");
        ru[QStringLiteral("solid")] = QStringLiteral("Solid");
        ru[QStringLiteral("metallic")] = QStringLiteral("Metallic");
        ru[QStringLiteral("pearl")] = QStringLiteral("Pearl");
        ru[QStringLiteral("spectral_data")] = QStringLiteral("Спектральные данные");
        ru[QStringLiteral("spectral_curve")] = QStringLiteral("Спектральная кривая");
        ru[QStringLiteral("color_data")] = QStringLiteral("Данные цвета");
        ru[QStringLiteral("color_block")] = QStringLiteral("Цветовой блок");
        ru[QStringLiteral("wavelength")] = QStringLiteral("Длина волны (нм)");
        ru[QStringLiteral("reflectance")] = QStringLiteral("Отражение (%)");
        ru[QStringLiteral("search_placeholder")] = QStringLiteral("Поиск цвета, кода, модели");
        ru[QStringLiteral("search_btn")] = QStringLiteral("Поиск");
        ru[QStringLiteral("history_label")] = QStringLiteral("История:");
        ru[QStringLiteral("start_search_hint")] = QStringLiteral("Введите ключевые слова");
        ru[QStringLiteral("enter_search_keyword")] = QStringLiteral("Введите ключевые слова");
        ru[QStringLiteral("no_more")] = QStringLiteral("— Больше нет —");
        ru[QStringLiteral("color_code_label")] = QStringLiteral("Код:");
        ru[QStringLiteral("color_name_label")] = QStringLiteral("Имя:");
        ru[QStringLiteral("car_model_label")] = QStringLiteral("Модель:");
        ru[QStringLiteral("no_match_found")] = QStringLiteral("Формулы не найдены");
        ru[QStringLiteral("scroll_load_more")] = QStringLiteral("Прокрутите для загрузки");
        ru[QStringLiteral("register_link_text")] = QStringLiteral("Нет аккаунта? <a href='#' style='font-size:13px;'>Зарегистрироваться</a>");
        ru[QStringLiteral("formula_save_path_title")] = QStringLiteral("Путь сохранения формул");
        ru[QStringLiteral("save_path_desc")] = QStringLiteral("Файлы формул будут сохранены:");
        ru[QStringLiteral("select_path_placeholder")] = QStringLiteral("Выберите путь...");
        ru[QStringLiteral("browse_dots")] = QStringLiteral("Обзор...");
        ru[QStringLiteral("save_settings_btn")] = QStringLiteral("Сохранить");
        ru[QStringLiteral("select_path_dialog")] = QStringLiteral("Выберите путь");
        ru[QStringLiteral("select_path_warn")] = QStringLiteral("Выберите путь сохранения");
        ru[QStringLiteral("path_not_exist")] = QStringLiteral("Путь не существует");
        ru[QStringLiteral("settings_saved_check")] = QStringLiteral("✓ Настройки сохранены");
        ru[QStringLiteral("current_version")] = QStringLiteral("Текущая версия: ");
        ru[QStringLiteral("check_update_status")] = QStringLiteral("Нажмите для проверки обновлений");
        ru[QStringLiteral("check_update_btn")] = QStringLiteral("Проверить");
        ru[QStringLiteral("update_now_btn")] = QStringLiteral("Обновить");
        ru[QStringLiteral("close_btn")] = QStringLiteral("Закрыть");
        ru[QStringLiteral("checking_update")] = QStringLiteral("Проверка обновлений...");
        ru[QStringLiteral("latest_version_msg")] = QStringLiteral("Актуальная версия");

        // -- 补齐（与 en 同步）--
        ru[QStringLiteral("brand_series_title")] = QStringLiteral("Марки и модели");
        ru[QStringLiteral("brand_select")] = QStringLiteral("Выбрать марку");
        ru[QStringLiteral("series_select")] = QStringLiteral("Выбрать серию");
        ru[QStringLiteral("year_select")] = QStringLiteral("Выбрать год");
        ru[QStringLiteral("color_select")] = QStringLiteral("Выбрать цвет");
        ru[QStringLiteral("candy")] = QStringLiteral("Кэнди");
        ru[QStringLiteral("mixed")] = QStringLiteral("Смешанный");
        ru[QStringLiteral("tri_coat_pearl")] = QStringLiteral("Перламутр, 3 слоя");
        ru[QStringLiteral("color_search_settings_title")] = QStringLiteral("Настройки поиска цвета");
        ru[QStringLiteral("color_search_range")] = QStringLiteral("Область поиска");
        ru[QStringLiteral("color_search_accuracy")] = QStringLiteral("Точность поиска");
        ru[QStringLiteral("request_failed")] = QStringLiteral("Ошибка запроса");
        ru[QStringLiteral("update_version")] = QStringLiteral("Версия");
        ru[QStringLiteral("update_size")] = QStringLiteral("Размер");
        ru[QStringLiteral("formula_id")] = QStringLiteral("№ рецепта");
        ru[QStringLiteral("formula_color_name")] = QStringLiteral("Название цвета");
        ru[QStringLiteral("formula_add")] = QStringLiteral("Добавить слой");
        ru[QStringLiteral("formula_save")] = QStringLiteral("Сохранить");
        ru[QStringLiteral("formula_cancel")] = QStringLiteral("Отмена");
        ru[QStringLiteral("sc_geometry")] = QStringLiteral("Угол измерения");
        ru[QStringLiteral("sc_illuminant")] = QStringLiteral("Источник света");
        ru[QStringLiteral("sc_color_source")] = QStringLiteral("Источник цвета");
        ru[QStringLiteral("sc_formula_id")] = QStringLiteral("№ рецепта");
        ru[QStringLiteral("sc_color_name")] = QStringLiteral("Название цвета");
        ru[QStringLiteral("sc_page")] = QStringLiteral("Стр.");
        ru[QStringLiteral("sc_prev_page")] = QStringLiteral("Назад");
        ru[QStringLiteral("sc_next_page")] = QStringLiteral("Вперёд");
        ru[QStringLiteral("sc_correction_detail")] = QStringLiteral("Детали корректировки");
        ru[QStringLiteral("sc_initial_color")] = QStringLiteral("Исходный цвет");
        ru[QStringLiteral("sc_standard_color")] = QStringLiteral("Эталонный цвет");
        ru[QStringLiteral("sc_target_color")] = QStringLiteral("Целевой цвет");
        ru[QStringLiteral("sc_repaired_color")] = QStringLiteral("Восстановленный цвет");
        ru[QStringLiteral("sc_difference")] = QStringLiteral("Разница");
        ru[QStringLiteral("sc_tolerance")] = QStringLiteral("Допуск");
        ru[QStringLiteral("sc_pass")] = QStringLiteral("Прошёл");
        ru[QStringLiteral("sc_fail")] = QStringLiteral("Не прошёл");
        ru[QStringLiteral("sc_type")] = QStringLiteral("Тип");
        ru[QStringLiteral("sc_name")] = QStringLiteral("Название");
        ru[QStringLiteral("sc_value")] = QStringLiteral("Значение");
        ru[QStringLiteral("sc_action")] = QStringLiteral("Действие");
        ru[QStringLiteral("sc_status")] = QStringLiteral("Статус");
        ru[QStringLiteral("sc_description")] = QStringLiteral("Описание");
        ru[QStringLiteral("sc_remark")] = QStringLiteral("Примечание");
        ru[QStringLiteral("sc_create_time")] = QStringLiteral("Время создания");
        ru[QStringLiteral("sc_update_time")] = QStringLiteral("Время изменения");
        ru[QStringLiteral("sc_operator")] = QStringLiteral("Оператор");

        // ============ 法语 fr ============
        auto &fr = langMap[QStringLiteral("fr")];
        fr[QStringLiteral("app_title")] = QStringLiteral("Système de couleurs Kaiqi");
        fr[QStringLiteral("user")] = QStringLiteral("Utilisateur");
        fr[QStringLiteral("avatar_char")] = QStringLiteral("U");
        fr[QStringLiteral("profile")] = QStringLiteral("Profil");
        fr[QStringLiteral("logout")] = QStringLiteral("Déconnexion");
        fr[QStringLiteral("nav_search")] = QStringLiteral("Recherche");
        fr[QStringLiteral("nav_brand")] = QStringLiteral("Marque et modèle");
        fr[QStringLiteral("nav_autosc")] = QStringLiteral("Auto S&C");
        fr[QStringLiteral("settings")] = QStringLiteral("Paramètres");
        fr[QStringLiteral("formula_download_settings")] = QStringLiteral("Téléchargement formules");
        fr[QStringLiteral("color_search_settings")] = QStringLiteral("Recherche couleur");
        fr[QStringLiteral("software_update")] = QStringLiteral("Mise à jour");
        fr[QStringLiteral("feature_in_development")] = QStringLiteral("Fonction en développement...");
        fr[QStringLiteral("logout_confirm_title")] = QStringLiteral("Confirmer la déconnexion");
        fr[QStringLiteral("logout_confirm_text")] = QStringLiteral("Voulez-vous vraiment vous déconnecter ?");
        fr[QStringLiteral("confirm")] = QStringLiteral("Confirmer");
        fr[QStringLiteral("cancel")] = QStringLiteral("Annuler");
        fr[QStringLiteral("login_title")] = QStringLiteral("Système de couleurs Kaiqi");
        fr[QStringLiteral("login_subtitle")] = QStringLiteral("Plateforme professionnelle de gestion des couleurs");
        fr[QStringLiteral("login_account_placeholder")] = QStringLiteral("Entrez le compte");
        fr[QStringLiteral("login_password_placeholder")] = QStringLiteral("Entrez le mot de passe");
        fr[QStringLiteral("login_remember_account")] = QStringLiteral("Mémoriser le compte");
        fr[QStringLiteral("login_btn")] = QStringLiteral("Connexion");
        fr[QStringLiteral("login_register_link")] = QStringLiteral("Pas de compte ? Inscrivez-vous");
        fr[QStringLiteral("login_success")] = QStringLiteral("Connexion réussie");
        fr[QStringLiteral("login_account_title")] = QStringLiteral("Connexion au compte");
        fr[QStringLiteral("captcha")] = QStringLiteral("Captcha");
        fr[QStringLiteral("remember_me")] = QStringLiteral("Se souvenir de moi");
        fr[QStringLiteral("login_btn_text")] = QStringLiteral("Connexion");
        fr[QStringLiteral("agree_text")] = QStringLiteral("En vous connectant, vous acceptez la %1 et les %2");
        fr[QStringLiteral("agree_privacy_policy")] = QStringLiteral("Politique de confidentialité");
        fr[QStringLiteral("agree_user_agreement")] = QStringLiteral("Conditions d'utilisation");
        fr[QStringLiteral("logging_in")] = QStringLiteral("Connexion en cours...");
        fr[QStringLiteral("register_title")] = QStringLiteral("Inscription");
        fr[QStringLiteral("register_username_placeholder")] = QStringLiteral("Entrez le nom d'utilisateur");
        fr[QStringLiteral("register_phone_placeholder")] = QStringLiteral("Entrez le numéro de téléphone");
        fr[QStringLiteral("register_password_placeholder")] = QStringLiteral("Entrez le mot de passe (min. 6 caractères)");
        fr[QStringLiteral("register_confirm_password_placeholder")] = QStringLiteral("Confirmez le mot de passe");
        fr[QStringLiteral("register_company_placeholder")] = QStringLiteral("Entrez le nom de l'entreprise");
        fr[QStringLiteral("register_btn")] = QStringLiteral("S'inscrire");
        fr[QStringLiteral("register_login_link")] = QStringLiteral("Déjà un compte ? Connectez-vous");
        fr[QStringLiteral("register_success")] = QStringLiteral("Inscription réussie");
        fr[QStringLiteral("register_password_mismatch")] = QStringLiteral("Les mots de passe ne correspondent pas");
        fr[QStringLiteral("register_password_short")] = QStringLiteral("Le mot de passe doit comporter au moins 6 caractères");
        fr[QStringLiteral("sc_job_definition")] = QStringLiteral("Définition du travail");
        fr[QStringLiteral("sc_target_job")] = QStringLiteral("Travail cible");
        fr[QStringLiteral("sc_select_target_job")] = QStringLiteral("Sélectionnez un travail cible");
        fr[QStringLiteral("sc_measurement_job")] = QStringLiteral("Travail de mesure");
        fr[QStringLiteral("sc_select_measurement_job")] = QStringLiteral("Sélectionnez un travail de mesure");
        fr[QStringLiteral("sc_color_data")] = QStringLiteral("Données de couleur");
        fr[QStringLiteral("sc_spectral_data")] = QStringLiteral("Données spectrales");
        fr[QStringLiteral("sc_spectral_curve")] = QStringLiteral("Courbe spectrale");
        fr[QStringLiteral("sc_color_block")] = QStringLiteral("Bloc de couleur");
        fr[QStringLiteral("sc_visualization_panel")] = QStringLiteral("Panneau de visualisation");
        fr[QStringLiteral("sc_recipe_search")] = QStringLiteral("Recherche de recette");
        fr[QStringLiteral("sc_search_placeholder")] = QStringLiteral("Entrez le nom de couleur / ID de formule");
        fr[QStringLiteral("sc_search_btn")] = QStringLiteral("Rechercher");
        fr[QStringLiteral("sc_no_data")] = QStringLiteral("Aucune donnée");
        fr[QStringLiteral("sc_loading")] = QStringLiteral("Chargement...");
        fr[QStringLiteral("sc_save_recipe")] = QStringLiteral("Enregistrer la recette");
        fr[QStringLiteral("sc_load_recipe")] = QStringLiteral("Charger la recette");
        fr[QStringLiteral("sc_delete")] = QStringLiteral("Supprimer");
        fr[QStringLiteral("sc_export")] = QStringLiteral("Exporter");
        fr[QStringLiteral("sc_import")] = QStringLiteral("Importer");
        fr[QStringLiteral("sc_refresh")] = QStringLiteral("Actualiser");
        fr[QStringLiteral("sc_total")] = QStringLiteral("Total");
        fr[QStringLiteral("sc_records")] = QStringLiteral("enregistrements");
        fr[QStringLiteral("settings_save_path")] = QStringLiteral("Chemin de sauvegarde");
        fr[QStringLiteral("settings_browse")] = QStringLiteral("Parcourir");
        fr[QStringLiteral("settings_save")] = QStringLiteral("Enregistrer");
        fr[QStringLiteral("settings_saved")] = QStringLiteral("Paramètres enregistrés");
        fr[QStringLiteral("settings_invalid_path")] = QStringLiteral("Chemin invalide");
        fr[QStringLiteral("profile_title")] = QStringLiteral("Profil");
        fr[QStringLiteral("profile_username")] = QStringLiteral("Nom d'utilisateur");
        fr[QStringLiteral("profile_phone")] = QStringLiteral("Téléphone");
        fr[QStringLiteral("profile_company")] = QStringLiteral("Entreprise");
        fr[QStringLiteral("profile_email")] = QStringLiteral("E-mail");
        fr[QStringLiteral("profile_save")] = QStringLiteral("Enregistrer");
        fr[QStringLiteral("profile_close")] = QStringLiteral("Fermer");
        fr[QStringLiteral("profile_update_success")] = QStringLiteral("Profil mis à jour");
        fr[QStringLiteral("formula_detail")] = QStringLiteral("Détail de la formule");
        fr[QStringLiteral("formula_layers")] = QStringLiteral("Couches");
        fr[QStringLiteral("formula_total")] = QStringLiteral("Total");
        fr[QStringLiteral("formula_calculate")] = QStringLiteral("Calculer");
        fr[QStringLiteral("update_title")] = QStringLiteral("Mise à jour");
        fr[QStringLiteral("update_checking")] = QStringLiteral("Vérification des mises à jour...");
        fr[QStringLiteral("update_available")] = QStringLiteral("Nouvelle version disponible");
        fr[QStringLiteral("update_latest")] = QStringLiteral("Déjà à jour");
        fr[QStringLiteral("update_download")] = QStringLiteral("Télécharger");
        fr[QStringLiteral("update_install")] = QStringLiteral("Installer");
        fr[QStringLiteral("update_later")] = QStringLiteral("Plus tard");
        fr[QStringLiteral("ok")] = QStringLiteral("OK");
        fr[QStringLiteral("yes")] = QStringLiteral("Oui");
        fr[QStringLiteral("no")] = QStringLiteral("Non");
        fr[QStringLiteral("error")] = QStringLiteral("Erreur");
        fr[QStringLiteral("warning")] = QStringLiteral("Avertissement");
        fr[QStringLiteral("info")] = QStringLiteral("Information");
        fr[QStringLiteral("success")] = QStringLiteral("Succès");
        fr[QStringLiteral("loading")] = QStringLiteral("Chargement...");
        fr[QStringLiteral("no_data")] = QStringLiteral("Aucune donnée");
        fr[QStringLiteral("network_error")] = QStringLiteral("Erreur réseau");
        fr[QStringLiteral("solid")] = QStringLiteral("Solid");
        fr[QStringLiteral("metallic")] = QStringLiteral("Metallic");
        fr[QStringLiteral("pearl")] = QStringLiteral("Pearl");
        fr[QStringLiteral("spectral_data")] = QStringLiteral("Données spectrales");
        fr[QStringLiteral("spectral_curve")] = QStringLiteral("Courbe spectrale");
        fr[QStringLiteral("color_data")] = QStringLiteral("Données de couleur");
        fr[QStringLiteral("color_block")] = QStringLiteral("Bloc de couleur");
        fr[QStringLiteral("wavelength")] = QStringLiteral("Longueur d'onde (nm)");
        fr[QStringLiteral("reflectance")] = QStringLiteral("Réflectance (%)");
        fr[QStringLiteral("search_placeholder")] = QStringLiteral("Rechercher couleur, code, modèle");
        fr[QStringLiteral("search_btn")] = QStringLiteral("Rechercher");
        fr[QStringLiteral("history_label")] = QStringLiteral("Historique:");
        fr[QStringLiteral("start_search_hint")] = QStringLiteral("Entrez des mots-clés");
        fr[QStringLiteral("enter_search_keyword")] = QStringLiteral("Entrez des mots-clés");
        fr[QStringLiteral("no_more")] = QStringLiteral("— Fin —");
        fr[QStringLiteral("color_code_label")] = QStringLiteral("Code:");
        fr[QStringLiteral("color_name_label")] = QStringLiteral("Nom:");
        fr[QStringLiteral("car_model_label")] = QStringLiteral("Modèle:");
        fr[QStringLiteral("no_match_found")] = QStringLiteral("Aucune formule trouvée");
        fr[QStringLiteral("scroll_load_more")] = QStringLiteral("Défiler pour charger");
        fr[QStringLiteral("register_link_text")] = QStringLiteral("Pas de compte? <a href='#' style='font-size:13px;'>Inscrivez-vous</a>");
        fr[QStringLiteral("formula_save_path_title")] = QStringLiteral("Chemin des formules");
        fr[QStringLiteral("save_path_desc")] = QStringLiteral("Les fichiers seront enregistrés:");
        fr[QStringLiteral("select_path_placeholder")] = QStringLiteral("Sélectionner le chemin...");
        fr[QStringLiteral("browse_dots")] = QStringLiteral("Parcourir...");
        fr[QStringLiteral("save_settings_btn")] = QStringLiteral("Enregistrer");
        fr[QStringLiteral("select_path_dialog")] = QStringLiteral("Sélectionner le chemin");
        fr[QStringLiteral("select_path_warn")] = QStringLiteral("Sélectionnez un chemin");
        fr[QStringLiteral("path_not_exist")] = QStringLiteral("Le chemin n'existe pas");
        fr[QStringLiteral("settings_saved_check")] = QStringLiteral("✓ Paramètres enregistrés");
        fr[QStringLiteral("current_version")] = QStringLiteral("Version actuelle: ");
        fr[QStringLiteral("check_update_status")] = QStringLiteral("Cliquez pour vérifier");
        fr[QStringLiteral("check_update_btn")] = QStringLiteral("Vérifier");
        fr[QStringLiteral("update_now_btn")] = QStringLiteral("Mettre à jour");
        fr[QStringLiteral("close_btn")] = QStringLiteral("Fermer");
        fr[QStringLiteral("checking_update")] = QStringLiteral("Vérification...");
        fr[QStringLiteral("latest_version_msg")] = QStringLiteral("Déjà à jour");

        // -- 补齐（与 en 同步）--
        fr[QStringLiteral("brand_series_title")] = QStringLiteral("Marques et modèles");
        fr[QStringLiteral("brand_select")] = QStringLiteral("Choisir la marque");
        fr[QStringLiteral("series_select")] = QStringLiteral("Choisir la série");
        fr[QStringLiteral("year_select")] = QStringLiteral("Choisir l'année");
        fr[QStringLiteral("color_select")] = QStringLiteral("Choisir la couleur");
        fr[QStringLiteral("candy")] = QStringLiteral("Candy");
        fr[QStringLiteral("mixed")] = QStringLiteral("Mélangé");
        fr[QStringLiteral("tri_coat_pearl")] = QStringLiteral("Nacré 3 couches");
        fr[QStringLiteral("color_search_settings_title")] = QStringLiteral("Paramètres de recherche de couleur");
        fr[QStringLiteral("color_search_range")] = QStringLiteral("Étendue de recherche");
        fr[QStringLiteral("color_search_accuracy")] = QStringLiteral("Précision de recherche");
        fr[QStringLiteral("request_failed")] = QStringLiteral("Échec de la requête");
        fr[QStringLiteral("update_version")] = QStringLiteral("Version");
        fr[QStringLiteral("update_size")] = QStringLiteral("Taille");
        fr[QStringLiteral("formula_id")] = QStringLiteral("N° de formule");
        fr[QStringLiteral("formula_color_name")] = QStringLiteral("Nom de la couleur");
        fr[QStringLiteral("formula_add")] = QStringLiteral("Ajouter une couche");
        fr[QStringLiteral("formula_save")] = QStringLiteral("Enregistrer");
        fr[QStringLiteral("formula_cancel")] = QStringLiteral("Annuler");
        fr[QStringLiteral("sc_geometry")] = QStringLiteral("Angle de mesure");
        fr[QStringLiteral("sc_illuminant")] = QStringLiteral("Illuminant");
        fr[QStringLiteral("sc_color_source")] = QStringLiteral("Source de couleur");
        fr[QStringLiteral("sc_formula_id")] = QStringLiteral("N° de formule");
        fr[QStringLiteral("sc_color_name")] = QStringLiteral("Nom de la couleur");
        fr[QStringLiteral("sc_page")] = QStringLiteral("Page");
        fr[QStringLiteral("sc_prev_page")] = QStringLiteral("Précédent");
        fr[QStringLiteral("sc_next_page")] = QStringLiteral("Suivant");
        fr[QStringLiteral("sc_correction_detail")] = QStringLiteral("Détail de la correction");
        fr[QStringLiteral("sc_initial_color")] = QStringLiteral("Couleur initiale");
        fr[QStringLiteral("sc_standard_color")] = QStringLiteral("Couleur standard");
        fr[QStringLiteral("sc_target_color")] = QStringLiteral("Couleur cible");
        fr[QStringLiteral("sc_repaired_color")] = QStringLiteral("Couleur réparée");
        fr[QStringLiteral("sc_difference")] = QStringLiteral("Différence");
        fr[QStringLiteral("sc_tolerance")] = QStringLiteral("Tolérance");
        fr[QStringLiteral("sc_pass")] = QStringLiteral("Conforme");
        fr[QStringLiteral("sc_fail")] = QStringLiteral("Non conforme");
        fr[QStringLiteral("sc_type")] = QStringLiteral("Type");
        fr[QStringLiteral("sc_name")] = QStringLiteral("Nom");
        fr[QStringLiteral("sc_value")] = QStringLiteral("Valeur");
        fr[QStringLiteral("sc_action")] = QStringLiteral("Action");
        fr[QStringLiteral("sc_status")] = QStringLiteral("Statut");
        fr[QStringLiteral("sc_description")] = QStringLiteral("Description");
        fr[QStringLiteral("sc_remark")] = QStringLiteral("Remarque");
        fr[QStringLiteral("sc_create_time")] = QStringLiteral("Date de création");
        fr[QStringLiteral("sc_update_time")] = QStringLiteral("Date de modification");
        fr[QStringLiteral("sc_operator")] = QStringLiteral("Opérateur");

        // ============ 阿拉伯语 ar ============
        auto &ar = langMap[QStringLiteral("ar")];
        ar[QStringLiteral("app_title")] = QStringLiteral("نظام ألوان كايكي");
        ar[QStringLiteral("user")] = QStringLiteral("المستخدم");
        ar[QStringLiteral("avatar_char")] = QStringLiteral("م");
        ar[QStringLiteral("profile")] = QStringLiteral("الملف الشخصي");
        ar[QStringLiteral("logout")] = QStringLiteral("تسجيل الخروج");
        ar[QStringLiteral("nav_search")] = QStringLiteral("بحث");
        ar[QStringLiteral("nav_brand")] = QStringLiteral("العلامة والطراز");
        ar[QStringLiteral("nav_autosc")] = QStringLiteral("Auto S&C");
        ar[QStringLiteral("settings")] = QStringLiteral("الإعدادات");
        ar[QStringLiteral("formula_download_settings")] = QStringLiteral("تحميل الوصفات");
        ar[QStringLiteral("color_search_settings")] = QStringLiteral("بحث الألوان");
        ar[QStringLiteral("software_update")] = QStringLiteral("تحديث البرنامج");
        ar[QStringLiteral("feature_in_development")] = QStringLiteral("الميزة قيد التطوير...");
        ar[QStringLiteral("logout_confirm_title")] = QStringLiteral("تأكيد تسجيل الخروج");
        ar[QStringLiteral("logout_confirm_text")] = QStringLiteral("هل أنت متأكد من تسجيل الخروج؟");
        ar[QStringLiteral("confirm")] = QStringLiteral("تأكيد");
        ar[QStringLiteral("cancel")] = QStringLiteral("إلغاء");
        ar[QStringLiteral("login_title")] = QStringLiteral("نظام ألوان كايكي");
        ar[QStringLiteral("login_subtitle")] = QStringLiteral("منصة احترافية لإدارة الألوان");
        ar[QStringLiteral("login_account_placeholder")] = QStringLiteral("أدخل الحساب");
        ar[QStringLiteral("login_password_placeholder")] = QStringLiteral("أدخل كلمة المرور");
        ar[QStringLiteral("login_remember_account")] = QStringLiteral("تذكر الحساب");
        ar[QStringLiteral("login_btn")] = QStringLiteral("دخول");
        ar[QStringLiteral("login_register_link")] = QStringLiteral("ليس لديك حساب؟ سجل الآن");
        ar[QStringLiteral("login_success")] = QStringLiteral("تم تسجيل الدخول");
        ar[QStringLiteral("login_account_title")] = QStringLiteral("تسجيل الدخول");
        ar[QStringLiteral("captcha")] = QStringLiteral("رمز التحقق");
        ar[QStringLiteral("remember_me")] = QStringLiteral("تذكرني");
        ar[QStringLiteral("login_btn_text")] = QStringLiteral("دخول");
        ar[QStringLiteral("agree_text")] = QStringLiteral("بتسجيل الدخول فإنك توافق على %1 و%2");
        ar[QStringLiteral("agree_privacy_policy")] = QStringLiteral("سياسة الخصوصية");
        ar[QStringLiteral("agree_user_agreement")] = QStringLiteral("اتفاقية المستخدم");
        ar[QStringLiteral("logging_in")] = QStringLiteral("جاري تسجيل الدخول...");
        ar[QStringLiteral("register_title")] = QStringLiteral("تسجيل حساب");
        ar[QStringLiteral("register_username_placeholder")] = QStringLiteral("أدخل اسم المستخدم");
        ar[QStringLiteral("register_phone_placeholder")] = QStringLiteral("أدخل رقم الهاتف");
        ar[QStringLiteral("register_password_placeholder")] = QStringLiteral("أدخل كلمة المرور (6 أحرف على الأقل)");
        ar[QStringLiteral("register_confirm_password_placeholder")] = QStringLiteral("أكد كلمة المرور");
        ar[QStringLiteral("register_company_placeholder")] = QStringLiteral("أدخل اسم الشركة");
        ar[QStringLiteral("register_btn")] = QStringLiteral("تسجيل");
        ar[QStringLiteral("register_login_link")] = QStringLiteral("لديك حساب؟ سجل الدخول");
        ar[QStringLiteral("register_success")] = QStringLiteral("تم التسجيل بنجاح");
        ar[QStringLiteral("register_password_mismatch")] = QStringLiteral("كلمات المرور غير متطابقة");
        ar[QStringLiteral("register_password_short")] = QStringLiteral("يجب أن تكون كلمة المرور 6 أحرف على الأقل");
        ar[QStringLiteral("sc_job_definition")] = QStringLiteral("تعريف المهمة");
        ar[QStringLiteral("sc_target_job")] = QStringLiteral("المهمة المستهدفة");
        ar[QStringLiteral("sc_select_target_job")] = QStringLiteral("اختر مهمة مستهدفة");
        ar[QStringLiteral("sc_measurement_job")] = QStringLiteral("مهمة القياس");
        ar[QStringLiteral("sc_select_measurement_job")] = QStringLiteral("اختر مهمة قياس");
        ar[QStringLiteral("sc_color_data")] = QStringLiteral("بيانات اللون");
        ar[QStringLiteral("sc_spectral_data")] = QStringLiteral("البيانات الطيفية");
        ar[QStringLiteral("sc_spectral_curve")] = QStringLiteral("المنحنى الطيفي");
        ar[QStringLiteral("sc_color_block")] = QStringLiteral("كتلة اللون");
        ar[QStringLiteral("sc_visualization_panel")] = QStringLiteral("لوحة العرض");
        ar[QStringLiteral("sc_recipe_search")] = QStringLiteral("بحث الوصفة");
        ar[QStringLiteral("sc_search_placeholder")] = QStringLiteral("أدخل اسم اللون / معرف الوصفة");
        ar[QStringLiteral("sc_search_btn")] = QStringLiteral("بحث");
        ar[QStringLiteral("sc_no_data")] = QStringLiteral("لا توجد بيانات");
        ar[QStringLiteral("sc_loading")] = QStringLiteral("جاري التحميل...");
        ar[QStringLiteral("sc_save_recipe")] = QStringLiteral("حفظ الوصفة");
        ar[QStringLiteral("sc_load_recipe")] = QStringLiteral("تحميل الوصفة");
        ar[QStringLiteral("sc_delete")] = QStringLiteral("حذف");
        ar[QStringLiteral("sc_export")] = QStringLiteral("تصدير");
        ar[QStringLiteral("sc_import")] = QStringLiteral("استيراد");
        ar[QStringLiteral("sc_refresh")] = QStringLiteral("تحديث");
        ar[QStringLiteral("sc_total")] = QStringLiteral("المجموع");
        ar[QStringLiteral("sc_records")] = QStringLiteral("سجلات");
        ar[QStringLiteral("settings_save_path")] = QStringLiteral("مسار الحفظ");
        ar[QStringLiteral("settings_browse")] = QStringLiteral("تصفح");
        ar[QStringLiteral("settings_save")] = QStringLiteral("حفظ");
        ar[QStringLiteral("settings_saved")] = QStringLiteral("تم حفظ الإعدادات");
        ar[QStringLiteral("settings_invalid_path")] = QStringLiteral("مسار غير صالح");
        ar[QStringLiteral("profile_title")] = QStringLiteral("الملف الشخصي");
        ar[QStringLiteral("profile_username")] = QStringLiteral("اسم المستخدم");
        ar[QStringLiteral("profile_phone")] = QStringLiteral("الهاتف");
        ar[QStringLiteral("profile_company")] = QStringLiteral("الشركة");
        ar[QStringLiteral("profile_email")] = QStringLiteral("البريد الإلكتروني");
        ar[QStringLiteral("profile_save")] = QStringLiteral("حفظ");
        ar[QStringLiteral("profile_close")] = QStringLiteral("إغلاق");
        ar[QStringLiteral("profile_update_success")] = QStringLiteral("تم تحديث الملف الشخصي");
        ar[QStringLiteral("formula_detail")] = QStringLiteral("تفاصيل الوصفة");
        ar[QStringLiteral("formula_layers")] = QStringLiteral("الطبقات");
        ar[QStringLiteral("formula_total")] = QStringLiteral("المجموع");
        ar[QStringLiteral("formula_calculate")] = QStringLiteral("حساب");
        ar[QStringLiteral("update_title")] = QStringLiteral("تحديث البرنامج");
        ar[QStringLiteral("update_checking")] = QStringLiteral("جاري التحقق من التحديثات...");
        ar[QStringLiteral("update_available")] = QStringLiteral("إصدار جديد متاح");
        ar[QStringLiteral("update_latest")] = QStringLiteral("الإصدار محدث");
        ar[QStringLiteral("update_download")] = QStringLiteral("تحميل");
        ar[QStringLiteral("update_install")] = QStringLiteral("تثبيت");
        ar[QStringLiteral("update_later")] = QStringLiteral("لاحقاً");
        ar[QStringLiteral("ok")] = QStringLiteral("موافق");
        ar[QStringLiteral("yes")] = QStringLiteral("نعم");
        ar[QStringLiteral("no")] = QStringLiteral("لا");
        ar[QStringLiteral("error")] = QStringLiteral("خطأ");
        ar[QStringLiteral("warning")] = QStringLiteral("تحذير");
        ar[QStringLiteral("info")] = QStringLiteral("معلومات");
        ar[QStringLiteral("success")] = QStringLiteral("نجاح");
        ar[QStringLiteral("loading")] = QStringLiteral("جاري التحميل...");
        ar[QStringLiteral("no_data")] = QStringLiteral("لا توجد بيانات");
        ar[QStringLiteral("network_error")] = QStringLiteral("خطأ في الشبكة");
        ar[QStringLiteral("solid")] = QStringLiteral("Solid");
        ar[QStringLiteral("metallic")] = QStringLiteral("Metallic");
        ar[QStringLiteral("pearl")] = QStringLiteral("Pearl");
        ar[QStringLiteral("spectral_data")] = QStringLiteral("البيانات الطيفية");
        ar[QStringLiteral("spectral_curve")] = QStringLiteral("المنحنى الطيفي");
        ar[QStringLiteral("color_data")] = QStringLiteral("بيانات اللون");
        ar[QStringLiteral("color_block")] = QStringLiteral("كتلة اللون");
        ar[QStringLiteral("wavelength")] = QStringLiteral("الطول الموجي (nm)");
        ar[QStringLiteral("reflectance")] = QStringLiteral("الانعكاس (%)");
        ar[QStringLiteral("search_placeholder")] = QStringLiteral("البحث عن لون، رمز، طراز");
        ar[QStringLiteral("search_btn")] = QStringLiteral("بحث");
        ar[QStringLiteral("history_label")] = QStringLiteral("السجل:");
        ar[QStringLiteral("start_search_hint")] = QStringLiteral("أدخل كلمات للبحث");
        ar[QStringLiteral("enter_search_keyword")] = QStringLiteral("أدخل كلمات للبحث");
        ar[QStringLiteral("no_more")] = QStringLiteral("— لا مزيد —");
        ar[QStringLiteral("color_code_label")] = QStringLiteral("الرمز:");
        ar[QStringLiteral("color_name_label")] = QStringLiteral("الاسم:");
        ar[QStringLiteral("car_model_label")] = QStringLiteral("الطراز:");
        ar[QStringLiteral("no_match_found")] = QStringLiteral("لم يتم العثور على صيغ");
        ar[QStringLiteral("scroll_load_more")] = QStringLiteral("مرر للأسفل للتحميل");
        ar[QStringLiteral("register_link_text")] = QStringLiteral("ليس لديك حساب؟ <a href='#' style='font-size:13px;'>سجل الآن</a>");
        ar[QStringLiteral("formula_save_path_title")] = QStringLiteral("مسار حفظ الوصفات");
        ar[QStringLiteral("save_path_desc")] = QStringLiteral("سيتم حفظ الملفات في:");
        ar[QStringLiteral("select_path_placeholder")] = QStringLiteral("اختر المسار...");
        ar[QStringLiteral("browse_dots")] = QStringLiteral("تصفح...");
        ar[QStringLiteral("save_settings_btn")] = QStringLiteral("حفظ");
        ar[QStringLiteral("select_path_dialog")] = QStringLiteral("اختر المسار");
        ar[QStringLiteral("select_path_warn")] = QStringLiteral("اختر مسار الحفظ");
        ar[QStringLiteral("path_not_exist")] = QStringLiteral("المسار غير موجود");
        ar[QStringLiteral("settings_saved_check")] = QStringLiteral("✓ تم الحفظ");
        ar[QStringLiteral("current_version")] = QStringLiteral("الإصدار الحالي: ");
        ar[QStringLiteral("check_update_status")] = QStringLiteral("انقر للتحقق من التحديثات");
        ar[QStringLiteral("check_update_btn")] = QStringLiteral("تحقق");
        ar[QStringLiteral("update_now_btn")] = QStringLiteral("تحديث");
        ar[QStringLiteral("close_btn")] = QStringLiteral("إغلاق");
        ar[QStringLiteral("checking_update")] = QStringLiteral("جاري التحقق...");
        ar[QStringLiteral("latest_version_msg")] = QStringLiteral("الإصدار محدث");

        // -- 补齐（与 en 同步）--
        ar[QStringLiteral("brand_series_title")] = QStringLiteral("الماركات والطرازات");
        ar[QStringLiteral("brand_select")] = QStringLiteral("اختيار الماركة");
        ar[QStringLiteral("series_select")] = QStringLiteral("اختيار السلسلة");
        ar[QStringLiteral("year_select")] = QStringLiteral("اختيار السنة");
        ar[QStringLiteral("color_select")] = QStringLiteral("اختيار اللون");
        ar[QStringLiteral("candy")] = QStringLiteral("كاندي");
        ar[QStringLiteral("mixed")] = QStringLiteral("مختلط");
        ar[QStringLiteral("tri_coat_pearl")] = QStringLiteral("لؤلؤي ثلاثي الطبقات");
        ar[QStringLiteral("color_search_settings_title")] = QStringLiteral("إعدادات البحث عن اللون");
        ar[QStringLiteral("color_search_range")] = QStringLiteral("نطاق البحث");
        ar[QStringLiteral("color_search_accuracy")] = QStringLiteral("دقة البحث");
        ar[QStringLiteral("request_failed")] = QStringLiteral("فشل الطلب");
        ar[QStringLiteral("update_version")] = QStringLiteral("الإصدار");
        ar[QStringLiteral("update_size")] = QStringLiteral("الحجم");
        ar[QStringLiteral("formula_id")] = QStringLiteral("رقم الوصفة");
        ar[QStringLiteral("formula_color_name")] = QStringLiteral("اسم اللون");
        ar[QStringLiteral("formula_add")] = QStringLiteral("إضافة طبقة");
        ar[QStringLiteral("formula_save")] = QStringLiteral("حفظ");
        ar[QStringLiteral("formula_cancel")] = QStringLiteral("إلغاء");
        ar[QStringLiteral("sc_geometry")] = QStringLiteral("زاوية القياس");
        ar[QStringLiteral("sc_illuminant")] = QStringLiteral("المضيء");
        ar[QStringLiteral("sc_color_source")] = QStringLiteral("مصدر اللون");
        ar[QStringLiteral("sc_formula_id")] = QStringLiteral("رقم الوصفة");
        ar[QStringLiteral("sc_color_name")] = QStringLiteral("اسم اللون");
        ar[QStringLiteral("sc_page")] = QStringLiteral("صفحة");
        ar[QStringLiteral("sc_prev_page")] = QStringLiteral("السابق");
        ar[QStringLiteral("sc_next_page")] = QStringLiteral("التالي");
        ar[QStringLiteral("sc_correction_detail")] = QStringLiteral("تفاصيل التصحيح");
        ar[QStringLiteral("sc_initial_color")] = QStringLiteral("اللون الأولي");
        ar[QStringLiteral("sc_standard_color")] = QStringLiteral("اللون القياسي");
        ar[QStringLiteral("sc_target_color")] = QStringLiteral("اللون المستهدف");
        ar[QStringLiteral("sc_repaired_color")] = QStringLiteral("اللون المُصلَح");
        ar[QStringLiteral("sc_difference")] = QStringLiteral("الفرق");
        ar[QStringLiteral("sc_tolerance")] = QStringLiteral("التفاوت المسموح");
        ar[QStringLiteral("sc_pass")] = QStringLiteral("ناجح");
        ar[QStringLiteral("sc_fail")] = QStringLiteral("راسب");
        ar[QStringLiteral("sc_type")] = QStringLiteral("النوع");
        ar[QStringLiteral("sc_name")] = QStringLiteral("الاسم");
        ar[QStringLiteral("sc_value")] = QStringLiteral("القيمة");
        ar[QStringLiteral("sc_action")] = QStringLiteral("إجراء");
        ar[QStringLiteral("sc_status")] = QStringLiteral("الحالة");
        ar[QStringLiteral("sc_description")] = QStringLiteral("الوصف");
        ar[QStringLiteral("sc_remark")] = QStringLiteral("ملاحظة");
        ar[QStringLiteral("sc_create_time")] = QStringLiteral("وقت الإنشاء");
        ar[QStringLiteral("sc_update_time")] = QStringLiteral("وقت التحديث");
        ar[QStringLiteral("sc_operator")] = QStringLiteral("المشغل");

        // ============ 越南语 vi ============
        auto &vi = langMap[QStringLiteral("vi")];
        vi[QStringLiteral("app_title")] = QStringLiteral("Hệ thống màu Kaiqi");
        vi[QStringLiteral("user")] = QStringLiteral("Người dùng");
        vi[QStringLiteral("avatar_char")] = QStringLiteral("N");
        vi[QStringLiteral("profile")] = QStringLiteral("Hồ sơ");
        vi[QStringLiteral("logout")] = QStringLiteral("Đăng xuất");
        vi[QStringLiteral("nav_search")] = QStringLiteral("Tìm kiếm");
        vi[QStringLiteral("nav_brand")] = QStringLiteral("Thương hiệu");
        vi[QStringLiteral("nav_autosc")] = QStringLiteral("Auto S&C");
        vi[QStringLiteral("settings")] = QStringLiteral("Cài đặt");
        vi[QStringLiteral("formula_download_settings")] = QStringLiteral("Tải công thức");
        vi[QStringLiteral("color_search_settings")] = QStringLiteral("Tìm kiếm màu");
        vi[QStringLiteral("software_update")] = QStringLiteral("Cập nhật");
        vi[QStringLiteral("feature_in_development")] = QStringLiteral("Đang phát triển...");
        vi[QStringLiteral("logout_confirm_title")] = QStringLiteral("Xác nhận đăng xuất");
        vi[QStringLiteral("logout_confirm_text")] = QStringLiteral("Bạn có chắc chắn muốn đăng xuất?");
        vi[QStringLiteral("confirm")] = QStringLiteral("Xác nhận");
        vi[QStringLiteral("cancel")] = QStringLiteral("Hủy");
        vi[QStringLiteral("login_title")] = QStringLiteral("Hệ thống màu Kaiqi");
        vi[QStringLiteral("login_subtitle")] = QStringLiteral("Nền tảng quản lý màu chuyên nghiệp");
        vi[QStringLiteral("login_account_placeholder")] = QStringLiteral("Nhập tài khoản");
        vi[QStringLiteral("login_password_placeholder")] = QStringLiteral("Nhập mật khẩu");
        vi[QStringLiteral("login_remember_account")] = QStringLiteral("Ghi nhớ tài khoản");
        vi[QStringLiteral("login_btn")] = QStringLiteral("Đăng nhập");
        vi[QStringLiteral("login_register_link")] = QStringLiteral("Chưa có tài khoản? Đăng ký ngay");
        vi[QStringLiteral("login_success")] = QStringLiteral("Đăng nhập thành công");
        vi[QStringLiteral("login_account_title")] = QStringLiteral("Đăng nhập tài khoản");
        vi[QStringLiteral("captcha")] = QStringLiteral("Mã xác thực");
        vi[QStringLiteral("remember_me")] = QStringLiteral("Ghi nhớ tôi");
        vi[QStringLiteral("login_btn_text")] = QStringLiteral("Đăng nhập");
        vi[QStringLiteral("agree_text")] = QStringLiteral("Đăng nhập nghĩa là bạn đã đọc và đồng ý với %1 và %2");
        vi[QStringLiteral("agree_privacy_policy")] = QStringLiteral("Chính sách bảo mật");
        vi[QStringLiteral("agree_user_agreement")] = QStringLiteral("Thỏa thuận người dùng");
        vi[QStringLiteral("logging_in")] = QStringLiteral("Đang đăng nhập...");
        vi[QStringLiteral("register_title")] = QStringLiteral("Đăng ký tài khoản");
        vi[QStringLiteral("register_username_placeholder")] = QStringLiteral("Nhập tên người dùng");
        vi[QStringLiteral("register_phone_placeholder")] = QStringLiteral("Nhập số điện thoại");
        vi[QStringLiteral("register_password_placeholder")] = QStringLiteral("Nhập mật khẩu (tối thiểu 6 ký tự)");
        vi[QStringLiteral("register_confirm_password_placeholder")] = QStringLiteral("Xác nhận mật khẩu");
        vi[QStringLiteral("register_company_placeholder")] = QStringLiteral("Nhập tên công ty");
        vi[QStringLiteral("register_btn")] = QStringLiteral("Đăng ký");
        vi[QStringLiteral("register_login_link")] = QStringLiteral("Đã có tài khoản? Đăng nhập");
        vi[QStringLiteral("register_success")] = QStringLiteral("Đăng ký thành công");
        vi[QStringLiteral("register_password_mismatch")] = QStringLiteral("Mật khẩu không khớp");
        vi[QStringLiteral("register_password_short")] = QStringLiteral("Mật khẩu phải có ít nhất 6 ký tự");
        vi[QStringLiteral("sc_job_definition")] = QStringLiteral("Định nghĩa công việc");
        vi[QStringLiteral("sc_target_job")] = QStringLiteral("Công việc mục tiêu");
        vi[QStringLiteral("sc_select_target_job")] = QStringLiteral("Chọn công việc mục tiêu");
        vi[QStringLiteral("sc_measurement_job")] = QStringLiteral("Công việc đo lường");
        vi[QStringLiteral("sc_select_measurement_job")] = QStringLiteral("Chọn công việc đo lường");
        vi[QStringLiteral("sc_color_data")] = QStringLiteral("Dữ liệu màu");
        vi[QStringLiteral("sc_spectral_data")] = QStringLiteral("Dữ liệu quang phổ");
        vi[QStringLiteral("sc_spectral_curve")] = QStringLiteral("Đường cong quang phổ");
        vi[QStringLiteral("sc_color_block")] = QStringLiteral("Khối màu");
        vi[QStringLiteral("sc_visualization_panel")] = QStringLiteral("Bảng hiển thị");
        vi[QStringLiteral("sc_recipe_search")] = QStringLiteral("Tìm kiếm công thức");
        vi[QStringLiteral("sc_search_placeholder")] = QStringLiteral("Nhập tên màu / ID công thức");
        vi[QStringLiteral("sc_search_btn")] = QStringLiteral("Tìm kiếm");
        vi[QStringLiteral("sc_no_data")] = QStringLiteral("Không có dữ liệu");
        vi[QStringLiteral("sc_loading")] = QStringLiteral("Đang tải...");
        vi[QStringLiteral("sc_save_recipe")] = QStringLiteral("Lưu công thức");
        vi[QStringLiteral("sc_load_recipe")] = QStringLiteral("Tải công thức");
        vi[QStringLiteral("sc_delete")] = QStringLiteral("Xóa");
        vi[QStringLiteral("sc_export")] = QStringLiteral("Xuất");
        vi[QStringLiteral("sc_import")] = QStringLiteral("Nhập");
        vi[QStringLiteral("sc_refresh")] = QStringLiteral("Làm mới");
        vi[QStringLiteral("sc_total")] = QStringLiteral("Tổng");
        vi[QStringLiteral("sc_records")] = QStringLiteral("bản ghi");
        vi[QStringLiteral("settings_save_path")] = QStringLiteral("Đường dẫn lưu");
        vi[QStringLiteral("settings_browse")] = QStringLiteral("Duyệt");
        vi[QStringLiteral("settings_save")] = QStringLiteral("Lưu");
        vi[QStringLiteral("settings_saved")] = QStringLiteral("Đã lưu cài đặt");
        vi[QStringLiteral("settings_invalid_path")] = QStringLiteral("Đường dẫn không hợp lệ");
        vi[QStringLiteral("profile_title")] = QStringLiteral("Hồ sơ");
        vi[QStringLiteral("profile_username")] = QStringLiteral("Tên người dùng");
        vi[QStringLiteral("profile_phone")] = QStringLiteral("Điện thoại");
        vi[QStringLiteral("profile_company")] = QStringLiteral("Công ty");
        vi[QStringLiteral("profile_email")] = QStringLiteral("Email");
        vi[QStringLiteral("profile_save")] = QStringLiteral("Lưu");
        vi[QStringLiteral("profile_close")] = QStringLiteral("Đóng");
        vi[QStringLiteral("profile_update_success")] = QStringLiteral("Đã cập nhật hồ sơ");
        vi[QStringLiteral("formula_detail")] = QStringLiteral("Chi tiết công thức");
        vi[QStringLiteral("formula_layers")] = QStringLiteral("Lớp");
        vi[QStringLiteral("formula_total")] = QStringLiteral("Tổng");
        vi[QStringLiteral("formula_calculate")] = QStringLiteral("Tính toán");
        vi[QStringLiteral("update_title")] = QStringLiteral("Cập nhật phần mềm");
        vi[QStringLiteral("update_checking")] = QStringLiteral("Đang kiểm tra cập nhật...");
        vi[QStringLiteral("update_available")] = QStringLiteral("Có phiên bản mới");
        vi[QStringLiteral("update_latest")] = QStringLiteral("Đã là phiên bản mới nhất");
        vi[QStringLiteral("update_download")] = QStringLiteral("Tải xuống");
        vi[QStringLiteral("update_install")] = QStringLiteral("Cài đặt");
        vi[QStringLiteral("update_later")] = QStringLiteral("Để sau");
        vi[QStringLiteral("ok")] = QStringLiteral("OK");
        vi[QStringLiteral("yes")] = QStringLiteral("Có");
        vi[QStringLiteral("no")] = QStringLiteral("Không");
        vi[QStringLiteral("error")] = QStringLiteral("Lỗi");
        vi[QStringLiteral("warning")] = QStringLiteral("Cảnh báo");
        vi[QStringLiteral("info")] = QStringLiteral("Thông tin");
        vi[QStringLiteral("success")] = QStringLiteral("Thành công");
        vi[QStringLiteral("loading")] = QStringLiteral("Đang tải...");
        vi[QStringLiteral("no_data")] = QStringLiteral("Không có dữ liệu");
        vi[QStringLiteral("network_error")] = QStringLiteral("Lỗi mạng");
        vi[QStringLiteral("solid")] = QStringLiteral("Solid");
        vi[QStringLiteral("metallic")] = QStringLiteral("Metallic");
        vi[QStringLiteral("pearl")] = QStringLiteral("Pearl");
        vi[QStringLiteral("spectral_data")] = QStringLiteral("Dữ liệu quang phổ");
        vi[QStringLiteral("spectral_curve")] = QStringLiteral("Đường cong quang phổ");
        vi[QStringLiteral("color_data")] = QStringLiteral("Dữ liệu màu");
        vi[QStringLiteral("color_block")] = QStringLiteral("Khối màu");
        vi[QStringLiteral("wavelength")] = QStringLiteral("Bước sóng (nm)");
        vi[QStringLiteral("reflectance")] = QStringLiteral("Phản xạ (%)");
        vi[QStringLiteral("search_placeholder")] = QStringLiteral("Tìm màu, mã, kiểu xe");
        vi[QStringLiteral("search_btn")] = QStringLiteral("Tìm");
        vi[QStringLiteral("history_label")] = QStringLiteral("Lịch sử:");
        vi[QStringLiteral("start_search_hint")] = QStringLiteral("Nhập từ khóa để tìm kiếm");
        vi[QStringLiteral("enter_search_keyword")] = QStringLiteral("Nhập từ khóa tìm kiếm");
        vi[QStringLiteral("no_more")] = QStringLiteral("— Hết rồi —");
        vi[QStringLiteral("color_code_label")] = QStringLiteral("Mã:");
        vi[QStringLiteral("color_name_label")] = QStringLiteral("Tên:");
        vi[QStringLiteral("car_model_label")] = QStringLiteral("Mẫu:");
        vi[QStringLiteral("no_match_found")] = QStringLiteral("Không tìm thấy công thức");
        vi[QStringLiteral("scroll_load_more")] = QStringLiteral("Cuộn để tải thêm");
        vi[QStringLiteral("register_link_text")] = QStringLiteral("Chưa có tài khoản? <a href='#' style='font-size:13px;'>Đăng ký ngay</a>");
        vi[QStringLiteral("formula_save_path_title")] = QStringLiteral("Đường dẫn lưu công thức");
        vi[QStringLiteral("save_path_desc")] = QStringLiteral("File sẽ được lưu vào:");
        vi[QStringLiteral("select_path_placeholder")] = QStringLiteral("Chọn đường dẫn...");
        vi[QStringLiteral("browse_dots")] = QStringLiteral("Duyệt...");
        vi[QStringLiteral("save_settings_btn")] = QStringLiteral("Lưu");
        vi[QStringLiteral("select_path_dialog")] = QStringLiteral("Chọn đường dẫn");
        vi[QStringLiteral("select_path_warn")] = QStringLiteral("Vui lòng chọn đường dẫn");
        vi[QStringLiteral("path_not_exist")] = QStringLiteral("Đường dẫn không tồn tại");
        vi[QStringLiteral("settings_saved_check")] = QStringLiteral("✓ Đã lưu");
        vi[QStringLiteral("current_version")] = QStringLiteral("Phiên bản: ");
        vi[QStringLiteral("check_update_status")] = QStringLiteral("Nhấn để kiểm tra cập nhật");
        vi[QStringLiteral("check_update_btn")] = QStringLiteral("Kiểm tra");
        vi[QStringLiteral("update_now_btn")] = QStringLiteral("Cập nhật");
        vi[QStringLiteral("close_btn")] = QStringLiteral("Đóng");
        vi[QStringLiteral("checking_update")] = QStringLiteral("Đang kiểm tra...");
        vi[QStringLiteral("latest_version_msg")] = QStringLiteral("Đã là bản mới nhất");
        // -- 补齐（与 en 同步）--
        vi[QStringLiteral("brand_series_title")] = QStringLiteral("Hãng và dòng xe");
        vi[QStringLiteral("brand_select")] = QStringLiteral("Chọn hãng");
        vi[QStringLiteral("series_select")] = QStringLiteral("Chọn dòng");
        vi[QStringLiteral("year_select")] = QStringLiteral("Chọn năm");
        vi[QStringLiteral("color_select")] = QStringLiteral("Chọn màu");
        vi[QStringLiteral("candy")] = QStringLiteral("Candy");
        vi[QStringLiteral("mixed")] = QStringLiteral("Pha trộn");
        vi[QStringLiteral("tri_coat_pearl")] = QStringLiteral("Ngọc trai 3 lớp");
        vi[QStringLiteral("color_search_settings_title")] = QStringLiteral("Cài đặt tìm kiếm màu");
        vi[QStringLiteral("color_search_range")] = QStringLiteral("Phạm vi tìm kiếm");
        vi[QStringLiteral("color_search_accuracy")] = QStringLiteral("Độ chính xác tìm kiếm");
        vi[QStringLiteral("request_failed")] = QStringLiteral("Yêu cầu thất bại");
        vi[QStringLiteral("update_version")] = QStringLiteral("Phiên bản");
        vi[QStringLiteral("update_size")] = QStringLiteral("Dung lượng");
        vi[QStringLiteral("formula_id")] = QStringLiteral("Mã công thức");
        vi[QStringLiteral("formula_color_name")] = QStringLiteral("Tên màu");
        vi[QStringLiteral("formula_add")] = QStringLiteral("Thêm lớp");
        vi[QStringLiteral("formula_save")] = QStringLiteral("Lưu");
        vi[QStringLiteral("formula_cancel")] = QStringLiteral("Hủy");
        vi[QStringLiteral("sc_geometry")] = QStringLiteral("Góc đo");
        vi[QStringLiteral("sc_illuminant")] = QStringLiteral("Nguồn sáng");
        vi[QStringLiteral("sc_color_source")] = QStringLiteral("Nguồn màu");
        vi[QStringLiteral("sc_formula_id")] = QStringLiteral("Mã công thức");
        vi[QStringLiteral("sc_color_name")] = QStringLiteral("Tên màu");
        vi[QStringLiteral("sc_page")] = QStringLiteral("Trang");
        vi[QStringLiteral("sc_prev_page")] = QStringLiteral("Trang trước");
        vi[QStringLiteral("sc_next_page")] = QStringLiteral("Trang sau");
        vi[QStringLiteral("sc_correction_detail")] = QStringLiteral("Chi tiết hiệu chỉnh");
        vi[QStringLiteral("sc_initial_color")] = QStringLiteral("Màu ban đầu");
        vi[QStringLiteral("sc_standard_color")] = QStringLiteral("Màu tiêu chuẩn");
        vi[QStringLiteral("sc_target_color")] = QStringLiteral("Màu đích");
        vi[QStringLiteral("sc_repaired_color")] = QStringLiteral("Màu đã sửa");
        vi[QStringLiteral("sc_difference")] = QStringLiteral("Chênh lệch");
        vi[QStringLiteral("sc_tolerance")] = QStringLiteral("Dung sai");
        vi[QStringLiteral("sc_pass")] = QStringLiteral("Đạt");
        vi[QStringLiteral("sc_fail")] = QStringLiteral("Không đạt");
        vi[QStringLiteral("sc_type")] = QStringLiteral("Loại");
        vi[QStringLiteral("sc_name")] = QStringLiteral("Tên");
        vi[QStringLiteral("sc_value")] = QStringLiteral("Giá trị");
        vi[QStringLiteral("sc_action")] = QStringLiteral("Thao tác");
        vi[QStringLiteral("sc_status")] = QStringLiteral("Trạng thái");
        vi[QStringLiteral("sc_description")] = QStringLiteral("Mô tả");
        vi[QStringLiteral("sc_remark")] = QStringLiteral("Ghi chú");
        vi[QStringLiteral("sc_create_time")] = QStringLiteral("Thời gian tạo");
        vi[QStringLiteral("sc_update_time")] = QStringLiteral("Thời gian cập nhật");
        vi[QStringLiteral("sc_operator")] = QStringLiteral("Người thao tác");

        // ============ 按模块拆分的翻译数据 ============
        // 每个 .inc 内含该模块全部键的 6 种语言条目，键带模块前缀避免冲突；
        // 此处 zh/en/ru/fr/ar/vi 六个局部引用均在作用域内。
#include "i18n/SearchPage.i18n.inc"
#include "i18n/AutoSc.i18n.inc"
#include "i18n/FormulaDetail.i18n.inc"
#include "i18n/ColorSearchSettings.i18n.inc"
#include "i18n/BrandSeries.i18n.inc"
#include "i18n/Profile.i18n.inc"
#include "i18n/Register.i18n.inc"
#include "i18n/Export.i18n.inc"
#include "i18n/Chart.i18n.inc"
#include "i18n/Update.i18n.inc"
#include "i18n/Common.i18n.inc"
    }

    // 加载对应语言的翻译
    m_fallbackZh = langMap.value(QStringLiteral("zh"));
    auto it = langMap.constFind(langCode);
    if (it != langMap.constEnd()) {
        m_translations = it.value();
    }
}

void I18nManager::buildInMemoryTranslations()
{
    // 翻译数据在 loadLanguage 中静态构建，此处无需额外操作
}
