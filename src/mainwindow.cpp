#include "mainwindow.h"
#include "settings.h"

#include <QHBoxLayout>
#include <QDir>
#include <QStringList>
#include <iostream>

#include <QHBoxLayout>

void MainWindow::initialize() {
    // Main OpenGL widget
    realtime = new Realtime;

    // Keep aspect ratio 4:3 for the viewport
    aspectRatioWidget = new AspectRatioWidget(this);
    aspectRatioWidget->setAspectWidget(realtime, 3.f / 4.f);

    // JUST the game viewport, no sidebar
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(aspectRatioWidget, 1);
    setLayout(layout);

    setWindowTitle("CS1230 Project 6 - Snake Terrain");
}

void MainWindow::finish() {
    if (realtime) {
        realtime->finish();
        delete realtime;
        realtime = nullptr;
    }
}
