#pragma once

#include <QDialog>
#include "ui_ChatDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ChatDialogClass; };
QT_END_NAMESPACE

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog(QWidget *parent = nullptr);
	~ChatDialog();

private:
	Ui::ChatDialogClass *ui;
};

