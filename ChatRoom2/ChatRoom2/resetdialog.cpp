#include "resetdialog.h"
#include "httpmanager.h"
#include "timerbtn.h"
ResetDialog::ResetDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::ResetDialogClass())
{
	ui->setupUi(this);
	ui->err_tip->clear();

	// 替换掉get_code_btn为自定义控件 timerbtn
	{
		// 保存旧的 QPushButton 占位符
		QPushButton* old_btn = ui->get_code_btn;

		// 构造新的 timerbtn 替代
		timerbtn* new_btn = new timerbtn(this);
		new_btn->setObjectName("get_code_btn");
		new_btn->setText(old_btn->text());

		// 如果有布局，则替换到原位置
		QLayout* layout = old_btn->parentWidget()->layout();
		if (layout) {
			layout->replaceWidget(old_btn, new_btn);
			old_btn->hide();
		}
		old_btn->hide();
		old_btn->deleteLater();

		// 强制类型转换并重新赋值给 UI 指针
		ui->get_code_btn = new_btn;
	}
	qDebug() << "get_code_btn is of type:" << ui->get_code_btn->metaObject()->className();

	connect(ui->return_btn, &QPushButton::clicked, this, &ResetDialog::switch_to_login);

	connect(HttpManager::getInstance().get(), &HttpManager::sig_reset_mod_finish, this, &ResetDialog::on_reset_mod_finish);

	// on_xxx_do 不需要绑定，QT会自动绑定槽函数
	connect(ui->get_code_btn, &QPushButton::clicked, this, &ResetDialog::on_get_code_btn_clicked);

	connect(ui->reset_btn, &QPushButton::clicked, this, &ResetDialog::on_reset_btn_clicked);

	showTip(tr("enter email and passwd to reset"), true);

	initHttpHandlers();

	// 绑定输入框检查事件
	connect(ui->email_text, &QLineEdit::editingFinished, this, [this]() {
		checkEmailValid();
		});

	connect(ui->passwd_text, &QLineEdit::editingFinished, this, [this]() {
		checkPasswdValid();
		});

	connect(ui->confirm_text, &QLineEdit::editingFinished, this, [this]() {
		checkConfirmValid();
		});

	connect(ui->code_text, &QLineEdit::editingFinished, this, [this]() {
		checkCodeValid();
		});

	// 设置密码可见按钮的悬浮光标
	ui->passwd_visible->setCursor(Qt::PointingHandCursor);
	ui->confirm_visible->setCursor(Qt::PointingHandCursor);

	// 设置按钮初始状态
	ui->passwd_visible->setState("unvisible", "unvisible_hover", "", "visible",
		"visible_hover", "");
	ui->confirm_visible->setState("unvisible", "unvisible_hover", "", "visible",
		"visible_hover", "");

	// 密码可见性按钮状态点击切换
	connect(ui->passwd_visible, &clickedlabel::clicked, [this]() {
		auto state = ui->passwd_visible->getCurrentState();
		if (state == ClickLabelState::NORMAL) {
			ui->passwd_text->setEchoMode(QLineEdit::Password);
		}
		else {
			ui->passwd_text->setEchoMode(QLineEdit::Normal);
		}
		qDebug() << "passwd visible label was clicked, state: " << state;
		});

	connect(ui->confirm_visible, &clickedlabel::clicked, [this]() {
		auto state = ui->confirm_visible->getCurrentState();
		if (state == ClickLabelState::NORMAL) {
			ui->confirm_text->setEchoMode(QLineEdit::Password);
		}
		else {
			ui->confirm_text->setEchoMode(QLineEdit::Normal);
		}
		qDebug() << "confirm visible label was clicked, state: " << state;
		});

}

ResetDialog::~ResetDialog()
{
	delete ui;
}

void ResetDialog::on_get_code_btn_clicked() {
	auto email = ui->email_text->text();
	// 格式检查
	QRegularExpression email_regex(R"(^\w+(-+.\w+)*@\w+(-.\w+)*.\w+(-.\w+)*$)");
	if (!email_regex.match(email).hasMatch()) {
        showTip(tr("email format error"), false);
		qDebug() << "email format error";
		return;
	}
    showTip(tr("getting varify code..."), true);
	QJsonObject request;
	request["email"] = email;
	request["error"] = 0;
	HttpManager::getInstance()->postHttpRequest(QUrl(gate_url_prefix + "/" + "get_varify_code"), request, RequestID::ID_GET_VARIFY_CODE, Modules::RESET_MODEL);
}

void ResetDialog::on_reset_mod_finish(RequestID id, QString res, ErrorCodes err) {
	if (err != ErrorCodes::SUCCESS) {
		showTip(tr("Get code failed, Http reuqest error"), false);
		qDebug() << "Get code failed, Http reuqest error";
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());

	if (doc.isNull() || !doc.isObject()) {
		showTip(tr("Get code failed, Json parse error"), false);
		qDebug() << "Get code failed, Json parse error";
		return;
	}

	_request_handlers[id](doc.object());
}

void ResetDialog::on_reset_btn_clicked() {
	auto email = ui->email_text->text();
	auto passwd = ui->passwd_text->text();
	auto confirm = ui->confirm_text->text();
	auto code = ui->code_text->text();
	// check text format
	if (email == "") {
		showTip("email can't be empty", false);
		return;
	}
	if (passwd == "") {
		showTip("password can't be empty", false);
		return;
	}
	if (confirm != passwd) {
		showTip("confirm not match password", false);
		return;
	}
	if (code == "") {
		showTip("code can't be empty", false);
		return;
	}

	QJsonObject json_obj;
	json_obj["email"] = email;
	json_obj["passwd"] = md5Encrypt(passwd);
	json_obj["confirm"] = md5Encrypt(confirm);
	json_obj["code"] = code;

	// send register http request
	HttpManager::getInstance()->postHttpRequest(QUrl(gate_url_prefix + "/user_reset"),
		json_obj, RequestID::ID_RESET_USER, Modules::RESET_MODEL);
}

void ResetDialog::initHttpHandlers() {
	_request_handlers.insert(RequestID::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj) {
		int error = jsonObj["error"].toInt();
		if (error != ErrorCodes::SUCCESS) {
			showTip(tr("param error"), false);
			return;
		}
		showTip(tr("varified code has been sent to email"), true);
		});

	_request_handlers.insert(RequestID::ID_RESET_USER, [this](const QJsonObject& jsonObj) {
		int error = jsonObj["error"].toInt();
		switch (error) {
		case ErrorCodes::SUCCESS:
			showTip(tr("reset success"), true);
			qDebug("reset success, emit signals");
			emit this->reset_success();
			return;
		case ErrorCodes::ERROR_EMAIL_NOT_MATCH:
			showTip(tr("email is not exists!"), false);
			return;
		case ErrorCodes::ERROR_VARIFY_EXPIRED:
			showTip(tr("ERROR_VARIFY_EXPIRED!"), false);
			return;
		case ErrorCodes::ERROR_VARIFY_WRONG:
			showTip(tr("ERROR_VARIFY_WRONG!"), false);
			return;
		case ErrorCodes::ERROR_PASSWORD_INVALID:
			showTip(tr("ERROR_PASSWORD_INVALID!"), false);
			return;
		default:
			showTip(tr("unknow error"), false);
			qDebug("Unknown error: ");
			return;
		}
		});
}


bool ResetDialog::checkEmailValid()
{
	//验证邮箱的地址正则表达式
	auto email = ui->email_text->text();
	// 邮箱地址的正则表达式
	QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
	bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
	if (!match) {
		//提示邮箱不正确
		addTipErr(TipErr::TIP_EMAIL_ERR, tr("email address is incorrect"));
		return false;
	}
	delTipErr(TipErr::TIP_EMAIL_ERR);
	return true;
}

bool ResetDialog::checkPasswdValid()
{
	auto pass = ui->passwd_text->text();
	if (pass.length() < 6 || pass.length() > 15) {
		//提示长度不准确
		addTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
		return false;
	}
	// 创建一个正则表达式对象，按照上述密码要求
	// 这个正则表达式解释：
	// ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
	QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
	bool match = regExp.match(pass).hasMatch();
	if (!match) {
		//提示字符非法
		addTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
		return false;;
	}
	delTipErr(TipErr::TIP_PWD_ERR);
	return true;
}

bool ResetDialog::checkConfirmValid()
{
	auto pass = ui->passwd_text->text();
	auto confirm = ui->confirm_text->text();
	if (pass != confirm) {
		addTipErr(TipErr::TIP_CONFIRM_ERR, tr("密码不一致"));
		return false;
	}
	delTipErr(TipErr::TIP_CONFIRM_ERR);
	return true;
}

bool ResetDialog::checkCodeValid()
{
	auto pass = ui->code_text->text();
	if (pass.isEmpty()) {
		addTipErr(TipErr::TIP_VARIFY_ERR, tr("code can not be empty"));
		return false;
	}
	delTipErr(TipErr::TIP_VARIFY_ERR);
	return true;
}

void ResetDialog::showTip(QString str, bool success)
{
	if (success) {
		ui->err_tip->setProperty("state", "normal");
	}
	else {
		ui->err_tip->setProperty("state", "error");
	}
	repolish(ui->err_tip);
	ui->err_tip->setText(str);
}
void ResetDialog::addTipErr(TipErr err, QString tips)
{
	_tip_errs[err] = tips;
	showTip(tips, false);
}

void ResetDialog::delTipErr(TipErr err)
{
	_tip_errs.remove(err);
	if (_tip_errs.size() == 0) {
		ui->err_tip->clear();
		return;
	}

	showTip(_tip_errs.first(), false);
}
