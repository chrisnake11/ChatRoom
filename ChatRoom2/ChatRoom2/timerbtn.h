#ifndef TIMERBTN_H
#define TIMERBTN_H
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <QMouseEvent>

class timerbtn : public QPushButton
{
public:
    explicit timerbtn(QWidget *parent = nullptr);
    ~timerbtn();
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
private:
    QTimer *_timer;
    int _counter;
};

#endif // TIMERBTN_H
