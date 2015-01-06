#ifndef VERSION_H
#define VERSION_H

#include <QString>

#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#define VERSION_PATCH 0

#define VERSION_STRING QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH)

#endif // VERSION_H
