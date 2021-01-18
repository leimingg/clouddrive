#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include "common/common.h"
#include "mainwindow.h"

namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = 0);
    ~Login();

    // 注册信息打包函数
    QByteArray getRegisterJson(QString name, QString nick, QString passwd, QString phone, QString email);
    // 登录信息打包函数
    QByteArray getLoginJson(QString name, QString passwd);
    // 解析服务器返回的json字符串
    QStringList parseLoginJson(QByteArray json);
    // 登录时加载配置文件
    void loadLoginConfig();


protected:
    // 绘图事件函数
    void paintEvent(QPaintEvent *event);

private slots:
    void on_toolButton_clicked();

    void on_login_btn_clicked();

    void on_reg_btn_clicked();

    void on_reg_btn_2_clicked();

private:
    Ui::Login *ui;

    Common m_cm;
    MainWindow* p_main;
};

#endif // LOGIN_H
