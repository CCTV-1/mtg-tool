#ifndef UTILS_H
#define UTILS_H

#include <QString>

#include "enums.h"
#include "tool_settings.h"

inline QString setcode2legalname( const QString& code )
{
    #ifdef Q_OS_WINDOWS
    if ( code.toUpper() == QString("CON") )
    {
        ToolSetting settings;
        int format = settings.get_image_name_format();
        if ( format == int( ImageNameFormatEnums::EnumContent::FORGE ) )
        {
            return QString("CFX");
        }
        else if ( format == int( ImageNameFormatEnums::EnumContent::XMAGE ) )
        {
            return QString("COX");
        }
    }
    #endif
    return code.toUpper();
}

#endif // UTILS_H
