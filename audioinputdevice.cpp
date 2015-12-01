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


#include "audioinputdevice.h"
#include <QDebug>
#include <QtEndian>


class AudioInputDevicePrivate {
public:
  AudioInputDevicePrivate(const QAudioFormat &format, QMutex *mutex)
    : format(format)
    , maxAmplitude(AudioInputDevice::maxAmplitudeForFormat(format))
    , level(0.0)
    , sampleBufferMutex(mutex)
  { /* ... */ }
  ~AudioInputDevicePrivate()
  { /* ... */ }
  const QAudioFormat format;
  quint32 maxAmplitude;
  qreal level;
  QByteArray sampleBuffer;
  QMutex *sampleBufferMutex;
};


AudioInputDevice::AudioInputDevice(const QAudioFormat &format, QMutex *mutex, QObject *parent)
  : QIODevice(parent)
  , d_ptr(new AudioInputDevicePrivate(format, mutex))
{
  /* ... */
}


quint32 AudioInputDevice::maxAmplitudeForFormat(const QAudioFormat &format)
{
  quint32 maxAmplitude;
  switch (format.sampleSize()) {
  case 8:
    switch (format.sampleType()) {
    case QAudioFormat::UnSignedInt:
      maxAmplitude = 255;
      break;
    case QAudioFormat::SignedInt:
      maxAmplitude = 127;
      break;
    default:
      break;
    }
    break;
  case 16:
    switch (format.sampleType()) {
    case QAudioFormat::UnSignedInt:
      maxAmplitude = 65535;
      break;
    case QAudioFormat::SignedInt:
      maxAmplitude = 32767;
      break;
    default:
      break;
    }
    break;
  case 32:
    switch (format.sampleType()) {
    case QAudioFormat::UnSignedInt:
      maxAmplitude = 0xffffffff;
      break;
    case QAudioFormat::SignedInt:
      maxAmplitude = 0x7fffffff;
      break;
    case QAudioFormat::Float:
      maxAmplitude = 0x7fffffff; // Kind of
    default:
      break;
    }
    break;
  default:
    break;
  }
  return maxAmplitude;
}


AudioInputDevice::~AudioInputDevice()
{
  stop();
}


void AudioInputDevice::start(void)
{
  open(QIODevice::WriteOnly);
}


void AudioInputDevice::stop(void)
{
  close();
}


qreal AudioInputDevice::level(void) const
{
  return d_ptr->level;
}


quint32 AudioInputDevice::maxAmplitude(void) const
{
  return d_ptr->maxAmplitude;
}


const QByteArray &AudioInputDevice::sampleBuffer(void) const
{
  return d_ptr->sampleBuffer;
}


qint64 AudioInputDevice::readData(char *data, qint64 maxlen)
{
  Q_UNUSED(data)
  Q_UNUSED(maxlen)
  return 0;
}


qint64 AudioInputDevice::writeData(const char *data, qint64 len)
{
  Q_D(AudioInputDevice);
  d->sampleBufferMutex->lock();
  d->sampleBuffer = QByteArray::fromRawData(data, len);
  d->sampleBufferMutex->unlock();
  if (d->maxAmplitude != 0) {
    Q_ASSERT(d->format.sampleSize() % 8 == 0);
    const int channelBytes = d->format.sampleSize() / 8;
    const int sampleBytes = d->format.channelCount() * channelBytes;
    Q_ASSERT(len % sampleBytes == 0);
    const int numSamples = len / sampleBytes;
    quint32 maxValue = 0;
    const uchar *ptr = reinterpret_cast<const uchar *>(data);
    for (int i = 0; i < numSamples; ++i) {
      for (int j = 0; j < d->format.channelCount(); ++j) {
        quint32 value = 0;
        if (d->format.sampleSize() == 8 && d->format.sampleType() == QAudioFormat::UnSignedInt) {
          value = *reinterpret_cast<const quint8*>(ptr);
        }
        else if (d->format.sampleSize() == 8 && d->format.sampleType() == QAudioFormat::SignedInt) {
          value = qAbs(*reinterpret_cast<const qint8*>(ptr));
        }
        else if (d->format.sampleSize() == 16 && d->format.sampleType() == QAudioFormat::UnSignedInt) {
          if (d->format.byteOrder() == QAudioFormat::LittleEndian) {
            value = qFromLittleEndian<quint16>(ptr);
          }
          else {
            value = qFromBigEndian<quint16>(ptr);
          }
        }
        else if (d->format.sampleSize() == 16 && d->format.sampleType() == QAudioFormat::SignedInt) {
          if (d->format.byteOrder() == QAudioFormat::LittleEndian) {
            value = qAbs(qFromLittleEndian<qint16>(ptr));
          }
          else {
            value = qAbs(qFromBigEndian<qint16>(ptr));
          }
        }
        else if (d->format.sampleSize() == 32 && d->format.sampleType() == QAudioFormat::UnSignedInt) {
          if (d->format.byteOrder() == QAudioFormat::LittleEndian) {
            value = qFromLittleEndian<quint32>(ptr);
          }
          else {
            value = qFromBigEndian<quint32>(ptr);
          }
        }
        else if (d->format.sampleSize() == 32 && d->format.sampleType() == QAudioFormat::SignedInt) {
          if (d->format.byteOrder() == QAudioFormat::LittleEndian) {
            value = qAbs(qFromLittleEndian<qint32>(ptr));
          }
          else {
            value = qAbs(qFromBigEndian<qint32>(ptr));
          }
        }
        else if (d->format.sampleSize() == 32 && d->format.sampleType() == QAudioFormat::Float) {
          value = qAbs(*reinterpret_cast<const float*>(ptr) * 0x7fffffff); // assumes 0-1.0
        }
        maxValue = qMax(value, maxValue);
        ptr += channelBytes;
      }
    }
    maxValue = qMin(maxValue, d->maxAmplitude);
    d->level = qreal(maxValue) / d->maxAmplitude;
  }

  emit update();
  return len;
}
