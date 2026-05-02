#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_SBTI.h"

class SBTI : public QMainWindow
{
    Q_OBJECT

public:
    SBTI(QWidget *parent = nullptr);
    ~SBTI();

private:
    Ui::SBTIClass ui;
};

