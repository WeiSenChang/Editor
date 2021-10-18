#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QKeyEvent>
#include <QColor>
#include <QFont>
#include <QtMath>
#include <QWheelEvent>
#include <QFileDialog>
#include <QClipboard>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->mFile=new QFile();

    this->mLineIdx=0;
    this->mColIdx=0;
    this->mTSize=20;

    //初始化窗口最大可显示行列数
    this->mLineMax=0;
    while((++this->mLineMax+1)*this->mTSize<this->height());
    (--this->mLineMax)--;
    this->mColMax=0;
    while(++this->mColMax*this->mTSize<this->width());
    --this->mColMax;

    //初始化文本队列
    this->mLineList.insert(0,"");

    //初始化状态
    this->mIsCursorShow=false;
    this->mIsCtrlDown=false;

    this->cursorTread=new MyCursor();//新建线程对象

    //和线程通信
    connect(this->cursorTread,SIGNAL(sendCursorStatus(bool)),this,SLOT(recevieCursorStatus(bool)));//线程向进程发送光标是否显示的信号
    connect(this,SIGNAL(sendBreak()),this->cursorTread,SLOT(receiveBreak()));//进程向线程发送是否关闭线程的信号

    //开始线程
    this->cursorTread->start();

    //初始化光标位置
    this->mCursorPoint.setX(0);
    this->mCursorPoint.setY(0);
    this->mStartPoint.setX(0);
    this->mStartPoint.setY(0);
    this->mEndPoint.setX(0);
    this->mEndPoint.setY(0);
    this->setXY();

    //获取到系统输入法状态
    this->setAttribute(Qt::WA_InputMethodEnabled,true);
}

MainWindow::~MainWindow()
{
    emit this->sendBreak();//发送退出线程的消息
    this->cursorTread->wait(500);//等待500毫秒
    this->cursorTread->exit(0);//结束线程
    delete this->mFile;//销毁文件指针
    delete this->cursorTread;//销毁线程
    delete ui;//销毁ui
}

//画笔事件
void MainWindow::paintEvent(QPaintEvent*){

    this->updateMaxLineCol();//更新窗口最大可显示的行列数

    QPainter painter(this);

    //绘制背景
    painter.setPen(QColor(255,255,255));
    for(int x=0;x<this->width();x++){
        painter.drawLine(x,24+this->mTSize,x,this->height());
    }
    painter.setPen(QColor(64,64,64));
    for(int y=22;y<44;y++){
        painter.drawLine(0,y,this->width(),y);
    }

    this->drawChooseTextBK(painter);

    this->drawWindowText(painter);

    //绘制状态栏
    int _y=this->mCursorPoint.y();
    int _x=this->mCursorPoint.x();
    painter.setFont(QFont("楷体",12));
    painter.setPen(QColor(255,255,255));
    QString tmpStr="Line："+QString::number(_y+1+this->mLineIdx)+
            "，Col："+QString::number(_x+1+this->mColIdx);
    painter.drawText(this->width()-(tmpStr.length()-1)*14,
                     38,tmpStr);

    //绘制光标
    if(this->mIsCursorShow){
        painter.setPen(QColor(0,0,0));
    }else{
        painter.setPen(QColor(255,255,255));
    }
    if(_y>=0&&_y<=this->mLineMax-1){
        painter.drawLine(_x*this->mTSize,_y*this->mTSize+this->mTSize*9/7+24,_x*this->mTSize,(_y+1)*this->mTSize+24+this->mTSize);
    }

}

//将选中的文本进行标识
void MainWindow::drawChooseTextBK(QPainter& painter){
    painter.setPen(QColor(120,120,255));
    int tmpStarX=this->mStarX,tmpEndX=this->mEndX;
    for(int i=this->mStarY;i<=this->mEndY;i++){
        int tmpColLen=this->mLineList[i+this->mLineIdx].length()-this->mColIdx<this->mColMax+this->mColIdx?
                    this->mLineList[i+this->mLineIdx].length()-this->mColIdx:this->mColMax+this->mColIdx;
        if(i>=0&&i<=this->mLineMax-1){
            if(this->mStarY!=this->mEndY){
                tmpStarX=tmpStarX<0?0:tmpStarX;
                tmpEndX=tmpEndX>this->mColMax?this->mColMax:tmpEndX;
                if(i==this->mStarY){
                    for(int y=i*this->mTSize+this->mTSize*9/7+24;y<=(i+1)*this->mTSize+24+this->mTSize;y++){
                        painter.drawLine(tmpStarX*this->mTSize,y,tmpColLen*this->mTSize,y);
                    }
                }else if(i>this->mStarY&&i<this->mEndY){
                    for(int y=i*this->mTSize+this->mTSize*9/7+24;y<=(i+1)*this->mTSize+24+this->mTSize;y++){
                        painter.drawLine(0,y,tmpColLen*this->mTSize,y);
                    }
                }else if(i==this->mEndY){
                    for(int y=i*this->mTSize+this->mTSize*9/7+24;y<=(i+1)*this->mTSize+24+this->mTSize;y++){
                        painter.drawLine(0,y,tmpEndX*this->mTSize,y);
                    }
                }
            }else if(this->mStarX!=this->mEndX){
                for(int y=i*this->mTSize+this->mTSize*9/7+24;y<=(i+1)*this->mTSize+24+this->mTSize;y++){
                    painter.drawLine(this->mStarX*this->mTSize,y,this->mEndX*this->mTSize,y);
                }
            }
        }
    }
    this->update();
}

//将文本输出到窗口显示
void MainWindow::drawWindowText(QPainter& painter){
    int tmpLineLen=this->mLineList.size()<this->mLineMax+this->mLineIdx?
                this->mLineList.size():this->mLineMax+this->mLineIdx;
    for(int i=this->mLineIdx;i<tmpLineLen;i++){
        painter.setFont(QFont("楷体",this->mTSize*5/7));
        int tmpColLen=this->mLineList[i].length()<this->mColMax+this->mColIdx?
                    this->mLineList[i].length():this->mColMax+this->mColIdx;
        for(int j=this->mColIdx;j<tmpColLen;j++){
            if(this->mStarY!=this->mEndY){
                if(i==this->mStarY+this->mLineIdx&&j>=this->mStarX+this->mColIdx){
                    painter.setPen(QColor(255,255,255));
                }else if(i>this->mStarY+this->mLineIdx&&i<this->mEndY+this->mLineIdx){
                    painter.setPen(QColor(255,255,255));
                }else if(i==this->mEndY+this->mLineIdx&&j<this->mEndX+this->mColIdx){
                    painter.setPen(QColor(255,255,255));
                }else{
                    painter.setPen(QColor(0,0,0));
                }
            }else{
                if(i==this->mStarY+this->mLineIdx&&j>=this->mStarX+this->mColIdx&&j<this->mEndX+this->mColIdx){
                    painter.setPen(QColor(255,255,255));
                }else{
                    painter.setPen(QColor(0,0,0));
                }
            }
            painter.drawText((j-this->mColIdx)*this->mTSize,
            24+this->mTSize+this->mTSize*(i-this->mLineIdx+1),this->mLineList.at(i).at(j));
        }
    }
    this->update();
}

//接收光标是否显示的信号
void MainWindow::recevieCursorStatus(bool isShow){
    this->mIsCursorShow=isShow;
    this->update();
}

//键盘按下事件
void MainWindow::keyPressEvent(QKeyEvent *event){
    //获取当前光标位置
    int _x=this->mCursorPoint.x()+this->mColIdx;
    int _y=this->mCursorPoint.y()+this->mLineIdx;
    //临时变量
    QString tmpLine=this->mLineList[_y];
    QString _text=event->text();
    QString tabStr="    ";

    switch(event->key()){
    case Qt::Key_Left://向左方向键（光标向左移动）
        if(_x>this->mColIdx){
            this->mCursorPoint.setX(_x-1-this->mColIdx);
        }else if(this->mColIdx>0){
            this->mColIdx--;
        }else if(_y>this->mLineIdx){
            this->mCursorPoint.setY(_y-1-this->mLineIdx);
            int tmpColMax=this->mLineList[_y-1].length()>this->mColMax?
                        this->mColMax:this->mLineList[_y-1].length();
            this->mCursorPoint.setX(tmpColMax);
            if(this->mLineList[_y-1].length()>this->mColMax){
                this->mColIdx=this->mLineList[_y-1].length()-this->mColMax;
            }else{
                this->mColIdx=0;
            }
        }
        break;
    case Qt::Key_Right://向右方向键（光标向右移动）
    {
        int tmpColMax=this->mLineList[_y].length()>this->mColMax?
                    this->mColMax:this->mLineList[_y].length();
        if(_x<tmpColMax+this->mColIdx){
            this->mCursorPoint.setX(_x+1-this->mColIdx);
        }else if(this->mColIdx<this->mLineList[_y].length()-this->mColMax){
            this->mColIdx++;
        }else if(_y<this->mLineList.size()-1){
            this->mCursorPoint.setX(0);
            this->mCursorPoint.setY(_y+1-this->mLineIdx);
            this->mColIdx=0;
        }
        break;
    }
    case Qt::Key_Up://向上方向键（光标向上移动）
        if(_y>this->mLineIdx){
            this->mCursorPoint.setY(_y-1-this->mLineIdx);
        }else if(this->mLineIdx>0){
            this->mLineIdx--;
        }else{
            return;
        }
        if(_x>this->mLineList[_y-1].length()-this->mColIdx){
            int tmpColMax=this->mLineList[_y-1].length()>this->mColMax?
                        this->mColMax:this->mLineList[_y-1].length();
            this->mCursorPoint.setX(tmpColMax);
            if(this->mLineList[_y-1].length()>this->mColMax){
                this->mColIdx=this->mLineList[_y-1].length()-this->mColMax;
            }else{
                this->mColIdx=0;
            }
        }
        break;
    case Qt::Key_Down://向下方向键（光标向下移动）
        if(_y<this->mLineList.size()-1){
            this->mCursorPoint.setY(_y+1-this->mLineIdx);
        }else if(this->mLineIdx<this->mLineList.size()-this->mLineMax){
            this->mLineIdx++;
        }else{
            return;
        }
        if(_x>this->mLineList[_y+1].length()-this->mColIdx){
            int tmpColMax=this->mLineList[_y+1].length()>this->mColMax?
                        this->mColMax:this->mLineList[_y+1].length();
            this->mCursorPoint.setX(tmpColMax);
            if(this->mLineList[_y+1].length()>this->mColMax){
                this->mColIdx=this->mLineList[_y+1].length()-this->mColMax;
            }else{
                this->mColIdx=0;
            }
        }
        break;
    case Qt::Key_Enter://回车
        if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){
            this->setChooseText();
        }else{
            QString rightStr=tmpLine.right(tmpLine.length()-_x);//将光标所在行的所在列的右边的字符串取出
            this->mLineList.insert(_y+1,rightStr);//并插入到下一行
            this->mLineList[_y]=tmpLine.left(_x);//重新赋值之前光标所在行
            this->mCursorPoint.setX(0);//光标x位置置为0
            this->mColIdx=0;//列偏移量置为0
            if(_y<this->mLineMax-1){//当前光标位置不在底部时
                this->mCursorPoint.setY(_y+1-this->mLineIdx);//光标y位置增加
            }else{
                this->mLineIdx++;//否则行偏移量增加
            }
        }
        break;
    case Qt::Key_Return://换行（和回车一致）
        if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){
            this->setChooseText();
        }else{
            QString rightStr=tmpLine.right(tmpLine.length()-_x);
            this->mLineList.insert(_y+1,rightStr);
            this->mLineList[_y]=tmpLine.left(_x);
            this->mCursorPoint.setX(0);
            this->mColIdx=0;
            if(_y<this->mLineMax-1+this->mLineIdx){
                this->mCursorPoint.setY(_y+1-this->mLineIdx);
            }else{
                this->mLineIdx++;
            }
        }
        break;
    case Qt::Key_Backspace://退格键往前删一个字符
        if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){//选中时只覆盖选中文本，不继续删除字符
            this->setChooseText();
            break;
        }
        if(_x>0){//光标x位置大于0
            this->mLineList[_y]=tmpLine.left(_x-1)+tmpLine.right(tmpLine.length()-_x);
            if(this->mColIdx>0){//列偏移量大于0时，先减少列偏移量
                this->mColIdx--;
            }else{
                this->mCursorPoint.setX(_x-1-this->mColIdx);//否则光标往前移
            }
        }else if(_y>0){//光标y位置大于0
            int tmpColMax=this->mLineList[_y-1].length()>this->mColMax?
                        this->mColMax:this->mLineList[_y-1].length();
            this->mCursorPoint.setX(tmpColMax);//光标x位置移至列尾
            if(this->mLineList[_y-1].length()>this->mColMax){//重新设置列偏移量
                this->mColIdx=this->mLineList[_y-1].length()-this->mColMax;
            }else{
                this->mColIdx=0;
            }
            if(this->mLineIdx>0){//行偏移量大于0时，先减少行偏移量
                this->mLineIdx--;
            }else{
                this->mCursorPoint.setY(_y-1-this->mLineIdx);//否则光标往上移
            }
            if(this->mLineList[_y-1].length()==0){//如果前一行为空，删除该行
                this->mLineList.removeAt(_y-1);
            }else{
                this->mLineList[_y-1]+=tmpLine;//否则重新赋值该行
                this->mLineList.removeAt(_y);//并删除之前光标所在行
            }
        }
        break;
    case Qt::Key_Delete://Del键往后删一个字符
        if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){//选中时只覆盖选中文本，不继续删除字符
            this->setChooseText();
            break;
        }
        if(_x<this->mLineList[_y].length()){//光标位置小于这一行的长度
            this->mLineList[_y]=tmpLine.left(_x)+tmpLine.right(tmpLine.length()-_x-1);
        }else if(_y<this->mLineList.size()-1){//下一行移至本行，并删除下一行
            this->mLineList[_y]+=this->mLineList[_y+1];
            this->mLineList.removeAt(_y+1);
        }
        break;
    case Qt::Key_Tab://Tab键输入4个空格
        this->mLineList[_y]=tmpLine.left(_x)+tabStr+tmpLine.right(tmpLine.length()-_x);
        if(_x+tabStr.length()<=this->mColMax+this->mColIdx){
            this->mCursorPoint.setX(_x+tabStr.length()-this->mColIdx);
        }else{
            this->mColIdx+=_x+tabStr.length()-this->mColMax-this->mColIdx;
        }
        break;
    case Qt::Key_End://End键去到列尾
    {
        int tmpColMax=this->mLineList[_y].length()>this->mColMax?
                    this->mColMax:this->mLineList[_y].length();
        if(this->mLineList[_y].length()>this->mColMax){
            this->mColIdx=this->mLineList[_y].length()-this->mColMax;
        }
        this->mCursorPoint.setX(tmpColMax);
        break;
    }
    case Qt::Key_Home://Hone键回到列首
        this->mColIdx=0;
        this->mCursorPoint.setX(0);
        break;
    case Qt::Key_Control://ctrl键
        this->mIsCtrlDown=true;
        break;
    case Qt::Key_A:
        if(this->mIsCtrlDown){//按下ctrl键时全选
            this->mEndY=this->mLineList.size()-1-this->mLineIdx;
            this->mStarY=-this->mLineIdx;
            this->mEndX=this->mLineList[this->mLineList.size()-1].length()-this->mColIdx;
            this->mStarX=-this->mColIdx;
        }else{//否则输入a
            if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){
                this->setChooseText();
            }
            this->mLineList[_y]=tmpLine.left(_x)+_text+tmpLine.right(tmpLine.length()-_x);
            if(_x+_text.length()<this->mColMax+this->mColIdx){
                this->mCursorPoint.setX(_x+_text.length()-this->mColIdx);
            }else{
                this->mColIdx+=_x+_text.length()-this->mColMax+this->mColIdx;
            }
        }
        break;
    case Qt::Key_C:
        if(this->mIsCtrlDown){//按下ctrl键时复制
            QString selectText="";
            QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
            if(this->mStarY==this->mEndY){
                QString tmpLine= this->mLineList[mStarY+this->mLineIdx];
                selectText+=tmpLine.mid(this->mStarX+this->mColIdx,this->mEndX-this->mStarX);
            }else{
                for(int i=this->mStarY;i<=this->mEndY;i++){
                    if(i==this->mStarY){
                        selectText+=this->mLineList[i+this->mLineIdx].right(this->mLineList[i+this->mLineIdx].length()-this->mStarX-this->mColIdx)+"\n";
                    }else if(i!=this->mEndY){
                        selectText+=this->mLineList[i+this->mLineIdx]+"\n";
                    }else{
                        selectText+=this->mLineList[i+this->mLineIdx].left(this->mEndX+this->mColIdx)+"\n";
                    }
                }
            }
            clipboard->setText(selectText);//设置剪切板文本信息
        }else{//否则输入c
            if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){
                this->setChooseText();
            }
            this->mLineList[_y]=tmpLine.left(_x)+_text+tmpLine.right(tmpLine.length()-_x);
            if(_x+_text.length()<this->mColMax+this->mColIdx){
                this->mCursorPoint.setX(_x+_text.length()-this->mColIdx);
            }else{
                this->mColIdx+=_x+_text.length()-this->mColMax+this->mColIdx;
            }
        }
        break;
    case Qt::Key_V://按下ctrl时黏贴
        if(this->mIsCtrlDown){
            if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){
                this->setChooseText();
            }
            _x=this->mCursorPoint.x()+this->mColIdx;
            _y=this->mCursorPoint.y()+this->mLineIdx;
            QClipboard *clipboard = QApplication::clipboard();//获取系统剪贴板指针
            QVector<QVector<int>> tmpInt;
            int count=0;
            for(int i=0;i<clipboard->text().length();i++){
                QString str=clipboard->text().at(i);
                if(str=="\n"){
                    tmpInt.push_back({count++,i});
                }
            }
            if(tmpInt.size()==0){
                this->mLineList[_y]=tmpLine.left(_x)+clipboard->text()+tmpLine.right(tmpLine.length()-_x);
                if(_x+clipboard->text().length()<this->mColMax+this->mColIdx){
                    this->mCursorPoint.setX(_x+clipboard->text().length()-this->mColIdx);
                }else{
                    this->mColIdx+=_x+clipboard->text().length()-this->mColMax-this->mColIdx;
                    this->mCursorPoint.setX(this->mColMax);
                }
            }else{
                int tmpLen;
                QString tmpStr=this->mLineList[_y].right(this->mLineList[_y].length()-_x);
                for(int i=0;i<tmpInt.size();i++){
                    if(i==0){
                        this->mLineList[_y]=tmpLine.left(_x)+clipboard->text().left(tmpInt[i][1]);
                    }else {
                        tmpLen=tmpInt[i][1]-tmpInt[i-1][1]-1;
                        this->mLineList.insert(_y+i,clipboard->text().mid(tmpInt[i-1][1]+1,tmpLen));
                    }
                    if(i==tmpInt.size()-1){
                        this->mLineList[_y+i]+=tmpStr;
                    }
                }
                if(tmpLen<this->mColMax){
                    this->mCursorPoint.setX(tmpLen-this->mColIdx);
                }else{
                    this->mColIdx+=tmpLen-this->mColMax;
                    this->mCursorPoint.setX(this->mColMax);
                }
                if(_y+tmpInt.size()-this->mLineIdx<this->mLineMax){
                    this->mCursorPoint.setY(_y+tmpInt.size()-this->mLineIdx-1);
                }else{
                    this->mCursorPoint.setY(this->mLineMax-1);
                    this->mLineIdx+=_y+tmpInt.size()-this->mLineIdx-mLineMax;
                }

            }
        }else{//否则输入v
            if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){
                this->setChooseText();
            }
            this->mLineList[_y]=tmpLine.left(_x)+_text+tmpLine.right(tmpLine.length()-_x);
            if(_x+_text.length()<=this->mColMax+this->mColIdx){
                this->mCursorPoint.setX(_x+_text.length()-this->mColIdx);
            }else{
                this->mColIdx+=_x+_text.length()-this->mColMax-this->mColIdx;
            }
        }
        break;
    default:
        //英文输入
        if(event->key()>=0&&event->key()<=127){
            if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){
                this->setChooseText();
            }
            this->mLineList[_y]=tmpLine.left(_x)+_text+tmpLine.right(tmpLine.length()-_x);
            if(_x+_text.length()<=this->mColMax+this->mColIdx){
                this->mCursorPoint.setX(_x+_text.length()-this->mColIdx);
            }else{
                this->mColIdx+=_x+_text.length()-this->mColMax-this->mColIdx;
            }
        }
        break;
    }
    if(!this->mIsCtrlDown){
        this->resetCursor();//如果光标不在当前窗口显示，使之重新回到当前窗口
    }
    this->update();
}

//按键弹起
void MainWindow::keyReleaseEvent(QKeyEvent* event){
    switch(event->key()){
    case Qt::Key_Control:
        this->mIsCtrlDown=false;
        break;
    }
}

//获取（中文）输入法
void MainWindow::inputMethodEvent(QInputMethodEvent *event){
    QString _text=event->commitString();
    if(_text.length()>0){
        //覆盖选中的文本
        if(this->mEndY!=this->mStarY||this->mEndX!=this->mStarX){
            this->setChooseText();
        }
        //获取当前光标位置
        int _x=this->mCursorPoint.x()+this->mColIdx;
        int _y=this->mCursorPoint.y()+this->mLineIdx;
        QString tmpLine=this->mLineList[_y];//获取当前文本行
        this->mLineList[_y]=tmpLine.left(_x)+_text+tmpLine.right(tmpLine.length()-_x);//更新当前文本行
        //重新设置光标x位置
        if(_x+_text.length()<this->mColMax+this->mColIdx){
            this->mCursorPoint.setX(_x+_text.length()-this->mColIdx);
        }else{
            this->mCursorPoint.setX(this->mColMax);
            this->mColIdx=this->mLineList[_y].length()-this->mColMax;
        }
        this->resetCursor();//如果光标不在当前窗口显示，使之重新回到当前窗口
        this->update();
    }
}

//鼠标点击事件
void MainWindow::mousePressEvent(QMouseEvent *event){
    if(event->button()==Qt::LeftButton){
        if(event->x()>=0&&event->y()>=44&&event->x()<this->width()&&event->y()<this->height()){
            //获取点击位置
            int _y=(event->y()-44)/this->mTSize;
            _y=this->mLineList.size()-1<_y?this->mLineList.size()-1:_y;
            int _x=round((double)event->x()/this->mTSize);
            _x=_x>this->mLineList[_y+this->mLineIdx].length()-this->mColIdx?this->mLineList[_y+this->mLineIdx].length()-this->mColIdx:_x;
            //设置光标、文本选中开始和结束位置
            this->mCursorPoint.setX(_x);
            this->mCursorPoint.setY(_y);
            this->resetCursor();
            this->mStartPoint.setX(_x);
            this->mStartPoint.setY(_y);
            this->mEndPoint.setX(_x);
            this->mEndPoint.setY(_y);
            this->setXY();
            this->update();
        }
    }
}

//鼠标移动事件
void MainWindow::mouseMoveEvent(QMouseEvent *event){
    //获取移动后位置
    int tmpLineMax=this->mLineList.size()>this->mLineMax?this->mLineMax:this->mLineList.size();
    if(event->y()>=44){
        int _y=(event->y()-44)/this->mTSize;
        _y=tmpLineMax-1<_y?tmpLineMax-1:_y;
        this->mEndPoint.setY(_y);
    }else{
        this->mEndPoint.setY(0);
    }
    int tmpColMax=this->mLineList[this->mEndPoint.y()+this->mLineIdx].length()-this->mColIdx>this->mColMax?
                this->mColMax:this->mLineList[this->mEndPoint.y()+this->mLineIdx].length()-this->mColIdx;
    if(event->x()>=0){
        int _x=round((double)event->x()/this->mTSize);
        _x=_x>tmpColMax?tmpColMax:_x;
        this->mEndPoint.setX(_x);
    }else{
        this->mEndPoint.setX(0);
    }
    //简单的越界处理
    if(event->y()>this->height()){//y>窗口高度
        if(this->mLineIdx+this->mLineMax<=this->mLineList.size()-3){
            this->mLineIdx+=3;
            this->mStartPoint.setY(this->mStartPoint.y()-3);
            this->mCursorPoint.setY(this->mCursorPoint.y()-3);
        }else if(this->mLineIdx+this->mLineMax<=this->mLineList.size()-2){
            this->mLineIdx+=2;
            this->mStartPoint.setY(this->mStartPoint.y()-2);
            this->mCursorPoint.setY(this->mCursorPoint.y()-2);
        }else if(this->mLineIdx+this->mLineMax<=this->mLineList.size()-1){
            this->mLineIdx+=1;
            this->mStartPoint.setY(this->mStartPoint.y()-1);
            this->mCursorPoint.setY(this->mCursorPoint.y()-1);
        }
    }else if(event->y()<44){//y<顶部状态栏高度
        if(this->mLineIdx>=3){
            this->mLineIdx-=3;
            this->mStartPoint.setY(this->mStartPoint.y()+3);
            this->mCursorPoint.setY(this->mCursorPoint.y()+3);
        }else if(this->mLineIdx>=2){
            this->mLineIdx-=2;
            this->mStartPoint.setY(this->mStartPoint.y()+2);
            this->mCursorPoint.setY(this->mCursorPoint.y()+2);
        }else if(this->mLineIdx>=1){
            this->mLineIdx-=1;
            this->mStartPoint.setY(this->mStartPoint.y()+1);
            this->mCursorPoint.setY(this->mCursorPoint.y()+1);
        }
    }
    if(event->x()>this->width()){//x>窗口宽度
        if(this->mColIdx+this->mColMax<=this->mLineList[this->mEndPoint.y()+this->mLineIdx].length()-3){
            this->mColIdx+=3;
            this->mStartPoint.setX(this->mStartPoint.x()-3);
            this->mCursorPoint.setX(this->mCursorPoint.x()-3);
        }else if(this->mColIdx+this->mColMax<=this->mLineList[this->mEndPoint.y()+this->mLineIdx].length()-2){
            this->mColIdx+=2;
            this->mStartPoint.setX(this->mStartPoint.x()-2);
            this->mCursorPoint.setX(this->mCursorPoint.x()-2);
        }else if(this->mColIdx+this->mColMax<=this->mLineList[this->mEndPoint.y()+this->mLineIdx].length()-1){
            this->mColIdx+=1;
            this->mStartPoint.setX(this->mStartPoint.x()-1);
            this->mCursorPoint.setX(this->mCursorPoint.x()-1);
        }
    }else if(event->x()<0){//x<0
        if(this->mColIdx>=3){
            this->mColIdx-=3;
            this->mStartPoint.setX(this->mStartPoint.x()+3);
            this->mCursorPoint.setX(this->mCursorPoint.x()+3);
        }else if(this->mColIdx>=2){
            this->mColIdx-=2;
            this->mStartPoint.setX(this->mStartPoint.x()+2);
            this->mCursorPoint.setX(this->mCursorPoint.x()+2);
        }else if(this->mColIdx>=1){
            this->mColIdx-=1;
            this->mStartPoint.setX(this->mStartPoint.x()+1);
            this->mCursorPoint.setX(this->mCursorPoint.x()+1);
        }
    }
    //设置选中文本的前后位置
    this->setXY();
    this->update();
}

//获取鼠标滚动消息
void MainWindow::wheelEvent(QWheelEvent *event){
    QPoint p=event->angleDelta();
    if(p.y()>0){//向上
        if(this->mLineIdx>=3){
            this->mLineIdx-=3;
            this->mStarY+=3;
            this->mEndY+=3;
            this->mCursorPoint.setY(this->mCursorPoint.y()+3);
        }else if(this->mLineIdx>=2){
            this->mLineIdx-=2;
            this->mStarY+=2;
            this->mEndY+=2;
            this->mCursorPoint.setY(this->mCursorPoint.y()+2);
        }else if(this->mLineIdx>=1){
            this->mLineIdx-=1;
            this->mStarY+=1;
            this->mEndY+=1;
            this->mCursorPoint.setY(this->mCursorPoint.y()+1);
        }
    }else if(p.y()<0){//向下
        if(this->mLineIdx+this->mLineMax<=this->mLineList.size()-3){
            this->mLineIdx+=3;
            this->mStarY-=3;
            this->mEndY-=3;
            this->mCursorPoint.setY(this->mCursorPoint.y()-3);
        }else if(this->mLineIdx+this->mLineMax<=this->mLineList.size()-2){
            this->mLineIdx+=2;
            this->mStarY-=2;
            this->mEndY-=2;
            this->mCursorPoint.setY(this->mCursorPoint.y()-2);
        }else if(this->mLineIdx+this->mLineMax<=this->mLineList.size()-1){
            this->mLineIdx+=1;
            this->mStarY-=1;
            this->mEndY-=1;
            this->mCursorPoint.setY(this->mCursorPoint.y()-1);
        }
    }
    this->update();

}

//选中文本后输入将覆盖选中的文本
void MainWindow::setChooseText(){
    this->mLineList[this->mStarY+this->mLineIdx]=this->mLineList[this->mStarY+this->mLineIdx].left(this->mStarX+this->mColIdx)+
            this->mLineList[this->mEndY+this->mLineIdx].right(mLineList[this->mEndY+this->mLineIdx].length()-this->mEndX-this->mColIdx);
    for(int i=this->mStarY;i<this->mEndY;i++){
        this->mLineList.remove(mStarY+this->mLineIdx+1);
    }
    this->mCursorPoint.setX(this->mStarX);
    this->mCursorPoint.setY(this->mStarY);
    this->resetCursor();
    this->resetXY();
    this->update();
}

//设置选中文本的前后位置
void MainWindow::setXY(){
    if(this->mStartPoint.y()<this->mEndPoint.y()){
        this->mStarX=this->mStartPoint.x();
        this->mStarY=this->mStartPoint.y();
        this->mEndX=this->mEndPoint.x();
        this->mEndY=this->mEndPoint.y();
    }else if(this->mStartPoint.y()==this->mEndPoint.y()){
        this->mStarY=this->mStartPoint.y();
        this->mEndY=this->mEndPoint.y();
        if(this->mStartPoint.x()<this->mEndPoint.x()){
            this->mStarX=this->mStartPoint.x();
            this->mEndX=this->mEndPoint.x();
        }else{
            this->mStarX=this->mEndPoint.x();
            this->mEndX=this->mStartPoint.x();
        }
    }else{
        this->mStarX=this->mEndPoint.x();
        this->mStarY=this->mEndPoint.y();
        this->mEndX=this->mStartPoint.x();
        this->mEndY=this->mStartPoint.y();
    }
}

//重置光标位置，使之回到窗口显示
void MainWindow::resetCursor(){
    if(this->mCursorPoint.y()<0){
        this->mLineIdx-=-this->mCursorPoint.y();
        this->mCursorPoint.setY(0);
    }else if(this->mCursorPoint.y()>this->mLineMax-1){
        this->mLineIdx+=this->mCursorPoint.y()-this->mLineMax+1;
        this->mCursorPoint.setY(this->mLineMax-1);
    }
    if(this->mCursorPoint.x()<0){
        this->mColIdx-=-this->mCursorPoint.x();
        this->mCursorPoint.setX(0);
    }else if(this->mCursorPoint.x()>this->mColMax){
        this->mColIdx+=this->mCursorPoint.x()-this->mColMax;
        this->mCursorPoint.setX(this->mColMax);
    }
    this->update();
}

//重置选中文本的先后位置
void MainWindow::resetXY(){
    this->mStartPoint.setX(this->mCursorPoint.x());
    this->mStartPoint.setY(this->mCursorPoint.y());
    this->mEndPoint.setX(this->mCursorPoint.x());
    this->mEndPoint.setY(this->mCursorPoint.y());
    this->update();
    this->setXY();
}

//更新窗口最大可显示行列
void MainWindow::updateMaxLineCol(){
    this->mLineMax=0;
    while((++this->mLineMax+1)*this->mTSize<this->height());
    (--this->mLineMax)--;
    this->mColMax=0;
    while(++this->mColMax*this->mTSize<this->width());
    --this->mColMax;
    this->update();
}


//新建文件
void MainWindow::on_action_triggered()
{
    this->mLineList.clear();
    this->mFileName="";
    this->mLineList.insert(0,"");
    this->mLineIdx=0;
    this->mColIdx=0;
    this->mCursorPoint.setX(0);
    this->mCursorPoint.setY(0);
    this->update();
}

//打开文件
void MainWindow::on_action_2_triggered()
{
    this->mFileName=QFileDialog::getOpenFileName(this,"请选择要打开的文件",".","All files (*);;Text file (*.txt);;C/C++ source files (*.c *.cpp *.cc)");
    this->mFile->setFileName(this->mFileName);
    if(this->mFile->exists()){
        if(this->mFile->open(QIODevice::ReadOnly|QIODevice::Text)){
            this->mLineList.clear();
            int i=0;
            while(!QTextStream(this->mFile).atEnd()){
                this->mLineList.insert(i,this->mFile->readLine());
                this->mLineList[i].remove("\n");
                i++;
            }
            if(i==0){
                this->mLineList.insert(i,"");
            }
        }
    }
    this->mFile->close();
}

//保存文件
void MainWindow::on_actionbaocun_triggered()
{
    if(this->mFile->open(QIODevice::WriteOnly|QIODevice::Text)){
        for(int i=0;i<this->mLineList.size();i++){
            QTextStream(this->mFile)<<(this->mLineList[i])<<"\n";
        }
    }
    this->mFile->close();
}

//文件另存为
void MainWindow::on_action_3_triggered()
{
    this->mFileName=QFileDialog::getSaveFileName(this,"另存为",".","All files (*);;Text file (*.txt);;C/C++ source files (*.c *.cpp *.cc)");
    if(this->mFileName==""){
        return;
    }
    this->mFile->setFileName(this->mFileName);
    if(this->mFile->open(QIODevice::WriteOnly|QIODevice::Text)){
        for(int i=0;i<this->mLineList.size();i++){
            QTextStream(this->mFile)<<(this->mLineList[i])<<"\n";
        }
    }
    this->mFile->close();
}
