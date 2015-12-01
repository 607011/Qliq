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

#include <QDebug>
#include <QBitArray>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QSysInfo>
#include <QVBoxLayout>
#include <QSlider>
#include <QMutex>


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
    , inverter(false)
    , sampleBufferMutex(new QMutex)
    , lastDeltaT(0)
  {
    audioFormat.setSampleRate(192000);
    audioFormat.setChannelCount(1);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleSize(16);
    audioFormat.setByteOrder(QAudioFormat::Endian(QSysInfo::ByteOrder));
    audioFormat.setSampleType(QAudioFormat::SignedInt);
  }
  ~MainWindowPrivate()
  {
    // ...
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
  bool inverter;
  QMutex *sampleBufferMutex;
  qint64 lastDeltaT;
};


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
  ui->layout->addWidget(d->volumeRenderArea);

  d->waveRenderArea = new WaveRenderArea(d->sampleBufferMutex);
  ui->layout->addWidget(d->waveRenderArea);
  d->waveRenderArea->setAudioFormat(d->audioFormat);
  QObject::connect(d->waveRenderArea, SIGNAL(click(qint64)), SLOT(onClick(qint64)));

  d->audioInput  = new AudioInputDevice(d->audioFormat, d->sampleBufferMutex, this);
  QObject::connect(d->audioInput, SIGNAL(update()), SLOT(onRefreshDisplay()));

  QSlider *volumeSlider = new QSlider(Qt::Horizontal);
  volumeSlider->setRange(0, 100);
  QObject::connect(volumeSlider, SIGNAL(valueChanged(int)), SLOT(onVolumeSliderChanged(int)));
  ui->layout->addWidget(volumeSlider);

  d->audio = new QAudioInput(d->audioDeviceInfo, d->audioFormat, this);
  volumeSlider->setValue(qRound(100 * d->audio->volume()));
  QObject::connect(d->audio, SIGNAL(stateChanged(QAudio::State)), SLOT(onAudioStateChanged(QAudio::State)));

  ui->layout->addStretch();

  ui->statusBar->showMessage(d->audioDeviceInfo.deviceName());

  d->audioInput->start();
  d->audio->start(d->audioInput);
}


MainWindow::~MainWindow()
{
  delete ui;
}


void MainWindow::onAudioStateChanged(QAudio::State audioState)
{
  Q_D(MainWindow);
  qDebug() << audioState;
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
    break;
  default:
    break;
  }
}


void MainWindow::onRefreshDisplay(void)
{
  Q_D(MainWindow);
  d->volumeRenderArea->setLevel(d->audioInput->level());
  d->waveRenderArea->setData(d->audioInput->sampleBuffer());
}


void MainWindow::onVolumeSliderChanged(int value)
{
  Q_D(MainWindow);
  if (d->audio != Q_NULLPTR) {
    d->audio->setVolume(1e-2 * value);
  }
}


void MainWindow::onClick(const qint64 dt)
{
  Q_D(MainWindow);
  bool bit = dt > d->lastDeltaT;
  addBit(bit ^ d->inverter);
  d->inverter = !d->inverter;
  d->lastDeltaT = dt;
}


void MainWindow::addBit(bool bit)
{
  Q_D(MainWindow);
  d->currentByte |= bit << d->currentByteIndex;
  ++d->currentByteIndex;
  if (d->currentByteIndex > 7) {
    // ui->statusBar->showMessage(QString("%1").arg(d->currentByte));
    qDebug() << d->currentByte;
    d->randomBytes.append(d->currentByte);
    d->currentByte = 0;
    d->currentByteIndex = 0;
  }
}
