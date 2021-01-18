#include "login.h"
#include "ui_login.h"
#include <QPainter>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "common/logininfoinstance.h"
#include "common/des.h"


Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);

    // 去掉创建的边框
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());

    // 设置当前窗口所有的字体
    this->setFont(QFont("新宋体", 12, QFont::Bold, false));

    // titlewidget信号处理
    connect(ui->title_wg, &TitleWg::closeWindow, [=]()
    {
        // 判断当前stackedWidget显示的页面
        if(ui->stackedWidget->currentWidget() == ui->set_page)
        {
            // 切换到登录
            ui->stackedWidget->setCurrentWidget(ui->login_page);
            // 清空控件数据
            ui->address->clear();
            ui->port->clear();
        }
        else if(ui->stackedWidget->currentWidget() == ui->reg_page)
        {
            // 切换到登录
            ui->stackedWidget->setCurrentWidget(ui->login_page);
            // 清空控件数据
            ui->name_reg->clear();
            ui->nike_name->clear();
            ui->passwd_reg->clear();
            ui->pwd_confirm_reg->clear();
            ui->phone->clear();
            ui->email->clear();
        }
        else
        {
            this->close();
        }
    });
    connect(ui->title_wg, &TitleWg::showSetWg, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->set_page);
        // 设置焦点
        ui->address->setFocus();
    });

    // 密码输入框 设置为密码方式显示
    ui->passwd_login->setEchoMode(QLineEdit::Password);
    ui->passwd_reg->setEchoMode(QLineEdit::Password);
    ui->pwd_confirm_reg->setEchoMode(QLineEdit::Password);

    ui->address->setText("192.168.1.25");
    ui->port->setText("80");

    p_main = new MainWindow;
    ui->stackedWidget->setCurrentWidget(ui->login_page);
    loadLoginConfig();
}

Login::~Login()
{
    delete ui;
}

// 将注册信息打包成json格式字符串
QByteArray Login::getRegisterJson(QString name, QString nick, QString passwd, QString phone, QString email)
{
//    {
//        userName:xxxx,
//        nickName:xxx,
//        firstPwd:xxx,
//        phone:xxx,
//        email:xxx
//     }

    QMap<QString, QVariant> reg;
    reg.insert("userName", name);
    reg.insert("nickName", nick);
    reg.insert("firstPwd", passwd);
    reg.insert("phone", phone);
    reg.insert("email", email);

    QJsonDocument doc = QJsonDocument::fromVariant(reg);
    cout << "reg json" << doc.toJson();

    return doc.toJson();
}

QByteArray Login::getLoginJson(QString name, QString passwd)
{
    QJsonObject obj;
    obj.insert("user", name);
    obj.insert("pwd", passwd);
    QJsonDocument doc(obj);
    QByteArray array = doc.toJson();

    return array;
}

QStringList Login::parseLoginJson(QByteArray json)
{
    QStringList list;
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if(doc.isObject())
    {
        QJsonObject obj = doc.object();
        list.append(obj.value("code").toString());
        list.append(obj.value("token").toString());
    }
    return list;
}

void Login::loadLoginConfig()
{
    QString user = m_cm.getCfgValue("login", "user");
    QString passwd = m_cm.getCfgValue("login", "pwd");
    QString remember = m_cm.getCfgValue("login", "remember");

    int len = 0;
    unsigned char buf[BUFSIZ];
    // 记住密码
    if(remember == "yes")
    {
        // 解码base64
        QByteArray pwd = QByteArray::fromBase64(passwd.toUtf8());
        // 解码des
        DesDec((unsigned char *)pwd.data(), pwd.size(), buf, &len);

        // 设置密码
        ui->passwd_login->setText(QString::fromUtf8((const char*)buf, len));
        ui->remember_pwd->setChecked(true);
    }
    else
    {
        ui->passwd_login->clear();
        ui->remember_pwd->setChecked(false);
    }

    // 用户名
    // 解码base64
    QByteArray usr = QByteArray::fromBase64(user.toUtf8());
    // 解码des
    DesDec((unsigned char*)usr.data(), usr.size(), buf, &len);

    // 设置用户名
    ui->user_login->setText(QString::fromUtf8((const char*)buf, len));
}

void Login::paintEvent(QPaintEvent *event)
{
    // 给窗口画背景图
    QPainter p(this);
    QPixmap pixmap(":/images/login_bk.jpg");
    p.drawPixmap(0, 0, this->width(), this->height(), pixmap);
}

// 没有账号, 注册一个按钮
void Login::on_toolButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->reg_page);
    // 设置焦点
    ui->name_reg->setFocus();
}

// 登录
void Login::on_login_btn_clicked()
{
    // 取值
    QString user = ui->user_login->text();
    QString passwd = ui->passwd_login->text();
    QString address = ui->address->text();
    QString port = ui->port->text();

    // 数据校验
    // 是否保存密码
    bool remember = ui->remember_pwd->isChecked();

    // 将数据保存到配置文件
    m_cm.writeLoginInfo(user, passwd, remember);
    // 登录的json数据包
    QByteArray data = getLoginJson(user, m_cm.getStrMd5(passwd));

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
    QString url = QString("http://%1:%2/login").arg(address).arg(port);
    request.setUrl(QUrl(url));
    QNetworkReply* reply = manager->post(request, data);
    connect(reply, &QNetworkReply::readyRead, [=]()
    {
        QByteArray json = reply->readAll();
        // 解析json字符串
        QStringList list = parseLoginJson(json);
        if(list.at(0) == "000")
        {
            // 登录成功
            // 当前用户信息存储起来
            LoginInfoInstance* info = LoginInfoInstance::getInstance();
            info->setLoginInfo(user, address, port, list.at(1));
            // 隐藏当前窗口
            this->hide();
            // 显示主窗口
            p_main->show();
        }
        else
        {
            // 登录失败
            QMessageBox::warning(this, "登录失败", "用户名或密码不正确！！！");
        }
    });
}

// 注册
void Login::on_reg_btn_clicked()
{
    // 取值 从输入框取得数据
    QString name = ui->name_reg->text();
    QString nickName = ui->nike_name->text();
    QString passwd = ui->passwd_reg->text();
    QString pwd_confirm = ui->pwd_confirm_reg->text();
    QString phone = ui->phone->text();
    QString email = ui->email->text();
    QString address = ui->address->text();
    QString port = ui->port->text();
    // 数据校验
    QRegExp regexp(USER_REG);
    if(!regexp.exactMatch(name))
    {
        QMessageBox::warning(this ,"警告","用户名格式不正确");
        //清除输入框中的内容
        ui->name_reg->clear();
        //光标定位到这里
        ui->name_reg->setFocus();
    }

     regexp.setPattern(USER_REG);
     if(!regexp.exactMatch(nickName))
     {
         QMessageBox::warning(this ,"警告","昵称格式不正确");
         //清除输入框中的内容
         ui->nike_name->clear();
         //光标定位到这里
         ui->nike_name->setFocus();
     }

    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(passwd))
    {
        QMessageBox::warning(this ,"警告","密码格式不正确");
        //清除输入框中的内容
        ui->passwd_reg->clear();
        //光标定位到这里
        ui->passwd_reg->setFocus();
    }


    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(pwd_confirm))
    {
        QMessageBox::warning(this ,"警告","确认密码格式不正确");
        //清除输入框中的内容
        ui->pwd_confirm_reg->clear();
        //光标定位到这里
        ui->pwd_confirm_reg->setFocus();
    }


    regexp.setPattern(PHONE_REG);
    if(!regexp.exactMatch(phone))
    {
        QMessageBox::warning(this ,"警告","手机号码格式不正确");
        //清除输入框中的内容
        ui->phone->clear();
        //光标定位到这里
        ui->phone->setFocus();
    }


    regexp.setPattern(EMAIL_REG);
    if(!regexp.exactMatch(email))
    {
        QMessageBox::warning(this ,"警告","邮箱格式不正确");
        //清除输入框中的内容
        ui->email->clear();
        //光标定位到这里
        ui->email->setFocus();
    }



    // 组织要发送的json字符串
    QByteArray data = getRegisterJson(name, nickName, m_cm.getStrMd5(passwd), phone, email);


    // 发送 http 请求协议给server - > post请求
    QNetworkAccessManager *manager = Common::getNetManager();
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
    QString url = QString("http://%1:%2/reg").arg(address).arg(port);
    request.setUrl(QUrl(url));
    QNetworkReply* reply = manager->post(request, data);
    // 读server发回的数据
    connect(reply, &QNetworkReply::readyRead, [=]()
    {
        QByteArray json = reply->readAll();
//      成功 	{"code":"002"}
//		该用户已存在	{"code":"003"}
//		失败	{"code":"004"}
        QString status = m_cm.getCode(json);
        if("002" == status)
        {
            // 成功提示
            QMessageBox::information(this, "恭喜", "用户名注册成功");
            // 清空注册页面
            ui->name_reg->clear();
            ui->nike_name->clear();
            ui->passwd_reg->clear();
            ui->pwd_confirm_reg->clear();
            ui->phone->clear();
            ui->email->clear();
            // 将数据设置到登录窗口控件中
            ui->user_login->setText(name);
            ui->passwd_login->setText(passwd);
            // 切换到登录页面
            ui->stackedWidget->setCurrentWidget(ui->login_page);
        }
        else if("003" == status)
        {
            QMessageBox::warning(this, "注册失败", QString("[%1]该用户已经存在!!!").arg(name));
        }
        else
        {
            QMessageBox::warning(this, "注册失败", "注册失败！！！");
        }
        // 释放资源
        reply->deleteLater();
    });

}

// 服务器设置 - 确定按钮
void Login::on_reg_btn_2_clicked()
{
    // 读数据
    QString address = ui->address->text();
    QString port = ui->port->text();

    // 数据校验
    QRegExp reg(IP_REG);
    if(!reg.exactMatch(address))
    {
        QMessageBox::warning(this, "警告", "IP地址格式错误, 请重新输入!");
        ui->address->clear();
        ui->address->setFocus();
        return;
    }
    // 端口
    // 数据写入配置文件中
    m_cm.writeWebInfo(address, port);
    // 跳转到登录界面
    ui->stackedWidget->setCurrentWidget(ui->login_page);
}
