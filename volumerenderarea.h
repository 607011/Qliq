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


#ifndef __VOLUMERENDERAREA_H_
#define __VOLUMERENDERAREA_H_

#include <QWidget>
#include <QScopedPointer>


class VolumeRenderAreaPrivate;

class VolumeRenderArea : public QWidget
{
  Q_OBJECT
public:
  explicit VolumeRenderArea(QWidget *parent = Q_NULLPTR);
  ~VolumeRenderArea();
  void setLevel(qreal value);

protected:
  void paintEvent(QPaintEvent *);

signals:

public slots:

private:
  QScopedPointer<VolumeRenderAreaPrivate> d_ptr;
  Q_DECLARE_PRIVATE(VolumeRenderArea)
  Q_DISABLE_COPY(VolumeRenderArea)

};

#endif // __VOLUMERENDERAREA_H_
