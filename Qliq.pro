# Copyright (c) 2015 Oliver Lau <ola@ct.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

TARGET = Qliq
TEMPLATE = app
QT       += core gui multimedia widgets

include(Qliq.pri)
VERSION = -$${QLIQ_VERSION}
DEFINES += QLIQ_VERSION=\\\"$${QLIQ_VERSION}\\\"

SOURCES += main.cpp\
    mainwindow.cpp \
    global.cpp \
    volumerenderarea.cpp \
    waverenderarea.cpp \
    audioinputdevice.cpp \
    util.cpp

HEADERS  += mainwindow.h \
    global.h \
    volumerenderarea.h \
    waverenderarea.h \
    audioinputdevice.h \
    util.h

FORMS += mainwindow.ui

DISTFILES += \
    README.md
