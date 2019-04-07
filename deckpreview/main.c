#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <curl/curl.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "deckparsing.h"
#include "previewconfig.h"

#define LOGO_FILE "logo.ico"
#define BUFFSIZE 1024

struct DeckObject
{
    gchar * deckfullname;
    gchar * targetdirectory;
};

struct ScrollDescription
{
    GtkWidget * scrollwindow;
    guint scroll_handler_id;
};

struct ImagesLayout
{
    GtkWidget * layout;
    gint32 width;
    gint32 height;
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

gint32 process_deck( struct DeckObject * );

//do main
static GSList * get_deckfile_list( const gchar * deck_path );
static void delete_deck_list( GSList * deck_list );
static gboolean remove_directory( const gchar * dir );
static gboolean make_directory( const gchar * dir );

//do process_deck
static void preview_add_title( enum CardLocal card_local );
static void preview_add_card( struct CardObject * card , struct ScrollDescription * desc );
static void preview_display( void );
static void preview_destroy( void );

static gboolean copy_file( const gchar * source_uri , const gchar * destination_uri );
static char * stem_name( const char * filename );
static gchar * make_imagefile_uri( const gchar * cardname , const gchar * cardseries );
static gchar * make_targetfile_uri( const char * targetdirectory , const gchar * cardname , const gchar * cardseries , gsize retry_count );

static gboolean get_deckpreview( GtkWidget * window , GdkEvent * event , gpointer data );
static void download_file( gpointer data , gpointer user_data );
static void get_clipboard_content( GtkClipboard * clipboard , const gchar * text , gpointer user_data );
static void drag_begin_handler( GtkWidget * widget , GdkDragContext * context , gpointer data );
static void drag_end_handler( GtkWidget * widget , GdkDragContext * context , gpointer data );
static void darg_data_get_handler( GtkWidget * widget , GdkDragContext * context , GtkSelectionData * data , guint info , guint timestamp , gpointer user_data );
static void darg_data_received_handler( GtkWidget * widget , GdkDragContext * context , gint x , gint y , GtkSelectionData * data , guint info , guint timestamp , gpointer user_data );

int main ( int argc , char * argv[] )
{
    gtk_init( &argc , &argv );
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

gint32 process_deck( struct DeckObject * deck )
{
    gint32 copy_success_count = 0;
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
    g_signal_connect( G_OBJECT( preview_object.window ) , "delete-event" , G_CALLBACK( get_deckpreview ) , deck->targetdirectory );

    GtkWidget * scrolled = gtk_scrolled_window_new( NULL , NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ) , GTK_POLICY_AUTOMATIC , GTK_POLICY_AUTOMATIC );

    preview_object.command_layout.layout = gtk_layout_new( NULL , NULL );
    preview_object.command_layout.width = 0;
    preview_object.command_layout.height = 0;

    preview_object.main_layout.layout = gtk_layout_new( NULL , NULL );
    preview_object.main_layout.width = 0;
    preview_object.main_layout.height = 0;

    preview_object.sideboard_layout.layout = gtk_layout_new( NULL , NULL );
    preview_object.sideboard_layout.width = 0;
    preview_object.sideboard_layout.height = 0;

    preview_object.layout = gtk_layout_new( NULL , NULL );
    preview_object.layout_height = 0;
    gtk_container_add( GTK_CONTAINER( scrolled ) , preview_object.layout );
    gtk_container_add( GTK_CONTAINER( preview_object.window ) , scrolled );

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

    struct ScrollDescription desc = 
    {
        scrolled , 0
    };
    temp = cardlist;
    while( temp != NULL )
    {
        struct CardObject * card = ( struct CardObject * )temp->data;
        preview_add_card( card , &desc );
        if ( config_object.copy_file == TRUE )
        {
            gchar * imagefile_uri = make_imagefile_uri( card->cardname , card->cardseries );
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
        gchar * dir_name = g_strdup_printf( "%s/%s" , dir , filename );
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
        if ( g_error_matches( g_error , G_IO_ERROR_EXISTS , G_IO_ERROR_FAILED ) )
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

static void preview_add_card( struct CardObject * card , struct ScrollDescription * desc )
{
    gchar * imagefile_uri = make_imagefile_uri( card->cardname , card->cardseries );
    gchar * imagefile_path = g_filename_from_uri( imagefile_uri , NULL , NULL );
    g_free( imagefile_uri );
    gint32 cardnumber = card->cardnumber;
    GtkTargetEntry * entries = gtk_target_entry_new( "image/jpeg" , GTK_TARGET_SAME_APP , 0 );
    while ( cardnumber-- )
    {
        GtkWidget * button = gtk_event_box_new();
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
        gtk_container_add( GTK_CONTAINER( button ) , image );
        gtk_drag_source_set( button , GDK_BUTTON1_MASK , entries , 1 , GDK_ACTION_MOVE );
        gtk_drag_dest_set( button , GTK_DEST_DEFAULT_ALL , entries , 1 , GDK_ACTION_MOVE );
        g_signal_connect_after( G_OBJECT( button ) , "drag-begin" , G_CALLBACK( drag_begin_handler ) , desc );
        g_signal_connect( G_OBJECT( button ) , "drag-end" , G_CALLBACK( drag_end_handler ) , desc );
        //call default drag-failed signal handler,if don't do this,drag failed drag-end signal handler not call.
        g_signal_connect( G_OBJECT( button ) , "drag-failed" , G_CALLBACK( gtk_false ) , NULL );
        g_signal_connect( G_OBJECT( button ) , "drag-data-get" , G_CALLBACK( darg_data_get_handler ) , NULL );
        g_signal_connect( G_OBJECT( button ) , "drag-data-received" , G_CALLBACK( darg_data_received_handler ) , NULL );
        g_object_unref( pixbuf );

        struct ImagesLayout * target_layout = NULL;
        switch ( card->card_local )
        {
            //if you simply break default,decode nullptr(e.g target_layout->layout)
            default:
            case COMMAND_LOCAL:
            {
                target_layout = &preview_object.command_layout;
                break;
            }
            case MAIN_LOCAL:
            {
                target_layout = &preview_object.main_layout;
                break;
            }
            case SIDEBOARD_LOCAL:
            {
                target_layout = &preview_object.sideboard_layout;
                break;
            }
        }

        gtk_layout_put( GTK_LAYOUT( target_layout->layout ) , button , target_layout->width , target_layout->height );
        target_layout->width += config_object.card_width;
        if ( target_layout->width == ( config_object.line_card_number )*( config_object.card_width ) )
        {
            target_layout->width = 0;
            target_layout->height += config_object.card_height;
        }

    }
    gtk_target_entry_free( entries );
    g_free( imagefile_path );
}

static void preview_display( void )
{
    GList * children_list = gtk_container_get_children( GTK_CONTAINER( preview_object.command_layout.layout ) );
    guint command_layout_len = g_list_length( children_list );
    g_list_free( children_list );
    if ( command_layout_len != 0 )
    {
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.command_layout.layout , 0 , preview_object.layout_height );
        if ( ( command_layout_len % config_object.line_card_number ) != 0 )
            preview_object.command_layout.height += config_object.card_height;
        preview_object.layout_height += preview_object.command_layout.height;
    }

    children_list = gtk_container_get_children( GTK_CONTAINER( preview_object.main_layout.layout ) );
    guint main_layout_len = g_list_length( children_list );
    g_list_free( children_list );
    if ( main_layout_len != 0 )
    {
        preview_add_title( MAIN_LOCAL );
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.main_layout.layout , 0 , preview_object.layout_height );
        if ( ( main_layout_len % config_object.line_card_number ) != 0 )
            preview_object.main_layout.height += config_object.card_height;
        preview_object.layout_height += preview_object.main_layout.height;
    }

    children_list = gtk_container_get_children( GTK_CONTAINER( preview_object.sideboard_layout.layout ) );
    guint sideboard_layout_len = g_list_length( children_list );
    g_list_free( children_list );
    if ( sideboard_layout_len != 0 )
    {
        preview_add_title( SIDEBOARD_LOCAL );
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.sideboard_layout.layout , 0 , preview_object.layout_height );
        if ( ( sideboard_layout_len % config_object.line_card_number ) != 0 )
            preview_object.sideboard_layout.height += config_object.card_height;
        preview_object.layout_height += preview_object.sideboard_layout.height;
    }

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

static gboolean automatic_scroll( gpointer data )
{
    struct ScrollDescription * desc = ( struct ScrollDescription * )data;
    double x = 0.0 , y = 0.0;
    gdk_window_get_device_position_double( gtk_widget_get_window( desc->scrollwindow ) , gdk_seat_get_pointer(
        gdk_display_get_default_seat( gdk_window_get_display( gtk_widget_get_window( desc->scrollwindow ) ) )
    ) , &x , &y , NULL );

    GtkAllocation allocation;
    gtk_widget_get_allocation( desc->scrollwindow , &allocation );

    GtkAdjustment * hadjustment = gtk_scrolled_window_get_hadjustment( GTK_SCROLLED_WINDOW( desc->scrollwindow ) );
    GtkAdjustment * vadjustment = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW( desc->scrollwindow ) );

    //horizontal rollbar exist
    if ( hadjustment != NULL )
    {
        double value = gtk_adjustment_get_value( hadjustment );
        double lower = gtk_adjustment_get_lower( hadjustment );
        double upper = gtk_adjustment_get_upper( hadjustment );
        double setp = gtk_adjustment_get_step_increment( hadjustment );
        if ( x < 0.1 * allocation.width )
        {
            value = ( ( value - setp > lower ) ? ( value - setp ) : lower );
            gtk_adjustment_set_value( hadjustment , value );
        }
        else if ( x > 0.9 * allocation.width )
        {
            value = ( ( value + setp > upper ) ? upper : ( value + setp ) );
            gtk_adjustment_set_value( hadjustment , value );
        }
    }
    //vertical rollbar exist
    if ( vadjustment != NULL )
    {
        double value = gtk_adjustment_get_value( vadjustment );
        double lower = gtk_adjustment_get_lower( vadjustment );
        double upper = gtk_adjustment_get_upper( vadjustment );
        double setp = gtk_adjustment_get_step_increment( vadjustment );
        if ( y < 0.1 * allocation.height )
        {
            value = ( ( value - setp > lower ) ? ( value - setp ) : lower );
            gtk_adjustment_set_value( vadjustment , value );
        }
        else if ( y > 0.9 * allocation.height )
        {
            value = ( ( value + setp > upper ) ? upper : ( value + setp ) );
            gtk_adjustment_set_value( vadjustment , value );
        }
    }

    return TRUE;
}

static void drag_begin_handler( GtkWidget * widget , GdkDragContext * context , gpointer data )
{
    ( void )data;
    ( void )context;
    struct ScrollDescription * desc = ( struct ScrollDescription * )data;
    if ( desc->scroll_handler_id == 0 )
    {
        desc->scroll_handler_id = g_timeout_add( 100 , automatic_scroll , desc );
    }

    GtkAllocation alloc;
    gtk_widget_get_allocation( widget , &alloc );
    cairo_surface_t * surface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32 , alloc.width , alloc.height );
    cairo_t * cairo = cairo_create( surface );
    gtk_widget_draw( widget , cairo );
    gtk_drag_set_icon_surface( context , surface );

    cairo_destroy( cairo );
    cairo_surface_destroy( surface );
}

static void drag_end_handler( GtkWidget * widget , GdkDragContext * context , gpointer data )
{
    ( void )widget;
    ( void )context;
    struct ScrollDescription * desc = ( struct ScrollDescription * )data;
    if ( desc->scroll_handler_id != 0 )
    {
        g_source_remove( desc->scroll_handler_id );
        desc->scroll_handler_id = 0;
    }
}

static void darg_data_get_handler( GtkWidget * widget , GdkDragContext * context , GtkSelectionData * data , guint info , guint timestamp , gpointer user_data )
{
    ( void )context;
    ( void )info;
    ( void )timestamp;
    ( void )user_data;
    gtk_selection_data_set( data , gdk_atom_intern_static_string( "SOURCE_WIDGET" ) , 32 , ( const guchar * )&widget , sizeof( gpointer ) );
}

static void darg_data_received_handler( GtkWidget * widget , GdkDragContext * context , gint x , gint y , GtkSelectionData * data , guint info , guint timestamp , gpointer user_data )
{
    ( void )x;
    ( void )y;
    ( void )info;
    ( void )timestamp;
    ( void )user_data;
    GtkWidget * source_widget = *( gpointer * )gtk_selection_data_get_data( data );
    GtkWidget * target_widget = widget;
    if ( source_widget == target_widget )
        return ;
    
    GtkWidget * source_parent = gtk_widget_get_ancestor( source_widget , GTK_TYPE_LAYOUT );
    GtkWidget * target_parent = gtk_widget_get_ancestor( target_widget , GTK_TYPE_LAYOUT );
    if ( ( GTK_IS_LAYOUT( source_parent ) == FALSE ) || ( GTK_IS_LAYOUT( target_parent ) == FALSE ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "drag target not support swap image" );
        return ;
    }

    g_object_ref( source_widget );
    g_object_ref( target_widget );

    GValue source_x = G_VALUE_INIT;
    g_value_init( &source_x , G_TYPE_INT );
    GValue source_y = G_VALUE_INIT;
    g_value_init( &source_y , G_TYPE_INT );
    gtk_container_child_get_property( GTK_CONTAINER( source_parent ) , source_widget , "x" , &source_x );
    gtk_container_child_get_property( GTK_CONTAINER( source_parent ) , source_widget , "y" , &source_y );

    GValue target_x = G_VALUE_INIT;
    g_value_init( &target_x , G_TYPE_INT );
    GValue target_y = G_VALUE_INIT;
    g_value_init( &target_y , G_TYPE_INT );
    gtk_container_child_get_property( GTK_CONTAINER( target_parent ) , target_widget , "x" , &target_x );
    gtk_container_child_get_property( GTK_CONTAINER( target_parent ) , target_widget , "y" , &target_y );

    gtk_container_remove( GTK_CONTAINER( source_parent ) , source_widget );
    gtk_container_remove( GTK_CONTAINER( target_parent ) , target_widget );
    gtk_layout_put( GTK_LAYOUT( source_parent ) , target_widget , g_value_get_int( &source_x ) , g_value_get_int( &source_y ) );
    gtk_layout_put( GTK_LAYOUT( target_parent ) , source_widget , g_value_get_int( &target_x ) , g_value_get_int( &target_y ) );

    g_object_unref( source_widget );
    g_object_unref( target_widget );

    gtk_drag_finish( context , TRUE , TRUE , timestamp );
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
    if ( curl_handle == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_WARNING , "curl init failure." , destination_path );
        return ;
    }
    FILE * download_file = fopen( destination_path , "wb" );
    if ( download_file == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_WARNING , "open file:\'%s\' failure." , destination_path );
        return ;
    }

    int re_try = 4;
    long default_timeout = 30L;
    char error_buff[BUFFSIZE];
    CURLcode res = CURLE_OK;
    do
    {
        curl_easy_setopt( curl_handle , CURLOPT_URL , source_url );
        curl_easy_setopt( curl_handle , CURLOPT_VERBOSE , 0L );
        curl_easy_setopt( curl_handle , CURLOPT_FOLLOWLOCATION , 1L );
        curl_easy_setopt( curl_handle , CURLOPT_NOPROGRESS , 1L );
        curl_easy_setopt( curl_handle , CURLOPT_TIMEOUT , default_timeout );
        curl_easy_setopt( curl_handle , CURLOPT_WRITEFUNCTION , NULL );
        curl_easy_setopt( curl_handle , CURLOPT_ERRORBUFFER , error_buff );
        curl_easy_setopt( curl_handle , CURLOPT_WRITEDATA , download_file );

        res = curl_easy_perform( curl_handle );
        if ( res != CURLE_OK )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "url:\'%s\',error type:\'%s\',error message:\'%s\'" , 
                    source_url , curl_easy_strerror( res ) , error_buff );
            if ( re_try == 0 )
                break;
            default_timeout += 10L;
            re_try--;
            g_usleep( G_USEC_PER_SEC );
        }
    } while ( res != CURLE_OK );

    fclose( download_file );
    curl_easy_cleanup( curl_handle );
    g_free( destination_path );
    g_free( source_url );
}
