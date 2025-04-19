#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QString>
#include <QDebug>
#include <QSurfaceFormat>
#include <QCoreApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    // Force OpenGL as the rendering backend before QApplication is initialized
    qputenv("QSG_RHI_BACKEND", "opengl");
    
    // Force desktop OpenGL usage (not OpenGL ES)
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    
    // Set OpenGL format for the application
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);  // Request OpenGL 3.3
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSamples(4); // Enable multisampling for smoother rendering
    QSurfaceFormat::setDefaultFormat(format);
    
    QApplication app(argc, argv);
    QApplication::setApplicationName("QtProjectMVisualizer");
    QApplication::setApplicationVersion("1.0");
    
    // pwd debug
    qInfo() << "Current directory:" << QDir::currentPath();
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Qt ProjectM Music Visualizer");
    parser.addHelpOption();
    parser.addVersionOption();
    // positional arg audio file
    parser.addPositionalArgument("audiofile", QApplication::translate("main", "Audio file to visualize."), "[audiofile]");

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    QString audioFilePath;
    if (!args.isEmpty()) {
        audioFilePath = args.at(0);
        QFileInfo check_file(audioFilePath);
        // Does it exist???
        if (!check_file.exists() || !check_file.isFile()) {
             qWarning() << "Error: Provided audio file path does not exist or is not a file:" << audioFilePath;
             // Can I find it???
             QFileInfo current_dir_file(QDir::currentPath() + "/" + audioFilePath);
             if (current_dir_file.exists() && current_dir_file.isFile()) {
                 audioFilePath = current_dir_file.absoluteFilePath();
                 qInfo() << "Found audio file in current directory:" << audioFilePath;
             } else {
                 audioFilePath.clear(); // dummy audio
             }
        } else {
            qInfo() << "Using audio file:" << audioFilePath;
        }
    } else {
        qWarning() << "No audio file provided. Visualization will use simulated audio.";
    }

    MainWindow w; // Create main window

    // Pass the audio file path (which might be empty) to the main window.
    w.setAudioFile(audioFilePath);

    w.show();
    return app.exec();
}