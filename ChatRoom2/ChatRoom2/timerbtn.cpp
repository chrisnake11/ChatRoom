#include "timerbtn.h"

timerbtn::timerbtn(QWidget *parent):QPushButton(parent), _counter(10){
    _timer = new QTimer(this);

    connect(_timer, &QTimer::timeout, [this](){
        _counter--;

        // time out
        if(_counter <= 0){
            _timer->stop();
            _counter = 10; // 10 seconds
            // reset get code text
            this->setText("获取验证码");
            this->setEnabled(true);
            return;
        }

        // update time
        this->setText(QString::number(_counter));
    });
}

timerbtn::~timerbtn(){
    if(_timer != nullptr){
        _timer->stop();
        delete _timer;
    }
}

void timerbtn::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton){
        qDebug() << "MyButton was Released.";
        // ban button and set timer.
        this->setEnabled(false);
        this->setText(QString::number(_counter));
        _timer->start(1000);
        emit clicked();
    }

    // call basic class event, ensuring the event is handled correctly.
    QPushButton::mouseReleaseEvent(e);

}
