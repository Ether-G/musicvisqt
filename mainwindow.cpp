#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "projectmwindow.h"

#include <QVBoxLayout>
#include <QWindow>
#include <QWidget>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // ProjectMWindow (direct OpenGL window) Will probably refactor later, but I had alot of difficulty using opengl directly as a widget in QT
    m_projectMWindow = new ProjectMWindow();
    m_projectMWindow->setMinimumSize(QSize(400, 300));
    
    // container widget to hold the native window
    m_containerWidget = QWidget::createWindowContainer(m_projectMWindow, this);
    m_containerWidget->setFocusPolicy(Qt::StrongFocus);
    m_containerWidget->setFocus();
    
    // Set it central
    setCentralWidget(m_containerWidget);
    
    // window title
    setWindowTitle("MusicVisQT");
    
    // initial size
    resize(800, 600);
}

MainWindow::~MainWindow()
{
    delete ui;
    // The m_containerWidget and m_projectMWindow will be deleted when parent is destroyed
}

void MainWindow::setAudioFile(const QString& filePath)
{
    if (m_projectMWindow) {
        m_projectMWindow->setAudioFile(filePath);
    } else {
        qWarning() << "MainWindow could not find ProjectMWindow instance to set audio file.";
    }
}