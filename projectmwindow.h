#ifndef PROJECTMWINDOW_H
#define PROJECTMWINDOW_H

#include <QWindow>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QKeyEvent>
#include <memory>
#include <string>
#include <vector>
#include <sndfile.h>

// projectM classes
namespace libprojectM {
    class ProjectM;
    namespace Audio {
        class PCM;
    }
}

class ProjectMWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit ProjectMWindow(QWindow *parent = nullptr);
    ~ProjectMWindow();

    void setPresetPath(const std::string& path);
    void setTexturePaths(const std::vector<std::string>& paths);
    void setAudioFile(const QString& filePath);
    
    // Preset management functions
    void loadAvailablePresets();
    void nextPreset();
    void previousPreset();
    void setPresetDuration(double seconds);

    // Media player access
    QMediaPlayer* mediaPlayer() const { return m_mediaPlayer; }
    QAudioOutput* audioOutput() const { return m_audioOutput; }

protected:
    bool event(QEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void render();
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void handleMediaError(QMediaPlayer::Error error, const QString &errorString);

private:
    bool openAudioFile();
    void closeAudioFile();
    void processAudioChunk();
    void initialize();
    void cleanup();

    // OpenGL context
    QOpenGLContext *m_context = nullptr;
    bool m_initialized = false;

    // Audio file handling members
    QString m_audioFilePath;
    SNDFILE* m_sndFile = nullptr;
    SF_INFO m_sfInfo;
    std::vector<float> m_audioReadBuffer;

    // Media playback members
    QMediaPlayer* m_mediaPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;

    // ProjectM members
    std::unique_ptr<libprojectM::ProjectM> m_projectM;
    libprojectM::Audio::PCM* m_projectMPcm = nullptr;

    // Preset management
    std::vector<std::string> m_presetFiles;
    int m_currentPresetIndex = 0;
    QTimer m_presetTimer;
    double m_presetDuration = 30.0; // seconds

    QTimer m_renderTimer;
    QElapsedTimer m_elapsedTimer;
    
    // FPS calculation
    QElapsedTimer m_fpsTimer;
    int m_frameCount = 0;

    // Paths
    std::string m_presetPath;
    std::vector<std::string> m_texturePaths;

    // Dummy data for fallback
    std::vector<short> m_dummyPcmData;
    int m_pcmCounter = 0;

    int m_width = 0;
    int m_height = 0;
    bool m_texturesSetup = false;  // Flag to track if textures are set up
    bool m_textureCheckDone = false; // Flag to track texture check
};

#endif // PROJECTMWINDOW_H