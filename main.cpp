#include "mainwindow.h"
#include <QApplication>
#include <objbase.h>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    // Initialize Windows COM library for Core Audio APIs
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return -1;
    }

    QApplication app(argc, argv);

    // Prevent application from quitting when the main window is hidden
    QApplication::setQuitOnLastWindowClosed(false);

    MainWindow w;
    // Note: w.show() is omitted here so it starts silently in the system tray

    int result = app.exec();

    // Clean up COM resources upon exit
    CoUninitialize();

    return result;
}