#pragma once

#include <QtWidgets/QMainWindow>
#include <QTranslator>
#include "ui_rxclient.h"

class rxclient : public QMainWindow {
	Q_OBJECT

public:
	rxclient(QWidget *parent = nullptr);

private slots:
	void langChange();

private:
	void loadLanguage(const QString &s);

private:
	Ui::rxclientClass ui;
	QTranslator trans;
};
