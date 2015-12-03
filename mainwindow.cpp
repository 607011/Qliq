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
#include "wavfile.h"

#include <QDebug>
#include <QBitArray>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QSysInfo>
#include <QVBoxLayout>
#include <QSlider>
#include <QMutex>
#include <QFile>


class MainWindowPrivate {
public:
  MainWindowPrivate(void)
    : audioDeviceInfo(QAudioDeviceInfo::defaultInputDevice())
    , audio(Q_NULLPTR)
    , audioInput(Q_NULLPTR)
    , volumeRenderArea(Q_NULLPTR)
    , waveRenderArea(Q_NULLPTR)
    , volumeSlider(new QSlider(Qt::Horizontal))
    , currentByte(0)
    , currentByteIndex(0)
    , bit(0)
    , sampleBufferMutex(new QMutex)
    , lastDeltaT(0)
  {
    audioFormat.setSampleRate(32000);
    audioFormat.setChannelCount(1);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleSize(16);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    volumeSlider->setTickPosition(QSlider::TicksBelow);
    volumeSlider->setRange(0, 100);

    WavFile wavFile;
    if (wavFile.open("..\\Qliq\\ClickSound-32kHz-Mono-Signed-LE-16.wav")) {
      QByteArray wav = wavFile.readAll();
      Q_ASSERT(wav.size() % sizeof(qint16) == 0);
      const qint16 *ptr = reinterpret_cast<const qint16*>(wav.data());
      const int nSamples = wav.size() / sizeof(qint16);
      clickSound.resize(nSamples);
      for (int i = 0; i < nSamples; ++i) {
        clickSound[i] = ptr[i];
      }
      qDebug() << wavFile.fileName() << "loaded.";
    }
    wavFile.close();

    randomNumberFile.setFileName("..\\Qliq\\random-numbers.bin");
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
  QSlider *volumeSlider;
  QByteArray randomBytes;
  quint8 currentByte;
  int currentByteIndex;
  int bit;
  QMutex *sampleBufferMutex;
  qint64 lastDeltaT;
  QVector<int> clickSound;
  QFile randomNumberFile;
};


static const int MaxRandomBufferSize = 1;


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
  d->waveRenderArea->setPattern(d->clickSound);
  QObject::connect(d->waveRenderArea, SIGNAL(click(qint64)), SLOT(onClick(qint64)));

  d->audioInput  = new AudioInputDevice(d->audioFormat, d->sampleBufferMutex, this);
  QObject::connect(d->audioInput, SIGNAL(update()), SLOT(refreshDisplay()));

  QObject::connect(d->volumeSlider, SIGNAL(valueChanged(int)), SLOT(onVolumeSliderChanged(int)));
  ui->layout->addWidget(d->volumeSlider);

  d->audio = new QAudioInput(d->audioDeviceInfo, d->audioFormat, this);
  d->volumeSlider->setValue(qRound(100 * d->audio->volume()));
  QObject::connect(d->audio, SIGNAL(stateChanged(QAudio::State)), SLOT(onAudioStateChanged(QAudio::State)));

  ui->layout->addStretch();

  ui->statusBar->showMessage(d->audioDeviceInfo.deviceName());

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


void MainWindow::refreshDisplay(void)
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
  int bit = (dt > d->lastDeltaT) ? d->bit : (d->bit ^ 1);
  addBit(bit);
  qDebug() << bit << d->bit << dt << d->lastDeltaT;
  d->bit ^= 1;
  d->lastDeltaT = dt;
}


void MainWindow::addBit(int bit)
{
  Q_D(MainWindow);
  d->currentByte |= bit;
  d->currentByte <<= 1;
  ++d->currentByteIndex;
  if (d->currentByteIndex > 7) {
    ui->statusBar->showMessage(QString("random byte: %1 (%2b) %3h")
                               .arg(d->currentByte)
                               .arg(uint(d->currentByte), 8, 2, QChar('0'))
                               .arg(uint(d->currentByte), 2, 16, QChar('0')), 3500);
    d->randomBytes.append(d->currentByte);
    if (d->randomBytes.size() >= MaxRandomBufferSize) {
      d->randomNumberFile.open(QIODevice::WriteOnly | QIODevice::Append);
      d->randomNumberFile.write(d->randomBytes);
      d->randomNumberFile.close();
      d->randomBytes.clear();
    }
    d->currentByte = 0;
    d->currentByteIndex = 0;
  }
}
