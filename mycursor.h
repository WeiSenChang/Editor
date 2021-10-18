#ifndef MYCURSOR_H
#define MYCURSOR_H

#include <QThread>

class MyCursor:public QThread
{
    Q_OBJECT;
public:
    MyCursor(QObject *parent = nullptr);

signals://信号
    void sendCursorStatus(bool isShow);//发送是否显示光标的消息

private slots://槽
    void receiveBreak();//接收是否关闭线程的消息

protected:
    void run();//多线程运行

private:
    bool mIsShow;//光标是否显示
    bool mIsBreak;//线程是否关闭
};

#endif // MYCURSOR_H
