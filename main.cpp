#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);//创建QApplication实例，把argc、argv参数交给Qt处理
    MainWindow w;//新建一个窗口
    w.show();//使窗口显示
    return a.exec();//将程序交给Qt处理，进入循环
}
