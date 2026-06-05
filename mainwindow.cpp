#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QScrollArea>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_audioBackend(new AudioBackend(this)) {

    buildUI();
    setupTrayIcon();

    connect(m_audioBackend, &AudioBackend::sessionsUpdated, this, &MainWindow::updateUIWithSessions);
    connect(m_audioBackend, &AudioBackend::masterVolumeChanged, this, [this](float vol){
        int pctInt = static_cast<int>(vol * 100);
        double pctDouble = static_cast<double>(vol * 100.0f);

        if (m_masterSlider) {
            m_masterSlider->blockSignals(true);
            m_masterSlider->setValue(pctInt);
            m_masterSlider->blockSignals(false);
        }
        if (m_masterSpinBox) {
            m_masterSpinBox->blockSignals(true);
            m_masterSpinBox->setValue(pctDouble);
            m_masterSpinBox->blockSignals(false);
        }
    });

    if (m_audioBackend->initialize()) {
        m_audioBackend->refreshSessions();
    }
}

MainWindow::~MainWindow() {}

void MainWindow::buildUI() {
    // Strip standard OS title bar frame
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    this->setMinimumSize(380, 450);
    this->resize(390, 500);

    // --- Comprehensive Classic Utility Stylesheet ---
    this->setStyleSheet(
        "QMainWindow { background-color: #f0f0f0; border: 1px solid #8a8a8a; }"

        // Custom Toolbar Styling
        "QWidget#TitleBar { background-color: #e1e1e1; border-bottom: 1px solid #b5b5b5; }"
        "QPushButton#MinButton, QPushButton#CloseButton { background-color: transparent; border: 1px solid transparent; font-family: 'Segoe UI', sans-serif; font-size: 12px; color: #000000; }"
        "QPushButton#MinButton:hover { background-color: #cccccc; border: 1px solid #aaaaaa; }"
        "QPushButton#CloseButton:hover { background-color: #e81123; color: #ffffff; border: 1px solid #b0101a; }"

        // Hardware Group Frames
        "QGroupBox { border: 1px solid #b0b0b0; margin-top: 12px; font-family: 'Segoe UI', sans-serif; font-size: 11px; font-weight: bold; color: #000000; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 3px; left: 10px; }"

        "QLabel { color: #000000; font-family: 'Segoe UI', sans-serif; font-size: 11px; }"
        "QDoubleSpinBox { background-color: #ffffff; color: #000000; border: 1px solid #7a7a7a; font-family: 'Segoe UI', sans-serif; font-size: 11px; padding: 2px; }"

        // Classic Sliders
        "QSlider::groove:horizontal { border: 1px solid #999999; height: 3px; background: #ffffff; }"
        "QSlider::handle:horizontal { background: #f5f5f5; border: 1px solid #666666; width: 8px; margin: -6px 0; border-radius: 1px; }"
        "QSlider::handle:horizontal:hover { background: #e0e0e0; border: 1px solid #0078d4; }"

        "QScrollArea { background-color: #ffffff; border: 1px solid #b0b0b0; }"
        );

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); // Frameless content spans edge to edge
    mainLayout->setSpacing(0);

    // --- 1. Custom Technical Toolbar ---
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("TitleBar");
    topBar->setFixedHeight(28);

    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(10, 0, 4, 0);
    topBarLayout->setSpacing(2);

    QLabel* titleLabel = new QLabel("Volume Mixer", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 11px; color: #333333;");

    QPushButton* minButton = new QPushButton("-", this);
    minButton->setObjectName("MinButton");
    minButton->setFixedSize(28, 22);

    QPushButton* closeButton = new QPushButton("×", this);
    closeButton->setObjectName("CloseButton");
    closeButton->setFixedSize(28, 22);

    topBarLayout->addWidget(titleLabel);
    topBarLayout->addStretch();
    topBarLayout->addWidget(minButton);
    topBarLayout->addWidget(closeButton);
    mainLayout->addWidget(topBar);

    // --- 2. Application Body Wrapper (restores content internal margins) ---
    QWidget* bodyContent = new QWidget(this);
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyContent);
    bodyLayout->setContentsMargins(8, 4, 8, 8);
    bodyLayout->setSpacing(6);

    // System Volume Group
    QGroupBox* masterGroupBox = new QGroupBox("Processor Audio / System Volume", this);
    QVBoxLayout* masterGroupBoxLayout = new QVBoxLayout(masterGroupBox);
    masterGroupBoxLayout->setContentsMargins(8, 14, 8, 8);

    QHBoxLayout* masterControlsLayout = new QHBoxLayout();
    masterControlsLayout->setSpacing(8);

    m_masterSlider = new QSlider(Qt::Horizontal, this);
    m_masterSlider->setRange(0, 100);

    m_masterSpinBox = new QDoubleSpinBox(this);
    m_masterSpinBox->setRange(0.0, 100.0);
    m_masterSpinBox->setDecimals(1);
    m_masterSpinBox->setSuffix(" %");
    m_masterSpinBox->setFixedWidth(65);
    m_masterSpinBox->setAlignment(Qt::AlignCenter);
    m_masterSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

    masterControlsLayout->addWidget(m_masterSlider, 1);
    masterControlsLayout->addWidget(m_masterSpinBox);
    masterGroupBoxLayout->addLayout(masterControlsLayout);
    bodyLayout->addWidget(masterGroupBox);

    // Applications Group
    QGroupBox* appsGroupBox = new QGroupBox("Active Audio Sessions", this);
    QVBoxLayout* appsGroupBoxLayout = new QVBoxLayout(appsGroupBox);
    appsGroupBoxLayout->setContentsMargins(4, 12, 4, 4);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_sessionsContainer = new QWidget(scrollArea);
    m_sessionsContainer->setStyleSheet("background-color: #ffffff;");

    m_sessionsLayout = new QVBoxLayout(m_sessionsContainer);
    m_sessionsLayout->setAlignment(Qt::AlignTop);
    m_sessionsLayout->setContentsMargins(2, 2, 2, 2);
    m_sessionsLayout->setSpacing(4);

    scrollArea->setWidget(m_sessionsContainer);
    appsGroupBoxLayout->addWidget(scrollArea);
    bodyLayout->addWidget(appsGroupBox, 1);

    mainLayout->addWidget(bodyContent);
    this->setCentralWidget(centralWidget);

    // Functional bindings
    connect(minButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::close);
    connect(m_masterSlider, &QSlider::valueChanged, this, &MainWindow::handleMasterVolumeSlider);
    connect(m_masterSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::handleMasterVolumeSpinBox);
}

// Window movement tracking logic
void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void MainWindow::setupTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(QIcon::fromTheme("audio-volume-high"), this);
    m_trayMenu = new QMenu(this);

    QAction* openAction = m_trayMenu->addAction("Open Audio Manager");
    QAction* refreshAction = m_trayMenu->addAction("Refresh");
    m_trayMenu->addSeparator();
    QAction* quitAction = m_trayMenu->addAction("Exit");

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(openAction, &QAction::triggered, this, &MainWindow::showNormal);
    connect(refreshAction, &QAction::triggered, m_audioBackend, &AudioBackend::refreshSessions);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

    m_trayIcon->show();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        if (this->isVisible()) {
            this->hide();
        } else {
            m_audioBackend->refreshSessions();
            this->showNormal();
            this->activateWindow();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_trayIcon->isVisible()) {
        this->hide();
        event->ignore();
    }
}

void MainWindow::updateUIWithSessions(const std::vector<AudioSessionData>& sessions) {
    QLayoutItem* item;
    while ((item = m_sessionsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    for (const auto& session : sessions) {
        QWidget* rowWidget = new QWidget(m_sessionsContainer);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(4, 2, 4, 2);
        rowLayout->setSpacing(6);

        QLabel* iconLabel = new QLabel(rowWidget);
        iconLabel->setPixmap(session.appIcon.pixmap(16, 16));
        iconLabel->setFixedSize(16, 16);

        QLabel* nameLabel = new QLabel(session.appName, rowWidget);
        nameLabel->setFixedWidth(95);

        QSlider* volSlider = new QSlider(Qt::Horizontal, rowWidget);
        volSlider->setRange(0, 100);
        volSlider->setValue(static_cast<int>(session.volume * 100));

        QDoubleSpinBox* appSpinBox = new QDoubleSpinBox(rowWidget);
        appSpinBox->setRange(0.0, 100.0);
        appSpinBox->setDecimals(1);
        appSpinBox->setSuffix(" %");
        appSpinBox->setFixedWidth(65);
        appSpinBox->setAlignment(Qt::AlignCenter);
        appSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        appSpinBox->setValue(static_cast<double>(session.volume * 100.0f));

        connect(volSlider, &QSlider::valueChanged, this, [this, pid = session.processId, appSpinBox](int value){
            appSpinBox->blockSignals(true);
            appSpinBox->setValue(static_cast<double>(value));
            appSpinBox->blockSignals(false);
            m_audioBackend->setSessionVolume(pid, static_cast<float>(value) / 100.0f);
        });

        connect(appSpinBox, &QDoubleSpinBox::valueChanged, this, [this, pid = session.processId, volSlider](double value){
            volSlider->blockSignals(true);
            volSlider->setValue(static_cast<int>(value));
            volSlider->blockSignals(false);
            m_audioBackend->setSessionVolume(pid, static_cast<float>(value) / 100.0f);
        });

        rowLayout->addWidget(iconLabel);
        rowLayout->addWidget(nameLabel);
        rowLayout->addWidget(volSlider, 1);
        rowLayout->addWidget(appSpinBox);

        m_sessionsLayout->addWidget(rowWidget);
    }
}

void MainWindow::handleMasterVolumeSlider(int value) {
    m_masterSpinBox->blockSignals(true);
    m_masterSpinBox->setValue(static_cast<double>(value));
    m_masterSpinBox->blockSignals(false);

    m_audioBackend->setMasterVolume(static_cast<float>(value) / 100.0f);
}

void MainWindow::handleMasterVolumeSpinBox(double value) {
    m_masterSlider->blockSignals(true);
    m_masterSlider->setValue(static_cast<int>(value));
    m_masterSlider->blockSignals(false);

    m_audioBackend->setMasterVolume(static_cast<float>(value) / 100.0f);
}