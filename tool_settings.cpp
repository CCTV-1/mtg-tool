#include "tool_settings.h"
#include "enums.h"
#include "sets_model.h"

#include <QFileInfo>

ToolSetting::ToolSetting():
    QSettings()
{
    QFileInfo download_dir( "./images/" );
    QString default_path = download_dir.absoluteFilePath();
    QUrl default_url = QUrl( default_path );
    default_url.setScheme( "file" );
    this->default_cache_url = default_url;
}

ToolSetting::~ToolSetting()
{
    this->sync();
}

int ToolSetting::get_image_lanuage( void )
{
    return this->value( "download_preferences/image_language" , int( LanguageEnums::EnumContent::en ) ).toInt();
}

void ToolSetting::set_image_lanuage( int enum_value )
{
    this->setValue( "download_preferences/image_language" , enum_value );
}

int ToolSetting::get_image_name_format( void )
{
    return this->value( "download_preferences/name_format" , int( ImageNameFormatEnums::EnumContent::FORGE ) ).toInt();
}

void ToolSetting::set_image_name_format( int enum_value )
{
    this->setValue( "download_preferences/name_format" , enum_value );
}

int ToolSetting::get_image_resolution( void )
{
    return this->value( "download_preferences/image_resolution" , int( ImageStylesEnums::EnumContent::large ) ).toInt();
}

void ToolSetting::set_image_resolution( int enum_value )
{
    this->setValue( "download_preferences/image_resolution" , enum_value );
}

QUrl ToolSetting::get_cache_directory( void )
{
    return this->value( "download_preferences/download_directory" , this->default_cache_url ).toUrl();
}

void ToolSetting::set_cache_directory( QUrl local_uri )
{
    this->setValue( "download_preferences/download_directory" , local_uri );
}

int ToolSetting::get_image_width( void )
{
    return this->value( "preview_preferences/image_width" , 105 ).toInt();
}

void ToolSetting::set_image_width( int width )
{
    this->setValue( "preview_preferences/image_width" , width );
}

int ToolSetting::get_image_height( void )
{
    return this->value( "preview_preferences/image_height" , 150 ).toInt();
}

void ToolSetting::set_image_height( int height )
{
    this->setValue( "preview_preferences/image_height" , height );
}

int ToolSetting::get_filter_role( void )
{
    return this->value( "filter_preferences/sets_role" , int( SetsModel::SetsRoles::SetName ) ).toInt();
}

void ToolSetting::set_filter_role( int enum_value )
{
    this->setValue( "filter_preferences/sets_role" , enum_value );
}
