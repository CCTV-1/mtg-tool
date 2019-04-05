#include <stdlib.h>

#include <glib.h>
#include <jansson.h>
#include "previewconfig.h"
#define CONFIG_FILE "config.json"

struct ConfigObject config_object;

static void get_string_node( json_t * root , const gchar * nodename , gchar ** variable , const gchar * default_value )
{
    if ( root == NULL )
        return ;
    if ( nodename == NULL )
        return ;
    if ( variable == NULL )
        return ;
    if ( default_value == NULL )
        return ;

    json_t * node = json_object_get( root , nodename );
    if( !json_is_string( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a string node" , nodename );
        *variable = g_strdup_printf( "%s" , default_value );
        return ;
    }
    *variable = g_strdup_printf( "%s" , json_string_value( node ) );
}

static void get_integer_node( json_t * root , const gchar * nodename , int * variable , int default_value )
{
    if ( root == NULL )
        return ;
    if ( nodename == NULL )
        return ;
    if ( variable == NULL )
        return ;

    json_t * node = json_object_get( root , nodename );
    if ( !json_is_integer( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a integer node" , nodename );
        *variable = default_value;
        return ;
    }
    *variable = json_integer_value( node );
}

static void get_boolean_node( json_t * root , const gchar * nodename , gboolean * variable , gboolean default_value )
{
    if ( root == NULL )
        return ;
    if ( nodename == NULL )
        return ;
    if ( variable == NULL )
        return ;

    json_t * node = json_object_get( root , nodename );
    if ( !json_is_boolean( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a boolean node" , nodename );
        *variable = default_value;
        return ;
    }
    *variable = json_boolean_value( node );
}

void config_inital( void )
{
    json_t * root;
    json_error_t error;

    root = json_load_file( CONFIG_FILE , 0 , &error );

    if( root == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: on line %d : %s" , error.line , error.text );
        exit( EXIT_FAILURE );
    }

    get_integer_node( root , "ImageNameFormat" , ( int * )( &config_object.image_name_format ) , FORGE_NAME_FORMAT );
    get_integer_node( root , "WindowWidth" , &config_object.window_width , 1050 );
    get_integer_node( root , "WindowHeight" , &config_object.window_height , 600 );
    get_integer_node( root , "CardWidth" , &config_object.card_width , 70 );
    get_integer_node( root , "CardHeight" , &config_object.card_height , 100 );
    get_integer_node( root , "LineCardNumber" , &config_object.line_card_number , 15 );
    get_integer_node( root , "TitleFontSize" , &config_object.title_font_size , 20 );

    gchar * default_path = g_get_current_dir();
    get_string_node( root , "ImageRootDirectory" , &config_object.image_root_directory , default_path );
    get_string_node( root , "ImageSuffix" , &config_object.image_suffix , ".jpg" );
    get_string_node( root , "DeckFileDirectory" , &config_object.deck_file_directory , "./" );
    get_string_node( root , "TargetRootDirectory" , &config_object.target_root_directory , "./" );
    g_free( default_path );

    get_boolean_node( root , "HideTitleBar" , &config_object.hide_title_bar , FALSE );
    get_boolean_node( root , "CopyFile" , &config_object.copy_file , FALSE );
    get_boolean_node( root , "UseNetworkImage" , &config_object.use_network_image , FALSE );

    json_decref( root );
}

void config_destroy( void )
{
    g_free( config_object.image_root_directory );
    g_free( config_object.image_suffix );
    g_free( config_object.deck_file_directory );
    g_free( config_object.target_root_directory );
}