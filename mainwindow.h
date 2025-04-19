#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QWindow>
#include <QWidget>
#include "projectmwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setAudioFile(const QString& filePath);

private:
    Ui::MainWindow *ui;
    ProjectMWindow *m_projectMWindow;
    QWidget *m_containerWidget;
};
#endif // MAINWINDOW_H