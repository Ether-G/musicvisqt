#include "playercontroller.h"
#include <QTime>

PlayerController::PlayerController(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void PlayerController::setMediaPlayer(QMediaPlayer *player)
{
    m_mediaPlayer = player;
    if (m_mediaPlayer) {
        connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
                this, &PlayerController::updatePlayPauseButton);
        connect(m_mediaPlayer, &QMediaPlayer::positionChanged,
                this, &PlayerController::updatePosition);
        connect(m_mediaPlayer, &QMediaPlayer::durationChanged,
                this, &PlayerController::updateDuration);
    }
}

void PlayerController::setAudioOutput(QAudioOutput *audioOutput)
{
    m_audioOutput = audioOutput;
    if (m_audioOutput) {
        m_volumeSlider->setValue(static_cast<int>(m_audioOutput->volume() * 100));
    }
}

void PlayerController::setupUI()
{
    // Create buttons
    m_playPauseButton = new QPushButton(this);
    m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playPauseButton->setToolTip(tr("Play/Pause"));
    
    m_stopButton = new QPushButton(this);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setToolTip(tr("Stop"));
    
    // Create sliders
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 0);
    m_positionSlider->setToolTip(tr("Position"));
    
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    m_volumeSlider->setToolTip(tr("Volume"));
    
    // Create labels
    m_positionLabel = new QLabel("00:00", this);
    m_durationLabel = new QLabel("00:00", this);
    
    // Connect signals
    connect(m_playPauseButton, &QPushButton::clicked, this, &PlayerController::playPause);
    connect(m_stopButton, &QPushButton::clicked, this, &PlayerController::stop);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &PlayerController::setVolume);
    connect(m_positionSlider, &QSlider::sliderMoved, this, &PlayerController::setPosition);
    
    // Layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_playPauseButton);
    layout->addWidget(m_stopButton);
    layout->addWidget(m_positionLabel);
    layout->addWidget(m_positionSlider);
    layout->addWidget(m_durationLabel);
    layout->addWidget(new QLabel(tr("Vol:")));
    layout->addWidget(m_volumeSlider);
    
    setLayout(layout);
}

void PlayerController::playPause()
{
    if (!m_mediaPlayer) return;
    
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
    } else {
        m_mediaPlayer->play();
    }
}

void PlayerController::stop()
{
    if (!m_mediaPlayer) return;
    m_mediaPlayer->stop();
}

void PlayerController::setVolume(int volume)
{
    if (!m_audioOutput) return;
    m_audioOutput->setVolume(volume / 100.0);
}

void PlayerController::updatePlayPauseButton()
{
    if (!m_mediaPlayer) return;
    
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    } else {
        m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void PlayerController::updatePosition(qint64 position)
{
    m_positionSlider->setValue(static_cast<int>(position));
    m_positionLabel->setText(formatTime(position));
}

void PlayerController::updateDuration(qint64 duration)
{
    m_positionSlider->setRange(0, static_cast<int>(duration));
    m_durationLabel->setText(formatTime(duration));
}

void PlayerController::setPosition(int position)
{
    if (!m_mediaPlayer) return;
    m_mediaPlayer->setPosition(position);
}

QString PlayerController::formatTime(qint64 ms)
{
    QTime time(0, 0);
    time = time.addMSecs(ms);
    return time.toString("mm:ss");
} 