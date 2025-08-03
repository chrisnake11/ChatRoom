#pragma once

#include <QLabel>
#include "global.h"
#include <QEvent>
#include <QMouseEvent>
class clickedlabel  : public QLabel
{
	Q_OBJECT

public:
	clickedlabel(QWidget *parent);
	~clickedlabel();
	void mousePressEvent(QMouseEvent* ev) override;
	virtual void enterEvent(QEnterEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;
	void setCurrentState(ClickLabelState state);
	void setState(QString normal, QString hover, QString press, QString selected, QString selected_hover, QString selected_press);
	ClickLabelState getCurrentState();
private:
	// 6种鼠标事件状态
	QString _normal;	// 默认状态
	QString _normal_hover; // 悬浮
	QString _normal_press; // 按住

	QString _selected; // 选中状态
    QString _selected_hover; // 悬浮
    QString _selected_press; // 按住

	// 按钮状态
	ClickLabelState _current_state;

signals:
	void clicked();
};

