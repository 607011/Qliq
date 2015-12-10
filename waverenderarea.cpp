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
#include <QDateTime>
#include <qmath.h>
#include <limits>

class WaveRenderAreaPrivate
{
public:
  explicit WaveRenderAreaPrivate(QMutex *mutex)
    : doWritePixmap(false)
    , maxAmplitude(-1)
    , sampleBufferMutex(mutex)
    , threshold(std::numeric_limits<int>::max())
    , mouseDown(false)
    , pos1(0)
    , pos2(0)
    , lockTimeNs(4 * 1000 * 1000)
    , lastClickTimestampNs(0)
    , frameTimestampNs(0)
  {
    Q_ASSERT(mutex != Q_NULLPTR);
  }
  ~WaveRenderAreaPrivate() { /* ... */ }
  QAudioFormat audioFormat;
  QPixmap pixmap;
  bool doWritePixmap;
  quint32 maxAmplitude;
  QMutex *sampleBufferMutex;
  QVector<int> sampleBuffer;
  int threshold;
  QVector<int> peakPos;
  bool mouseDown;
  int pos1;
  int pos2;
  qint64 lockTimeNs;
  qint64 lastClickTimestampNs;
  qint64 frameTimestampNs;
};


WaveRenderArea::WaveRenderArea(QMutex *mutex, QWidget *parent)
  : QWidget(parent)
  , d_ptr(new WaveRenderAreaPrivate(mutex))
{
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setMinimumHeight(56);
  reset();
}


WaveRenderArea::~WaveRenderArea() { /* ... */ }


QSize WaveRenderArea::sizeHint(void) const
{
 return QSize(512, 96);
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


void WaveRenderArea::mousePressEvent(QMouseEvent *e)
{
  Q_D(WaveRenderArea);
  if (e->button() == Qt::LeftButton) {
    d->mouseDown = true;
    d->pos1 = e->x();
    d->pos2 = d->pos1;
    drawPixmap();
  }
}


void WaveRenderArea::mouseReleaseEvent(QMouseEvent *e)
{
  Q_D(WaveRenderArea);
  if (e->button() == Qt::LeftButton) {
    d->mouseDown = false;
    drawPixmap();
    d->lockTimeNs = (qMax(d->pos1, d->pos2) - qMin(d->pos1, d->pos2)) * 1000 * d->audioFormat.durationForFrames(d->sampleBuffer.size()) / width();
  }
}


void WaveRenderArea::mouseMoveEvent(QMouseEvent *e)
{
  Q_D(WaveRenderArea);
  if (d->mouseDown) {
    d->pos2 = e->x();
    drawPixmap();
  }
}


void WaveRenderArea::setThreshold(int threshold)
{
  Q_D(WaveRenderArea);
  d->threshold = threshold;
  drawPixmap();
}


void WaveRenderArea::setLockTimeNs(qint64 lockTimeNs)
{
  Q_D(WaveRenderArea);
  d->lockTimeNs = lockTimeNs;
  drawPixmap();
}


void WaveRenderArea::drawPixmap(void)
{
  Q_D(WaveRenderArea);
  if (d->pixmap.isNull())
    return;
  Q_ASSERT(d->audioFormat.channelCount() == 1);
  static const QColor BackgroundColor(17, 33, 17);
  static const QPen WaveLinePen(QBrush(QColor(54, 255, 54)), 0.5);
  static const QPen PeakPen(QColor(255, 0, 0));
  QPainter p(&d->pixmap);
  p.fillRect(d->pixmap.rect(), BackgroundColor);
  if (d->audioFormat.isValid() && d->maxAmplitude > 0 && !d->sampleBuffer.isEmpty()) {
    const int halfHeight = d->pixmap.height() / 2;
    QPointF origin(0, halfHeight);
    QLineF waveLine(origin, origin);
    const qreal xd = qreal(d->pixmap.width()) / d->sampleBuffer.size();
    if (!d->peakPos.isEmpty()) {
      p.setRenderHint(QPainter::Antialiasing, false);
      p.setPen(PeakPen);
      for (int i = 0; i < d->peakPos.size(); ++i) {
        const int x = int(d->peakPos.at(i) * xd);
        p.drawLine(QLine(x, 0, x, height()));
      }
    }
    qreal x = 0.0;
    for (int i = 0; i < d->sampleBuffer.size(); ++i) {
      p.setRenderHint(QPainter::Antialiasing, true);
      const qreal y = qreal(d->sampleBuffer.at(i)) / d->maxAmplitude;
      waveLine.setP2(QPointF(x, halfHeight - y * halfHeight));
      p.setPen(WaveLinePen);
      p.drawLine(waveLine);
      waveLine.setP1(waveLine.p2());
      x += xd;
    }
    static const QBrush ThresholdBrush(QColor(255, 255, 255, 72), Qt::SolidPattern);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(QRectF(0, 0, width(), halfHeight - qreal(d->threshold) / d->maxAmplitude * halfHeight), ThresholdBrush);
    if (d->mouseDown) {
      static const QBrush MarkerBrush(QColor(255, 255, 0, 72), Qt::SolidPattern);
      p.fillRect(QRectF(d->pos1, 0, (d->pos2 - d->pos1), height()), MarkerBrush);
    }
    if (d->doWritePixmap && !d->peakPos.isEmpty()) {
      p.end();
      d->pixmap.save(QString("..\\Qliq\\screenshots\\%1.png").arg(d->frameTimestampNs / 1000 / 1000, 12, 10, QChar('0')));
    }
  }
  update();
}


void WaveRenderArea::findPeaks(void)
{
  Q_D(WaveRenderArea);
  d->lastClickTimestampNs = d->frameTimestampNs;
  d->peakPos.clear();
  const qint64 nsPerFrame = 1000 * d->audioFormat.durationForFrames(d->sampleBuffer.size());
  const qint64 skipLength = d->audioFormat.framesForDuration(d->lockTimeNs / 1000);
  int i = 0;
  while (i < d->sampleBuffer.size()) {
    const int sample = d->sampleBuffer.at(i);
    const qint64 dtNs = nsPerFrame * i / d->sampleBuffer.size();
    const qint64 currentTimestampNs = d->frameTimestampNs + dtNs;
    const qint64 nsElapsed = currentTimestampNs - d->lastClickTimestampNs;
    if (sample > d->threshold && nsElapsed > d->lockTimeNs) {
      emit click(nsElapsed);
      d->lastClickTimestampNs = currentTimestampNs;
      d->peakPos.append(i);
      i += skipLength;
    }
    else {
      i += 1;
    }
  }
  d->frameTimestampNs += nsPerFrame;
}


void WaveRenderArea::setData(const QVector<int> &data)
{
  Q_D(WaveRenderArea);
  QMutexLocker(d->sampleBufferMutex);
  d->sampleBuffer = data;
  findPeaks();
  drawPixmap();
}


void WaveRenderArea::setAudioFormat(const QAudioFormat &format)
{
  Q_D(WaveRenderArea);
  Q_ASSERT(format.channelCount() == 1);
  d->audioFormat = format;
  d->maxAmplitude = AudioInputDevice::maxAmplitudeForFormat(d->audioFormat);
}


void WaveRenderArea::setWritePixmap(bool doWritePixmap)
{
  Q_D(WaveRenderArea);
  d->doWritePixmap = doWritePixmap;
}


void WaveRenderArea::reset(void)
{
  Q_D(WaveRenderArea);
  d->frameTimestampNs = 0;
  d->lastClickTimestampNs = 0;
}


qint64 WaveRenderArea::lockTimeNs(void) const
{
  return d_ptr->lockTimeNs;
}


int WaveRenderArea::threshold(void) const
{
  return d_ptr->threshold;
}
