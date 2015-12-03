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


#ifndef __WAVERENDERAREA_H_
#define __WAVERENDERAREA_H_

#include <QWidget>
#include <QByteArray>
#include <QAudioFormat>
#include <QResizeEvent>
#include <QMutex>
#include <QPixmap>

class WaveRenderAreaPrivate;

class WaveRenderArea : public QWidget
{
  Q_OBJECT
public:
  WaveRenderArea(QMutex *mutex, QWidget *parent = Q_NULLPTR);
  void setPattern(const QVector<int> &);
  void setData(const QVector<int> &);
  void setAudioFormat(const QAudioFormat &format);
  const QPixmap &pixmap(void) const;

protected:
  virtual QSize sizeHint(void) const;
  void paintEvent(QPaintEvent *);
  void resizeEvent(QResizeEvent *);

signals:
  void click(qint64 nsElapsed);

public slots:

private:
  QScopedPointer<WaveRenderAreaPrivate> d_ptr;
  Q_DECLARE_PRIVATE(WaveRenderArea)
  Q_DISABLE_COPY(WaveRenderArea)

  typedef qint16 SampleType;

private: // methods
  void drawPixmap(void);
  void correlate(void);
};

#endif // __WAVERENDERAREA_H_
