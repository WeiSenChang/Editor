#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QList>
#include <QPoint>
#include <QFile>
#include "mycursor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent*);//画笔

    void keyPressEvent(QKeyEvent* event);//键盘按下
    void keyReleaseEvent(QKeyEvent* event);//键盘松开

    void mousePressEvent(QMouseEvent *event);//鼠标单击
    void mouseMoveEvent(QMouseEvent *event);//鼠标移动

    void inputMethodEvent(QInputMethodEvent *event);//输入法切换

    void wheelEvent(QWheelEvent *event);//鼠标滚轮

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateMaxLineCol();//更新窗口最大可显示行列
    void setXY();//设置选中文本的先后位置
    void resetXY();//重置选中文本的先后位置
    void setChooseText();//选中文本后输入将覆盖选中的文本
    void resetCursor();//重置光标位置，使之回到窗口显示
    void drawChooseTextBK(QPainter& painter);//对选中的文本进行标识
    void drawWindowText(QPainter& painter);//在窗口显示文本

signals://信号
    void sendBreak();//退出线程

private slots://槽
    void recevieCursorStatus(bool isShow);//光标是否显示

    void on_action_triggered();//新建文件

    void on_action_2_triggered();//打开文件

    void on_actionbaocun_triggered();//保存文件

    void on_action_3_triggered();//文件另存为

private:
    Ui::MainWindow *ui;

    int mTSize;//字体大小
    int mLineMax;//窗口最大显示行数
    int mColMax;//窗口最大显示列数
    int mLineIdx;//开始显示的行数
    int mColIdx;//开始显示的列数
    int mStarX;//选中文本的前x位置
    int mStarY;//选中文本的前y位置
    int mEndX;//选中文本的后x位置
    int mEndY;//选中文本的后y位置
    bool mIsCursorShow;//光标是否显示
    bool mIsCtrlDown;//Ctrl键是否按下

    QList<QString> mLineList;//队列，保存每行的文本数据
    QString mFileName;//文件名
    QFile *mFile;//文件指针

    MyCursor* cursorTread;//线程

    QPoint mCursorPoint;//光标所在坐标点
    QPoint mStartPoint;//鼠标开始点击的坐标点
    QPoint mEndPoint;//鼠标最后停留的坐标点
};
#endif // MAINWINDOW_H
