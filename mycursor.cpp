#include "mycursor.h"

MyCursor::MyCursor(QObject *parent):QThread(parent){
    this->mIsShow=false;//默认不显示光标
    this->mIsBreak=false;//默认不关闭线程
}

void MyCursor::run(){
    while(true){
        if(this->mIsBreak){//如果收到关闭线程的消息
            break;//结束运行，退出线程
        }
        if(!this->mIsShow){//如果光标不显示
            this->mIsShow=true;//就让他显示
        }else{
            this->mIsShow=false;//否则，就让他不显示
        }
        emit this->sendCursorStatus(this->mIsShow);//自定义信号，发送光标是否显示的消息
        this->msleep(500);//等待500毫秒
    }
}
void MyCursor::receiveBreak(){
    this->mIsBreak=true;//接收到关闭线程的消息,使线程关闭
}
