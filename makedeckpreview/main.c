#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <curl/curl.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <jansson.h>

#define CONFIG_FILE "config.json"
#define LOGO_FILE "logo.ico"
#define BUFFSIZE 1024

enum DeckType
{
    FORGE_DECK_FORMAT = 0,
    XMAGE_DECK_FORMAT,
    MTGA_DECK_FORMAT,
    GOLDFISH_DECK_FORMAT,
    UNKNOWN_DECK_FORMAT,
};

enum CardLocal
{
    COMMAND_LOCAL = 0,
    MAIN_LOCAL,
    SIDEBOARD_LOCAL,
};

enum ImageNameFormat
{
    FORGE_NAME_FORMAT = 0,
    XMAGE_NAME_FORMAT,
    UNKNOWN_NAME_FORMAT
};

struct ConfigObject
{
    enum ImageNameFormat image_name_format;
    gchar * target_root_directory;
    gchar * image_root_directory;
    gchar * image_suffix;
    gchar * deck_file_directory;
    json_int_t window_width;
    json_int_t window_height;
    json_int_t card_width;
    json_int_t card_height;
    json_int_t line_card_number;
    json_int_t title_font_size;
    gint hide_title_bar;
    gint copy_file;
    gint use_network_image;
}config_object;

struct DeckObject
{
    gchar * deckfullname;
    gchar * targetdirectory;
};

struct ImagesLayout
{
    GtkWidget * layout;
    GArray * images;
    gint32 width;
    gint32 height;
    gboolean select_target;
    gint32 dx;
    gint32 dy;
    gint32 select_x;
    gint32 select_y;
};

struct PreviewObject
{
    GtkWidget * window;
    GtkWidget * layout;
    gint32 layout_height;
    struct ImagesLayout command_layout;
    struct ImagesLayout main_layout;
    struct ImagesLayout sideboard_layout;
}preview_object;

struct CardObject
{
    enum CardLocal card_local;
    gchar * cardname;
    gchar * cardseries;
    gint32 cardid;
    gint32 cardnumber;
};

void config_inital( void );
void config_destroy( void );
gint32 process_deck( struct DeckObject * );

//do main
static GSList * get_deckfile_list( const gchar * deck_path );
static void delete_deck_list( GSList * deck_list );
static gboolean remove_directory( const gchar * dir );
static gboolean make_directory( const gchar * dir );

//do config_inital
static gboolean get_integer_node( json_t * root , const gchar * nodename , json_int_t * variable );
static gchar * get_string_node( json_t * root, const gchar * nodename );
static gboolean get_boolean_node( json_t * root , const gchar * nodename , gboolean * variable );

//do process_deck
static void preview_init( struct DeckObject * deck );
static void preview_add_title( enum CardLocal card_local );
static void preview_add_card( struct CardObject * card );
static void preview_display( void );
static void preview_destroy( void );

static gboolean copy_file( const gchar * source_uri , const gchar * destination_uri );
static enum DeckType get_deck_type( const gchar * deck_filename );

static GSList * get_cardlist( const gchar * deckfilename );
static GSList * get_cardlist_forge( const gchar * deckfilename );
static GSList * get_cardlist_xmage( const gchar * deckfilename );
static GSList * get_cardlist_mtga( const gchar * deckfilename );
static GSList * get_cardlist_goldfish( const gchar * deckfilename );
static void delete_cardlist( GSList * cardlist );
static struct CardObject * allocate_cardobject( void );
static void free_cardobject( struct CardObject * card );

static char * stem_name( const char * filename );
static gchar * strreplace( const gchar * source_str , const gchar * from_str , const gchar * to_str );
static gchar * cardname_to_imagename( const gchar * cardname , enum DeckType deck_type );
static gchar * make_imagefile_uri( const gchar * cardname , const gchar * cardseries );
static gchar * make_targetfile_uri( const char * targetdirectory , const gchar * cardname , const gchar * cardseries , gsize retry_count );

static gboolean get_deckpreview( GtkWidget * window , GdkEvent * event , gpointer data );
static void download_file( gpointer data , gpointer user_data );
static void get_clipboard_content( GtkClipboard * clipboard , const gchar * text , gpointer user_data );
static gboolean motion_notify_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data );
static gboolean button_release_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data );
static gboolean button_press_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data );

int main ( int argc , char * argv[] )
{
    gtk_init( &argc, &argv );
    curl_global_init( CURL_GLOBAL_ALL );
    config_inital();

    GSList * decklist = get_deckfile_list( config_object.deck_file_directory );
    if ( decklist == NULL )
        return EXIT_FAILURE;

    GSList * temp_ptr = decklist;
    while ( temp_ptr != NULL )
    {
        gchar * deckfile_fullname = ( gchar * )temp_ptr->data;
        //****.extname --> ****"
        gchar * deckfile_shortname = stem_name( deckfile_fullname );
        gchar * targetdirectory = g_strdup_printf( "%s%s/" , config_object.target_root_directory , deckfile_shortname );
        struct DeckObject deck = { deckfile_fullname , targetdirectory };
        if ( remove_directory( deck.targetdirectory ) == FALSE )
            continue;
        if ( make_directory( deck.targetdirectory ) == FALSE )
            continue;
        process_deck( &deck );

        g_free( deckfile_shortname );
        g_free( targetdirectory );
        temp_ptr = g_slist_next( temp_ptr );
    }

    delete_deck_list( decklist );
    curl_global_cleanup();
    config_destroy();
    return EXIT_SUCCESS;
}

void config_inital( void )
{
    json_t * root;
    json_error_t error;

    root = json_load_file( CONFIG_FILE , 0 , &error );

    if( root == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: on line %d : %s", error.line , error.text );
        exit( EXIT_FAILURE );
    }

    gboolean status = FALSE;

    status = get_integer_node( root , "ImageNameFormat" , ( json_int_t * )( &config_object.image_name_format ) );
    if ( status == FALSE )
    {
        config_object.image_name_format = FORGE_NAME_FORMAT;
    }

    status = get_integer_node( root , "WindowWidth" , &config_object.window_width );
    if ( status == FALSE )
    {
        config_object.window_width = 1050;
    }

    status = get_integer_node( root , "WindowHeight" , &config_object.window_height );
    if ( status == FALSE )
    {
        config_object.window_height = 600;
    }

    status = get_integer_node( root , "CardWidth" , &config_object.card_width );
    if ( status == FALSE )
    {
        config_object.card_width = 70;
    }

    status = get_integer_node( root , "CardHeight" , &config_object.card_height );
    if ( status == FALSE )
    {
        config_object.card_height = 100;
    }

    status = get_integer_node( root , "LineCardNumber" , &config_object.line_card_number );
    if ( status == FALSE )
    {
        config_object.line_card_number = 15;
    }

    status = get_integer_node( root , "TitleFontSize" , &config_object.title_font_size );
    if ( status == FALSE )
    {
        config_object.title_font_size = 20;
    }

    config_object.image_root_directory = get_string_node( root, "ImageRootDirectory" );
    if ( config_object.image_root_directory == NULL )
    {
        config_object.image_root_directory = g_get_current_dir();
    }

    config_object.image_suffix = get_string_node( root, "ImageSuffix" );
    if ( config_object.image_suffix == NULL )
    {
        config_object.image_suffix = g_strdup_printf( "%s" , ".jpg" );
    }

    config_object.deck_file_directory = get_string_node( root, "DeckFileDirectory" );
    if ( config_object.deck_file_directory == NULL )
    {
        config_object.deck_file_directory = g_strdup_printf( "%s" , "./" );
    }

    config_object.target_root_directory = get_string_node( root , "TargetRootDirectory" );
    if ( config_object.target_root_directory == NULL )
    {
        config_object.target_root_directory = g_strdup_printf( "%s" , "./" );
    }
 
    status = get_boolean_node( root , "HideTitleBar" , &config_object.hide_title_bar );
    if ( status == FALSE )
    {
        config_object.hide_title_bar = FALSE;
    }
    
    status = get_boolean_node( root , "CopyFile" , &config_object.copy_file );
    if ( status == FALSE )
    {
        config_object.copy_file = FALSE;
    }

    status = get_boolean_node( root , "UseNetworkImage" , &config_object.use_network_image );
    if ( status == FALSE )
    {
        config_object.use_network_image = FALSE;
    }

    json_decref( root );
}

void config_destroy( void )
{
    g_free( config_object.image_root_directory );
    g_free( config_object.image_suffix );
    g_free( config_object.deck_file_directory );
    g_free( config_object.target_root_directory );
}

gint32 process_deck( struct DeckObject * deck )
{
    gint32 copy_success_count = 0;
    preview_init( deck );

    //NULL is empty GSList
    GSList * cardlist = get_cardlist( deck->deckfullname );
    if ( cardlist == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "deck \"%s\" format unknown,parsing stop" , deck->deckfullname );
        return 0;
    }

    GSList * temp = cardlist;
    //Download Image
    if ( config_object.use_network_image == TRUE )
    {
        GThreadPool * thr_pool = g_thread_pool_new( download_file , NULL , 8 , TRUE , NULL );
        while( temp != NULL )
        {
            struct CardObject * card = ( struct CardObject * )temp->data;
            g_thread_pool_push( thr_pool , card , NULL );
            temp = g_slist_next( temp );
        }
        g_thread_pool_free( thr_pool , FALSE , TRUE );
    }

    temp = cardlist;
    while( temp != NULL )
    {
        struct CardObject * card = ( struct CardObject * )temp->data;
        preview_add_card( card );
        if ( config_object.copy_file == TRUE )
        {
            gchar * imagefile_uri = make_imagefile_uri( card->cardname , card->cardseries );;
            gchar * targetfile_uri = make_targetfile_uri( deck->targetdirectory , card->cardname , card->cardseries , card->cardnumber );
            gsize rename_count = 1;
            while ( g_file_test( targetfile_uri , G_FILE_TEST_EXISTS ) == TRUE )
            {
                g_free( targetfile_uri );
                targetfile_uri = make_targetfile_uri( deck->targetdirectory , card->cardname , card->cardseries , card->cardnumber + rename_count*100 );
                rename_count++;
            }
            if ( copy_file( imagefile_uri , targetfile_uri ) == TRUE )
                copy_success_count++;
            g_free( imagefile_uri );
            g_free( targetfile_uri );
        }
        temp = g_slist_next( temp );
    }

    preview_display();
    preview_destroy();
    delete_cardlist( cardlist );

    return copy_success_count;
}

static GSList * get_deckfile_list( const gchar * deck_path )
{
    GDir * dir_ptr = g_dir_open( deck_path , 0 , NULL );
    GSList * deck_list = NULL;

    GtkClipboard * clipboard = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    gtk_clipboard_request_text( clipboard , get_clipboard_content , &deck_list );
    //do gtk loop get clipboard content;
    gtk_main();

    if ( dir_ptr == NULL )
        return deck_list;

    //exists and type check
    if ( g_file_test( deck_path , G_FILE_TEST_EXISTS ) != TRUE )
    {
        g_dir_close( dir_ptr );
        return deck_list;
    }
    if ( g_file_test( deck_path , G_FILE_TEST_IS_DIR ) != TRUE )
    {
        g_dir_close( dir_ptr );
        return deck_list;
    }

    //g_dir_read_name auto ignore "." ".."
    const gchar * filename;
    while ( ( filename = g_dir_read_name( dir_ptr ) ) != NULL )
    {
        gchar * deck_file_url = g_strdup_printf( "%s/%s" , deck_path , filename );
        deck_list = g_slist_append( deck_list , ( gpointer )deck_file_url );
    }

    g_dir_close( dir_ptr );
    return deck_list;
}

static void delete_deck_list( GSList * deck_list )
{
    g_slist_free_full( deck_list , ( GDestroyNotify )g_free );
}

static gboolean remove_directory( const gchar * dir )
{
    GDir * dir_ptr;

    if ( g_file_test( dir , G_FILE_TEST_EXISTS ) != TRUE )
        return TRUE;
    //if is a regular file direct remove
    if ( g_file_test( dir , G_FILE_TEST_IS_DIR ) != TRUE )
        return g_remove( dir );

    dir_ptr = g_dir_open( dir , 0 , NULL );

    if ( dir_ptr == NULL )
        return FALSE;

    //g_dir_read_name auto ignore "." ".."
    const gchar * filename;
    while ( ( filename = g_dir_read_name( dir_ptr ) ) != NULL )
    {
        gchar * dir_name = g_strdup_printf ( "%s/%s" , dir , filename );
        remove_directory( dir_name );
        g_free( dir_name );
    }
    g_dir_close( dir_ptr );
    g_rmdir( dir );
    return TRUE;
}

static gboolean make_directory( const gchar * dir )
{
    if ( dir == NULL )
        return FALSE;

    GError * g_error;
    GFile * targetdirectory = g_file_new_for_path( dir );
    if ( g_file_make_directory_with_parents( targetdirectory , NULL , &g_error ) == FALSE )
    {
        g_object_unref( targetdirectory );
        if ( g_error_matches( g_error , G_IO_ERROR_EXISTS, G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "directory %s already exists" , dir );
            return FALSE;
        }
        else if ( g_error_matches( g_error , G_IO_ERROR_NOT_SUPPORTED , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "file system does not support creating directories" );
            return FALSE;
        }
        else
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "create directory %s unknown error" , dir );
            return FALSE;
        }
    }
    g_object_unref( targetdirectory );
    return TRUE;
}

static gchar * get_string_node( json_t * root , const gchar * nodename )
{
    json_t * node = json_object_get( root, nodename );
    if( !json_is_string( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a string node", nodename );
        return NULL;
    }
    const gchar * onlyread_string = json_string_value( node );
    gchar * return_string = g_strdup_printf( "%s" , onlyread_string );
    return return_string;
}

static gboolean get_integer_node( json_t * root , const gchar * nodename , json_int_t * variable )
{
    json_t * node = json_object_get( root , nodename );
    if ( !json_is_integer( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a integer node" , nodename );
        return FALSE;
    }
    *variable = json_integer_value( node );
    
    return TRUE;
}

static gboolean get_boolean_node( json_t * root , const gchar * nodename , gboolean * variable )
{
    json_t * node = json_object_get( root , nodename );
    if ( !json_is_boolean( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a boolean node" , nodename );
        //identify get value error
        return FALSE;
    }

    *variable = json_boolean_value( node );
    return TRUE;
}

static gboolean copy_file( const gchar * source_uri , const gchar * destination_uri )
{
    if ( source_uri == NULL )
        return FALSE;
    if ( destination_uri == NULL )
        return FALSE;
    GFile * source = g_file_new_for_uri( source_uri );
    GFile * destination = g_file_new_for_uri( destination_uri );
    GError * g_error;
    if ( g_file_copy( source , destination , G_FILE_COPY_OVERWRITE , NULL , NULL , NULL , &g_error ) == FALSE )
    {
        g_object_unref( source );
        g_object_unref( destination );
        if ( g_error_matches( g_error , G_IO_ERROR_NOT_FOUND , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s not exist" , source_uri );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_EXISTS , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s exist" , destination_uri );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_IS_DIRECTORY , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s is directory" , destination_uri );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_WOULD_MERGE , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s and %s" , source_uri , destination_uri );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_WOULD_RECURSE , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s is directory or " , source_uri );
            return FALSE;
        }
    }
    g_object_unref( source );
    g_object_unref( destination );
    return TRUE;
}

static void preview_init( struct DeckObject * deck )
{
    preview_object.window = gtk_window_new( GTK_WINDOW_TOPLEVEL );

    GdkPixbuf * icon_pixbuf = gdk_pixbuf_new_from_file( LOGO_FILE , NULL );
    if ( icon_pixbuf == NULL )
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s not is image file or not found" , LOGO_FILE );
    else
        gtk_window_set_icon( GTK_WINDOW( preview_object.window ) , icon_pixbuf );
    g_object_unref( icon_pixbuf );

    if ( config_object.hide_title_bar )
    {
        gtk_window_set_decorated( GTK_WINDOW( preview_object.window ) , FALSE );
    }
    else
    {
        gtk_window_set_hide_titlebar_when_maximized( GTK_WINDOW( preview_object.window ) , TRUE );
        gtk_window_set_title( GTK_WINDOW( preview_object.window ) , deck->deckfullname );
    }

    gtk_window_set_default_size( GTK_WINDOW( preview_object.window ) , ( gint )config_object.window_width , ( gint )config_object.window_height );
    g_signal_connect( G_OBJECT( preview_object.window ) , "delete-event", G_CALLBACK( get_deckpreview ) , deck->targetdirectory );

    GtkWidget * scrolled = gtk_scrolled_window_new( NULL , NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ) , GTK_POLICY_AUTOMATIC , GTK_POLICY_AUTOMATIC );

    preview_object.command_layout.layout = gtk_layout_new( NULL , NULL );
    preview_object.command_layout.images = g_array_new( FALSE , FALSE , sizeof( GtkWidget * ) );
    preview_object.command_layout.select_target = FALSE;
    preview_object.command_layout.width = 0;
    preview_object.command_layout.height = 0;

    preview_object.main_layout.layout = gtk_layout_new( NULL , NULL );
    preview_object.main_layout.images = g_array_new( FALSE , FALSE , sizeof( GtkWidget * ) );
    preview_object.main_layout.select_target = FALSE;
    preview_object.main_layout.width = 0;
    preview_object.main_layout.height = 0;

    preview_object.sideboard_layout.layout = gtk_layout_new( NULL , NULL );
    preview_object.sideboard_layout.images = g_array_new( FALSE , FALSE , sizeof( GtkWidget * ) );
    preview_object.sideboard_layout.select_target = FALSE;
    preview_object.sideboard_layout.width = 0;
    preview_object.sideboard_layout.height = 0;

    preview_object.layout = gtk_layout_new( NULL , NULL );
    preview_object.layout_height = 0;
    gtk_container_add( GTK_CONTAINER( scrolled ) , preview_object.layout );
    gtk_container_add( GTK_CONTAINER( preview_object.window ) , scrolled );
}

static void preview_add_title( enum CardLocal card_local )
{
    gchar * title_label_buff = NULL;
    GtkWidget * title_label = gtk_label_new( NULL );
    if ( card_local == MAIN_LOCAL )
        title_label_buff = g_strdup_printf(  "<span font='%"G_GINT64_FORMAT"'>main</span>" , config_object.title_font_size );
    else if ( card_local == SIDEBOARD_LOCAL )
        title_label_buff = g_strdup_printf( "<span font='%"G_GINT64_FORMAT"'>sideboard</span>" , config_object.title_font_size );
    else if ( card_local == COMMAND_LOCAL )
        return ;
    gtk_label_set_markup( GTK_LABEL( title_label ) , title_label_buff );

    GtkWidget * title_box = gtk_event_box_new();
    gtk_widget_set_size_request( GTK_WIDGET( title_box ) , config_object.window_width , config_object.title_font_size );
    gtk_container_add( GTK_CONTAINER( title_box ) , title_label );

    gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , title_box , 0 , preview_object.layout_height );
    preview_object.layout_height += 2*config_object.title_font_size;
    g_free( title_label_buff );
}

static void preview_add_card( struct CardObject * card )
{
    gchar * imagefile_uri = make_imagefile_uri( card->cardname , card->cardseries );
    gchar * imagefile_path = g_filename_from_uri( imagefile_uri , NULL , NULL );
    g_free( imagefile_uri );
    gint32 cardnumber = card->cardnumber;
    while ( cardnumber-- )
    {
        //gtk_image_new_from_file never returns NULL
        GtkWidget * image = gtk_image_new_from_file( imagefile_path );
        GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
        if ( pixbuf == NULL )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s not is image file or not found" , imagefile_path );
            gtk_widget_destroy( image );
            return ;
        }
        pixbuf = gdk_pixbuf_scale_simple( GDK_PIXBUF( pixbuf ) , config_object.card_width , config_object.card_height , GDK_INTERP_BILINEAR );
        gtk_image_set_from_pixbuf( GTK_IMAGE( image ) , pixbuf );
        g_object_unref( pixbuf );

        switch ( card->card_local )
        {
            case COMMAND_LOCAL:
            {
                gtk_layout_put( GTK_LAYOUT( preview_object.command_layout.layout ) , image ,
                    preview_object.command_layout.width , preview_object.command_layout.height );
                g_array_append_val( preview_object.command_layout.images , image );
                preview_object.command_layout.width += config_object.card_width;
                if ( preview_object.command_layout.width == ( config_object.line_card_number )*( config_object.card_width ) )
                {
                    preview_object.command_layout.width = 0;
                    preview_object.command_layout.height += config_object.card_height;
                }
                break;
            }
            case MAIN_LOCAL:
            {
                gtk_layout_put( GTK_LAYOUT( preview_object.main_layout.layout ) , image ,
                    preview_object.main_layout.width , preview_object.main_layout.height );
                g_array_append_val( preview_object.main_layout.images , image );
                preview_object.main_layout.width += config_object.card_width;
                if ( preview_object.main_layout.width == ( config_object.line_card_number )*( config_object.card_width ) )
                {
                    preview_object.main_layout.width = 0;
                    preview_object.main_layout.height += config_object.card_height;
                }
                break;
            }
            case SIDEBOARD_LOCAL:
            {
                gtk_layout_put( GTK_LAYOUT( preview_object.sideboard_layout.layout ) , image ,
                    preview_object.sideboard_layout.width , preview_object.sideboard_layout.height );
                g_array_append_val( preview_object.sideboard_layout.images , image );
                preview_object.sideboard_layout.width += config_object.card_width;
                if ( preview_object.sideboard_layout.width == ( config_object.line_card_number )*( config_object.card_width ) )
                {
                    preview_object.sideboard_layout.width = 0;
                    preview_object.sideboard_layout.height += config_object.card_height;
                }
                break;
            }
            default:
                break;
        }

    }
    g_free( imagefile_path );
}

static void preview_display( void )
{
    if ( preview_object.command_layout.images->len != 0 )
    {
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.command_layout.layout , 0 , preview_object.layout_height );
        if ( ( preview_object.command_layout.images->len ) % config_object.line_card_number != 0 )
            preview_object.command_layout.height += config_object.card_height;
        preview_object.layout_height += preview_object.command_layout.height;
    }

    if ( preview_object.main_layout.images->len != 0 )
    {
        preview_add_title( MAIN_LOCAL );
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.main_layout.layout , 0 , preview_object.layout_height );
        if ( ( preview_object.main_layout.images->len ) % config_object.line_card_number != 0 )
            preview_object.main_layout.height += config_object.card_height;
        preview_object.layout_height += preview_object.main_layout.height;
    }

    if ( preview_object.sideboard_layout.images->len != 0 )
    {
        preview_add_title( SIDEBOARD_LOCAL );
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.sideboard_layout.layout , 0 , preview_object.layout_height );
        if ( ( preview_object.sideboard_layout.images->len ) % config_object.line_card_number != 0 )
            preview_object.sideboard_layout.height += config_object.card_height;
        preview_object.layout_height += preview_object.sideboard_layout.height;
    }

    g_signal_connect( G_OBJECT( preview_object.command_layout.layout ) , "motion-notify-event" , G_CALLBACK( motion_notify_handle ) , &preview_object.command_layout );
    g_signal_connect( G_OBJECT( preview_object.command_layout.layout ) , "button-press-event" , G_CALLBACK( button_press_handle ) , &preview_object.command_layout );
    g_signal_connect( G_OBJECT( preview_object.command_layout.layout ) , "button-release-event" , G_CALLBACK( button_release_handle ) , &preview_object.command_layout );
    g_signal_connect( G_OBJECT( preview_object.main_layout.layout ) , "motion-notify-event" , G_CALLBACK( motion_notify_handle ) , &preview_object.main_layout );
    g_signal_connect( G_OBJECT( preview_object.main_layout.layout ) , "button-press-event" , G_CALLBACK( button_press_handle ) , &preview_object.main_layout );
    g_signal_connect( G_OBJECT( preview_object.main_layout.layout ) , "button-release-event" , G_CALLBACK( button_release_handle ) , &preview_object.main_layout );
    g_signal_connect( G_OBJECT( preview_object.sideboard_layout.layout ) , "motion-notify-event" , G_CALLBACK( motion_notify_handle ) , &preview_object.sideboard_layout );
    g_signal_connect( G_OBJECT( preview_object.sideboard_layout.layout ) , "button-press-event" , G_CALLBACK( button_press_handle ) , &preview_object.sideboard_layout );
    g_signal_connect( G_OBJECT( preview_object.sideboard_layout.layout ) , "button-release-event" , G_CALLBACK( button_release_handle ) , &preview_object.sideboard_layout );

    gtk_widget_set_events( preview_object.command_layout.layout , gtk_widget_get_events( preview_object.layout )
        | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
        | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK );
    gtk_widget_set_events( preview_object.main_layout.layout , gtk_widget_get_events( preview_object.layout )
        | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
        | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK );
    gtk_widget_set_events( preview_object.sideboard_layout.layout , gtk_widget_get_events( preview_object.layout )
        | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
        | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK );

    gtk_widget_set_size_request( GTK_WIDGET( preview_object.command_layout.layout ) ,
        config_object.line_card_number*config_object.card_width , preview_object.command_layout.height );
    gtk_widget_set_size_request( GTK_WIDGET( preview_object.main_layout.layout ) ,
        config_object.line_card_number*config_object.card_width , preview_object.main_layout.height );
    gtk_widget_set_size_request( GTK_WIDGET( preview_object.sideboard_layout.layout ) ,
        config_object.line_card_number*config_object.card_width , preview_object.sideboard_layout.height );
    gtk_layout_set_size( GTK_LAYOUT( preview_object.layout ) , config_object.line_card_number*config_object.card_width , preview_object.layout_height );
    gtk_widget_show_all( GTK_WIDGET( preview_object.window ) );
    gtk_main();
}

static void preview_destroy( void )
{
    gtk_widget_destroy( GTK_WIDGET( preview_object.window ) );
}

static enum DeckType get_deck_type( const gchar * deck_filename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deck_filename , "r" );
    if ( deckfile == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure" , deck_filename );
        return UNKNOWN_DECK_FORMAT;
    }

    GError * g_error = NULL;
    GRegex * forge_regex = g_regex_new( "^([0-9]+)\\ ([^|]+)\\|([^|^\\r^\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( forge_regex == NULL )
        goto parse_err;

    GRegex * xmage_regex = g_regex_new( "^(SB:\\ )?(\\d+)\\ \\[([^:\\]]*):(\\d+)\\]\\ ([^\\r\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( xmage_regex == NULL )
        goto parse_err;

    GRegex * mtga_regex = g_regex_new( "^([0-9]+)\\ ([^\\(^\\)]+)\\ \\(([^\\ ]+)\\)\\ ([0-9^\\r^\\n]+)" ,
                        G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( mtga_regex == NULL )
        goto parse_err;

    GRegex * goldfish_regex = g_regex_new( "^([0-9]+)\\ ([^|\\r\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( goldfish_regex == NULL )
        goto parse_err;

    size_t forge_format_count = 0;
    size_t xmage_format_count = 0;
    size_t mtga_format_count = 0;
    size_t goldfish_format_count = 0;

    while ( fgets( line_buff , BUFFSIZE , deckfile ) != NULL )
    {
        if ( g_regex_match( forge_regex , line_buff , 0 , NULL ) == TRUE )
        {
            forge_format_count++;
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;

        if ( g_regex_match( xmage_regex , line_buff , 0 , NULL ) == TRUE )
        {
            xmage_format_count++;
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;

        if ( g_regex_match( mtga_regex , line_buff , 0 , NULL ) == TRUE )
        {
            mtga_format_count++;
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;

        //goldfish regex match mtga format deck,so first check mtga format.
        if ( g_regex_match( goldfish_regex , line_buff , 0 , NULL ) == TRUE )
        {
            goldfish_format_count++;
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;
    }

    g_regex_unref( forge_regex );
    g_regex_unref( mtga_regex );
    g_regex_unref( goldfish_regex );
    fclose( deckfile );

    if ( ( forge_format_count >= mtga_format_count )     &&
         ( forge_format_count >= xmage_format_count )    &&
         ( forge_format_count >= goldfish_format_count ) &&
         forge_format_count != 0
       )
        return FORGE_DECK_FORMAT;
    else if ( ( xmage_format_count >= forge_format_count )     &&
              ( xmage_format_count >= mtga_format_count )      &&
              ( xmage_format_count >= goldfish_format_count )  &&
              xmage_format_count != 0
            )
        return XMAGE_DECK_FORMAT;
    else if ( ( mtga_format_count >= forge_format_count )    &&
              ( mtga_format_count >= xmage_format_count )    &&
              ( mtga_format_count >= goldfish_format_count ) &&
              mtga_format_count != 0
            )
        return MTGA_DECK_FORMAT;
    else if ( goldfish_format_count != 0 )
        return GOLDFISH_DECK_FORMAT;
    //avoid no-return warning,can't use else
    return UNKNOWN_DECK_FORMAT;
    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s" , g_error->message );
        g_error_free( g_error );
        return UNKNOWN_DECK_FORMAT;
}

static GSList * get_cardlist( const gchar * deckfilename )
{
    enum DeckType deck_type = get_deck_type( deckfilename );
    switch ( deck_type )
    {
        case FORGE_DECK_FORMAT:
        {
            return get_cardlist_forge( deckfilename );
        }
        case XMAGE_DECK_FORMAT:
        {
            return get_cardlist_xmage( deckfilename );
        }
        case MTGA_DECK_FORMAT:
        {
            return get_cardlist_mtga( deckfilename );
        }
        case GOLDFISH_DECK_FORMAT:
        {
            return get_cardlist_goldfish( deckfilename );
        }
        default :
        {
            return NULL;
        }
    }

    return NULL;
}

static GSList * get_cardlist_forge( const gchar * deckfilename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure" , deckfilename );
        return NULL;
    }

    GSList * cardlist = NULL;
    gint32 card_local = COMMAND_LOCAL;
    GError * g_error = NULL;
    GMatchInfo * match_info;
    GRegex * regex = g_regex_new( "^([0-9]+)\\ ([^|]+)\\|([^|^\\r^\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( regex == NULL )
        goto parse_err;
    while ( fgets( line_buff , BUFFSIZE , deckfile ) != NULL )
    {
        if ( ( strncmp( line_buff , "[Main]" , 6 ) == 0 ) || ( strncmp( line_buff , "[main]" , 6 ) == 0 ) )
            card_local = MAIN_LOCAL;
        if ( ( strncmp( line_buff , "[Sideboard]" , 11 ) == 0 ) || ( strncmp( line_buff , "[sideboard]" , 11 ) == 0 ) )
            card_local = SIDEBOARD_LOCAL;

        if ( g_regex_match( regex , line_buff , 0 , &match_info ) == TRUE )
        {
            struct CardObject * card = allocate_cardobject();
            gchar ** words = g_regex_split( regex , line_buff , 0 );
            gint32 cardnumber = g_ascii_strtoll( words[1] , NULL , 10 );
            card->card_local = card_local;
            card->cardnumber = cardnumber;
            g_strlcat( card->cardname , words[2] , BUFFSIZE );
            g_strlcat( card->cardseries , words[3] , BUFFSIZE );
            //G_MAXINT32 is unknown id
            card->cardid = G_MAXINT32;

            gchar * image_name = cardname_to_imagename( card->cardname , FORGE_DECK_FORMAT );
            free( card->cardname );
            card->cardname = image_name;

            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;
    }

    g_regex_unref( regex );
    return cardlist;

    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s" , g_error->message );
        g_regex_unref( regex );
        g_error_free( g_error );
        exit( EXIT_FAILURE );
}

GSList * get_cardlist_xmage( const gchar * deckfilename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure" , deckfilename );
        return NULL;
    }

    GSList * cardlist = NULL;
    GError * g_error = NULL;
    GMatchInfo * match_info;
    GRegex * regex = g_regex_new( "^(SB:\\ )?(\\d+)\\ \\[([^:\\]]*):(\\d+)\\]\\ ([^\\r\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( regex == NULL )
        goto parse_err;
    while ( fgets( line_buff , BUFFSIZE , deckfile ) != NULL )
    {
        if ( g_regex_match( regex , line_buff , 0 , &match_info ) == TRUE )
        {
            struct CardObject * card = allocate_cardobject();
            gchar ** words = g_regex_split( regex , line_buff , 0 );
            
            if ( strnlen( words[1] , BUFFSIZE ) == 0 )
            {
                card->card_local = MAIN_LOCAL;
            }
            else
            {
                card->card_local = SIDEBOARD_LOCAL;
            }

            card->cardnumber = g_ascii_strtoll( words[2] , NULL , 10 );
            g_strlcat( card->cardseries , words[3] , BUFFSIZE );
            card->cardid = g_ascii_strtoll( words[4] , NULL , 10 );;
            g_strlcat( card->cardname , words[5] , BUFFSIZE );

            gchar * image_name = cardname_to_imagename( card->cardname , FORGE_DECK_FORMAT );
            free( card->cardname );
            card->cardname = image_name;

            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;
    }

    g_regex_unref( regex );
    return cardlist;

    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s" , g_error->message );
        g_regex_unref( regex );
        g_error_free( g_error );
        exit( EXIT_FAILURE );
}

static GSList * get_cardlist_mtga( const gchar * deckfilename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure" , deckfilename );
        return NULL;
    }

    GSList * cardlist = NULL;
    //mtga now only support limit and standard constructor,no command.
    gint32 card_local = MAIN_LOCAL;
    GError * g_error = NULL;
    GMatchInfo * match_info;
    GRegex * regex = g_regex_new( "^([0-9]+)\\ ([^\\(^\\)]+)\\ \\(([^\\ ]+)\\)\\ ([0-9^\\r^\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( regex == NULL )
        goto parse_err;
    while ( fgets( line_buff , BUFFSIZE , deckfile ) != NULL )
    {
        //to next local
        if ( ( strncmp( line_buff , "\r\n" , 4 ) == 0 ) || ( strncmp( line_buff , "\n" , 2 ) == 0 ) )
            card_local += 1;

        if ( g_regex_match( regex , line_buff , 0 , &match_info ) == TRUE )
        {
            struct CardObject * card = allocate_cardobject();
            gchar ** words = g_regex_split( regex , line_buff , 0 );
            gint32 cardnumber = g_ascii_strtoll( words[1] , NULL , 10 );
            gint32 cardid = g_ascii_strtoll( words[4] , NULL , 10 );;
            card->card_local = card_local;
            card->cardnumber = cardnumber;
            g_strlcat( card->cardname , words[2] , BUFFSIZE );
            //mtga format set "DOM" name is "DAR"
            if ( ( strncmp( "DAR" , words[3] , 4 ) == 0 ) || ( strncmp( "dar" , words[3] , 4 ) == 0 ) )
                g_strlcat( card->cardseries , "DOM" , BUFFSIZE );
            else
                g_strlcat( card->cardseries , words[3] , BUFFSIZE );
            card->cardid = cardid;

            gchar * image_name = cardname_to_imagename( card->cardname , MTGA_DECK_FORMAT );
            free( card->cardname );
            card->cardname = image_name;

            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;
    }

    g_regex_unref( regex );
    return cardlist;

    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s" , g_error->message );
        g_regex_unref( regex );
        g_error_free( g_error );
        exit( EXIT_FAILURE );
}

static GSList * get_cardlist_goldfish( const gchar * deckfilename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure" , deckfilename );
        return NULL;
    }

    GSList * cardlist = NULL;
    gint32 card_local = COMMAND_LOCAL;
    GError * g_error = NULL;
    GMatchInfo * match_info;
    GRegex * regex = g_regex_new( "^([0-9]+)\\ ([^|\\r\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( regex == NULL )
        goto parse_err;
    while ( fgets( line_buff , BUFFSIZE , deckfile ) != NULL )
    {
        if ( line_buff == NULL )
            continue;
        //to next local
        if ( ( strncmp( line_buff , "\r\n" , 4 ) == 0 ) || ( strncmp( line_buff , "\n" , 2 ) == 0 ) )
            card_local += 1;

        if ( g_regex_match( regex , line_buff , 0 , &match_info ) == TRUE )
        {
            struct CardObject * card = allocate_cardobject();
            gchar ** words = g_regex_split( regex , line_buff , 0 );
            gint32 cardnumber = g_ascii_strtoll( words[1] , NULL , 10 );
            card->card_local = card_local;
            card->cardnumber = cardnumber;
            g_strlcat( card->cardname , words[2] , BUFFSIZE );
            card->cardseries = NULL;
            //G_MAXINT32 is unknown id
            card->cardid = G_MAXINT32;
            //double face card format goldfish equal forge
            gchar * image_name = cardname_to_imagename( card->cardname , FORGE_DECK_FORMAT );
            free( card->cardname );
            card->cardname = image_name;

            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;
    }

    g_regex_unref( regex );
    return cardlist;

    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s" , g_error->message );
        g_regex_unref( regex );
        g_error_free( g_error );
        exit( EXIT_FAILURE );
}

static void delete_cardlist( GSList * cardlist )
{
    g_slist_free_full( cardlist , ( GDestroyNotify )free_cardobject );
}

static struct CardObject * allocate_cardobject( void )
{
    struct CardObject * card = ( struct CardObject * )calloc( 1 , sizeof( struct CardObject ) );
    if ( card == NULL )
        return card;
    card->cardname = calloc( 1 , BUFFSIZE*sizeof( gchar ) );
    if ( card->cardname == NULL )
    {
        free( card );
        return NULL;
    }
    card->cardseries = calloc( 1 , BUFFSIZE*sizeof( gchar ) );
    if ( card->cardseries == NULL )
    {
        free( card->cardname );
        free( card );
        return NULL;
    }
    return card;
}

static void free_cardobject( struct CardObject * card )
{
    if ( card == NULL )
        return ;
    free( card->cardseries );
    free( card->cardname );
    free( card );
}

static gchar * cardname_to_imagename( const gchar * cardname , enum DeckType deck_type )
{
    gchar * new_str = NULL;

    //forge double-faced,cookies,fusion card name:"Name1 // Name2"
    if ( ( deck_type == FORGE_DECK_FORMAT ) || ( deck_type == GOLDFISH_DECK_FORMAT ) )
    {
        if ( config_object.image_name_format == FORGE_NAME_FORMAT )
            new_str = strreplace( cardname , " // " , "" );
        else if ( config_object.image_name_format == XMAGE_NAME_FORMAT )
            new_str = strreplace( cardname , "//" , "-" );

        if ( new_str == NULL )
            return NULL;
    }

    //mtga double-faced,cookies,fusion card name:"Name1 /// Name2"
    if ( deck_type == MTGA_DECK_FORMAT )
    {
        if ( config_object.image_name_format == FORGE_NAME_FORMAT )
            new_str = strreplace( cardname , " /// " , "" );
        else if ( config_object.image_name_format == XMAGE_NAME_FORMAT )
            new_str = strreplace( cardname , "///" , "-" );

        if ( new_str == NULL )
            return NULL;
    }

    //file system name limmit replace:"\/:*?"<>|"

    return new_str;
}

char * stem_name( const char * filename )
{
    if ( filename == NULL )
        return NULL;

    const char * stem_name_start_ptr = filename;
    // "/home/username/doc/file.exp"  pointer to "file.exp"
    for ( const char * find_ptr = NULL ; ( find_ptr = strchr( stem_name_start_ptr , '/' ) ) != NULL ; )
        stem_name_start_ptr = find_ptr + 1;

    // unix file name space '\\' in $(filename) is valid
    #ifdef G_OS_WIN32
    // if "/home/user\\name/doc\\file.exp" pointer to "file.exp"
    for ( const char * find_ptr = NULL ; ( find_ptr = strchr( stem_name_start_ptr , '\\' ) ) != NULL ; )
        stem_name_start_ptr = find_ptr + 1;
    #endif

    const char * stem_name_end_ptr = stem_name_start_ptr;
    // 'file.name.content..sss.format" pointer to "format"
    for ( const char * find_ptr = NULL ; ( find_ptr = strchr( stem_name_end_ptr , '.' ) ) != NULL ; )
        stem_name_end_ptr = find_ptr + 1;


    size_t stem_name_len = stem_name_end_ptr - stem_name_start_ptr;
    // ".vimrc" unix hide file name format,the stem name: ".vimrc" or if "filenamecontent" , ""
    if ( stem_name_len <= 1 )
    {
        stem_name_len = strnlen( stem_name_start_ptr , BUFFSIZE );
    }
    // "/usr/bin/.."
    else if ( strncmp( stem_name_start_ptr , ".." , BUFFSIZE ) == 0 )
    {
        stem_name_len = 2;
    }
    // 'file.name.content..sss.format" pointer to ".format"
    else
    {
        stem_name_len -= 1;
    }

    char * stem_name = ( char * )malloc( ( stem_name_len + 1 )*sizeof( char ) );
    if ( stem_name == NULL )
    {
        return NULL;
    }

    memcpy( stem_name , stem_name_start_ptr , stem_name_len );
    stem_name[stem_name_len] = '\0';

    return stem_name;
}

static gchar * strreplace( const gchar * source_str , const gchar * from_str , const gchar * to_str )
{
    if ( source_str == NULL )
        return NULL;
    if ( from_str == NULL )
        return NULL;
    if ( to_str == NULL )
        return NULL;

    size_t source_len = strnlen( source_str , BUFFSIZE );
    size_t from_len = strnlen( from_str , BUFFSIZE );
    size_t to_len = strnlen( to_str , BUFFSIZE );
    size_t replace_count = 0;

    if ( ( from_len == 0 ) || ( source_len == 0 ) )
    {
        gchar * new_str = ( gchar * )malloc( ( source_len + 1 ) * sizeof( gchar ) );
        if ( new_str == NULL )
            return NULL;
        memcpy( new_str , source_str , source_len + 1 );
        return new_str;
    }

    const gchar * temp_ptr = source_str;
    for ( gchar * find_ptr = NULL ; ( find_ptr = strstr( temp_ptr , from_str ) ) != NULL ; replace_count++ )
        temp_ptr = find_ptr + from_len;

    if ( replace_count == 0 )
    {
        gchar * new_str = ( gchar * )malloc( ( source_len + 1 ) * sizeof( gchar ) );
        if ( new_str == NULL )
            return NULL;
        memcpy( new_str , source_str , source_len + 1 );
        return new_str;
    }

    gboolean len_up = FALSE;
    size_t chane_len = 0;
    if ( to_len > from_len )
    {
        chane_len = to_len - from_len;
        len_up = TRUE;
    }
    else
    {
        chane_len = from_len - to_len;
        len_up = FALSE;
    }

    //replace to overflow
    if ( chane_len > ( SIZE_MAX - source_len )/replace_count )
        return NULL;

    size_t new_str_len = 0;
    if ( len_up == TRUE )
        new_str_len = source_len + replace_count*chane_len;
    else
        new_str_len = source_len - replace_count*chane_len;

    //NUL in end
    gchar * new_str = ( gchar * )malloc( ( new_str_len + 1 ) * sizeof( gchar ) );
    if ( new_str == NULL )
        return NULL;

    const gchar * start_ptr = source_str;
    const gchar * end_ptr = source_str + source_len;
    gchar * copy_to_ptr = new_str;
    while( replace_count-- > 0 )
    {
        gchar *  offset_ptr = strstr( start_ptr , from_str );
        if ( offset_ptr == NULL )
            break;
        gchar * next_ptr = offset_ptr + from_len;
        memcpy( copy_to_ptr , start_ptr , offset_ptr - start_ptr );
        copy_to_ptr += ( offset_ptr - start_ptr );
        memcpy( copy_to_ptr , to_str , to_len );
        copy_to_ptr += to_len;
        start_ptr = next_ptr;
    }
    memcpy( copy_to_ptr , start_ptr , end_ptr - start_ptr + 1 );

    return new_str;
}

static gchar * make_imagefile_uri( const gchar * cardname , const gchar * cardseries )
{
    gchar * imagefile_uri = NULL;
    if ( cardname == NULL )
        return imagefile_uri;

    const gchar * forgeimagesuffix = ".full";
    const gchar * basiclandnamelist[] =
    {
        "Plains" , "Island" , "Swamp" , "Mountain" , "Forest" ,
        "Snow-Covered Plains" , "Snow-Covered Island" ,
        "Snow-Covered Swamp" , "Snow-Covered Mountain" ,
        "Snow-Covered Forest" , "Wastes" , NULL
    };

    for ( gsize i = 0 ; basiclandnamelist[i] != NULL ; i++ )
        if ( strncmp( cardname , basiclandnamelist[i] , BUFFSIZE ) == 0 )
            forgeimagesuffix = "1.full";

    if ( cardseries != NULL )
        imagefile_uri = g_strdup_printf( "file:///%s%s/%s%s%s" ,
            config_object.image_root_directory , cardseries , cardname , forgeimagesuffix , config_object.image_suffix );
    else
        imagefile_uri = g_strdup_printf( "file:///%s%s%s%s" ,
            config_object.image_root_directory , cardname , forgeimagesuffix , config_object.image_suffix );

    return imagefile_uri;
}

static gchar * make_targetfile_uri( const char * targetdirectory , const gchar * cardname , const gchar * cardseries , gsize retry_count )
{
    gchar * targetfile_uri = NULL;
    if ( cardname == NULL )
        return targetfile_uri;
    if ( cardseries != NULL )
        targetfile_uri = g_strdup_printf( "file:///%s%s.%s%"G_GSIZE_FORMAT"%s" ,
            targetdirectory , cardname , cardseries , retry_count , config_object.image_suffix );
    else
        targetfile_uri = g_strdup_printf( "file:///%s%s%"G_GSIZE_FORMAT"%s" ,
            targetdirectory , cardname , retry_count , config_object.image_suffix );
    return targetfile_uri;
}

static void get_clipboard_content( GtkClipboard * clipboard , const gchar * text , gpointer user_data )
{
    ( void )clipboard;
    GSList ** deck_list = ( GSList ** )user_data;
    if ( text == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "clipboard text content is empty" );
        goto do_return;
    }
    gsize text_len = g_utf8_strlen( text , -1 );

    GFileIOStream * io_stream = NULL;
    GFile * temp_file_obj = g_file_new_tmp( NULL , &io_stream , NULL );
    if ( temp_file_obj == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "can not new temp file" );
       goto do_return;
    }
    GOutputStream * output_stream = g_io_stream_get_output_stream( G_IO_STREAM( io_stream ) );
    gssize write_size = g_output_stream_write( output_stream , text , text_len , NULL , NULL );
    if ( write_size == -1 )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "can not write clipboard text content to temp file" );
        g_object_unref( temp_file_obj );
        goto do_return;
    }

    gchar * temp_filename = g_file_get_parse_name( temp_file_obj );
    *deck_list = g_slist_append( *deck_list , ( gpointer )temp_filename );

    //delete_deck_list free
    //g_free( temp_filename );
    g_object_unref( temp_file_obj );
    do_return:
    gtk_main_quit();
}

static gboolean get_deckpreview( GtkWidget * window , GdkEvent * event , gpointer data )
{
    ( void )event;
    GdkWindow * gdk_window = gtk_widget_get_window( GTK_WIDGET( window ) );
    GdkPixbuf * deckpreview = gdk_pixbuf_get_from_window( GDK_WINDOW( gdk_window ) , 0 , 0 , config_object.window_width , config_object.window_height );
    if ( deckpreview == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get deck preview buff faliure" );
    }
    else
    {
        gchar * previewname = g_strdup_printf( "%sdeckpreview.jpg" , ( const gchar * )data );
        gdk_pixbuf_save( deckpreview , previewname , "jpeg" , NULL , "quality" , "100" , NULL );
        g_free( previewname );
    }
    gtk_main_quit();
    return TRUE;
}

static gboolean motion_notify_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data )
{
    ( void )widget;
    struct ImagesLayout * widget_layout = ( struct ImagesLayout * )data;
    if ( widget_layout == NULL )
        return TRUE;
    int x, y;
    GdkModifierType state;
    gdk_window_get_device_position( event->window , event->device , &x , &y , &state );
    if ( widget_layout->select_target == TRUE && event->type == GDK_MOTION_NOTIFY )
    {
        guint select_index = widget_layout->select_x/config_object.card_width + widget_layout->select_y/config_object.card_height*config_object.line_card_number;
        GtkWidget * select_image = g_array_index( widget_layout->images , GtkWidget * , select_index );

        //avoid underflow overflow
        if ( x < 0 )
        {
            x = widget_layout->dx;
        }
        if ( y < 0 )
        {
            y = widget_layout->dy;
        }
        if ( x > config_object.card_width*config_object.line_card_number )
        {
            x = config_object.card_width*config_object.line_card_number;
        }
        if ( y > config_object.card_height*widget_layout->height )
        {
            y = config_object.card_height*widget_layout->height;
        }

        gtk_layout_move( GTK_LAYOUT( widget_layout->layout ) , select_image , x - widget_layout->dx , y - widget_layout->dy );
    }

    return TRUE;
}

static gboolean button_release_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data )
{
    ( void )widget;
    struct ImagesLayout * widget_layout = ( struct ImagesLayout * )data;
    int x, y;
    GdkModifierType state;
    gdk_window_get_device_position( event->window , event->device , &x , &y , &state );

    //avoid underflow overflow
    if ( x < 0 )
    {
        x = widget_layout->dx;
    }
    if ( y < 0 )
    {
        y = widget_layout->dy;
    }
    if ( x > config_object.card_width*config_object.line_card_number )
    {
        x = config_object.card_width*config_object.line_card_number;
    }
    if ( y > config_object.card_height*widget_layout->height )
    {
        y = config_object.card_height*widget_layout->height;
    }

    if ( ( x == widget_layout->select_x ) && ( y == widget_layout->select_y ) )
    {
        widget_layout->select_target = FALSE;
        return TRUE;
    }
    if ( widget_layout->select_target == TRUE )
    {
        guint start_index = widget_layout->select_x/config_object.card_width + widget_layout->select_y/config_object.card_height*config_object.line_card_number;
        guint end_index = x/config_object.card_width + y/config_object.card_height*config_object.line_card_number;

        //avoid underflow overflow
        if ( start_index >= widget_layout->images->len )
            start_index = widget_layout->images->len - 1;
        if ( end_index >= widget_layout->images->len )
            end_index = widget_layout->images->len - 1;

        GtkWidget * start_image = g_array_index( widget_layout->images , GtkWidget * , start_index );
        if ( start_index > end_index )
        {
            for ( guint i = start_index ; i > end_index ; i-- )
            {
                GtkWidget * select_image = g_array_index( widget_layout->images , GtkWidget * , i - 1 );
                gtk_layout_move( GTK_LAYOUT( widget_layout->layout ) , select_image ,
                    i%( config_object.line_card_number )*config_object.card_width , i/( config_object.line_card_number )*config_object.card_height );
                g_array_index( widget_layout->images , GtkWidget * , i ) = select_image;
            }
        }
        else
        {
            for ( guint i = start_index ; i < end_index ; i++ )
            {
                GtkWidget * select_image = g_array_index( widget_layout->images , GtkWidget * , i + 1 );
                gtk_layout_move( GTK_LAYOUT( widget_layout->layout ) , select_image ,
                    i%( config_object.line_card_number )*config_object.card_width , i/( config_object.line_card_number )*config_object.card_height );
                g_array_index( widget_layout->images , GtkWidget * , i ) = select_image;
            }
        }
        gtk_layout_move( GTK_LAYOUT( widget_layout->layout ) , start_image ,
                end_index%( config_object.line_card_number )*config_object.card_width , end_index/( config_object.line_card_number )*config_object.card_height );
        g_array_index( widget_layout->images , GtkWidget * , end_index ) = start_image;
        widget_layout->select_target = FALSE;
    }
    return TRUE;
}

static gboolean button_press_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data )
{
    ( void )widget;
    struct ImagesLayout * widget_layout = ( struct ImagesLayout * )data;
    int x , y;
    GdkModifierType state;
    gdk_window_get_device_position( event->window , event->device , &x , &y , &state );
    if ( x > config_object.card_width*config_object.line_card_number || y > config_object.card_height*widget_layout->height )
        return TRUE;
    widget_layout->select_x = x;
    widget_layout->select_y = y;
    widget_layout->dx = widget_layout->select_x - x/config_object.card_width*config_object.card_width;
    widget_layout->dy = widget_layout->select_y - y/config_object.card_width*config_object.card_width;
    widget_layout->select_target = TRUE;

    return TRUE;
}

static void download_file( gpointer data , gpointer user_data )
{
    ( void )user_data;
    struct CardObject * card = ( struct CardObject * )data;
    g_usleep( 1000 );

    gchar * url = NULL;
    if ( card->cardseries == NULL )
        url = g_strdup_printf( "https://api.scryfall.com/cards/named?exact=%s&format=image" , card->cardname );
    else
        url = g_strdup_printf( "https://api.scryfall.com/cards/named?exact=%s&set=%s&format=image" , card->cardname , card->cardseries );

    gchar * source_url = g_uri_escape_string( url , G_URI_RESERVED_CHARS_ALLOWED_IN_PATH"?" , FALSE );
    g_free( url );
    gchar * destination_uri = make_imagefile_uri( card->cardname , card->cardseries );
    gchar * destination_path = g_filename_from_uri( destination_uri , NULL , NULL );
    g_free( destination_uri );

    if ( g_file_test( destination_path , G_FILE_TEST_EXISTS ) == TRUE )
    {
        GtkWidget * image = gtk_image_new_from_file( destination_path );
        GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
        if ( pixbuf != NULL )
        {
            gtk_widget_destroy( image );
            return ;
        }

        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "'%s' don't is image file,delete and redownload" , destination_path );
        g_remove( destination_path );
        gtk_widget_destroy( image );
    }

    if ( card->cardseries != NULL )
    {
        gchar * download_dir = g_strdup_printf( "%s%s" , config_object.image_root_directory , card->cardseries );
        if ( g_file_test( download_dir , G_FILE_TEST_EXISTS ) != TRUE )
            g_mkdir_with_parents( download_dir , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
        g_free( download_dir );
    }
    else
    {
        gchar * download_dir = g_strdup_printf( "%s" , config_object.image_root_directory );
        if ( g_file_test( download_dir , G_FILE_TEST_EXISTS ) != TRUE )
            g_mkdir_with_parents( download_dir , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
        g_free( download_dir );
    }

    CURL * curl_handle = curl_easy_init();
    long default_timeout = 30L;

    curl_easy_setopt( curl_handle , CURLOPT_URL , source_url );
    curl_easy_setopt( curl_handle , CURLOPT_VERBOSE , 0L );
    curl_easy_setopt( curl_handle , CURLOPT_FOLLOWLOCATION , 1L );
    curl_easy_setopt( curl_handle , CURLOPT_NOPROGRESS , 1L );
    curl_easy_setopt( curl_handle , CURLOPT_TIMEOUT , default_timeout );
    curl_easy_setopt( curl_handle , CURLOPT_WRITEFUNCTION , NULL );

    FILE * download_file = fopen( destination_path, "wb" );
    if ( download_file )
    {
        char error_buff[BUFFSIZE];
        curl_easy_setopt( curl_handle , CURLOPT_ERRORBUFFER , error_buff );
        curl_easy_setopt( curl_handle , CURLOPT_WRITEDATA , download_file );
        CURLcode res = curl_easy_perform( curl_handle );
        int re_try = 4;
        while ( res != CURLE_OK )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "url:\'%s\',error type:\'%s\',error message:\'%s\'" , 
                    source_url , curl_easy_strerror( res ) , error_buff );
            if ( re_try == 0 )
                break;
            default_timeout *= 1.5;
            curl_easy_setopt( curl_handle , CURLOPT_TIMEOUT , default_timeout );
            res = curl_easy_perform( curl_handle );
            re_try--;
        }
        fclose( download_file );
    }

    curl_easy_cleanup( curl_handle );

    g_free( destination_path );
    g_free( source_url );
}
