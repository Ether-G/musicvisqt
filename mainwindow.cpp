#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "projectmwindow.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMenu>
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Create central widget and layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);
    
    // Create ProjectM window
    m_projectMWindow = new ProjectMWindow();
    m_projectMWindow->setMinimumSize(QSize(800, 600));
    
    // Create container widget for the ProjectM window
    m_containerWidget = QWidget::createWindowContainer(m_projectMWindow, this);
    m_containerWidget->setFocusPolicy(Qt::StrongFocus);
    m_containerWidget->setFocus();
    
    // Create player controller
    m_playerController = new PlayerController(this);
    m_playerController->setMediaPlayer(m_projectMWindow->mediaPlayer());
    m_playerController->setAudioOutput(m_projectMWindow->audioOutput());
    
    // Add widgets to layout
    layout->addWidget(m_containerWidget);
    layout->addWidget(m_playerController);
    
    // Set up menu actions
    QAction *openAction = new QAction(tr("Open Audio File"), this);
    connect(openAction, &QAction::triggered, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this,
            tr("Open Audio File"), "",
            tr("Audio Files (*.mp3 *.wav *.ogg *.flac);;All Files (*)"));
            
        if (!filePath.isEmpty()) {
            m_projectMWindow->setAudioFile(filePath);
        }
    });
    
    // Add menu bar
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(openAction);
    
    // Set window title
    setWindowTitle(tr("MusicVisQT"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setAudioFile(const QString& filePath)
{
    if (m_projectMWindow) {
        m_projectMWindow->setAudioFile(filePath);
    }
}