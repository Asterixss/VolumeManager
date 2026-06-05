#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QPoint>
#include <QMouseEvent>
#include "audiobackend.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

    // Drag handlers for our custom toolbar
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void updateUIWithSessions(const std::vector<AudioSessionData>& sessions);
    void handleMasterVolumeSlider(int value);
    void handleMasterVolumeSpinBox(double value);

private:
    void setupTrayIcon();
    void buildUI();

    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
    AudioBackend* m_audioBackend = nullptr;

    QSlider* m_masterSlider = nullptr;
    QDoubleSpinBox* m_masterSpinBox = nullptr;
    QWidget* m_sessionsContainer = nullptr;
    QVBoxLayout* m_sessionsLayout = nullptr;

    QPoint m_dragPosition; // Tracks window movement vector
};