#include "audiobackend.h"
#include <QFileIconProvider>
#include <QFileInfo>
#include <QDebug>
#include <iostream>

// Helper macro to safely release COM objects
template <class T> void SafeRelease(T **ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

AudioBackend::AudioBackend(QObject *parent) : QObject(parent) {}

AudioBackend::~AudioBackend() {
    cleanup();
}

void AudioBackend::cleanup() {
    SafeRelease(&m_sessionManager);
    SafeRelease(&m_endpointVolume);
    SafeRelease(&m_defaultDevice);
    SafeRelease(&m_deviceEnumerator);
}

bool AudioBackend::initialize() {
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void**)&m_deviceEnumerator);
    if (FAILED(hr)) return false;

    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_defaultDevice);
    if (FAILED(hr)) return false;

    // Initialize Master Volume Interface
    hr = m_defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&m_endpointVolume);
    if (FAILED(hr)) return false;

    // Initialize Session Manager for Per-App Volume
    hr = m_defaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&m_sessionManager);
    if (FAILED(hr)) return false;

    return true;
}

float AudioBackend::getMasterVolume() {
    if (!m_endpointVolume) return 0.0f;
    float volume = 0.0f;
    m_endpointVolume->GetMasterVolumeLevelScalar(&volume);
    return volume;
}

void AudioBackend::setMasterVolume(float level) {
    if (m_endpointVolume) {
        m_endpointVolume->SetMasterVolumeLevelScalar(level, nullptr);
    }
}

void AudioBackend::setSessionVolume(DWORD processId, float level) {
    if (!m_sessionManager) return;

    IAudioSessionEnumerator* sessionEnumerator = nullptr;
    if (SUCCEEDED(m_sessionManager->GetSessionEnumerator(&sessionEnumerator))) {
        int count = 0;
        sessionEnumerator->GetCount(&count);

        for (int i = 0; i < count; i++) {
            IAudioSessionControl* sessionControl = nullptr;
            IAudioSessionControl2* sessionControl2 = nullptr;

            if (SUCCEEDED(sessionEnumerator->GetSession(i, &sessionControl))) {
                if (SUCCEEDED(sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2))) {
                    DWORD pid = 0;
                    sessionControl2->GetProcessId(&pid);

                    if (pid == processId) {
                        ISimpleAudioVolume* simpleVolume = nullptr;
                        if (SUCCEEDED(sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume))) {
                            simpleVolume->SetMasterVolume(level, nullptr);
                            simpleVolume->Release();
                        }
                    }
                    sessionControl2->Release();
                }
                sessionControl->Release();
            }
        }
        sessionEnumerator->Release();
    }
}

void AudioBackend::refreshSessions() {
    if (!m_sessionManager) return;

    std::vector<AudioSessionData> currentSessions;
    IAudioSessionEnumerator* sessionEnumerator = nullptr;

    if (SUCCEEDED(m_sessionManager->GetSessionEnumerator(&sessionEnumerator))) {
        int count = 0;
        sessionEnumerator->GetCount(&count);

        for (int i = 0; i < count; i++) {
            IAudioSessionControl* sessionControl = nullptr;
            IAudioSessionControl2* sessionControl2 = nullptr;
            ISimpleAudioVolume* simpleVolume = nullptr;

            if (SUCCEEDED(sessionEnumerator->GetSession(i, &sessionControl))) {
                if (SUCCEEDED(sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2))) {
                    DWORD pid = 0;
                    sessionControl2->GetProcessId(&pid);

                    // Skip System Sounds (PID 0) or inactive sessions
                    AudioSessionState state;
                    sessionControl->GetState(&state);

                    if (pid != 0 && state != AudioSessionStateExpired) {
                        AudioSessionData data;
                        data.processId = pid;

                        if (SUCCEEDED(sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume))) {
                            simpleVolume->GetMasterVolume(&data.volume);

                            BOOL isMuted;
                            simpleVolume->GetMute(&isMuted);
                            data.isMuted = isMuted;
                            simpleVolume->Release();
                        }

                        // Get Process Name and Icon
                        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                        if (hProcess) {
                            WCHAR path[MAX_PATH];
                            DWORD size = MAX_PATH;
                            if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
                                QString fullPath = QString::fromWCharArray(path);
                                QFileInfo fileInfo(fullPath);
                                data.appName = fileInfo.baseName();

                                QFileIconProvider iconProvider;
                                data.appIcon = iconProvider.icon(fileInfo);
                            }
                            CloseHandle(hProcess);
                            currentSessions.push_back(data);
                        }
                    }
                    sessionControl2->Release();
                }
                sessionControl->Release();
            }
        }
        sessionEnumerator->Release();
    }

    emit masterVolumeChanged(getMasterVolume());
    emit sessionsUpdated(currentSessions);
}