// Register 模块翻译数据（由 I18nManager.cpp 在 langMap 构建块内包含）
// 每行形如：  zh[QStringLiteral("key")] = QStringLiteral("文本");
// 必须同时提供 zh/en/ru/fr/ar/vi 六种语言，键前缀唯一
// 说明：主表中已有的 register_title / register_phone_placeholder /
// register_password_placeholder / register_confirm_password_placeholder /
// register_btn / register_success / register_password_mismatch 直接复用，不在此重复定义。

// ===== 页面标题与引导 =====
        zh[QStringLiteral("register_create_account")] = QStringLiteral("创建账户");
        en[QStringLiteral("register_create_account")] = QStringLiteral("Create Account");
        ru[QStringLiteral("register_create_account")] = QStringLiteral("Создать аккаунт");
        fr[QStringLiteral("register_create_account")] = QStringLiteral("Créer un compte");
        ar[QStringLiteral("register_create_account")] = QStringLiteral("إنشاء حساب");
        vi[QStringLiteral("register_create_account")] = QStringLiteral("Tạo tài khoản");

        zh[QStringLiteral("register_subtitle")] = QStringLiteral("加入恺祁颜色系统");
        en[QStringLiteral("register_subtitle")] = QStringLiteral("Join the Kaiqi Color System");
        ru[QStringLiteral("register_subtitle")] = QStringLiteral("Присоединяйтесь к системе цветов Kaiqi");
        fr[QStringLiteral("register_subtitle")] = QStringLiteral("Rejoignez le système de couleurs Kaiqi");
        ar[QStringLiteral("register_subtitle")] = QStringLiteral("انضم إلى نظام الألوان Kaiqi");
        vi[QStringLiteral("register_subtitle")] = QStringLiteral("Tham gia Hệ thống màu Kaiqi");

// ===== 注册方式切换 =====
        zh[QStringLiteral("register_phone_tab")] = QStringLiteral("手机号注册");
        en[QStringLiteral("register_phone_tab")] = QStringLiteral("Register with Phone");
        ru[QStringLiteral("register_phone_tab")] = QStringLiteral("Регистрация по телефону");
        fr[QStringLiteral("register_phone_tab")] = QStringLiteral("Inscription par téléphone");
        ar[QStringLiteral("register_phone_tab")] = QStringLiteral("التسجيل برقم الهاتف");
        vi[QStringLiteral("register_phone_tab")] = QStringLiteral("Đăng ký bằng số điện thoại");

        zh[QStringLiteral("register_email_tab")] = QStringLiteral("邮箱注册");
        en[QStringLiteral("register_email_tab")] = QStringLiteral("Register with Email");
        ru[QStringLiteral("register_email_tab")] = QStringLiteral("Регистрация по эл. почте");
        fr[QStringLiteral("register_email_tab")] = QStringLiteral("Inscription par e-mail");
        ar[QStringLiteral("register_email_tab")] = QStringLiteral("التسجيل بالبريد الإلكتروني");
        vi[QStringLiteral("register_email_tab")] = QStringLiteral("Đăng ký bằng email");

// ===== 输入框占位提示 =====
        zh[QStringLiteral("register_email_placeholder")] = QStringLiteral("请输入邮箱");
        en[QStringLiteral("register_email_placeholder")] = QStringLiteral("Please enter email");
        ru[QStringLiteral("register_email_placeholder")] = QStringLiteral("Введите адрес электронной почты");
        fr[QStringLiteral("register_email_placeholder")] = QStringLiteral("Veuillez saisir l'adresse e-mail");
        ar[QStringLiteral("register_email_placeholder")] = QStringLiteral("أدخل البريد الإلكتروني");
        vi[QStringLiteral("register_email_placeholder")] = QStringLiteral("Vui lòng nhập email");

        zh[QStringLiteral("register_code_placeholder")] = QStringLiteral("请输入验证码");
        en[QStringLiteral("register_code_placeholder")] = QStringLiteral("Please enter verification code");
        ru[QStringLiteral("register_code_placeholder")] = QStringLiteral("Введите код подтверждения");
        fr[QStringLiteral("register_code_placeholder")] = QStringLiteral("Veuillez saisir le code de vérification");
        ar[QStringLiteral("register_code_placeholder")] = QStringLiteral("أدخل رمز التحقق");
        vi[QStringLiteral("register_code_placeholder")] = QStringLiteral("Vui lòng nhập mã xác minh");

        zh[QStringLiteral("register_nickname_placeholder")] = QStringLiteral("请输入昵称");
        en[QStringLiteral("register_nickname_placeholder")] = QStringLiteral("Please enter nickname");
        ru[QStringLiteral("register_nickname_placeholder")] = QStringLiteral("Введите никнейм");
        fr[QStringLiteral("register_nickname_placeholder")] = QStringLiteral("Veuillez saisir le pseudonyme");
        ar[QStringLiteral("register_nickname_placeholder")] = QStringLiteral("أدخل الاسم المستعار");
        vi[QStringLiteral("register_nickname_placeholder")] = QStringLiteral("Vui lòng nhập biệt danh");

        zh[QStringLiteral("register_custom_brand_placeholder")] = QStringLiteral("请输入自定义品牌名");
        en[QStringLiteral("register_custom_brand_placeholder")] = QStringLiteral("Please enter custom brand name");
        ru[QStringLiteral("register_custom_brand_placeholder")] = QStringLiteral("Введите название своего бренда");
        fr[QStringLiteral("register_custom_brand_placeholder")] = QStringLiteral("Veuillez saisir le nom de la marque personnalisée");
        ar[QStringLiteral("register_custom_brand_placeholder")] = QStringLiteral("أدخل اسم العلامة التجارية المخصص");
        vi[QStringLiteral("register_custom_brand_placeholder")] = QStringLiteral("Vui lòng nhập tên thương hiệu tùy chỉnh");

        zh[QStringLiteral("register_supplier_placeholder")] = QStringLiteral("请输入供应商");
        en[QStringLiteral("register_supplier_placeholder")] = QStringLiteral("Please enter supplier");
        ru[QStringLiteral("register_supplier_placeholder")] = QStringLiteral("Введите поставщика");
        fr[QStringLiteral("register_supplier_placeholder")] = QStringLiteral("Veuillez saisir le fournisseur");
        ar[QStringLiteral("register_supplier_placeholder")] = QStringLiteral("أدخل المورد");
        vi[QStringLiteral("register_supplier_placeholder")] = QStringLiteral("Vui lòng nhập nhà cung cấp");

// ===== 下拉框默认项 =====
        zh[QStringLiteral("register_select_country")] = QStringLiteral("请选择国家/地区");
        en[QStringLiteral("register_select_country")] = QStringLiteral("Select country/region");
        ru[QStringLiteral("register_select_country")] = QStringLiteral("Выберите страну/регион");
        fr[QStringLiteral("register_select_country")] = QStringLiteral("Veuillez sélectionner le pays/la région");
        ar[QStringLiteral("register_select_country")] = QStringLiteral("اختر البلد/المنطقة");
        vi[QStringLiteral("register_select_country")] = QStringLiteral("Vui lòng chọn quốc gia/khu vực");

        zh[QStringLiteral("register_select_province")] = QStringLiteral("请选择省份");
        en[QStringLiteral("register_select_province")] = QStringLiteral("Select province");
        ru[QStringLiteral("register_select_province")] = QStringLiteral("Выберите провинцию");
        fr[QStringLiteral("register_select_province")] = QStringLiteral("Veuillez sélectionner la province");
        ar[QStringLiteral("register_select_province")] = QStringLiteral("اختر المقاطعة");
        vi[QStringLiteral("register_select_province")] = QStringLiteral("Vui lòng chọn tỉnh/thành phố");

        zh[QStringLiteral("register_select_brand")] = QStringLiteral("请选择品牌");
        en[QStringLiteral("register_select_brand")] = QStringLiteral("Select brand");
        ru[QStringLiteral("register_select_brand")] = QStringLiteral("Выберите бренд");
        fr[QStringLiteral("register_select_brand")] = QStringLiteral("Veuillez sélectionner la marque");
        ar[QStringLiteral("register_select_brand")] = QStringLiteral("اختر العلامة التجارية");
        vi[QStringLiteral("register_select_brand")] = QStringLiteral("Vui lòng chọn thương hiệu");

        zh[QStringLiteral("register_custom_brand")] = QStringLiteral("自定义");
        en[QStringLiteral("register_custom_brand")] = QStringLiteral("Custom");
        ru[QStringLiteral("register_custom_brand")] = QStringLiteral("Свой бренд");
        fr[QStringLiteral("register_custom_brand")] = QStringLiteral("Marque personnalisée");
        ar[QStringLiteral("register_custom_brand")] = QStringLiteral("علامة تجارية مخصصة");
        vi[QStringLiteral("register_custom_brand")] = QStringLiteral("Tùy chỉnh");

// ===== 校验与提示 =====
        zh[QStringLiteral("register_invalid_phone")] = QStringLiteral("请输入正确的手机号");
        en[QStringLiteral("register_invalid_phone")] = QStringLiteral("Please enter a valid phone number");
        ru[QStringLiteral("register_invalid_phone")] = QStringLiteral("Введите корректный номер телефона");
        fr[QStringLiteral("register_invalid_phone")] = QStringLiteral("Veuillez saisir un numéro de téléphone valide");
        ar[QStringLiteral("register_invalid_phone")] = QStringLiteral("أدخل رقم هاتف صحيح");
        vi[QStringLiteral("register_invalid_phone")] = QStringLiteral("Vui lòng nhập số điện thoại hợp lệ");

        zh[QStringLiteral("register_invalid_email")] = QStringLiteral("请输入正确的邮箱");
        en[QStringLiteral("register_invalid_email")] = QStringLiteral("Please enter a valid email address");
        ru[QStringLiteral("register_invalid_email")] = QStringLiteral("Введите корректный адрес электронной почты");
        fr[QStringLiteral("register_invalid_email")] = QStringLiteral("Veuillez saisir une adresse e-mail valide");
        ar[QStringLiteral("register_invalid_email")] = QStringLiteral("أدخل بريدًا إلكترونيًا صحيحًا");
        vi[QStringLiteral("register_invalid_email")] = QStringLiteral("Vui lòng nhập email hợp lệ");

        zh[QStringLiteral("register_sms_sent")] = QStringLiteral("短信已发送");
        en[QStringLiteral("register_sms_sent")] = QStringLiteral("SMS sent");
        ru[QStringLiteral("register_sms_sent")] = QStringLiteral("СМС отправлено");
        fr[QStringLiteral("register_sms_sent")] = QStringLiteral("SMS envoyé");
        ar[QStringLiteral("register_sms_sent")] = QStringLiteral("تم إرسال الرسالة النصية");
        vi[QStringLiteral("register_sms_sent")] = QStringLiteral("Đã gửi tin nhắn SMS");

        zh[QStringLiteral("register_email_sent")] = QStringLiteral("邮件已发送");
        en[QStringLiteral("register_email_sent")] = QStringLiteral("Email sent");
        ru[QStringLiteral("register_email_sent")] = QStringLiteral("Письмо отправлено");
        fr[QStringLiteral("register_email_sent")] = QStringLiteral("E-mail envoyé");
        ar[QStringLiteral("register_email_sent")] = QStringLiteral("تم إرسال البريد الإلكتروني");
        vi[QStringLiteral("register_email_sent")] = QStringLiteral("Đã gửi email");

        zh[QStringLiteral("register_success_detail")] = QStringLiteral("注册成功！账号：%1\n请使用新账号登录。");
        en[QStringLiteral("register_success_detail")] = QStringLiteral("Registration successful. Account: %1\nPlease log in with your new account.");
        ru[QStringLiteral("register_success_detail")] = QStringLiteral("Регистрация успешна. Аккаунт: %1\nВойдите, используя новый аккаунт.");
        fr[QStringLiteral("register_success_detail")] = QStringLiteral("Inscription réussie. Compte : %1\nVeuillez vous connecter avec votre nouveau compte.");
        ar[QStringLiteral("register_success_detail")] = QStringLiteral("تم التسجيل بنجاح. الحساب: %1\nيرجى تسجيل الدخول بحسابك الجديد.");
        vi[QStringLiteral("register_success_detail")] = QStringLiteral("Đăng ký thành công. Tài khoản: %1\nVui lòng đăng nhập bằng tài khoản mới.");

// ===== 按钮与倒计时 =====
        zh[QStringLiteral("register_send_code")] = QStringLiteral("发送验证码");
        en[QStringLiteral("register_send_code")] = QStringLiteral("Send Code");
        ru[QStringLiteral("register_send_code")] = QStringLiteral("Отправить код");
        fr[QStringLiteral("register_send_code")] = QStringLiteral("Envoyer le code");
        ar[QStringLiteral("register_send_code")] = QStringLiteral("إرسال الرمز");
        vi[QStringLiteral("register_send_code")] = QStringLiteral("Gửi mã");

        zh[QStringLiteral("register_resend_countdown")] = QStringLiteral("重新发送(%1s)");
        en[QStringLiteral("register_resend_countdown")] = QStringLiteral("Resend (%1s)");
        ru[QStringLiteral("register_resend_countdown")] = QStringLiteral("Отправить снова (%1 с)");
        fr[QStringLiteral("register_resend_countdown")] = QStringLiteral("Renvoyer le code (%1 s)");
        ar[QStringLiteral("register_resend_countdown")] = QStringLiteral("إعادة الإرسال (%1 ث)");
        vi[QStringLiteral("register_resend_countdown")] = QStringLiteral("Gửi lại (%1s)");

        zh[QStringLiteral("register_submitting")] = QStringLiteral("注册中...");
        en[QStringLiteral("register_submitting")] = QStringLiteral("Registering...");
        ru[QStringLiteral("register_submitting")] = QStringLiteral("Идёт регистрация...");
        fr[QStringLiteral("register_submitting")] = QStringLiteral("Inscription en cours...");
        ar[QStringLiteral("register_submitting")] = QStringLiteral("جارٍ التسجيل...");
        vi[QStringLiteral("register_submitting")] = QStringLiteral("Đang đăng ký...");

        zh[QStringLiteral("register_have_account")] = QStringLiteral("已有账号？");
        en[QStringLiteral("register_have_account")] = QStringLiteral("Already have an account?");
        ru[QStringLiteral("register_have_account")] = QStringLiteral("Уже есть аккаунт?");
        fr[QStringLiteral("register_have_account")] = QStringLiteral("Vous avez déjà un compte ?");
        ar[QStringLiteral("register_have_account")] = QStringLiteral("هل لديك حساب بالفعل؟");
        vi[QStringLiteral("register_have_account")] = QStringLiteral("Đã có tài khoản?");

        zh[QStringLiteral("register_login_now")] = QStringLiteral("立即登录");
        en[QStringLiteral("register_login_now")] = QStringLiteral("Log in now");
        ru[QStringLiteral("register_login_now")] = QStringLiteral("Войти");
        fr[QStringLiteral("register_login_now")] = QStringLiteral("Se connecter");
        ar[QStringLiteral("register_login_now")] = QStringLiteral("تسجيل الدخول");
        vi[QStringLiteral("register_login_now")] = QStringLiteral("Đăng nhập");

// ===== 国家/地区下拉项（仅显示文本，数据值保持中文） =====
        zh[QStringLiteral("register_country_china")] = QStringLiteral("中国");
        en[QStringLiteral("register_country_china")] = QStringLiteral("China");
        ru[QStringLiteral("register_country_china")] = QStringLiteral("Китай");
        fr[QStringLiteral("register_country_china")] = QStringLiteral("Chine");
        ar[QStringLiteral("register_country_china")] = QStringLiteral("الصين");
        vi[QStringLiteral("register_country_china")] = QStringLiteral("Trung Quốc");

        zh[QStringLiteral("register_country_usa")] = QStringLiteral("美国");
        en[QStringLiteral("register_country_usa")] = QStringLiteral("United States");
        ru[QStringLiteral("register_country_usa")] = QStringLiteral("США");
        fr[QStringLiteral("register_country_usa")] = QStringLiteral("États-Unis");
        ar[QStringLiteral("register_country_usa")] = QStringLiteral("الولايات المتحدة");
        vi[QStringLiteral("register_country_usa")] = QStringLiteral("Hoa Kỳ");

        zh[QStringLiteral("register_country_uk")] = QStringLiteral("英国");
        en[QStringLiteral("register_country_uk")] = QStringLiteral("United Kingdom");
        ru[QStringLiteral("register_country_uk")] = QStringLiteral("Великобритания");
        fr[QStringLiteral("register_country_uk")] = QStringLiteral("Royaume-Uni");
        ar[QStringLiteral("register_country_uk")] = QStringLiteral("المملكة المتحدة");
        vi[QStringLiteral("register_country_uk")] = QStringLiteral("Vương quốc Anh");

        zh[QStringLiteral("register_country_germany")] = QStringLiteral("德国");
        en[QStringLiteral("register_country_germany")] = QStringLiteral("Germany");
        ru[QStringLiteral("register_country_germany")] = QStringLiteral("Германия");
        fr[QStringLiteral("register_country_germany")] = QStringLiteral("Allemagne");
        ar[QStringLiteral("register_country_germany")] = QStringLiteral("ألمانيا");
        vi[QStringLiteral("register_country_germany")] = QStringLiteral("Đức");

        zh[QStringLiteral("register_country_france")] = QStringLiteral("法国");
        en[QStringLiteral("register_country_france")] = QStringLiteral("France");
        ru[QStringLiteral("register_country_france")] = QStringLiteral("Франция");
        fr[QStringLiteral("register_country_france")] = QStringLiteral("France");
        ar[QStringLiteral("register_country_france")] = QStringLiteral("فرنسا");
        vi[QStringLiteral("register_country_france")] = QStringLiteral("Pháp");

        zh[QStringLiteral("register_country_japan")] = QStringLiteral("日本");
        en[QStringLiteral("register_country_japan")] = QStringLiteral("Japan");
        ru[QStringLiteral("register_country_japan")] = QStringLiteral("Япония");
        fr[QStringLiteral("register_country_japan")] = QStringLiteral("Japon");
        ar[QStringLiteral("register_country_japan")] = QStringLiteral("اليابان");
        vi[QStringLiteral("register_country_japan")] = QStringLiteral("Nhật Bản");

        zh[QStringLiteral("register_country_korea")] = QStringLiteral("韩国");
        en[QStringLiteral("register_country_korea")] = QStringLiteral("South Korea");
        ru[QStringLiteral("register_country_korea")] = QStringLiteral("Южная Корея");
        fr[QStringLiteral("register_country_korea")] = QStringLiteral("Corée du Sud");
        ar[QStringLiteral("register_country_korea")] = QStringLiteral("كوريا الجنوبية");
        vi[QStringLiteral("register_country_korea")] = QStringLiteral("Hàn Quốc");

        zh[QStringLiteral("register_country_australia")] = QStringLiteral("澳大利亚");
        en[QStringLiteral("register_country_australia")] = QStringLiteral("Australia");
        ru[QStringLiteral("register_country_australia")] = QStringLiteral("Австралия");
        fr[QStringLiteral("register_country_australia")] = QStringLiteral("Australie");
        ar[QStringLiteral("register_country_australia")] = QStringLiteral("أستراليا");
        vi[QStringLiteral("register_country_australia")] = QStringLiteral("Úc");

        zh[QStringLiteral("register_country_canada")] = QStringLiteral("加拿大");
        en[QStringLiteral("register_country_canada")] = QStringLiteral("Canada");
        ru[QStringLiteral("register_country_canada")] = QStringLiteral("Канада");
        fr[QStringLiteral("register_country_canada")] = QStringLiteral("Canada");
        ar[QStringLiteral("register_country_canada")] = QStringLiteral("كندا");
        vi[QStringLiteral("register_country_canada")] = QStringLiteral("Canada");

        zh[QStringLiteral("register_country_singapore")] = QStringLiteral("新加坡");
        en[QStringLiteral("register_country_singapore")] = QStringLiteral("Singapore");
        ru[QStringLiteral("register_country_singapore")] = QStringLiteral("Сингапур");
        fr[QStringLiteral("register_country_singapore")] = QStringLiteral("Singapour");
        ar[QStringLiteral("register_country_singapore")] = QStringLiteral("سنغافورة");
        vi[QStringLiteral("register_country_singapore")] = QStringLiteral("Singapore");

        zh[QStringLiteral("register_country_malaysia")] = QStringLiteral("马来西亚");
        en[QStringLiteral("register_country_malaysia")] = QStringLiteral("Malaysia");
        ru[QStringLiteral("register_country_malaysia")] = QStringLiteral("Малайзия");
        fr[QStringLiteral("register_country_malaysia")] = QStringLiteral("Malaisie");
        ar[QStringLiteral("register_country_malaysia")] = QStringLiteral("ماليزيا");
        vi[QStringLiteral("register_country_malaysia")] = QStringLiteral("Malaysia");

        zh[QStringLiteral("register_country_thailand")] = QStringLiteral("泰国");
        en[QStringLiteral("register_country_thailand")] = QStringLiteral("Thailand");
        ru[QStringLiteral("register_country_thailand")] = QStringLiteral("Таиланд");
        fr[QStringLiteral("register_country_thailand")] = QStringLiteral("Thaïlande");
        ar[QStringLiteral("register_country_thailand")] = QStringLiteral("تايلاند");
        vi[QStringLiteral("register_country_thailand")] = QStringLiteral("Thái Lan");

        zh[QStringLiteral("register_country_india")] = QStringLiteral("印度");
        en[QStringLiteral("register_country_india")] = QStringLiteral("India");
        ru[QStringLiteral("register_country_india")] = QStringLiteral("Индия");
        fr[QStringLiteral("register_country_india")] = QStringLiteral("Inde");
        ar[QStringLiteral("register_country_india")] = QStringLiteral("الهند");
        vi[QStringLiteral("register_country_india")] = QStringLiteral("Ấn Độ");

        zh[QStringLiteral("register_country_russia")] = QStringLiteral("俄罗斯");
        en[QStringLiteral("register_country_russia")] = QStringLiteral("Russia");
        ru[QStringLiteral("register_country_russia")] = QStringLiteral("Россия");
        fr[QStringLiteral("register_country_russia")] = QStringLiteral("Russie");
        ar[QStringLiteral("register_country_russia")] = QStringLiteral("روسيا");
        vi[QStringLiteral("register_country_russia")] = QStringLiteral("Nga");

        zh[QStringLiteral("register_country_italy")] = QStringLiteral("意大利");
        en[QStringLiteral("register_country_italy")] = QStringLiteral("Italy");
        ru[QStringLiteral("register_country_italy")] = QStringLiteral("Италия");
        fr[QStringLiteral("register_country_italy")] = QStringLiteral("Italie");
        ar[QStringLiteral("register_country_italy")] = QStringLiteral("إيطاليا");
        vi[QStringLiteral("register_country_italy")] = QStringLiteral("Ý");

        zh[QStringLiteral("register_country_spain")] = QStringLiteral("西班牙");
        en[QStringLiteral("register_country_spain")] = QStringLiteral("Spain");
        ru[QStringLiteral("register_country_spain")] = QStringLiteral("Испания");
        fr[QStringLiteral("register_country_spain")] = QStringLiteral("Espagne");
        ar[QStringLiteral("register_country_spain")] = QStringLiteral("إسبانيا");
        vi[QStringLiteral("register_country_spain")] = QStringLiteral("Tây Ban Nha");

        zh[QStringLiteral("register_country_netherlands")] = QStringLiteral("荷兰");
        en[QStringLiteral("register_country_netherlands")] = QStringLiteral("Netherlands");
        ru[QStringLiteral("register_country_netherlands")] = QStringLiteral("Нидерланды");
        fr[QStringLiteral("register_country_netherlands")] = QStringLiteral("Pays-Bas");
        ar[QStringLiteral("register_country_netherlands")] = QStringLiteral("هولندا");
        vi[QStringLiteral("register_country_netherlands")] = QStringLiteral("Hà Lan");

        zh[QStringLiteral("register_country_switzerland")] = QStringLiteral("瑞士");
        en[QStringLiteral("register_country_switzerland")] = QStringLiteral("Switzerland");
        ru[QStringLiteral("register_country_switzerland")] = QStringLiteral("Швейцария");
        fr[QStringLiteral("register_country_switzerland")] = QStringLiteral("Suisse");
        ar[QStringLiteral("register_country_switzerland")] = QStringLiteral("سويسرا");
        vi[QStringLiteral("register_country_switzerland")] = QStringLiteral("Thụy Sĩ");

        zh[QStringLiteral("register_country_uae")] = QStringLiteral("阿联酋");
        en[QStringLiteral("register_country_uae")] = QStringLiteral("United Arab Emirates");
        ru[QStringLiteral("register_country_uae")] = QStringLiteral("ОАЭ");
        fr[QStringLiteral("register_country_uae")] = QStringLiteral("Émirats arabes unis");
        ar[QStringLiteral("register_country_uae")] = QStringLiteral("الإمارات العربية المتحدة");
        vi[QStringLiteral("register_country_uae")] = QStringLiteral("Các Tiểu vương quốc Ả Rập Thống nhất");

        zh[QStringLiteral("register_country_saudi")] = QStringLiteral("沙特阿拉伯");
        en[QStringLiteral("register_country_saudi")] = QStringLiteral("Saudi Arabia");
        ru[QStringLiteral("register_country_saudi")] = QStringLiteral("Саудовская Аравия");
        fr[QStringLiteral("register_country_saudi")] = QStringLiteral("Arabie saoudite");
        ar[QStringLiteral("register_country_saudi")] = QStringLiteral("المملكة العربية السعودية");
        vi[QStringLiteral("register_country_saudi")] = QStringLiteral("Ả Rập Xê Út");

        zh[QStringLiteral("register_country_turkey")] = QStringLiteral("土耳其");
        en[QStringLiteral("register_country_turkey")] = QStringLiteral("Turkey");
        ru[QStringLiteral("register_country_turkey")] = QStringLiteral("Турция");
        fr[QStringLiteral("register_country_turkey")] = QStringLiteral("Turquie");
        ar[QStringLiteral("register_country_turkey")] = QStringLiteral("تركيا");
        vi[QStringLiteral("register_country_turkey")] = QStringLiteral("Thổ Nhĩ Kỳ");

        zh[QStringLiteral("register_country_vietnam")] = QStringLiteral("越南");
        en[QStringLiteral("register_country_vietnam")] = QStringLiteral("Vietnam");
        ru[QStringLiteral("register_country_vietnam")] = QStringLiteral("Вьетнам");
        fr[QStringLiteral("register_country_vietnam")] = QStringLiteral("Viêt Nam");
        ar[QStringLiteral("register_country_vietnam")] = QStringLiteral("فيتنام");
        vi[QStringLiteral("register_country_vietnam")] = QStringLiteral("Việt Nam");

        zh[QStringLiteral("register_country_indonesia")] = QStringLiteral("印度尼西亚");
        en[QStringLiteral("register_country_indonesia")] = QStringLiteral("Indonesia");
        ru[QStringLiteral("register_country_indonesia")] = QStringLiteral("Индонезия");
        fr[QStringLiteral("register_country_indonesia")] = QStringLiteral("Indonésie");
        ar[QStringLiteral("register_country_indonesia")] = QStringLiteral("إندونيسيا");
        vi[QStringLiteral("register_country_indonesia")] = QStringLiteral("Indonesia");

        zh[QStringLiteral("register_country_philippines")] = QStringLiteral("菲律宾");
        en[QStringLiteral("register_country_philippines")] = QStringLiteral("Philippines");
        ru[QStringLiteral("register_country_philippines")] = QStringLiteral("Филиппины");
        fr[QStringLiteral("register_country_philippines")] = QStringLiteral("Philippines");
        ar[QStringLiteral("register_country_philippines")] = QStringLiteral("الفلبين");
        vi[QStringLiteral("register_country_philippines")] = QStringLiteral("Philippines");

        zh[QStringLiteral("register_country_brazil")] = QStringLiteral("巴西");
        en[QStringLiteral("register_country_brazil")] = QStringLiteral("Brazil");
        ru[QStringLiteral("register_country_brazil")] = QStringLiteral("Бразилия");
        fr[QStringLiteral("register_country_brazil")] = QStringLiteral("Brésil");
        ar[QStringLiteral("register_country_brazil")] = QStringLiteral("البرازيل");
        vi[QStringLiteral("register_country_brazil")] = QStringLiteral("Brazil");

        zh[QStringLiteral("register_country_mexico")] = QStringLiteral("墨西哥");
        en[QStringLiteral("register_country_mexico")] = QStringLiteral("Mexico");
        ru[QStringLiteral("register_country_mexico")] = QStringLiteral("Мексика");
        fr[QStringLiteral("register_country_mexico")] = QStringLiteral("Mexique");
        ar[QStringLiteral("register_country_mexico")] = QStringLiteral("المكسيك");
        vi[QStringLiteral("register_country_mexico")] = QStringLiteral("Mexico");

        zh[QStringLiteral("register_country_south_africa")] = QStringLiteral("南非");
        en[QStringLiteral("register_country_south_africa")] = QStringLiteral("South Africa");
        ru[QStringLiteral("register_country_south_africa")] = QStringLiteral("ЮАР");
        fr[QStringLiteral("register_country_south_africa")] = QStringLiteral("Afrique du Sud");
        ar[QStringLiteral("register_country_south_africa")] = QStringLiteral("جنوب أفريقيا");
        vi[QStringLiteral("register_country_south_africa")] = QStringLiteral("Nam Phi");

        zh[QStringLiteral("register_country_new_zealand")] = QStringLiteral("新西兰");
        en[QStringLiteral("register_country_new_zealand")] = QStringLiteral("New Zealand");
        ru[QStringLiteral("register_country_new_zealand")] = QStringLiteral("Новая Зеландия");
        fr[QStringLiteral("register_country_new_zealand")] = QStringLiteral("Nouvelle-Zélande");
        ar[QStringLiteral("register_country_new_zealand")] = QStringLiteral("نيوزيلندا");
        vi[QStringLiteral("register_country_new_zealand")] = QStringLiteral("New Zealand");

        zh[QStringLiteral("register_country_other")] = QStringLiteral("其他");
        en[QStringLiteral("register_country_other")] = QStringLiteral("Other");
        ru[QStringLiteral("register_country_other")] = QStringLiteral("Другая");
        fr[QStringLiteral("register_country_other")] = QStringLiteral("Autre");
        ar[QStringLiteral("register_country_other")] = QStringLiteral("أخرى");
        vi[QStringLiteral("register_country_other")] = QStringLiteral("Khác");

// ===== 省份下拉项（仅在所选国家为中国时显示，数据值保持中文） =====
        zh[QStringLiteral("register_province_beijing")] = QStringLiteral("北京市");
        en[QStringLiteral("register_province_beijing")] = QStringLiteral("Beijing");
        ru[QStringLiteral("register_province_beijing")] = QStringLiteral("Пекин");
        fr[QStringLiteral("register_province_beijing")] = QStringLiteral("Pékin");
        ar[QStringLiteral("register_province_beijing")] = QStringLiteral("بكين");
        vi[QStringLiteral("register_province_beijing")] = QStringLiteral("Bắc Kinh");

        zh[QStringLiteral("register_province_tianjin")] = QStringLiteral("天津市");
        en[QStringLiteral("register_province_tianjin")] = QStringLiteral("Tianjin");
        ru[QStringLiteral("register_province_tianjin")] = QStringLiteral("Тяньцзинь");
        fr[QStringLiteral("register_province_tianjin")] = QStringLiteral("Tianjin");
        ar[QStringLiteral("register_province_tianjin")] = QStringLiteral("تيانجين");
        vi[QStringLiteral("register_province_tianjin")] = QStringLiteral("Thiên Tân");

        zh[QStringLiteral("register_province_hebei")] = QStringLiteral("河北省");
        en[QStringLiteral("register_province_hebei")] = QStringLiteral("Hebei");
        ru[QStringLiteral("register_province_hebei")] = QStringLiteral("Хэбэй");
        fr[QStringLiteral("register_province_hebei")] = QStringLiteral("Hebei");
        ar[QStringLiteral("register_province_hebei")] = QStringLiteral("خبي");
        vi[QStringLiteral("register_province_hebei")] = QStringLiteral("Hà Bắc");

        zh[QStringLiteral("register_province_shanxi")] = QStringLiteral("山西省");
        en[QStringLiteral("register_province_shanxi")] = QStringLiteral("Shanxi");
        ru[QStringLiteral("register_province_shanxi")] = QStringLiteral("Шаньси");
        fr[QStringLiteral("register_province_shanxi")] = QStringLiteral("Shanxi");
        ar[QStringLiteral("register_province_shanxi")] = QStringLiteral("شانسي");
        vi[QStringLiteral("register_province_shanxi")] = QStringLiteral("Sơn Tây");

        zh[QStringLiteral("register_province_inner_mongolia")] = QStringLiteral("内蒙古自治区");
        en[QStringLiteral("register_province_inner_mongolia")] = QStringLiteral("Inner Mongolia");
        ru[QStringLiteral("register_province_inner_mongolia")] = QStringLiteral("Внутренняя Монголия");
        fr[QStringLiteral("register_province_inner_mongolia")] = QStringLiteral("Mongolie-Intérieure");
        ar[QStringLiteral("register_province_inner_mongolia")] = QStringLiteral("منغوليا الداخلية");
        vi[QStringLiteral("register_province_inner_mongolia")] = QStringLiteral("Nội Mông");

        zh[QStringLiteral("register_province_liaoning")] = QStringLiteral("辽宁省");
        en[QStringLiteral("register_province_liaoning")] = QStringLiteral("Liaoning");
        ru[QStringLiteral("register_province_liaoning")] = QStringLiteral("Ляонин");
        fr[QStringLiteral("register_province_liaoning")] = QStringLiteral("Liaoning");
        ar[QStringLiteral("register_province_liaoning")] = QStringLiteral("لياونينغ");
        vi[QStringLiteral("register_province_liaoning")] = QStringLiteral("Liêu Ninh");

        zh[QStringLiteral("register_province_jilin")] = QStringLiteral("吉林省");
        en[QStringLiteral("register_province_jilin")] = QStringLiteral("Jilin");
        ru[QStringLiteral("register_province_jilin")] = QStringLiteral("Цзилинь");
        fr[QStringLiteral("register_province_jilin")] = QStringLiteral("Jilin");
        ar[QStringLiteral("register_province_jilin")] = QStringLiteral("جيلين");
        vi[QStringLiteral("register_province_jilin")] = QStringLiteral("Cát Lâm");

        zh[QStringLiteral("register_province_heilongjiang")] = QStringLiteral("黑龙江省");
        en[QStringLiteral("register_province_heilongjiang")] = QStringLiteral("Heilongjiang");
        ru[QStringLiteral("register_province_heilongjiang")] = QStringLiteral("Хэйлунцзян");
        fr[QStringLiteral("register_province_heilongjiang")] = QStringLiteral("Heilongjiang");
        ar[QStringLiteral("register_province_heilongjiang")] = QStringLiteral("هيلونغجيانغ");
        vi[QStringLiteral("register_province_heilongjiang")] = QStringLiteral("Hắc Long Giang");

        zh[QStringLiteral("register_province_shanghai")] = QStringLiteral("上海市");
        en[QStringLiteral("register_province_shanghai")] = QStringLiteral("Shanghai");
        ru[QStringLiteral("register_province_shanghai")] = QStringLiteral("Шанхай");
        fr[QStringLiteral("register_province_shanghai")] = QStringLiteral("Shanghai");
        ar[QStringLiteral("register_province_shanghai")] = QStringLiteral("شنغهاي");
        vi[QStringLiteral("register_province_shanghai")] = QStringLiteral("Thượng Hải");

        zh[QStringLiteral("register_province_jiangsu")] = QStringLiteral("江苏省");
        en[QStringLiteral("register_province_jiangsu")] = QStringLiteral("Jiangsu");
        ru[QStringLiteral("register_province_jiangsu")] = QStringLiteral("Цзянсу");
        fr[QStringLiteral("register_province_jiangsu")] = QStringLiteral("Jiangsu");
        ar[QStringLiteral("register_province_jiangsu")] = QStringLiteral("جيانغسو");
        vi[QStringLiteral("register_province_jiangsu")] = QStringLiteral("Giang Tô");

        zh[QStringLiteral("register_province_zhejiang")] = QStringLiteral("浙江省");
        en[QStringLiteral("register_province_zhejiang")] = QStringLiteral("Zhejiang");
        ru[QStringLiteral("register_province_zhejiang")] = QStringLiteral("Чжэцзян");
        fr[QStringLiteral("register_province_zhejiang")] = QStringLiteral("Zhejiang");
        ar[QStringLiteral("register_province_zhejiang")] = QStringLiteral("تشجيانغ");
        vi[QStringLiteral("register_province_zhejiang")] = QStringLiteral("Chiết Giang");

        zh[QStringLiteral("register_province_anhui")] = QStringLiteral("安徽省");
        en[QStringLiteral("register_province_anhui")] = QStringLiteral("Anhui");
        ru[QStringLiteral("register_province_anhui")] = QStringLiteral("Аньхой");
        fr[QStringLiteral("register_province_anhui")] = QStringLiteral("Anhui");
        ar[QStringLiteral("register_province_anhui")] = QStringLiteral("أنهوي");
        vi[QStringLiteral("register_province_anhui")] = QStringLiteral("An Huy");

        zh[QStringLiteral("register_province_fujian")] = QStringLiteral("福建省");
        en[QStringLiteral("register_province_fujian")] = QStringLiteral("Fujian");
        ru[QStringLiteral("register_province_fujian")] = QStringLiteral("Фуцзянь");
        fr[QStringLiteral("register_province_fujian")] = QStringLiteral("Fujian");
        ar[QStringLiteral("register_province_fujian")] = QStringLiteral("فوجيان");
        vi[QStringLiteral("register_province_fujian")] = QStringLiteral("Phúc Kiến");

        zh[QStringLiteral("register_province_jiangxi")] = QStringLiteral("江西省");
        en[QStringLiteral("register_province_jiangxi")] = QStringLiteral("Jiangxi");
        ru[QStringLiteral("register_province_jiangxi")] = QStringLiteral("Цзянси");
        fr[QStringLiteral("register_province_jiangxi")] = QStringLiteral("Jiangxi");
        ar[QStringLiteral("register_province_jiangxi")] = QStringLiteral("جيانغشي");
        vi[QStringLiteral("register_province_jiangxi")] = QStringLiteral("Giang Tây");

        zh[QStringLiteral("register_province_shandong")] = QStringLiteral("山东省");
        en[QStringLiteral("register_province_shandong")] = QStringLiteral("Shandong");
        ru[QStringLiteral("register_province_shandong")] = QStringLiteral("Шаньдун");
        fr[QStringLiteral("register_province_shandong")] = QStringLiteral("Shandong");
        ar[QStringLiteral("register_province_shandong")] = QStringLiteral("شاندونغ");
        vi[QStringLiteral("register_province_shandong")] = QStringLiteral("Sơn Đông");

        zh[QStringLiteral("register_province_henan")] = QStringLiteral("河南省");
        en[QStringLiteral("register_province_henan")] = QStringLiteral("Henan");
        ru[QStringLiteral("register_province_henan")] = QStringLiteral("Хэнань");
        fr[QStringLiteral("register_province_henan")] = QStringLiteral("Henan");
        ar[QStringLiteral("register_province_henan")] = QStringLiteral("خنان");
        vi[QStringLiteral("register_province_henan")] = QStringLiteral("Hà Nam");

        zh[QStringLiteral("register_province_hubei")] = QStringLiteral("湖北省");
        en[QStringLiteral("register_province_hubei")] = QStringLiteral("Hubei");
        ru[QStringLiteral("register_province_hubei")] = QStringLiteral("Хубэй");
        fr[QStringLiteral("register_province_hubei")] = QStringLiteral("Hubei");
        ar[QStringLiteral("register_province_hubei")] = QStringLiteral("هوبي");
        vi[QStringLiteral("register_province_hubei")] = QStringLiteral("Hồ Bắc");

        zh[QStringLiteral("register_province_hunan")] = QStringLiteral("湖南省");
        en[QStringLiteral("register_province_hunan")] = QStringLiteral("Hunan");
        ru[QStringLiteral("register_province_hunan")] = QStringLiteral("Хунань");
        fr[QStringLiteral("register_province_hunan")] = QStringLiteral("Hunan");
        ar[QStringLiteral("register_province_hunan")] = QStringLiteral("خونان");
        vi[QStringLiteral("register_province_hunan")] = QStringLiteral("Hồ Nam");

        zh[QStringLiteral("register_province_guangdong")] = QStringLiteral("广东省");
        en[QStringLiteral("register_province_guangdong")] = QStringLiteral("Guangdong");
        ru[QStringLiteral("register_province_guangdong")] = QStringLiteral("Гуандун");
        fr[QStringLiteral("register_province_guangdong")] = QStringLiteral("Guangdong");
        ar[QStringLiteral("register_province_guangdong")] = QStringLiteral("غوانغدونغ");
        vi[QStringLiteral("register_province_guangdong")] = QStringLiteral("Quảng Đông");

        zh[QStringLiteral("register_province_guangxi")] = QStringLiteral("广西壮族自治区");
        en[QStringLiteral("register_province_guangxi")] = QStringLiteral("Guangxi");
        ru[QStringLiteral("register_province_guangxi")] = QStringLiteral("Гуанси");
        fr[QStringLiteral("register_province_guangxi")] = QStringLiteral("Guangxi");
        ar[QStringLiteral("register_province_guangxi")] = QStringLiteral("غوانغشي");
        vi[QStringLiteral("register_province_guangxi")] = QStringLiteral("Quảng Tây");

        zh[QStringLiteral("register_province_hainan")] = QStringLiteral("海南省");
        en[QStringLiteral("register_province_hainan")] = QStringLiteral("Hainan");
        ru[QStringLiteral("register_province_hainan")] = QStringLiteral("Хайнань");
        fr[QStringLiteral("register_province_hainan")] = QStringLiteral("Hainan");
        ar[QStringLiteral("register_province_hainan")] = QStringLiteral("هاينان");
        vi[QStringLiteral("register_province_hainan")] = QStringLiteral("Hải Nam");

        zh[QStringLiteral("register_province_chongqing")] = QStringLiteral("重庆市");
        en[QStringLiteral("register_province_chongqing")] = QStringLiteral("Chongqing");
        ru[QStringLiteral("register_province_chongqing")] = QStringLiteral("Чунцин");
        fr[QStringLiteral("register_province_chongqing")] = QStringLiteral("Chongqing");
        ar[QStringLiteral("register_province_chongqing")] = QStringLiteral("تشونغتشينغ");
        vi[QStringLiteral("register_province_chongqing")] = QStringLiteral("Trùng Khánh");

        zh[QStringLiteral("register_province_sichuan")] = QStringLiteral("四川省");
        en[QStringLiteral("register_province_sichuan")] = QStringLiteral("Sichuan");
        ru[QStringLiteral("register_province_sichuan")] = QStringLiteral("Сычуань");
        fr[QStringLiteral("register_province_sichuan")] = QStringLiteral("Sichuan");
        ar[QStringLiteral("register_province_sichuan")] = QStringLiteral("سيتشوان");
        vi[QStringLiteral("register_province_sichuan")] = QStringLiteral("Tứ Xuyên");

        zh[QStringLiteral("register_province_guizhou")] = QStringLiteral("贵州省");
        en[QStringLiteral("register_province_guizhou")] = QStringLiteral("Guizhou");
        ru[QStringLiteral("register_province_guizhou")] = QStringLiteral("Гуйчжоу");
        fr[QStringLiteral("register_province_guizhou")] = QStringLiteral("Guizhou");
        ar[QStringLiteral("register_province_guizhou")] = QStringLiteral("قويتشو");
        vi[QStringLiteral("register_province_guizhou")] = QStringLiteral("Quý Châu");

        zh[QStringLiteral("register_province_yunnan")] = QStringLiteral("云南省");
        en[QStringLiteral("register_province_yunnan")] = QStringLiteral("Yunnan");
        ru[QStringLiteral("register_province_yunnan")] = QStringLiteral("Юньнань");
        fr[QStringLiteral("register_province_yunnan")] = QStringLiteral("Yunnan");
        ar[QStringLiteral("register_province_yunnan")] = QStringLiteral("يونان");
        vi[QStringLiteral("register_province_yunnan")] = QStringLiteral("Vân Nam");

        zh[QStringLiteral("register_province_tibet")] = QStringLiteral("西藏自治区");
        en[QStringLiteral("register_province_tibet")] = QStringLiteral("Tibet");
        ru[QStringLiteral("register_province_tibet")] = QStringLiteral("Тибет");
        fr[QStringLiteral("register_province_tibet")] = QStringLiteral("Tibet");
        ar[QStringLiteral("register_province_tibet")] = QStringLiteral("التبت");
        vi[QStringLiteral("register_province_tibet")] = QStringLiteral("Tây Tạng");

        zh[QStringLiteral("register_province_shaanxi")] = QStringLiteral("陕西省");
        en[QStringLiteral("register_province_shaanxi")] = QStringLiteral("Shaanxi");
        ru[QStringLiteral("register_province_shaanxi")] = QStringLiteral("Шэньси");
        fr[QStringLiteral("register_province_shaanxi")] = QStringLiteral("Shaanxi");
        ar[QStringLiteral("register_province_shaanxi")] = QStringLiteral("شنشي");
        vi[QStringLiteral("register_province_shaanxi")] = QStringLiteral("Thiểm Tây");

        zh[QStringLiteral("register_province_gansu")] = QStringLiteral("甘肃省");
        en[QStringLiteral("register_province_gansu")] = QStringLiteral("Gansu");
        ru[QStringLiteral("register_province_gansu")] = QStringLiteral("Ганьсу");
        fr[QStringLiteral("register_province_gansu")] = QStringLiteral("Gansu");
        ar[QStringLiteral("register_province_gansu")] = QStringLiteral("قانسو");
        vi[QStringLiteral("register_province_gansu")] = QStringLiteral("Cam Túc");

        zh[QStringLiteral("register_province_qinghai")] = QStringLiteral("青海省");
        en[QStringLiteral("register_province_qinghai")] = QStringLiteral("Qinghai");
        ru[QStringLiteral("register_province_qinghai")] = QStringLiteral("Цинхай");
        fr[QStringLiteral("register_province_qinghai")] = QStringLiteral("Qinghai");
        ar[QStringLiteral("register_province_qinghai")] = QStringLiteral("تشينغهاي");
        vi[QStringLiteral("register_province_qinghai")] = QStringLiteral("Thanh Hải");

        zh[QStringLiteral("register_province_ningxia")] = QStringLiteral("宁夏回族自治区");
        en[QStringLiteral("register_province_ningxia")] = QStringLiteral("Ningxia");
        ru[QStringLiteral("register_province_ningxia")] = QStringLiteral("Нинся");
        fr[QStringLiteral("register_province_ningxia")] = QStringLiteral("Ningxia");
        ar[QStringLiteral("register_province_ningxia")] = QStringLiteral("نينغشيا");
        vi[QStringLiteral("register_province_ningxia")] = QStringLiteral("Ninh Hạ");

        zh[QStringLiteral("register_province_xinjiang")] = QStringLiteral("新疆维吾尔自治区");
        en[QStringLiteral("register_province_xinjiang")] = QStringLiteral("Xinjiang");
        ru[QStringLiteral("register_province_xinjiang")] = QStringLiteral("Синьцзян");
        fr[QStringLiteral("register_province_xinjiang")] = QStringLiteral("Xinjiang");
        ar[QStringLiteral("register_province_xinjiang")] = QStringLiteral("شينجيانغ");
        vi[QStringLiteral("register_province_xinjiang")] = QStringLiteral("Tân Cương");

        zh[QStringLiteral("register_province_taiwan")] = QStringLiteral("台湾省");
        en[QStringLiteral("register_province_taiwan")] = QStringLiteral("Taiwan");
        ru[QStringLiteral("register_province_taiwan")] = QStringLiteral("Тайвань");
        fr[QStringLiteral("register_province_taiwan")] = QStringLiteral("Taïwan");
        ar[QStringLiteral("register_province_taiwan")] = QStringLiteral("تايوان");
        vi[QStringLiteral("register_province_taiwan")] = QStringLiteral("Đài Loan");

        zh[QStringLiteral("register_province_hong_kong")] = QStringLiteral("香港特别行政区");
        en[QStringLiteral("register_province_hong_kong")] = QStringLiteral("Hong Kong");
        ru[QStringLiteral("register_province_hong_kong")] = QStringLiteral("Гонконг");
        fr[QStringLiteral("register_province_hong_kong")] = QStringLiteral("Hongkong");
        ar[QStringLiteral("register_province_hong_kong")] = QStringLiteral("هونغ كونغ");
        vi[QStringLiteral("register_province_hong_kong")] = QStringLiteral("Hồng Kông");

        zh[QStringLiteral("register_province_macao")] = QStringLiteral("澳门特别行政区");
        en[QStringLiteral("register_province_macao")] = QStringLiteral("Macao");
        ru[QStringLiteral("register_province_macao")] = QStringLiteral("Макао");
        fr[QStringLiteral("register_province_macao")] = QStringLiteral("Macao");
        ar[QStringLiteral("register_province_macao")] = QStringLiteral("ماكاو");
        vi[QStringLiteral("register_province_macao")] = QStringLiteral("Ma Cao");

// ===== 品牌下拉项（仅显示文本，数据值保持中文，与后端 brand 字段一致） =====
        zh[QStringLiteral("register_brand_mengying")] = QStringLiteral("猛鹰");
        en[QStringLiteral("register_brand_mengying")] = QStringLiteral("Mengying");
        ru[QStringLiteral("register_brand_mengying")] = QStringLiteral("Мэнъин");
        fr[QStringLiteral("register_brand_mengying")] = QStringLiteral("Mengying");
        ar[QStringLiteral("register_brand_mengying")] = QStringLiteral("منجين");
        vi[QStringLiteral("register_brand_mengying")] = QStringLiteral("Mengying");

        zh[QStringLiteral("register_brand_xiongtu")] = QStringLiteral("雄途");
        en[QStringLiteral("register_brand_xiongtu")] = QStringLiteral("Xiongtu");
        ru[QStringLiteral("register_brand_xiongtu")] = QStringLiteral("Сюнту");
        fr[QStringLiteral("register_brand_xiongtu")] = QStringLiteral("Xiongtu");
        ar[QStringLiteral("register_brand_xiongtu")] = QStringLiteral("شيونغ تو");
        vi[QStringLiteral("register_brand_xiongtu")] = QStringLiteral("Xiongtu");

        zh[QStringLiteral("register_brand_kaiqi")] = QStringLiteral("恺祁");
        en[QStringLiteral("register_brand_kaiqi")] = QStringLiteral("Kaiqi");
        ru[QStringLiteral("register_brand_kaiqi")] = QStringLiteral("Кайци");
        fr[QStringLiteral("register_brand_kaiqi")] = QStringLiteral("Kaiqi");
        ar[QStringLiteral("register_brand_kaiqi")] = QStringLiteral("كايكي");
        vi[QStringLiteral("register_brand_kaiqi")] = QStringLiteral("Kaiqi");

        zh[QStringLiteral("register_brand_shikasi")] = QStringLiteral("施卡斯");
        en[QStringLiteral("register_brand_shikasi")] = QStringLiteral("Shikasi");
        ru[QStringLiteral("register_brand_shikasi")] = QStringLiteral("Шикаси");
        fr[QStringLiteral("register_brand_shikasi")] = QStringLiteral("Shikasi");
        ar[QStringLiteral("register_brand_shikasi")] = QStringLiteral("شيكاسي");
        vi[QStringLiteral("register_brand_shikasi")] = QStringLiteral("Shikasi");

        zh[QStringLiteral("register_brand_lancaixing")] = QStringLiteral("篮彩星");
        en[QStringLiteral("register_brand_lancaixing")] = QStringLiteral("Lancaixing");
        ru[QStringLiteral("register_brand_lancaixing")] = QStringLiteral("Ланьцайсин");
        fr[QStringLiteral("register_brand_lancaixing")] = QStringLiteral("Lancaixing");
        ar[QStringLiteral("register_brand_lancaixing")] = QStringLiteral("لان تساي سين");
        vi[QStringLiteral("register_brand_lancaixing")] = QStringLiteral("Lancaixing");

        zh[QStringLiteral("register_brand_jijian")] = QStringLiteral("击剑");
        en[QStringLiteral("register_brand_jijian")] = QStringLiteral("Jijian");
        ru[QStringLiteral("register_brand_jijian")] = QStringLiteral("Цзыцзянь");
        fr[QStringLiteral("register_brand_jijian")] = QStringLiteral("Jijian");
        ar[QStringLiteral("register_brand_jijian")] = QStringLiteral("جي جيان");
        vi[QStringLiteral("register_brand_jijian")] = QStringLiteral("Jijian");

        zh[QStringLiteral("register_brand_suhang")] = QStringLiteral("速航");
        en[QStringLiteral("register_brand_suhang")] = QStringLiteral("Suhang");
        ru[QStringLiteral("register_brand_suhang")] = QStringLiteral("Сухан");
        fr[QStringLiteral("register_brand_suhang")] = QStringLiteral("Suhang");
        ar[QStringLiteral("register_brand_suhang")] = QStringLiteral("سو هانغ");
        vi[QStringLiteral("register_brand_suhang")] = QStringLiteral("Suhang");

        zh[QStringLiteral("register_brand_patemei")] = QStringLiteral("帕特美");
        en[QStringLiteral("register_brand_patemei")] = QStringLiteral("Patemei");
        ru[QStringLiteral("register_brand_patemei")] = QStringLiteral("Патэмэй");
        fr[QStringLiteral("register_brand_patemei")] = QStringLiteral("Patemei");
        ar[QStringLiteral("register_brand_patemei")] = QStringLiteral("با تيه مي");
        vi[QStringLiteral("register_brand_patemei")] = QStringLiteral("Patemei");

