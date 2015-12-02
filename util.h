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

#ifndef __UTIL_H_
#define __UTIL_H_


#include <cstring>
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVector>
#include <qmath.h>


template <class T>
void SafeRenew(T& a, T obj)
{
  if (a != Q_NULLPTR)
    delete a;
  a = obj;
}

template <class T>
void SafeRenewArray(T& a, T obj)
{
  if (a != Q_NULLPTR)
    delete [] a;
  a = obj;
}


template <class T>
void SafeDelete(T& a)
{
  SafeRenew<T>(a, Q_NULLPTR);
}

template <class T>
void SafeDeleteArray(T& a)
{
  SafeRenewArray<T>(a, Q_NULLPTR);
}

template <class T>
void SecureErase(T *p, size_t size)
{
  memset(p, 0, size);
}


template <class T>
void SecureErase(T &obj)
{
  for (typename T::iterator i = obj.begin(); i != obj.end(); ++i)
    *i = 0;
  obj.clear();
}


template<typename T>
T clamp(const T &x, const T &lo, const T &hi)
{
  return qMax(lo, qMin(hi, x));
}


#if defined(Q_CC_GNU)
extern void SecureErase(QString str);
#endif

#include <QtCore/qglobal.h>
#include <QDebug>

QT_FORWARD_DECLARE_CLASS(QAudioFormat)


qint64 audioDuration(const QAudioFormat &format, qint64 bytes);
qint64 audioLength(const QAudioFormat &format, qint64 microSeconds);

QString formatToString(const QAudioFormat &format);

qreal nyquistFrequency(const QAudioFormat &format);

// Scale PCM value to [-1.0, 1.0]
qreal pcmToReal(qint16 pcm);

// Scale real value in [-1.0, 1.0] to PCM
qint16 realToPcm(qreal real);

// Check whether the audio format is PCM
bool isPCM(const QAudioFormat &format);

// Check whether the audio format is signed, little-endian, 16-bit PCM
bool isPCMS16LE(const QAudioFormat &format);

// Compile-time calculation of powers of two

template<int N> class PowerOfTwo
{ public: static const int Result = PowerOfTwo<N-1>::Result * 2; };

template<> class PowerOfTwo<0>
{ public: static const int Result = 1; };


class NullDebug
{
public:
    template <typename T>
    NullDebug& operator<<(const T&) { return *this; }
};

inline NullDebug nullDebug() { return NullDebug(); }

#ifdef LOG_ENGINE
#   define ENGINE_DEBUG qDebug()
#else
#   define ENGINE_DEBUG nullDebug()
#endif

#ifdef LOG_SPECTRUMANALYSER
#   define SPECTRUMANALYSER_DEBUG qDebug()
#else
#   define SPECTRUMANALYSER_DEBUG nullDebug()
#endif

#ifdef LOG_WAVEFORM
#   define WAVEFORM_DEBUG qDebug()
#else
#   define WAVEFORM_DEBUG nullDebug()
#endif


#endif // __UTIL_H_
