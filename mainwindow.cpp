/*

    Copyright (c) 2015 Oliver Lau <ola@ct.de>, Heise Medien GmbH & Co. KG

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "volumerenderarea.h"
#include "waverenderarea.h"
#include "audioinputdevice.h"
#include "global.h"

#include <QDebug>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QSysInfo>
#include <QVBoxLayout>
#include <QSlider>
#include <QMutex>
#include <QFile>
#include <QSettings>
#include <QElapsedTimer>
#include <limits>


class MainWindowPrivate {
public:
  MainWindowPrivate(void)
    : audioDeviceInfo(QAudioDeviceInfo::defaultInputDevice())
    , audio(Q_NULLPTR)
    , audioInput(Q_NULLPTR)
    , volumeRenderArea(Q_NULLPTR)
    , waveRenderArea(Q_NULLPTR)
    , currentByte(0)
    , currentByteIndex(0)
    , flipBit(false)
    , sampleBufferMutex(new QMutex)
    , lastDeltaT(0)
    , paused(false)
    , settings(QSettings::IniFormat, QSettings::UserScope, AppCompanyName, AppName)
    , bps(std::numeric_limits<qreal>::min())
  {
    audioFormat.setSampleRate(44100);
    audioFormat.setChannelCount(1);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleSize(16);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);

    randomNumberFile.setFileName("..\\Qliq\\random-numbers.bin");
    randomNumberFile.open(QIODevice::WriteOnly | QIODevice::Append);
  }
  ~MainWindowPrivate()
  {
    randomNumberFile.close();
  }
  QAudioDeviceInfo audioDeviceInfo;
  QAudioFormat audioFormat;
  QAudioInput *audio;
  AudioInputDevice *audioInput;
  VolumeRenderArea *volumeRenderArea;
  WaveRenderArea *waveRenderArea;
  QByteArray randomBytes;
  quint8 currentByte;
  int currentByteIndex;
  bool flipBit;
  QMutex *sampleBufferMutex;
  qint64 lastDeltaT;
  bool paused;
  QFile randomNumberFile;
  QSettings settings;
  QElapsedTimer timer;
  qreal bps;
};


static const int MaxRandomBufferSize = 8;


MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , d_ptr(new MainWindowPrivate)
{
  Q_D(MainWindow);
  ui->setupUi(this);

  if (!d->audioDeviceInfo.isFormatSupported(d->audioFormat)) {
    d->audioFormat = d->audioDeviceInfo.nearestFormat(d->audioFormat);
    qWarning() << "Default format not supported - trying to use nearest" << d->audioFormat;
  }

  d->volumeRenderArea = new VolumeRenderArea;

  d->waveRenderArea = new WaveRenderArea(d->sampleBufferMutex);
  d->waveRenderArea->setAudioFormat(d->audioFormat);
  d->waveRenderArea->setWritePixmap(false);
  QObject::connect(d->waveRenderArea, SIGNAL(click(qint64)), SLOT(onClick(qint64)));

  QObject::connect(ui->thresholdSlider, SIGNAL(valueChanged(int)), d->waveRenderArea, SLOT(setThreshold(int)));
  const quint32 maxAmpl = AudioInputDevice::maxAmplitudeForFormat(d->audioFormat);
  ui->thresholdSlider->setRange(maxAmpl / 100, maxAmpl);
  ui->thresholdSlider->setValue(ui->thresholdSlider->maximum() * 7 / 8);

  d->audioInput  = new AudioInputDevice(d->audioFormat, d->sampleBufferMutex, this);
  QObject::connect(d->audioInput, SIGNAL(update()), SLOT(refreshDisplay()));

  QObject::connect(ui->startStopButton, SIGNAL(clicked(bool)), SLOT(startStop()));

  QObject::connect(ui->volumeSlider, SIGNAL(valueChanged(int)), SLOT(onVolumeSliderChanged(int)));

  d->audio = new QAudioInput(d->audioDeviceInfo, d->audioFormat, this);
  ui->volumeSlider->setValue(qRound(100 * d->audio->volume()));
  QObject::connect(d->audio, SIGNAL(stateChanged(QAudio::State)), SLOT(onAudioStateChanged(QAudio::State)));

  ui->graphLayout->addWidget(d->waveRenderArea);
  ui->graphLayout->addWidget(d->volumeRenderArea);

  ui->statusBar->showMessage(d->audioDeviceInfo.deviceName());

  restoreSettings();

  d->audioInput->start();
  d->audio->start(d->audioInput);
}


MainWindow::~MainWindow()
{
  Q_D(MainWindow);
  d->audio->stop();
  d->audioInput->stop();
  delete ui;
}


void MainWindow::closeEvent(QCloseEvent *e)
{
  saveSettings();
  e->accept();
}


void MainWindow::saveSettings(void)
{
  Q_D(MainWindow);
  d->settings.setValue("mainwindow/geometry", saveGeometry());
  d->settings.setValue("analysis/threshold", d->waveRenderArea->threshold());
  d->settings.setValue("analysis/lockTimeNs", d->waveRenderArea->lockTimeNs());
  d->settings.sync();
}


void MainWindow::restoreSettings(void)
{
  Q_D(MainWindow);
  restoreGeometry(d->settings.value("mainwindow/geometry", 32000).toByteArray());
  d->waveRenderArea->setThreshold(d->settings.value("analysis/threshold", 32000).toInt());
  d->waveRenderArea->setLockTimeNs(d->settings.value("analysis/lockTimeNs", 1600 * 1000).toLongLong());
}


void MainWindow::refreshDisplay(void)
{
  Q_D(MainWindow);
  if (!d->paused) {
    d->volumeRenderArea->setLevel(d->audioInput->level());
    d->waveRenderArea->setData(d->audioInput->sampleBuffer());
  }
}


void MainWindow::onVolumeSliderChanged(int value)
{
  Q_D(MainWindow);
  if (d->audio != Q_NULLPTR) {
    d->audio->setVolume(1e-2 * value);
  }
}


void MainWindow::addBit(int bit)
{
  Q_D(MainWindow);
  d->currentByte |= bit << d->currentByteIndex;
  ++d->currentByteIndex;
  if (d->currentByteIndex > 7) {
    ui->statusBar->showMessage(QString("%1 (0x%2) %3 byte/s")
                               .arg(uint(d->currentByte), 8, 2, QChar('0'))
                               .arg(uint(d->currentByte), 2, 16, QChar('0'))
                               .arg(d->bps, 0, 'f', 1));
    d->randomBytes.append(d->currentByte);
    if (d->randomBytes.size() >= MaxRandomBufferSize) {
      d->bps = 1e9 * MaxRandomBufferSize / d->timer.nsecsElapsed();
      d->timer.restart();
      d->randomNumberFile.write(d->randomBytes);
      d->randomNumberFile.flush();
      d->randomBytes.clear();
    }
    d->currentByte = 0;
    d->currentByteIndex = 0;
  }
}


void MainWindow::onClick(const qint64 dt)
{
  Q_D(MainWindow);
  int bit = (dt > d->lastDeltaT);
  addBit(bit);
  // d->flipBit = !d->flipBit;
  d->lastDeltaT = dt;
}


void MainWindow::startStop(void)
{
  Q_D(MainWindow);
  d->paused = !d->paused;
}


void MainWindow::onAudioStateChanged(QAudio::State audioState)
{
  Q_D(MainWindow);
  switch (audioState) {
  case QAudio::StoppedState:
    if (d->audio->error() != QAudio::NoError) {
      // Error handling
    }
    else {
      // Finished recording
    }
    break;
  case QAudio::ActiveState:
    d->timer.start();
    break;
  default:
    break;
  }
}
