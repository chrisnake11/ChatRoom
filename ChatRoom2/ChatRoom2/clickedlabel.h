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
	// 6������¼�״̬
	QString _normal;	// Ĭ��״̬
	QString _normal_hover; // ����
	QString _normal_press; // ��ס

	QString _selected; // ѡ��״̬
    QString _selected_hover; // ����
    QString _selected_press; // ��ס

	// ��ť״̬
	ClickLabelState _current_state;

signals:
	void clicked();
};

