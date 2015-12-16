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
#include "healthcheck.h"

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
#include <QDateTime>
#include <QIcon>
#include <limits>


class MainWindowPrivate {
public:
  MainWindowPrivate(void)
    : startIcon(":/images/start.png")
    , stopIcon(":/images/stop.png")
    , audioDeviceInfo(QAudioDeviceInfo::defaultInputDevice())
    , audio(Q_NULLPTR)
    , audioInput(Q_NULLPTR)
    , volumeRenderArea(Q_NULLPTR)
    , waveRenderArea(Q_NULLPTR)
    , currentByte(0)
    , currentByteIndex(0)
    , flipBit(false)
    , sampleBufferMutex(new QMutex)
    , paused(false)
    , pauseOnNextClick(false)
    , settings(QSettings::IniFormat, QSettings::UserScope, AppCompanyName, AppName)
    , bps(std::numeric_limits<qreal>::min())
    , byteCounter(0)
    , dtIndex(0)
  {
// #define USE_PREFERRED_AUDIO_FORMAT
#ifdef USE_PREFERRED_AUDIO_FORMAT
    audioFormat = audioDeviceInfo.preferredFormat();
#else
    audioFormat.setSampleRate(11025);
    audioFormat.setChannelCount(1);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleSize(16);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
#endif
    qDebug() << audioFormat;

    randomNumberFile.setFileName("..\\Qliq\\random-numbers.bin");
    randomNumberFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    dtFile.setFileName("..\\Qliq\\dt.txt");
    dtFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
  }
  ~MainWindowPrivate()
  {
    randomNumberFile.close();
    dtFile.close();
  }
  QIcon startIcon;
  QIcon stopIcon;
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
  bool paused;
  bool pauseOnNextClick;
  QFile randomNumberFile;
  QFile dtFile;
  QSettings settings;
  QElapsedTimer timer;
  qreal bps;
  qint64 byteCounter;
  QElapsedTimer totalTimer;
  qint64 dtPair[2];
  int dtIndex;
};


static const int MaxRandomBufferSize = 20000 / 8;


MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , d_ptr(new MainWindowPrivate)
{
  Q_D(MainWindow);
  ui->setupUi(this);

  setWindowIcon(QIcon(":/images/qliq.ico"));

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

  ui->bufferProgressBar->setRange(0, MaxRandomBufferSize);
  ui->bufferProgressBar->setValue(0);

  restoreSettings();

  d->audioInput->start();
  d->audio->start(d->audioInput);
  if (!d->paused) {
    start();
  }
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
  d->settings.setValue("mainwindow/paused", d->paused);
  d->settings.setValue("options/preventBias", ui->preventBiasCheckBox->isChecked());
  d->settings.sync();
}


void MainWindow::restoreSettings(void)
{
  Q_D(MainWindow);
  restoreGeometry(d->settings.value("mainwindow/geometry").toByteArray());
  d->waveRenderArea->setThreshold(d->settings.value("analysis/threshold", 32000).toInt());
  d->waveRenderArea->setLockTimeNs(d->settings.value("analysis/lockTimeNs", 1600 * 1000).toLongLong());
  d->paused = d->settings.value("mainwindow/paused", false).toBool();
  ui->preventBiasCheckBox->setChecked(d->settings.value("options/preventBias", true).toBool());
  d->pauseOnNextClick = false;
}


void MainWindow::refreshDisplay(void)
{
  Q_D(MainWindow);
  if (!d->paused) {
    d->volumeRenderArea->setLevel(d->audioInput->level());
    d->waveRenderArea->setData(d->audioInput->sampleBuffer(), d->audio->processedUSecs());
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
    ui->statusLabel->setText(QString("%1 byte/min overall (%2 byte/s)")
                             .arg(d->byteCounter * 60 * 1000 / d->totalTimer.elapsed())
                             .arg(d->bps, 0, 'f', 1));
    d->randomBytes.append(d->currentByte);
    ui->bitsLcdNumber->display(QString("%1").arg(int(d->currentByte), 8, 2, QChar('0')));
    ui->byteLcdNumber->display(QString("%1").arg(int(d->currentByte), 2, 16, QChar('0')));
    ++d->byteCounter;
    ui->bufferProgressBar->setValue(d->randomBytes.size());
    if (d->randomBytes.size() >= MaxRandomBufferSize) {
      d->bps = 1e9 * MaxRandomBufferSize / d->timer.nsecsElapsed();
      d->timer.restart();
      bool healthy = healthCheck(d->randomBytes);
      if (healthy) {
        d->randomNumberFile.write(d->randomBytes);
        d->randomNumberFile.flush();
      }
      d->randomBytes.clear();
    }
    d->currentByte = 0;
    d->currentByteIndex = 0;
  }
}


void MainWindow::onClick(const qint64 dt)
{
  Q_D(MainWindow);
  d->dtPair[d->dtIndex++] = dt;
  if (d->dtIndex > 1) {
    d->dtIndex = 0;
    int bit = (d->dtPair[1] > d->dtPair[0]) ^ d->flipBit ? 0 : 1;
    addBit(bit);
    if (ui->preventBiasCheckBox->isChecked()) {
      d->flipBit = !d->flipBit;
    }
  }
  if (d->pauseOnNextClick) {
    stop();
  }
  d->dtFile.write(QString::number(dt).toLatin1().append("\n"));
}


void MainWindow::start(void)
{
  Q_D(MainWindow);
  d->timer.start();
  d->totalTimer.start();
  d->paused = false;
  d->pauseOnNextClick = false;
  d->byteCounter = 0;
  ui->startStopButton->setIcon(d->stopIcon);
}


void MainWindow::stop(void)
{
  Q_D(MainWindow);
  d->paused = true;
  ui->startStopButton->setIcon(d->startIcon);
}


void MainWindow::log(const QString &msg)
{
  ui->logPlainTextEdit->appendPlainText(tr("[%1] %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(msg));
}


bool MainWindow::healthCheck(const QByteArray &randomBytes)
{
  static const QPixmap HappyIcon(":/images/happy.png");
  static const QPixmap SadIcon(":/images/sad.png");
  bool healthy = true;
  bool ok;
  qreal entropy = testEntropy(randomBytes);
  log(tr("Entropy: %1").arg(entropy, 0, 'f'));
  int notPassedCount = 0;
  int testCount = 0;
  ok = testMonobit(randomBytes, notPassedCount, testCount);
  healthy &= ok;
  log(tr("FIPS 140-2 Monobit test %1.").arg(healthy ? tr("passed") : tr("failed")));
  ui->healthLabel->setPixmap(healthy ? HappyIcon : SadIcon);
  return healthy;
}


void MainWindow::startStop(void)
{
  Q_D(MainWindow);
  if (d->paused) {
    start();
  }
  else {
    d->pauseOnNextClick = true;
    d->paused = false;
  }
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
