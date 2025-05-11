#include "projectmwindow.h"

#include <ProjectM.hpp>
#include <Audio/PCM.hpp>

#include <QDebug>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QOpenGLContext>
#include <QElapsedTimer>
#include <QExposeEvent>
#include <QResizeEvent>
#include <QSurfaceFormat>
#include <QKeyEvent>
#include <stdexcept>
#include <cmath>
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <QDir>
#include <QCoreApplication>
#include <random>

// Constants
const int FPS_TARGET = 60;
const int AUDIO_FRAMES_PER_CHUNK = 1024;
const int PCM_BUFFER_SIZE = AUDIO_FRAMES_PER_CHUNK;

ProjectMWindow::ProjectMWindow(QWindow *parent)
    : QWindow(parent),
      m_dummyPcmData(PCM_BUFFER_SIZE * 2),
      m_pcmCounter(0)
{
    // Force using desktop OpenGL (not OpenGL ES)
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    
    // Set the surface format for this window
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSamples(4);
    setFormat(format);
    
    // OpenGL context
    m_context = new QOpenGLContext(this);
    m_context->setFormat(format);
    if (!m_context->create()) {
        qCritical() << "Failed to create OpenGL context!";
        return;
    }
    
    // Set up surface & make this window the current render target
    setSurfaceType(QWindow::OpenGLSurface);
    
    // basic window props
    setWidth(800);
    setHeight(600);
    setTitle("MusicVisQT");
    
    // Initialize media player and audio output
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    
    // set volume
    m_audioOutput->setVolume(0.5);
    
    // media player signals
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, 
            this, &ProjectMWindow::handleMediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred,
            this, &ProjectMWindow::handleMediaError);
            
    // Connect render timer
    connect(&m_renderTimer, &QTimer::timeout, this, &ProjectMWindow::render);
    
    // Connect preset timer
    connect(&m_presetTimer, &QTimer::timeout, this, &ProjectMWindow::nextPreset);

    // FPS calculation
    m_fpsTimer.invalidate();

    // base path
    std::string basePath = QCoreApplication::applicationDirPath().toStdString() + "/../presets/";
    
    // presets path
    m_presetPath = basePath;
    
    qInfo() << "Using preset path:" << QString::fromStdString(m_presetPath);
    
    m_texturePaths.clear();
    
    m_texturePaths.push_back(basePath + "Textures");
    
    // fallback
    m_texturePaths.push_back("/usr/share/projectM/textures");
    
    // log texture paths for debugging
    for (const auto& path : m_texturePaths) {
        qInfo() << "Texture search path:" << QString::fromStdString(path);
        
        // does it even exist?
        std::filesystem::path texturePath(path);
        if (!std::filesystem::exists(texturePath)) {
            qWarning() << "  - WARNING: This texture path does not exist!";
        } else if (!std::filesystem::is_directory(texturePath)) {
            qWarning() << "  - WARNING: This texture path is not a directory!";
        } else {
            qInfo() << "  - Path exists and is a directory.";
            
            // If it does what is in there?
            int count = 0;
            try {
                for (const auto& entry : std::filesystem::directory_iterator(texturePath)) {
                    if (entry.is_regular_file()) {
                        qInfo() << "    Found texture:" << QString::fromStdString(entry.path().filename().string());
                        if (++count >= 3) {
                            qInfo() << "    (and more...)";
                            break;
                        }
                    }
                }
                if (count == 0) {
                    qWarning() << "  - WARNING: No texture files found in directory!";
                }
            } catch (const std::exception& e) {
                qWarning() << "  - WARNING: Error listing directory:" << e.what();
            }
        }
    }
    
    m_texturesSetup = true;
}

ProjectMWindow::~ProjectMWindow() {
    cleanup();
    
    // Clean up audio resources
    closeAudioFile();
    delete m_mediaPlayer;
    delete m_audioOutput;
    
    // Clean up OpenGL context
    if (m_context) {
        delete m_context;
    }
}

bool ProjectMWindow::event(QEvent *event) {
    switch (event->type()) {
        case QEvent::UpdateRequest:
            render();
            return true;
        default:
            return QWindow::event(event);
    }
}

void ProjectMWindow::exposeEvent(QExposeEvent *event) {
    Q_UNUSED(event);
    
    if (isExposed()) {
        if (!m_initialized) {
            initialize();
            m_initialized = true;
            
            // render timer
            m_renderTimer.start(1000 / FPS_TARGET);
        }
        
        render();
    }
}

void ProjectMWindow::resizeEvent(QResizeEvent *event) {
    m_width = event->size().width();
    m_height = event->size().height();
    
    if (m_projectM && m_initialized) {
        if (m_context && m_context->makeCurrent(this)) {
            qInfo() << "Resizing projectM to" << m_width << "x" << m_height;
            m_projectM->SetWindowSize(m_width, m_height);
            m_context->doneCurrent();
        }
    }
    
    QWindow::resizeEvent(event);
}

void ProjectMWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Right:
        case Qt::Key_N:
            nextPreset();
            break;
            
        case Qt::Key_Left:
        case Qt::Key_P:
            previousPreset();
            break;
            
        default:
            QWindow::keyPressEvent(event);
    }
}

void ProjectMWindow::initialize() {
    if (!m_context || !m_context->makeCurrent(this)) {
        qCritical() << "Failed to make OpenGL context current for initialization!";
        return;
    }
    
    // Init OpenGL
    initializeOpenGLFunctions();
    
    // Debug OpenGL context information
    qInfo() << "OpenGL Context:" << m_context->format().majorVersion() << "." << m_context->format().minorVersion();
    qInfo() << "Context valid:" << m_context->isValid();
    qInfo() << "OpenGL Vendor:" << reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    qInfo() << "OpenGL Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    qInfo() << "OpenGL Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qInfo() << "GLSL Version:" << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    // Set background color and clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // depth testing and blending
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    m_width = width();
    m_height = height();
    qInfo() << "Initializing projectM (Composition)...";

    // preset path valid???
    if (m_presetPath.empty()) {
        qCritical() << "Preset path is empty! Cannot initialize projectM.";
        m_context->doneCurrent();
        return;
    }
    
    // do they exist?
    std::filesystem::path presetDir(m_presetPath);
    if (!std::filesystem::exists(presetDir)) {
        qCritical() << "Preset directory does not exist:" << QString::fromStdString(m_presetPath);
        m_context->doneCurrent();
        return;
    } else if (!std::filesystem::is_directory(presetDir)) {
        qCritical() << "Preset path is not a directory:" << QString::fromStdString(m_presetPath);
        m_context->doneCurrent();
        return;
    } else {
        // how many milk I have?
        int presetCount = 0;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(presetDir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".milk") {
                    presetCount++;
                }
            }
            qInfo() << "Found" << presetCount << "preset files in directory.";
        } catch (const std::exception& e) {
            qWarning() << "Error counting preset files:" << e.what();
        }
    }
    
    if (m_texturePaths.empty()) {
        qWarning() << "No texture paths set! Visuals may be broken.";
    }
    
    qInfo() << "Viewport:" << m_width << "x" << m_height;

    try {
        qInfo() << "Creating ProjectM instance...";
        m_projectM = std::make_unique<libprojectM::ProjectM>();
        
        qInfo() << "Setting window size:" << m_width << "x" << m_height;
        m_projectM->SetWindowSize(m_width, m_height);
        
        qInfo() << "Setting mesh size: 32x24";
        m_projectM->SetMeshSize(32, 24);
        
        qInfo() << "Setting preset duration:" << m_presetDuration << "seconds";
        m_projectM->SetPresetDuration(m_presetDuration);
        
        qInfo() << "Setting hard cut duration: 15.0 seconds";
        m_projectM->SetHardCutDuration(15.0);
        
        qInfo() << "Setting target FPS:" << FPS_TARGET;
        m_projectM->SetTargetFramesPerSecond(FPS_TARGET);

        qInfo() << "Setting texture paths in ProjectM...";
        m_projectM->SetTexturePaths(m_texturePaths);
        
        qInfo() << "Getting PCM object...";
        m_projectMPcm = &m_projectM->PCM();
        if (!m_projectMPcm) {
            qCritical() << "Failed to get PCM object from ProjectM!";
            throw std::runtime_error("Failed to get PCM object from ProjectM");
        }

        loadAvailablePresets();

        if (!m_presetFiles.empty()) {
            qInfo() << "Loading initial preset:" << QString::fromStdString(m_presetFiles[0]);
            m_projectM->LoadPresetFile(m_presetFiles[0], false);
            
            // preset timer for automatic cycling
            m_presetTimer.start(m_presetDuration * 1000);
        } else {
            qWarning() << "No presets found, using idle preset";
            m_projectM->LoadPresetFile("idle://", false);
        }

        // Open audio file
        if (!m_audioFilePath.isEmpty()) {
            openAudioFile();
        } else {
            qWarning() << "No audio file specified, will use dummy audio.";
            processAudioChunk();
        }

    } catch (const std::exception& e) {
        qCritical() << "Exception during projectM initialization: " << e.what();
        m_projectM.reset(); 
        m_projectMPcm = nullptr; 
        closeAudioFile(); 
        m_context->doneCurrent();
        return;
    } catch (...) {
        qCritical() << "An unknown error occurred during projectM initialization.";
        m_projectM.reset(); 
        m_projectMPcm = nullptr; 
        closeAudioFile(); 
        m_context->doneCurrent();
        return;
    }

    qInfo() << "projectM Initialized Successfully.";
    m_elapsedTimer.start();
    
    // Start FPS timer
    m_fpsTimer.start();
    m_frameCount = 0;

    m_context->doneCurrent();
}

void ProjectMWindow::cleanup() {
    if (m_context && m_context->makeCurrent(this)) {
        m_projectM.reset();
        m_projectMPcm = nullptr;
        m_context->doneCurrent();
    }
}

void ProjectMWindow::render() {
    if (!m_initialized || !isExposed() || !m_context || !m_projectM) {
        return;
    }
    
    if (!m_context->makeCurrent(this)) {
        qWarning() << "Failed to make OpenGL context current for rendering!";
        return;
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, m_width, m_height);

    processAudioChunk();

    try {
        m_projectM->RenderFrame();
        
        // Ensure OpenGL commands are executed
        glFlush();
        glFinish();
        
        // did you actually draw anything?
        GLubyte pixel[4];
        glReadPixels(width()/2, height()/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        if (m_frameCount % 60 == 0) {
            qInfo() << "Center pixel color (RGBA):" 
                    << (int)pixel[0] << (int)pixel[1] 
                    << (int)pixel[2] << (int)pixel[3];
        }

        m_frameCount++;
        if (m_frameCount >= 100) {
            if (m_fpsTimer.isValid()) {
                qint64 elapsed = m_fpsTimer.elapsed();
                if (elapsed > 0) {
                    double fps = m_frameCount * 1000.0 / elapsed;
                    qInfo() << "FPS:" << fps;
                }
            }
            m_frameCount = 0;
            m_fpsTimer.restart();
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception during projectM rendering:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception during projectM rendering";
    }
    
    // Swap buffers
    m_context->swapBuffers(this);
    
    // update plssss
    requestUpdate();
}

// Media player status handler
void ProjectMWindow::handleMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    qInfo() << "Media status changed:" << status;
    switch (status) {
        case QMediaPlayer::LoadedMedia:
            // Media loaded successfully, start playback
            qInfo() << "Media loaded successfully, starting playback.";
            m_mediaPlayer->play();
            break;
        case QMediaPlayer::EndOfMedia:
            // loop
            qInfo() << "End of media reached, looping.";
            m_mediaPlayer->setPosition(0);
            m_mediaPlayer->play();
            break;
        case QMediaPlayer::InvalidMedia:
            qWarning() << "Invalid media file:" << m_audioFilePath;
            break;
        default:
            break;
    }
}

// media player errors
void ProjectMWindow::handleMediaError(QMediaPlayer::Error error, const QString &errorString) {
    qCritical() << "Media player error occurred:" << error << "-" << errorString;
}

void ProjectMWindow::setPresetPath(const std::string& path) {
    m_presetPath = path;
}

void ProjectMWindow::setTexturePaths(const std::vector<std::string>& paths) {
    m_texturePaths = paths;
    m_texturesSetup = true;
    if (m_projectM) {
        qInfo() << "Setting texture paths in ProjectM instance.";
        m_projectM->SetTexturePaths(m_texturePaths);
    }
}

void ProjectMWindow::setAudioFile(const QString& filePath) {
    if (filePath == m_audioFilePath) return;

    m_audioFilePath = filePath;
    
    if (!m_audioFilePath.isEmpty()) {
        QUrl fileUrl = QUrl::fromLocalFile(m_audioFilePath);
        qInfo() << "Setting media source to:" << fileUrl;
        m_mediaPlayer->setSource(fileUrl);
        // Playback will start when LoadedMedia status is received
    }
    
    // If window already initialized, close old file and open new one for visualization
    if (m_initialized) {
         closeAudioFile();
         if (!m_audioFilePath.isEmpty()) {
             openAudioFile();
         }
    }
}

bool ProjectMWindow::openAudioFile() {
    if (m_audioFilePath.isEmpty()) {
        qWarning() << "Audio file path is empty, using dummy audio.";
        return false;
    }
    closeAudioFile();

    memset(&m_sfInfo, 0, sizeof(m_sfInfo));
    m_sndFile = sf_open(m_audioFilePath.toStdString().c_str(), SFM_READ, &m_sfInfo);

    if (!m_sndFile) {
        qCritical() << "Error opening audio file:" << m_audioFilePath
                    << "- Error:" << sf_strerror(NULL);
        return false;
    }

    qInfo() << "Opened audio file:" << m_audioFilePath;
    qInfo() << "  Frames:" << m_sfInfo.frames << "Samplerate:" << m_sfInfo.samplerate
            << "Channels:" << m_sfInfo.channels << "Format:" << m_sfInfo.format;

    if (m_sfInfo.channels < 1 || m_sfInfo.channels > 2) {
         qCritical() << "Error: Only mono or stereo files supported. Channels:" << m_sfInfo.channels;
         closeAudioFile();
         return false;
    }

    m_audioReadBuffer.resize(AUDIO_FRAMES_PER_CHUNK * m_sfInfo.channels);
    m_dummyPcmData.resize(PCM_BUFFER_SIZE * 2); // make absolutely sure dummy buffer matches PCM_BUFFER_SIZE

    return true;
}

void ProjectMWindow::closeAudioFile() {
    if (m_sndFile) {
        sf_close(m_sndFile);
        m_sndFile = nullptr;
        qInfo() << "Closed audio file.";
    }
}

void ProjectMWindow::processAudioChunk() {
    if (!m_projectMPcm) {
        qWarning() << "ProjectM PCM object is null, cannot process audio.";
        return;
    }

    if (!m_sndFile) {
        // --- Use Dummy Sine Wave Data ---
        m_pcmCounter++;
        size_t dummySamplesPerChannel = PCM_BUFFER_SIZE;
        m_dummyPcmData.resize(dummySamplesPerChannel * 2); // Ensure size before filling
        for (size_t i = 0; i < dummySamplesPerChannel; ++i) {
            float val = std::sin(static_cast<float>(m_pcmCounter * 10 + i) * 0.1f);
            short pcmValue = static_cast<short>(val * 32760.0f);
            m_dummyPcmData[i * 2] = pcmValue;
            m_dummyPcmData[i * 2 + 1] = pcmValue;
        }
        // Use the int16_t Add overload for dummy data
        m_projectMPcm->Add(reinterpret_cast<const int16_t*>(m_dummyPcmData.data()), 2, dummySamplesPerChannel);
        return;
    }

    // --- Read Real Audio Data ---
    m_audioReadBuffer.resize(AUDIO_FRAMES_PER_CHUNK * m_sfInfo.channels); // Ensure size before reading
    sf_count_t framesRead = sf_readf_float(m_sndFile, m_audioReadBuffer.data(), AUDIO_FRAMES_PER_CHUNK);

    if (framesRead > 0) {
        size_t samples_per_channel = static_cast<size_t>(framesRead);
        // Feed float data
        m_projectMPcm->Add(m_audioReadBuffer.data(), m_sfInfo.channels, samples_per_channel);
        // For debugging: print max amplitude of this chunk
        if (m_frameCount % 100 == 0) { // Only check occasionally to avoid log spam
            float maxAmp = 0.0f;
            for (size_t i = 0; i < framesRead * m_sfInfo.channels; ++i) {
                maxAmp = std::max(maxAmp, std::abs(m_audioReadBuffer[i]));
            }
            qDebug() << "Audio chunk max amplitude:" << maxAmp;
        }
    } else {
        qInfo() << "End of audio file reached or read error.";
        // loop
        sf_seek(m_sndFile, 0, SEEK_SET);
    }
}

// --- New Preset Management Methods ---

void ProjectMWindow::loadAvailablePresets() {
    m_presetFiles.clear();
    
    try {
        std::filesystem::path dirPath(m_presetPath);
        if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
            // find all .milk files need my milk
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".milk") {
                    m_presetFiles.push_back(entry.path().string());
                }
            }
            
            qInfo() << "Found" << m_presetFiles.size() << "preset files";
            
            // Random
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(m_presetFiles.begin(), m_presetFiles.end(), g);
        }
    } catch (const std::exception& e) {
        qWarning() << "Error loading presets:" << e.what();
    }
}

void ProjectMWindow::nextPreset() {
    if (m_presetFiles.empty() || !m_projectM) return;
    
    m_currentPresetIndex = (m_currentPresetIndex + 1) % m_presetFiles.size();
    QString presetName = QString::fromStdString(m_presetFiles[m_currentPresetIndex]);
    qInfo() << "Loading next preset:" << presetName;
    m_projectM->LoadPresetFile(m_presetFiles[m_currentPresetIndex], false);
}

void ProjectMWindow::previousPreset() {
    if (m_presetFiles.empty() || !m_projectM) return;
    
    m_currentPresetIndex--;
    if (m_currentPresetIndex < 0) 
        m_currentPresetIndex = m_presetFiles.size() - 1;
    
    QString presetName = QString::fromStdString(m_presetFiles[m_currentPresetIndex]);
    qInfo() << "Loading previous preset:" << presetName;
    m_projectM->LoadPresetFile(m_presetFiles[m_currentPresetIndex], false);
}

void ProjectMWindow::setPresetDuration(double seconds) {
    m_presetDuration = seconds;
    
    if (m_projectM) {
        m_projectM->SetPresetDuration(seconds);
    }
    
    // Restart timer with new duration
    if (m_presetTimer.isActive()) {
        m_presetTimer.stop();
        m_presetTimer.start(m_presetDuration * 1000);
    }
}