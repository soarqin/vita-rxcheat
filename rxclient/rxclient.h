#pragma once

#include "network.h"

#include "ui_rxclient.h"

#include <QtWidgets/QMainWindow>
#include <QTranslator>
#include <QTimer>

class rxclient: public QMainWindow {
    Q_OBJECT

public:
    rxclient(QWidget *parent = nullptr);

private slots:
    void langChange();
    void timerUpdate();

private:
    void loadLanguage(const QString &s);

private:
    Ui::rxclientClass ui;
    QTranslator trans;
    QTimer timer;
    Client client;
};
