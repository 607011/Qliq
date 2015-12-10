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


#include "volumerenderarea.h"

#include <QDebug>
#include <QPainter>


class VolumeRenderAreaPrivate
{
public:
  VolumeRenderAreaPrivate(void)
    : level(0.0)
  { /* ... */ }
  ~VolumeRenderAreaPrivate() { /* ... */ }
  qreal level;
};


VolumeRenderArea::VolumeRenderArea(QWidget *parent)
  : QWidget(parent)
  , d_ptr(new VolumeRenderAreaPrivate)
{
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
  setMinimumHeight(14);
}


VolumeRenderArea::~VolumeRenderArea()
{
  /* ... */
}


void VolumeRenderArea::paintEvent(QPaintEvent *)
{
  Q_D(VolumeRenderArea);
  QPainter p(this);
  static const QColor BorderColor(0x33, 0x33, 0x33);
  static const QColor FillColor(0xff, 0x66, 0x00);
  p.setPen(BorderColor);
  p.drawRect(QRect(p.viewport().left(), p.viewport().top(), p.viewport().right(), p.viewport().bottom()));
  if (d->level > 0.0) {
    const int pos = ((p.viewport().right()) - (p.viewport().left()) - 2) * d->level;
    p.fillRect(p.viewport().left() + 1,
               p.viewport().top() + 1,
               pos,
               p.viewport().height() - 2,
               FillColor);
  }
}


void VolumeRenderArea::setLevel(qreal value)
{
  Q_D(VolumeRenderArea);
  d->level = value;
  update();
}

