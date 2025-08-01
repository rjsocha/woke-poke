QT += core widgets network
CONFIG  += release
TARGET = woke-poke
TEMPLATE = app
SOURCES += src/main.cpp
RESOURCES += src/resource.qrc
QMAKE_LFLAGS_RELEASE += -s
