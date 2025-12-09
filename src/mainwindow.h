#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include "realtime.h"
#include "utils/aspectratiowidget/aspectratiowidget.hpp"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QWidget(parent)
    {}

    // Called from main.cpp after construction
    void initialize();
    void finish();

private:
    Realtime *realtime = nullptr;
    AspectRatioWidget *aspectRatioWidget = nullptr;
};
