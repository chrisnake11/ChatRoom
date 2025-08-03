#include "registerdialog.h"
#include "ui_registerdialog.h"
#include <QMessageBox>
#include "global.h"
#include <QRegularExpression>
#include "httpmanager.h"
#include "timerbtn.h"
#include <QLayout>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
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
        if(layout){
            layout->replaceWidget(old_btn, new_btn);
            old_btn->hide();
        }
        old_btn->hide();
        old_btn->deleteLater();

        // 强制类型转换并重新赋值给 UI 指针
        ui->get_code_btn = new_btn;
    }


    qDebug() << "get_code_btn is of type:" << ui->get_code_btn->metaObject()->className();


    // 绑定返回按钮点击事件
    connect(ui->return_btn, &QPushButton::clicked, this, &RegisterDialog::switch_to_login);

    // 绑定Http注册事件结束的信号
    connect(HttpManager::getInstance().get(), &HttpManager::sig_reg_mod_finish,
            this, &RegisterDialog::on_reg_mod_finish);

    // register btn clicked
    connect(ui->register_submit_btn, &QPushButton::clicked, this, &RegisterDialog::on_register_btn_clicked);

    // get varify code btn clicked
    connect(ui->get_code_btn, &QPushButton::clicked, this, &RegisterDialog::on_get_code_btn_clicked);

    showTip(tr("enter name and passwd to registe"), true);

    initHttpHandlers();

    // 绑定输入框检查事件
    connect(ui->name_text_reg, &QLineEdit::editingFinished, this, [this]() {
        checkNameValid();
        });

    connect(ui->email_text_reg, &QLineEdit::editingFinished, this, [this]() {
        checkEmailValid();
        });

    connect(ui->passwd_text_reg, &QLineEdit::editingFinished, this, [this]() {
        checkPasswdValid();
        });

    connect(ui->confirm_text_reg, &QLineEdit::editingFinished, this, [this]() {
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
            ui->passwd_text_reg->setEchoMode(QLineEdit::Password);
        }
        else {
            ui->passwd_text_reg->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "passwd visible label was clicked, state: " << state;
    });

    connect(ui->confirm_visible, &clickedlabel::clicked, [this]() {
        auto state = ui->confirm_visible->getCurrentState();
        if (state == ClickLabelState::NORMAL) {
            ui->confirm_text_reg->setEchoMode(QLineEdit::Password);
        }
        else {
            ui->confirm_text_reg->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "confirm visible label was clicked, state: " << state;
        });



}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}


void RegisterDialog::on_get_code_btn_clicked()
{
    // 读取表单数据
    auto email = ui->email_text_reg->text();
    // 格式检查
    static QRegularExpression email_regex(R"(^\w+(-+.\w+)*@\w+(-.\w+)*.\w+(-.\w+)*$)");
    if(!email_regex.match(email).hasMatch()){
        showTip(tr("email format error"), false);
        qDebug("email format error");
        return;
    }
    else{
        // 发送获取验证码的http请求
        QJsonObject json_obj;
        json_obj["email"] = email;
        json_obj["error"] = 0;
        HttpManager::getInstance()->postHttpRequest(QUrl(gate_url_prefix + "/get_varify_code"), json_obj, RequestID::ID_GET_VARIFY_CODE, Modules::REG_MODEL);
    }
}

void RegisterDialog::on_register_btn_clicked(){
    auto name = ui->name_text_reg->text();
    auto passwd = ui->passwd_text_reg->text();
    auto confirm = ui->confirm_text_reg->text();
    auto email = ui->email_text_reg->text();
    auto code = ui->code_text->text();
    // check text format
    if(name == ""){
        showTip("name can't be empty", false);
        return;
    }
    if(email == ""){
        showTip("email can't be empty", false);
        return;
    }
    if(passwd == ""){
        showTip("password can't be empty", false);
        return;
    }
    if(confirm != passwd){
        showTip("confirm not match password", false);
        return;
    }
    if(code == ""){
        showTip("code can't be empty", false);
        return;
    }

    QJsonObject json_obj;
    json_obj["name"] = name;
    json_obj["passwd"] = passwd;
    json_obj["confirm"] = confirm;
    // 加密后传输
    //json_obj["passwd"] = md5Encrypt(passwd);
    //json_obj["confirm"] = md5Encrypt(confirm);
    json_obj["email"] = email;
    json_obj["code"] = code;

    // send register http request
    HttpManager::getInstance()->postHttpRequest(QUrl(gate_url_prefix + "/user_register"),
                                                json_obj, RequestID::ID_REG_USER, Modules::REG_MODEL);

}

void RegisterDialog::showTip(QString str, bool success)
{
    if(success){
        ui->err_tip->setProperty("state", "normal");
    }
    else{
        ui->err_tip->setProperty("state", "error");
    }
    repolish(ui->err_tip);
    ui->err_tip->setText(str);
}

void RegisterDialog::on_reg_mod_finish(RequestID id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("network request failed"), false);
        return;
    }
    // parse json string
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());

    if(jsonDoc.isNull()){
        showTip(tr("json parse failed"), false);
        return;
    }
    if(!jsonDoc.isObject()){
        showTip(tr("json parse failed"), false);
        return;
    }

    // 根据消息ID，使用不同的回调函数处理收到的请求
    _handlers[id](jsonDoc.object());
    return;

}

void RegisterDialog::initHttpHandlers()
{
    // 注册获取验证码的回调函数
    _handlers.insert(RequestID::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("param error"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("varified code has been sent to email"), true);
    });

    // 注册事件的回调函数
    _handlers.insert(RequestID::ID_REG_USER, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        switch(error){
            case ErrorCodes::SUCCESS:
                showTip(tr("register success"), true);
                qDebug("register success, emit signals");
                emit this->register_success();
                return;
            case ErrorCodes::ERROR_USER_EXIST:
                showTip(tr("user name is already exists!"), false);
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

bool RegisterDialog::checkNameValid()
{
    if (ui->name_text_reg->text() == "") {
        addTipErr(TipErr::TIP_USER_ERR, tr("name can't be empty"));
        return false;
    }
    delTipErr(TipErr::TIP_USER_ERR);
    return true;;
}

bool RegisterDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_text_reg->text();
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

bool RegisterDialog::checkPasswdValid()
{
    auto pass = ui->passwd_text_reg->text();
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

bool RegisterDialog::checkConfirmValid()
{
    auto pass = ui->passwd_text_reg->text();
    auto confirm = ui->confirm_text_reg->text();
    if (pass != confirm) {
        addTipErr(TipErr::TIP_CONFIRM_ERR, tr("密码不一致"));
        return false;
    }
    delTipErr(TipErr::TIP_CONFIRM_ERR);
    return true;
}

bool RegisterDialog::checkCodeValid()
{
    auto pass = ui->code_text->text();
    if (pass.isEmpty()) {
        addTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }
    delTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

void RegisterDialog::addTipErr(TipErr err, QString tips)
{
    _tip_errs[err] = tips;
    showTip(tips, false);
}

void RegisterDialog::delTipErr(TipErr err)
{
    _tip_errs.remove(err);
    if (_tip_errs.size() == 0) {
        ui->err_tip->clear();
        return;
    }

    showTip(_tip_errs.first(), false);
}
