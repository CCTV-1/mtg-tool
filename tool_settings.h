#ifndef TOOL_SETTINGS_H
#define TOOL_SETTINGS_H

#include <QSettings>
#include <QString>
#include <QUrl>

class ToolSetting : public QSettings
{
    Q_OBJECT
public:
    ToolSetting();
    ~ToolSetting();

    Q_INVOKABLE int get_image_lanuage( void );
    Q_INVOKABLE void set_image_lanuage( int enum_value );

    Q_INVOKABLE int get_image_name_format( void );
    Q_INVOKABLE void set_image_name_format( int enum_value );

    Q_INVOKABLE int get_image_resolution( void );
    Q_INVOKABLE void set_image_resolution( int enum_value );

    Q_INVOKABLE QUrl get_cache_directory( void );
    Q_INVOKABLE void set_cache_directory( QUrl local_uri );

    Q_INVOKABLE int get_image_width( void );
    Q_INVOKABLE void set_image_width( int width );

    Q_INVOKABLE int get_image_height( void );
    Q_INVOKABLE void set_image_height( int height );

    Q_INVOKABLE int get_filter_role( void );
    Q_INVOKABLE void set_filter_role( int enum_value );
private:
    QUrl default_cache_url;
};

#endif // TOOL_SETTINGS_H
