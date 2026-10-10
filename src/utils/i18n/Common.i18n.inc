// Common 模块翻译数据（由 I18nManager.cpp 在 langMap 构建块内包含）
// 每行形如：  zh[QStringLiteral("key")] = QStringLiteral("文本");
// 必须同时提供 zh/en/ru/fr/ar/vi 六种语言，键前缀唯一

// ---------- SDX4 设备（XriteDevice）提示 ----------
zh[QStringLiteral("dev_unknown_error")] = QStringLiteral("未知错误");
en[QStringLiteral("dev_unknown_error")] = QStringLiteral("Unknown error");
ru[QStringLiteral("dev_unknown_error")] = QStringLiteral("Неизвестная ошибка");
fr[QStringLiteral("dev_unknown_error")] = QStringLiteral("Erreur inconnue");
ar[QStringLiteral("dev_unknown_error")] = QStringLiteral("خطأ غير معروف");
vi[QStringLiteral("dev_unknown_error")] = QStringLiteral("Lỗi không xác định");

zh[QStringLiteral("dev_dll_not_loaded")] = QStringLiteral("MAC.dll 未加载，SDX4 设备不可用");
en[QStringLiteral("dev_dll_not_loaded")] = QStringLiteral("MAC.dll is not loaded; SDX4 device unavailable");
ru[QStringLiteral("dev_dll_not_loaded")] = QStringLiteral("MAC.dll не загружен, устройство SDX4 недоступно");
fr[QStringLiteral("dev_dll_not_loaded")] = QStringLiteral("MAC.dll non chargée ; périphérique SDX4 indisponible");
ar[QStringLiteral("dev_dll_not_loaded")] = QStringLiteral("لم يتم تحميل MAC.dll؛ جهاز SDX4 غير متاح");
vi[QStringLiteral("dev_dll_not_loaded")] = QStringLiteral("Chưa tải MAC.dll; thiết bị SDX4 không khả dụng");

zh[QStringLiteral("dev_dll_missing")] = QStringLiteral("MAC.dll 未加载");
en[QStringLiteral("dev_dll_missing")] = QStringLiteral("MAC.dll not loaded");
ru[QStringLiteral("dev_dll_missing")] = QStringLiteral("MAC.dll не загружен");
fr[QStringLiteral("dev_dll_missing")] = QStringLiteral("MAC.dll non chargée");
ar[QStringLiteral("dev_dll_missing")] = QStringLiteral("لم يتم تحميل MAC.dll");
vi[QStringLiteral("dev_dll_missing")] = QStringLiteral("Chưa tải MAC.dll");

// 结尾空格用于与设备返回的原始错误串拼接，各语言均保留
zh[QStringLiteral("dev_connect_failed")] = QStringLiteral("连接失败: ");
en[QStringLiteral("dev_connect_failed")] = QStringLiteral("Connection failed: ");
ru[QStringLiteral("dev_connect_failed")] = QStringLiteral("Ошибка подключения: ");
fr[QStringLiteral("dev_connect_failed")] = QStringLiteral("Échec de la connexion : ");
ar[QStringLiteral("dev_connect_failed")] = QStringLiteral("فشل الاتصال: ");
vi[QStringLiteral("dev_connect_failed")] = QStringLiteral("Kết nối thất bại: ");

zh[QStringLiteral("dev_not_connected")] = QStringLiteral("设备未连接，请先连接设备");
en[QStringLiteral("dev_not_connected")] = QStringLiteral("Device not connected, please connect it first");
ru[QStringLiteral("dev_not_connected")] = QStringLiteral("Устройство не подключено, сначала подключите его");
fr[QStringLiteral("dev_not_connected")] = QStringLiteral("Périphérique non connecté, veuillez le connecter d'abord");
ar[QStringLiteral("dev_not_connected")] = QStringLiteral("الجهاز غير متصل، يرجى توصيله أولاً");
vi[QStringLiteral("dev_not_connected")] = QStringLiteral("Thiết bị chưa kết nối, vui lòng kết nối trước");

zh[QStringLiteral("dev_not_calibrated")] = QStringLiteral("设备未校准，请先进行校准");
en[QStringLiteral("dev_not_calibrated")] = QStringLiteral("Device not calibrated, please calibrate it first");
ru[QStringLiteral("dev_not_calibrated")] = QStringLiteral("Устройство не калибровано, сначала выполните калибровку");
fr[QStringLiteral("dev_not_calibrated")] = QStringLiteral("Périphérique non calibré, veuillez le calibrer d'abord");
ar[QStringLiteral("dev_not_calibrated")] = QStringLiteral("الجهاز غير معاير، يرجى معايرته أولاً");
vi[QStringLiteral("dev_not_calibrated")] = QStringLiteral("Thiết bị chưa hiệu chuẩn, vui lòng hiệu chuẩn trước");

zh[QStringLiteral("dev_trigger_failed")] = QStringLiteral("测量触发失败: ");
en[QStringLiteral("dev_trigger_failed")] = QStringLiteral("Measurement trigger failed: ");
ru[QStringLiteral("dev_trigger_failed")] = QStringLiteral("Не удалось запустить измерение: ");
fr[QStringLiteral("dev_trigger_failed")] = QStringLiteral("Échec du déclenchement de la mesure : ");
ar[QStringLiteral("dev_trigger_failed")] = QStringLiteral("فشل تشغيل القياس: ");
vi[QStringLiteral("dev_trigger_failed")] = QStringLiteral("Kích hoạt đo thất bại: ");

zh[QStringLiteral("dev_measure_timeout")] = QStringLiteral("测量超时，请重试");
en[QStringLiteral("dev_measure_timeout")] = QStringLiteral("Measurement timed out, please retry");
ru[QStringLiteral("dev_measure_timeout")] = QStringLiteral("Время измерения истекло, повторите попытку");
fr[QStringLiteral("dev_measure_timeout")] = QStringLiteral("Délai de mesure dépassé, veuillez réessayer");
ar[QStringLiteral("dev_measure_timeout")] = QStringLiteral("انتهت مهلة القياس، يرجى إعادة المحاولة");
vi[QStringLiteral("dev_measure_timeout")] = QStringLiteral("Hẹn giờ đo đã hết, vui lòng thử lại");

zh[QStringLiteral("dev_read_spectral_failed")] = QStringLiteral("读取光谱数据失败：无效的光谱集/波长数量");
en[QStringLiteral("dev_read_spectral_failed")] = QStringLiteral("Failed to read spectral data: invalid spectral set or wavelength count");
ru[QStringLiteral("dev_read_spectral_failed")] = QStringLiteral("Не удалось прочитать спектральные данные: недопустимое число наборов или длин волн");
fr[QStringLiteral("dev_read_spectral_failed")] = QStringLiteral("Échec de lecture des données spectrales : jeu spectral ou nombre de longueurs d'onde invalide");
ar[QStringLiteral("dev_read_spectral_failed")] = QStringLiteral("تعذر قراءة البيانات الطيفية: مجموعة طيفية أو عدد أطوال موجية غير صالح");
vi[QStringLiteral("dev_read_spectral_failed")] = QStringLiteral("Không đọc được dữ liệu quang phổ: bộ quang phổ hoặc số bước sóng không hợp lệ");

// ---------- HttpClient 错误提示 ----------
zh[QStringLiteral("http_network_error")] = QStringLiteral("网络请求失败，请检查后端服务是否可访问");
en[QStringLiteral("http_network_error")] = QStringLiteral("Request failed, please check that the backend service is reachable");
ru[QStringLiteral("http_network_error")] = QStringLiteral("Не удалось выполнить запрос, проверьте доступность сервера");
fr[QStringLiteral("http_network_error")] = QStringLiteral("La requête a échoué, vérifiez que le backend est accessible");
ar[QStringLiteral("http_network_error")] = QStringLiteral("فشل الطلب، يرجى التحقق من إمكانية الوصول إلى الخادم");
vi[QStringLiteral("http_network_error")] = QStringLiteral("Yêu cầu thất bại, vui lòng kiểm tra backend có khả dụng không");

zh[QStringLiteral("http_session_expired")] = QStringLiteral("登录已过期，请重新登录");
en[QStringLiteral("http_session_expired")] = QStringLiteral("Session expired, please sign in again");
ru[QStringLiteral("http_session_expired")] = QStringLiteral("Сессия истекла, войдите снова");
fr[QStringLiteral("http_session_expired")] = QStringLiteral("Session expirée, veuillez vous reconnecter");
ar[QStringLiteral("http_session_expired")] = QStringLiteral("انتهت الجلسة، يرجى تسجيل الدخول مجددًا");
vi[QStringLiteral("http_session_expired")] = QStringLiteral("Phiên đã hết hạn, vui lòng đăng nhập lại");

zh[QStringLiteral("http_bad_response")] = QStringLiteral("后端返回数据格式异常: ");
en[QStringLiteral("http_bad_response")] = QStringLiteral("Unexpected data from backend: ");
ru[QStringLiteral("http_bad_response")] = QStringLiteral("Сервер вернул данные неверного формата: ");
fr[QStringLiteral("http_bad_response")] = QStringLiteral("Données inattendues du backend : ");
ar[QStringLiteral("http_bad_response")] = QStringLiteral("بيانات غير متوقعة من الخادم: ");
vi[QStringLiteral("http_bad_response")] = QStringLiteral("Dữ liệu bất thường từ backend: ");

zh[QStringLiteral("http_business_failed")] = QStringLiteral("业务处理失败");
en[QStringLiteral("http_business_failed")] = QStringLiteral("Request could not be processed");
ru[QStringLiteral("http_business_failed")] = QStringLiteral("Не удалось обработать запрос");
fr[QStringLiteral("http_business_failed")] = QStringLiteral("La requête n'a pas pu être traitée");
ar[QStringLiteral("http_business_failed")] = QStringLiteral("تعذرت معالجة الطلب");
vi[QStringLiteral("http_business_failed")] = QStringLiteral("Không thể xử lý yêu cầu");
