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

#include "healthcheck.h"

#include <QDebug>
#include <QVector>
#include <QtMath>

const int PopCount[256] = {
   0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7
};


bool testMonobit(const QByteArray &ran, int &notPassedCount, int &testCount)
{
  static const int BitCount = 20000;
  int passedCount = 0;
  testCount = 0;
  if (ran.size() >= BitCount / 8) {
    const int stepLen = BitCount / 8;
    for (int i = 0; i < ran.size() - stepLen + 1; i += stepLen) {
      int bitCount = 0;
      for (int j = 0; j < stepLen; ++j) {
        quint8 r = quint8(ran.at(i + j));
        bitCount += PopCount[r];
      }
      if ((9654 < bitCount) && (bitCount < 10346)) {
        ++passedCount;
      }
      ++testCount;
    }
  }
  notPassedCount = testCount - passedCount;
  return notPassedCount == 0;
}


qreal testEntropy(const QByteArray &ran)
{
  qreal ent = 0.0;
  static const int Range = 256;
  const int N = ran.size();
  if (N > Range) {
    QVector<int> histo(Range, static_cast<int>(0));
    for (int i = 0; i < ran.size(); ++i) {
      ++histo[quint8(ran.at(i))];
    }
    for (int i = 0; i < Range; ++i) {
      qreal p = qreal(histo[i]) / N;
      if (p > 0.0) {
        ent += p * M_LOG2E * qLn(1.0 / p);
      }
    }
    for (int i = 0; i < Range; ++i) {
       qDebug().nospace().noquote() << QString("%1").arg(i, 2, 16, QChar('0')) << ": " << QString("%1").arg(histo[i], 3);
    }
  }
  return ent;
}
