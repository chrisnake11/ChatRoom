#pragma once

#include <QDialog>
#include "ui_resetdialog.h"
#include "global.h"
#include <Qmap>

QT_BEGIN_NAMESPACE
namespace Ui { class ResetDialogClass; };
QT_END_NAMESPACE

class ResetDialog : public QDialog
{
	Q_OBJECT

public:
	ResetDialog(QWidget *parent = nullptr);
	~ResetDialog();

private slots:
	void on_get_code_btn_clicked();
	void on_reset_mod_finish(RequestID id, QString res, ErrorCodes err);
	void on_reset_btn_clicked();

signals:
	void switch_to_login();
	void reset_success();

private:
	Ui::ResetDialogClass *ui;

	bool checkEmailValid();
	bool checkPasswdValid();
	bool checkConfirmValid();
	bool checkCodeValid();

	QMap<RequestID, std::function<void(const QJsonObject&)>> _request_handlers;
	void initHttpHandlers();
	
	QMap<TipErr, QString> _tip_errs;
	void showTip(QString msg, bool success);
	void addTipErr(TipErr err, QString tips);
	void delTipErr(TipErr err);

};

