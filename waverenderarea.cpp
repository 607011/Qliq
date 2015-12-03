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


#include "waverenderarea.h"
#include "audioinputdevice.h"

#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QLineF>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QDateTime>
#include <qmath.h>

class WaveRenderAreaPrivate
{
public:
  explicit WaveRenderAreaPrivate(QMutex *mutex)
    : lastClickTime(0)
    , maxAmplitude(-1)
    , sampleBufferMutex(mutex)
    , peakPos(-1)
    , maxCorrAmplitude(-1)
  {
    Q_ASSERT(mutex != Q_NULLPTR);
    timer.start();
  }
  ~WaveRenderAreaPrivate() { /* ... */ }
  QElapsedTimer timer;
  qint64 lastClickTime;
  qint64 dt0;
  QVector<int> sampleBuffer;
  QAudioFormat audioFormat;
  QPixmap pixmap;
  quint32 maxAmplitude;
  QMutex *sampleBufferMutex;
  QVector<int> correlated;
  QVector<int> pattern;
  int peakPos;
  qlonglong maxCorrAmplitude;
};


WaveRenderArea::WaveRenderArea(QMutex *mutex, QWidget *parent)
  : QWidget(parent)
  , d_ptr(new WaveRenderAreaPrivate(mutex))
{
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setMinimumHeight(64);
}


QSize WaveRenderArea::sizeHint(void) const
{
 return QSize(512, 128);
}


void WaveRenderArea::paintEvent(QPaintEvent *)
{
  Q_D(WaveRenderArea);
  QPainter p(this);
  QMutexLocker(d->sampleBufferMutex);
  p.drawPixmap(0, 0, d->pixmap);
}


void WaveRenderArea::resizeEvent(QResizeEvent *e)
{
  Q_D(WaveRenderArea);
  QMutexLocker(d->sampleBufferMutex);
  d->pixmap = QPixmap(e->size());
  drawPixmap();
}


static qlonglong XCorrAmplitudeThreshold = 400000000;

void WaveRenderArea::drawPixmap(void)
{
  Q_D(WaveRenderArea);
  // input data is supposed to be 16 bit mono
  Q_ASSERT(d->audioFormat.channelCount() == 1);
  Q_ASSERT(d->audioFormat.sampleSize() == sizeof(SampleType) * 8);

  static const QColor BackgroundColor(0x11, 0x22, 0x11);
  static const QPen WaveLinePen(QBrush(QColor(0x33, 0xff, 0x33)), 1.0);
  static const QPen XCorrLinePen(QBrush(QColor(0x33, 0xff, 0x33).darker(300)), 1.0);
  static const QPen PeakPen(QBrush(QColor(0xff, 0x00, 0x00)), 2.0);
  QPainter p(&d->pixmap);
  p.setRenderHint(QPainter::Antialiasing);
  p.fillRect(d->pixmap.rect(), BackgroundColor);
  if (d->audioFormat.isValid() && d->maxAmplitude > 0 && !d->sampleBuffer.isEmpty()) {
    const int halfHeight = d->pixmap.height() / 2;
    QPointF origin(0, halfHeight);
    QLineF waveLine(origin, origin);
    QLineF xCorrLine(origin, origin);
    const qreal xd = qreal(d->pixmap.width()) / d->sampleBuffer.size();
    qreal x = 0;
    for (int i = 0; i < d->sampleBuffer.size(); ++i) {
      if (d->maxCorrAmplitude > 0) {
        const qreal y2 = qAbs(qreal(d->correlated.at(i)) / d->maxCorrAmplitude);
        xCorrLine.setP1(QPointF(x, height()));
        xCorrLine.setP2(QPointF(x, height() - y2 * height()));
        p.setPen(XCorrLinePen);
        p.drawLine(xCorrLine);
      }
      const qreal y = qreal(d->sampleBuffer.at(i)) / d->maxAmplitude;
      waveLine.setP2(QPointF(x, halfHeight + y * halfHeight));
      p.setPen(WaveLinePen);
      p.drawLine(waveLine);
      waveLine.setP1(waveLine.p2());
      x += xd;
    }
    if (d->maxCorrAmplitude > XCorrAmplitudeThreshold) {
      p.setPen(PeakPen);
      p.drawLine(QLineF(d->peakPos * xd, 0, d->peakPos * xd, height()));
      p.end();
      d->pixmap.save(QString("..\\Qliq\\screenshot-%1.png").arg(QDateTime::currentMSecsSinceEpoch()));
    }
  }
  update();
}


void WaveRenderArea::correlate(void)
{
  Q_D(WaveRenderArea);
  d->peakPos = -1;
  d->maxCorrAmplitude = 0;
  d->correlated = QVector<int>(d->sampleBuffer.size(), static_cast<int>(0));
  QVector<int> transmitted = d->sampleBuffer + QVector<int>(d->pattern.size(), static_cast<int>(0));
  for (int i = 0; i < d->correlated.size(); ++i) {
    qlonglong corr = 0;
    for (int j = 0; j < d->pattern.size(); ++j) {
      corr += transmitted.at(i + j) * d->pattern.at(j);
    }
    if (corr > d->maxCorrAmplitude) {
      d->maxCorrAmplitude = corr;
      d->peakPos = i;
    }
    d->correlated[i] = corr;
  }
  if (d->maxCorrAmplitude > XCorrAmplitudeThreshold) {
    qint64 nsPerFrame = 1000 * d->audioFormat.durationForFrames(d->sampleBuffer.size());
    qDebug() << d->audioFormat.durationForFrames(d->sampleBuffer.size()) << nsPerFrame * d->peakPos / d->sampleBuffer.size() << d->maxCorrAmplitude;
  }
}


void WaveRenderArea::setPattern(const QVector<int> &pattern)
{
  Q_D(WaveRenderArea);
  d->pattern = pattern;
}


void WaveRenderArea::setData(const QVector<int> &data)
{
  Q_D(WaveRenderArea);
  d->dt0 = d->timer.nsecsElapsed();
  QMutexLocker(d->sampleBufferMutex);
  d->sampleBuffer = data;
  correlate();
  drawPixmap();
}


void WaveRenderArea::setAudioFormat(const QAudioFormat &format)
{
  Q_D(WaveRenderArea);
  // input data is supposed to be 16 bit mono
  Q_ASSERT(format.channelCount() == 1);
  Q_ASSERT(format.sampleSize() == sizeof(SampleType) * 8);
  d->audioFormat = format;
  d->maxAmplitude = AudioInputDevice::maxAmplitudeForFormat(d->audioFormat);
  update();
}
