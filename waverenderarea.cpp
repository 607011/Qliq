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


class WaveRenderAreaPrivate
{
public:
  explicit WaveRenderAreaPrivate(QMutex *mutex)
    : lastClickTime(0)
    , maxAmplitude(0)
    , sampleBufferMutex(mutex)
  {
    timer.start();
  }
  ~WaveRenderAreaPrivate() { /* ... */ }
  QElapsedTimer timer;
  qint64 lastClickTime;
  qint64 dt0;
  QByteArray sampleBuffer;
  QAudioFormat audioFormat;
  QPixmap pixmap;
  quint32 maxAmplitude;
  QMutex *sampleBufferMutex;
};


WaveRenderArea::WaveRenderArea(QMutex *mutex, QWidget *parent)
  : QWidget(parent)
  , d_ptr(new WaveRenderAreaPrivate(mutex))
{
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setMinimumHeight(64);
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


void WaveRenderArea::drawPixmap(void)
{
  Q_D(WaveRenderArea);
  static const QColor BackgroundColor(0x33, 0x33, 0x33);
  static const QPen LinePen(QBrush(QColor(0x00, 0xff, 0x00).darker(150)), 0.5);
  QPainter p(&d->pixmap);
  p.setRenderHint(QPainter::Antialiasing);
  p.fillRect(d->pixmap.rect(), BackgroundColor);
  if (d->audioFormat.isValid() && d->maxAmplitude > 0 && !d->sampleBuffer.isEmpty()) {
    qint64 nsPerFrame = 1000 * d->audioFormat.durationForFrames(d->sampleBuffer.size() / d->audioFormat.bytesPerFrame());
    const int halfHeight = d->pixmap.height() / 2;
    QPointF origin(0, halfHeight);
    QLineF line(origin, origin);
    p.setPen(LinePen);
    const int numFrames = d->sampleBuffer.size() / d->audioFormat.bytesPerFrame();
    const qreal xd = qreal(d->pixmap.width()) / numFrames;
    const qint16* buffer = reinterpret_cast<const qint16 *>(d->sampleBuffer.data());
    qreal x = 0;
    for (int i = 0; i < numFrames; ++i) {
      const qint16* ptr = buffer + i * d->audioFormat.channelCount();
      const qreal y = qreal(*ptr) / d->maxAmplitude;
      if (qAbs(y) > 0.5f) { // TODO: improve peak detection
        qint64 ddt = d->dt0 + nsPerFrame * i;
        qint64 dt = ddt - d->lastClickTime;
        d->lastClickTime = ddt;
        emit click(dt);
      }
      line.setP2(QPointF(x, halfHeight + y * halfHeight));
      p.drawLine(line);
      line.setP1(line.p2());
      x += xd;
    }
  }
  update();
}


void WaveRenderArea::setData(QByteArray data)
{
  Q_D(WaveRenderArea);
  d->dt0 = d->timer.nsecsElapsed();
  QMutexLocker(d->sampleBufferMutex);
  d->sampleBuffer = data;
  drawPixmap();
}


void WaveRenderArea::setAudioFormat(const QAudioFormat &format)
{
  Q_D(WaveRenderArea);
  d->audioFormat = format;
  d->maxAmplitude = AudioInputDevice::maxAmplitudeForFormat(d->audioFormat);
  update();
}
