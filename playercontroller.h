#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QStyle>

class PlayerController : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerController(QWidget *parent = nullptr);
    void setMediaPlayer(QMediaPlayer *player);
    void setAudioOutput(QAudioOutput *audioOutput);

private slots:
    void playPause();
    void stop();
    void setVolume(int volume);
    void updatePlayPauseButton();
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void setPosition(int position);

private:
    QMediaPlayer *m_mediaPlayer = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    
    QPushButton *m_playPauseButton;
    QPushButton *m_stopButton;
    QSlider *m_volumeSlider;
    QSlider *m_positionSlider;
    QLabel *m_positionLabel;
    QLabel *m_durationLabel;
    
    void setupUI();
    QString formatTime(qint64 ms);
};

#endif // PLAYERCONTROLLER_H 