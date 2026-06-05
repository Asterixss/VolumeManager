#pragma once

#include <QObject>
#include <QString>
#include <QIcon>
#include <vector>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>

struct AudioSessionData {
    DWORD processId;
    QString appName;
    QIcon appIcon;
    float volume;
    bool isMuted;
};

class AudioBackend : public QObject {
    Q_OBJECT

public:
    explicit AudioBackend(QObject *parent = nullptr);
    ~AudioBackend();

    bool initialize();
    void refreshSessions();

    // System Volume Controls
    float getMasterVolume();
    void setMasterVolume(float level);

    // Application-Specific Volume Controls
    void setSessionVolume(DWORD processId, float level);

signals:
    void masterVolumeChanged(float percentage);
    void sessionsUpdated(const std::vector<AudioSessionData>& sessions);

private:
    IMMDeviceEnumerator* m_deviceEnumerator = nullptr;
    IMMDevice* m_defaultDevice = nullptr;
    IAudioEndpointVolume* m_endpointVolume = nullptr;
    IAudioSessionManager2* m_sessionManager = nullptr;

    void cleanup();
    QIcon getIconFromProcessId(DWORD processId);
};