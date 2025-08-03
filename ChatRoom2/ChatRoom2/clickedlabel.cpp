#include "clickedlabel.h"

clickedlabel::clickedlabel(QWidget *parent)
	: QLabel(parent), _current_state(ClickLabelState::NORMAL)
{
}

clickedlabel::~clickedlabel()
{}

void clickedlabel::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::MouseButton::LeftButton) {
		if (_current_state == ClickLabelState::NORMAL) {
			qDebug() << "clicked, change to SELECTED: " << _selected_hover;
			_current_state = ClickLabelState::SELECTED;
			setProperty("state", _selected_hover);
			repolish(this); // 重绘
			update(); // 刷新组件
		}
		else {
			qDebug() << "clicked, change to NORMAL: " << _normal_hover;
			_current_state = ClickLabelState::NORMAL;
			setProperty("state", _normal_hover);
            repolish(this);
            update();
		}
		emit clicked();
	}
	QLabel::mousePressEvent(event);
}

void clickedlabel::enterEvent(QEnterEvent * event)
{
		if (_current_state == ClickLabelState::NORMAL) {
			qDebug() << "enter, change to normal HOVER: " << _normal_hover;
            setProperty("state", _normal_hover);
            repolish(this);
            update();
		}
		else {
            qDebug() << "enter, change to selected HOVER: " << _selected_hover;
            setProperty("state", _selected_hover);
            repolish(this);
            update();
		}
    QLabel::enterEvent(event);
}

void clickedlabel::leaveEvent(QEvent* event)
{
	if (_current_state == ClickLabelState::NORMAL) {
		qDebug() << "leave, change to NORMAL: " << _normal;
        setProperty("state", _normal);
        repolish(this);
        update();
	}
	else {
		qDebug() << "leave, change to SELECTED: " << _selected;
		setProperty("state", _selected);
        repolish(this);
        update();
	}
	QLabel::leaveEvent(event);
}

void clickedlabel::setCurrentState(ClickLabelState state)
{
	_current_state = state;
}

void clickedlabel::setState(QString normal, QString hover, QString press, QString selected, QString selected_hover, QString selected_press)
{
	_normal = normal;
	_normal_hover = hover;
	_normal_press = press;
    _selected = selected;
    _selected_hover = selected_hover;
    _selected_press = selected_press;
	setProperty("state", _normal);
	repolish(this);
    update();
}

ClickLabelState clickedlabel::getCurrentState()
{
	return _current_state;
}

