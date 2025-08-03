#include "ChatDialog.h"
#include "logindialog.h"
ChatDialog::ChatDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::ChatDialogClass())
{
	ui->setupUi(this);

}

ChatDialog::~ChatDialog()
{
	delete ui;
}

