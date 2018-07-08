#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <jansson.h>

#define CONFIG_FILE "config.json"
#define BUFFSIZE 1024

enum DeckType
{
    FORGE_FORMAT = 0,
    MTGA_FORMAT,
    GOLDFISH_FORMAT,
    UNKNOWN_FORMAT,
};

enum CardLocal
{
    COMMAND_LOCAL = 0,
    MAIN_LOCAL,
    SIDEBOARD_LOCAL,
};

struct ConfigObject
{
    gchar * TargetRootDirectory;
    gchar * ImageRootDirectory;
    gchar * ImageSuffix;
    gchar * DeckFileDirectory;
    json_int_t WindowWidth;
    json_int_t WindowHeight;
    json_int_t CardWidth;
    json_int_t CardHeight;
    json_int_t LineCardNumber;
    json_int_t TitleFontSize;
    gint HideTitleBar;
    gint CopyFile;
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
static json_int_t get_integer_node( json_t * root , const gchar * nodename );
static gchar * get_string_node( json_t * root, const gchar * nodename );
static gint get_boolean_node( json_t * root , const gchar * nodename );

//do process_deck
static void preview_init( struct DeckObject * deck );
static void preview_add_title( enum CardLocal card_local );
static void preview_add_card( struct CardObject * card );
static void preview_display( void );
static void preview_destroy( void );

static gboolean copy_file( const gchar * source_path , const gchar * destination_path );
static enum DeckType get_deck_type( const gchar * deck_filename );

static GSList * get_cardlist_forge( const gchar * deckfilename );
static GSList * get_cardlist_mtga( const gchar * deckfilename );
static GSList * get_cardlist_goldfish( const gchar * deckfilename );

static GSList * get_cardlist( const gchar * deckfilename );
static void delete_cardlist( GSList * cardlist );
static struct CardObject * allocate_cardobject( void );
static void free_cardobject( struct CardObject * card );
static void remove_forwardslash_forge( gchar * str );
static void remove_forwardslash_mtga( gchar * str );
static gchar * make_imagefile_path( const gchar * cardname , const gchar * cardseries );
static gchar * make_targetfile_path( const char * targetdirectory , const gchar * cardname , const gchar * cardseries , gsize retry_count );
static gboolean get_deckpreview( GtkWidget * window , GdkEvent * event , gpointer data );
static gboolean motion_notify_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data );
static gboolean button_release_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data );
static gboolean button_press_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data );

int main ( int argc, char * argv[] )
{
    gtk_init( &argc, &argv );
    config_inital();

    GSList * decklist = get_deckfile_list( config_object.DeckFileDirectory );

    if ( decklist == NULL )
        return EXIT_FAILURE;
    GSList * temp_ptr = decklist;
    while ( temp_ptr != NULL )
    {
        gchar * deckfile_fullname = ( gchar * )temp_ptr->data;
        gchar * deckfile_shortname = g_strrstr( ( gchar * )temp_ptr->data , "/" );
        if ( deckfile_shortname == NULL )
            //windows path style
            deckfile_shortname = g_strrstr( ( gchar * )temp_ptr->data , "\\" );
        //ASCII incompatible character sets may not work
        deckfile_shortname += 1;
        gchar * targetdirectory = g_strdup_printf( "%s%s" , config_object.TargetRootDirectory , deckfile_shortname );
        gsize targetdirectory_len = strnlen( targetdirectory , BUFFSIZE );
        //****.dck --> ****/$(NUL)"
        targetdirectory[targetdirectory_len-4] = '/';
        targetdirectory[targetdirectory_len-3] = '\0';
        struct DeckObject deck = { deckfile_fullname , targetdirectory };
        if ( remove_directory( deck.targetdirectory ) == FALSE )
            continue;
        if ( make_directory( deck.targetdirectory ) == FALSE )
            continue;
        gint32 copy_success_count = process_deck( &deck );
        if ( config_object.CopyFile == TRUE )
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "deck:\"%s\" successfully copied %"G_GINT32_FORMAT" card images \n" , deckfile_fullname , copy_success_count );
        else
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "set to do not copy files,deck:\"%s\"\n" , deckfile_fullname );
        g_free( targetdirectory );
        temp_ptr = g_slist_next( temp_ptr );
    }

    delete_deck_list( decklist );
    config_destroy();
    return EXIT_SUCCESS;
}

void config_inital( void )
{
    json_t * root;
    json_error_t error;

    root = json_load_file( CONFIG_FILE , 0 , &error );

    if( !root )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: on line %d: %s\n", error.line , error.text );
        exit( EXIT_FAILURE );
    }

    config_object.ImageRootDirectory = get_string_node( root, "ImageRootDirectory" );
    config_object.ImageSuffix = get_string_node( root, "ImageSuffix" );
    config_object.DeckFileDirectory = get_string_node( root, "DeckFileDirectory" );
    config_object.TargetRootDirectory = get_string_node( root , "TargetRootDirectory" );
    config_object.WindowWidth = get_integer_node( root , "WindowWidth" );
    config_object.WindowHeight = get_integer_node( root , "WindowHeight" );
    config_object.CardWidth = get_integer_node( root , "CardWidth" );
    config_object.CardHeight = get_integer_node( root , "CardHeight" );
    config_object.LineCardNumber = get_integer_node( root , "LineCardNumber" );
    config_object.TitleFontSize = get_integer_node( root , "TitleFontSize" );
    config_object.HideTitleBar = get_boolean_node( root , "HideTitleBar" );
    config_object.CopyFile = get_boolean_node( root , "CopyFile" );

    if ( config_object.ImageRootDirectory == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:ImageRootDirectory faliure,no exitst default configuration,programs exit\n" );
        json_decref( root );
        exit( EXIT_FAILURE );
    }
    if ( config_object.ImageSuffix == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:ImageSuffix faliure,use default ImageSuffix:\".jpg\"\n" );
        config_object.ImageSuffix = g_strdup_printf( "%s" , ".jpg" );
    }
    if ( config_object.DeckFileDirectory == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:DeckFileDirectory faliure,use default DeckFileDirectory:\"./\"\n" );
        config_object.DeckFileDirectory = g_strdup_printf( "%s" , "./");
    }
    if ( config_object.TargetRootDirectory == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:TargetRootDirectory faliure,use default TargetRootDirectory:\"./\"\n" );
        config_object.TargetRootDirectory = g_strdup_printf( "%s" , "./");
    }
    if ( config_object.WindowWidth == 0 ) 
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:WindowWidth faliure,use default WindowWidth:1050\n" );
        config_object.WindowWidth = 1050;
    }
    if ( config_object.WindowHeight == 0 )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:WindowHeight faliure,use default WindowHeight:600\n" );
        config_object.WindowHeight = 600;
    }
    if ( config_object.CardWidth == 0 )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:CardWidth faliure,use default CardWidth:70\n" );
        config_object.CardWidth = 70;
    }
    if ( config_object.CardHeight == 0 )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:CardHeight faliure,use default CardHeight:100\n" );
        config_object.CardHeight = 100;
    }
    if ( config_object.LineCardNumber == 0 )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:LineCardNumber faliure,use default LineCardNumber:15\n" );
        config_object.LineCardNumber = 15;
    }
    if ( config_object.TitleFontSize == 0 )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:LineCardNumber faliure,use default LineCardNumber:20\n" );
        config_object.TitleFontSize = 20;
    }
    if ( config_object.HideTitleBar == -1 )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:HideTitleBar faliure,use default HideTitleBar:FALSE\n" );
        config_object.HideTitleBar = FALSE;
    }
    if ( config_object.CopyFile == -1 )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get configuration:CopyFile faliure,use default CopyFile:FALSE\n" );
        config_object.CopyFile = TRUE;
    }

    json_decref( root );
}

void config_destroy( void )
{
    g_free( config_object.ImageRootDirectory );
    g_free( config_object.ImageSuffix );
    g_free( config_object.DeckFileDirectory );
    g_free( config_object.TargetRootDirectory );
}

gint32 process_deck( struct DeckObject * deck )
{
    gint32 copy_success_count = 0;
    preview_init( deck );

    //NULL is empty GSList
    GSList * cardlist = get_cardlist( deck->deckfullname );
    if ( cardlist == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "deck \"%s\" format unknown,parsing stop\n" , deck->deckfullname );
        return 0;
    }

    GSList * temp = cardlist;
    while( temp != NULL )
    {
        struct CardObject * card = ( struct CardObject * )temp->data;
        preview_add_card( card );
        if ( config_object.CopyFile == TRUE )
        {
            gchar * imagefilepath = make_imagefile_path( card->cardname , card->cardseries );;
            gchar * targetfilepath = make_targetfile_path( deck->targetdirectory , card->cardname , card->cardseries , card->cardnumber );;
            gsize rename_count = 1;
            while (  g_file_test( targetfilepath , G_FILE_TEST_EXISTS ) == TRUE )
            {
                g_free( targetfilepath );
                targetfilepath = make_targetfile_path( deck->targetdirectory , card->cardname , card->cardseries , card->cardnumber + rename_count*100 );
                rename_count++;
            }
            if ( copy_file( imagefilepath , targetfilepath ) == TRUE )
                copy_success_count++;
            g_free( imagefilepath );
            g_free( targetfilepath );
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

    //exists and type check
    if ( g_file_test( deck_path , G_FILE_TEST_EXISTS ) != TRUE )
        return NULL;
    if ( g_file_test( deck_path , G_FILE_TEST_IS_DIR ) != TRUE )
        return NULL;

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
    GError * g_error;
    GFile * targetdirectory = g_file_new_for_path( dir );
    if ( g_file_make_directory_with_parents( targetdirectory , NULL , &g_error ) == FALSE )
    {
        g_object_unref( targetdirectory );
        if ( g_error_matches( g_error , G_IO_ERROR_EXISTS, G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "directory %s already exists\n" , dir );
            return FALSE;
        }
        else if ( g_error_matches( g_error , G_IO_ERROR_NOT_SUPPORTED , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "file system does not support creating directories\n" );
            return FALSE;
        }
        else
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "create directory %s unknown error\n" , dir );
            return FALSE;
        }
    }
    g_object_unref( targetdirectory );
    return TRUE;
}

static gchar * get_string_node( json_t * root, const gchar * nodename )
{
    json_t * node = json_object_get( root, nodename );
    if( !json_is_string( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a string node\n", nodename );
        return NULL;
    }
    const gchar * onlyread_string = json_string_value( node );
    gchar * return_string = g_strdup_printf( "%s" , onlyread_string );
    return return_string;
}

static json_int_t get_integer_node( json_t * root , const gchar * nodename )
{
    json_t * node = json_object_get( root , nodename );
    if ( !json_is_integer( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a integer node\n" , nodename );
    }
    //if not integer node json_integer_value return 0
    return json_integer_value( node );
}

static gint get_boolean_node( json_t * root , const gchar * nodename )
{
    json_t * node = json_object_get( root , nodename );
    if ( !json_is_boolean( node ) )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "error: %s is not a boolean node\n" , nodename );
        //identify get value error
        return -1;
    }
    //consistent with the standard boolean behavior
    return json_boolean_value( node );
}

static gboolean copy_file( const gchar * source_path , const gchar * destination_path )
{
    GFile * source = g_file_new_for_path( source_path );
    GFile * destination = g_file_new_for_path( destination_path );
    GError * g_error;
    if ( g_file_copy( source , destination , G_FILE_COPY_NONE , NULL , NULL , NULL , &g_error ) == FALSE )
    {
        g_object_unref( source );
        g_object_unref( destination );
        if ( g_error_matches( g_error , G_IO_ERROR_NOT_FOUND , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s not exist\n" , source_path );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_EXISTS , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s exist\n" , destination_path );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_IS_DIRECTORY , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s is directory\n" , destination_path );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_WOULD_MERGE , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s and %s\n" , source_path , destination_path );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_WOULD_RECURSE , G_IO_ERROR_FAILED ) )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s is directory or \n" , source_path );
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

    GdkPixbuf * icon_pixbuf = gdk_pixbuf_new_from_file( "logo.ico" , NULL );
    if ( icon_pixbuf == NULL )
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s not is image file or not found\n" , "logo.ico" );
    else
        gtk_window_set_icon( GTK_WINDOW( preview_object.window ) , icon_pixbuf );

    if ( config_object.HideTitleBar )
    {
        gtk_window_set_decorated( GTK_WINDOW( preview_object.window ) , FALSE );
    }
    else
        gtk_window_set_title( GTK_WINDOW( preview_object.window ) , deck->deckfullname );

    gtk_window_set_default_size( GTK_WINDOW( preview_object.window ) , ( gint )config_object.WindowWidth , ( gint )config_object.WindowHeight );
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
        title_label_buff = g_strdup_printf(  "<span font='%"G_GINT64_FORMAT"'>main</span>" , config_object.TitleFontSize );
    else if ( card_local == SIDEBOARD_LOCAL )
        title_label_buff = g_strdup_printf( "<span font='%"G_GINT64_FORMAT"'>sideboard</span>" , config_object.TitleFontSize );
    else if ( card_local == COMMAND_LOCAL )
        return ;
    gtk_label_set_markup( GTK_LABEL( title_label ) , title_label_buff );

    GtkWidget * title_box = gtk_event_box_new();
    gtk_widget_set_size_request( GTK_WIDGET( title_box ) , config_object.WindowWidth , config_object.TitleFontSize );
    gtk_container_add( GTK_CONTAINER( title_box ) , title_label );
    
    gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , title_box , 0 , preview_object.layout_height );
    preview_object.layout_height += 2*config_object.TitleFontSize;
    g_free( title_label_buff );
}

static void preview_add_card( struct CardObject * card )
{
    gchar * imagefilepath = make_imagefile_path( card->cardname , card->cardseries );;
    gint32 cardnumber = card->cardnumber;
    while ( cardnumber-- )
    {
        GtkWidget * image = gtk_image_new_from_file( imagefilepath );
        if ( image == NULL )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s not is image file or not found\n" , imagefilepath );
            return ;
        }
        GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
        if ( pixbuf == NULL )
        {
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "%s not is image file or not found\n" , imagefilepath );
            return ;
        }
        pixbuf = gdk_pixbuf_scale_simple( GDK_PIXBUF( pixbuf ) , config_object.CardWidth , config_object.CardHeight , GDK_INTERP_BILINEAR );
        gtk_image_set_from_pixbuf( GTK_IMAGE( image ) , pixbuf );
        switch ( card->card_local )
        {
            case COMMAND_LOCAL:
            {
                gtk_layout_put( GTK_LAYOUT( preview_object.command_layout.layout ) , image , 
                    preview_object.command_layout.width , preview_object.command_layout.height );
                g_array_append_val( preview_object.command_layout.images , image );
                preview_object.command_layout.width += config_object.CardWidth;
                if ( preview_object.command_layout.width == ( config_object.LineCardNumber )*( config_object.CardWidth ) )
                {
                    preview_object.command_layout.width = 0;
                    preview_object.command_layout.height += config_object.CardHeight;
                }
                break;
            }
            case MAIN_LOCAL:
            {
                gtk_layout_put( GTK_LAYOUT( preview_object.main_layout.layout ) , image , 
                    preview_object.main_layout.width , preview_object.main_layout.height );
                g_array_append_val( preview_object.main_layout.images , image );
                preview_object.main_layout.width += config_object.CardWidth;
                if ( preview_object.main_layout.width == ( config_object.LineCardNumber )*( config_object.CardWidth ) )
                {
                    preview_object.main_layout.width = 0;
                    preview_object.main_layout.height += config_object.CardHeight;
                }
                break;
            }
            case SIDEBOARD_LOCAL:
            {
                gtk_layout_put( GTK_LAYOUT( preview_object.sideboard_layout.layout ) , image , 
                    preview_object.sideboard_layout.width , preview_object.sideboard_layout.height );
                g_array_append_val( preview_object.sideboard_layout.images , image );
                preview_object.sideboard_layout.width += config_object.CardWidth;
                if ( preview_object.sideboard_layout.width == ( config_object.LineCardNumber )*( config_object.CardWidth ) )
                {
                    preview_object.sideboard_layout.width = 0;
                    preview_object.sideboard_layout.height += config_object.CardHeight;
                }
                break;
            }
            default:
                break;
        }

    }
    g_free( imagefilepath );
}

static void preview_display( void )
{
    if ( preview_object.command_layout.images->len != 0 )
    {
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.command_layout.layout , 0 , preview_object.layout_height );
        if ( ( preview_object.command_layout.images->len ) % config_object.LineCardNumber != 0 )
            preview_object.command_layout.height += config_object.CardHeight;
        preview_object.layout_height += preview_object.command_layout.height;
    }

    if ( preview_object.main_layout.images->len != 0 ) 
    {
        preview_add_title( MAIN_LOCAL );              
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.main_layout.layout , 0 , preview_object.layout_height );
        if ( ( preview_object.main_layout.images->len ) % config_object.LineCardNumber != 0 )
            preview_object.main_layout.height += config_object.CardHeight;
        preview_object.layout_height += preview_object.main_layout.height;
    }
    
    if ( preview_object.sideboard_layout.images->len != 0 )
    {
        preview_add_title( SIDEBOARD_LOCAL );         
        gtk_layout_put( GTK_LAYOUT( preview_object.layout ) , preview_object.sideboard_layout.layout , 0 , preview_object.layout_height );
        if ( ( preview_object.sideboard_layout.images->len ) % config_object.LineCardNumber != 0 )
            preview_object.sideboard_layout.height += config_object.CardHeight;
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
        config_object.LineCardNumber*config_object.CardWidth , preview_object.command_layout.height );
    gtk_widget_set_size_request( GTK_WIDGET( preview_object.main_layout.layout ) , 
        config_object.LineCardNumber*config_object.CardWidth , preview_object.main_layout.height );
    gtk_widget_set_size_request( GTK_WIDGET( preview_object.sideboard_layout.layout ) , 
        config_object.LineCardNumber*config_object.CardWidth , preview_object.sideboard_layout.height );
    gtk_layout_set_size( GTK_LAYOUT( preview_object.layout ) , config_object.LineCardNumber*config_object.CardWidth , preview_object.layout_height );
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
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure\n" , deck_filename );
        return UNKNOWN_FORMAT;
    }

    GError * g_error = NULL;
    GRegex * forge_regex = g_regex_new( "^([0-9]+)\\ ([^|]+)\\|([^|^\\r^\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( forge_regex == NULL )
        goto parse_err;
    GRegex * mtga_regex = g_regex_new( "^([0-9]+)\\ ([^\\(^\\)]+)\\ \\(([^\\ ]+)\\)\\ ([0-9^\\r^\\n]+)" , 
                        G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( mtga_regex == NULL )
        goto parse_err;
    GRegex * goldfish_regex = g_regex_new( "^([0-9]+)\\ ([^|\\r\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( goldfish_regex == NULL )
        goto parse_err;

    size_t forge_format_count = 0;
    size_t mtga_format_count = 0;
    size_t goldfish_format_count = 0;

    while ( fgets( line_buff , BUFFSIZE , deckfile ) != NULL )
    {
        if ( g_regex_match( forge_regex , line_buff , 0 , NULL ) == TRUE )
        {
            forge_format_count++;
        }
        if ( g_error != NULL )
            goto parse_err;

        if ( g_regex_match( mtga_regex , line_buff , 0 , NULL ) == TRUE )
        {
            mtga_format_count++;
        }
        if ( g_error != NULL )
            goto parse_err;

        //goldfish regex match mtga format deck,so first check mtga format.
        if ( g_regex_match( goldfish_regex , line_buff , 0 , NULL ) == TRUE )
        {
            goldfish_format_count++;
        }
        if ( g_error != NULL )
            goto parse_err;
    }


    g_regex_unref( forge_regex );
    g_regex_unref( mtga_regex );
    g_regex_unref( goldfish_regex );
    fclose( deckfile );
        
    if ( ( forge_format_count >= mtga_format_count ) && ( forge_format_count >= goldfish_format_count ) )
        return FORGE_FORMAT;
    else if ( ( mtga_format_count >= forge_format_count ) && ( mtga_format_count >= goldfish_format_count ) )
        return MTGA_FORMAT;
    else if ( goldfish_format_count != 0 )
        return GOLDFISH_FORMAT;
    //avoid no-return warning,can't use else
    return UNKNOWN_FORMAT;
    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s\n" , g_error->message );
        g_error_free( g_error );
        exit( EXIT_FAILURE );
}

static GSList * get_cardlist( const gchar * deckfilename )
{
    enum DeckType deck_type = get_deck_type( deckfilename );
    switch ( deck_type )
    {
        case FORGE_FORMAT:
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "deck type:%s" , "FORGE DECK" );
            break;
        case MTGA_FORMAT:
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "deck type:%s" , "MTGA DECK" );
            break;
        case GOLDFISH_FORMAT:
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "deck type:%s" , "GOLDFISH DECK" );
            break;
        default:
            g_log( __func__ , G_LOG_LEVEL_MESSAGE , "deck type:%s" , "UNKNOWN DECK" );
    }
    switch ( deck_type )
    {
        case FORGE_FORMAT:
            return get_cardlist_forge( deckfilename );
        case MTGA_FORMAT:
            return get_cardlist_mtga( deckfilename );
        //goldfish regex match mtga format deck,so first check mtga format.
        case GOLDFISH_FORMAT:
            return get_cardlist_goldfish( deckfilename );
        default :
            return NULL;
    }

    return NULL;
}

static GSList * get_cardlist_forge( const gchar * deckfilename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure\n" , deckfilename );
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
            remove_forwardslash_forge( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;
    }
    return cardlist;

    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s\n" , g_error->message );
        g_error_free( g_error );
        exit( EXIT_FAILURE );
}

static GSList * get_cardlist_mtga( const gchar * deckfilename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure\n" , deckfilename );
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
            remove_forwardslash_mtga( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;
    }
    return cardlist;

    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s\n" , g_error->message );
        g_error_free( g_error );
        exit( EXIT_FAILURE );
}

static GSList * get_cardlist_goldfish( const gchar * deckfilename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "open deck:\"%s\" faliure\n" , deckfilename );
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
            remove_forwardslash_forge( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }
        if ( g_error != NULL )
            goto parse_err;
    }
    return cardlist;

    parse_err:
        g_log( __func__ , G_LOG_LEVEL_ERROR , "Error while matching: %s\n" , g_error->message );
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
    if ( card->cardseries != NULL )
        free( card->cardseries );
    if ( card->cardname != NULL )
        free( card->cardname );
    free( card );
}

//forge double-faced,cookies,fusion card name:"Name1 // Name2"
static void remove_forwardslash_forge( gchar * str )
{
    //fast check
    gint is_exists = strrchr( str , '/' ) == NULL;
    if ( is_exists == TRUE )
        return ;
    gsize str_len = strnlen( str, BUFFSIZE );
    gchar * pos_ptr = strstr( str , " // " );
    while( pos_ptr )
    {
        for ( gsize i = pos_ptr - str ; i <= str_len ; i++ )
            str[i] = str[i+4];
        pos_ptr = strstr( str , " // " );
    }
}

//mtga double-faced,cookies,fusion card name:"Name1 /// Name2"
static void remove_forwardslash_mtga( gchar * str )
{
    //fast check
    gint is_exists = strrchr( str , '/' ) == NULL;
    if ( is_exists == TRUE )
        return ;
    gsize str_len = strnlen( str, BUFFSIZE );
    gchar * pos_ptr = strstr( str , " /// " );
    while( pos_ptr )
    {
        for ( gsize i = pos_ptr - str ; i <= str_len ; i++ )
            str[i] = str[i+5];
        pos_ptr = strstr( str , " /// " );
    }
}

static gchar * make_imagefile_path( const gchar * cardname , const gchar * cardseries  )
{
    gchar * imagefilepath = NULL;
    if ( cardname == NULL )
        return imagefilepath;

    const gchar * forgeimagesuffix = ".full"; 
    const gchar * basiclandnamelist[] = 
    {
        "Plains","Island","Swamp","Mountain","Forest",
        "Snow-Covered Plains","Snow-Covered Island",
        "Snow-Covered Swamp","Snow-Covered Mountain",
        "Snow-Covered Forest","Wastes" , NULL
    };

    for ( gsize i = 0 ; basiclandnamelist[i] != NULL ; i++ )
        if ( strncmp( cardname , basiclandnamelist[i] , BUFFSIZE ) == 0 )
            forgeimagesuffix = "1.full";

    if ( cardseries != NULL )
        imagefilepath = g_strdup_printf( "%s%s/%s%s%s" , 
            config_object.ImageRootDirectory , cardseries , cardname , forgeimagesuffix , config_object.ImageSuffix );
    else
        imagefilepath = g_strdup_printf( "%s%s%s%s" , 
            config_object.ImageRootDirectory , cardname , forgeimagesuffix , config_object.ImageSuffix );

    return imagefilepath;
}

static gchar * make_targetfile_path( const char * targetdirectory , const gchar * cardname , const gchar * cardseries , gsize retry_count )
{
    gchar * targetfilepath = NULL;
    if ( cardname == NULL )
        return targetfilepath;
    if ( cardseries != NULL )
        targetfilepath = g_strdup_printf( "%s%s.%s%"G_GSIZE_FORMAT"%s" ,
            targetdirectory , cardname , cardseries , retry_count , config_object.ImageSuffix );
    else
        targetfilepath = g_strdup_printf( "%s%s%"G_GSIZE_FORMAT"%s" ,
            targetdirectory , cardname , retry_count , config_object.ImageSuffix );
    return targetfilepath;
}

static gboolean get_deckpreview( GtkWidget * window , GdkEvent * event , gpointer data )
{
    ( void )event;
    GdkWindow * gdk_window = gtk_widget_get_window( GTK_WIDGET( window ) );
    GdkPixbuf * deckpreview = gdk_pixbuf_get_from_window( GDK_WINDOW( gdk_window ) , 0 , 0 , config_object.WindowWidth , config_object.WindowHeight );
    if ( deckpreview == NULL )
    {
        g_log( __func__ , G_LOG_LEVEL_MESSAGE , "get deck preview buff faliure\n" );
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
        GtkWidget * select_image = g_array_index( widget_layout->images , GtkWidget * , 
            widget_layout->select_x/config_object.CardWidth + widget_layout->select_y/config_object.CardHeight*config_object.LineCardNumber );
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
    if ( x > config_object.CardWidth*config_object.LineCardNumber || y > config_object.CardHeight*widget_layout->height )
    {
        GtkWidget * select_image = g_array_index( widget_layout->images , GtkWidget * , 
            widget_layout->select_x/config_object.CardWidth + widget_layout->select_y/config_object.CardHeight*config_object.LineCardNumber );
        gtk_layout_move( GTK_LAYOUT( widget_layout->layout ) , select_image , widget_layout->select_x/config_object.CardWidth*config_object.CardWidth , widget_layout->select_y/config_object.CardHeight*config_object.CardHeight );
        widget_layout->select_target = FALSE;
        return TRUE;
    }
    if ( ( x == widget_layout->select_x ) && ( y == widget_layout->select_y ) )
    {
        widget_layout->select_target = FALSE;
        return TRUE;
    }
    if ( widget_layout->select_target == TRUE )
    {
        guint start_index = widget_layout->select_x/config_object.CardWidth + widget_layout->select_y/config_object.CardHeight*config_object.LineCardNumber;
        guint end_index = x/config_object.CardWidth + y/config_object.CardHeight*config_object.LineCardNumber;
        GtkWidget * start_image = g_array_index( widget_layout->images , GtkWidget * , start_index );
        if ( start_index > end_index )
        {
            for ( guint i = start_index ; i > end_index ; i-- )
            {
                GtkWidget * select_image = g_array_index( widget_layout->images , GtkWidget * , i - 1 );
                gtk_layout_move( GTK_LAYOUT( widget_layout->layout ) , select_image , 
                    i%( config_object.LineCardNumber )*config_object.CardWidth , i/( config_object.LineCardNumber )*config_object.CardHeight );
                g_array_index( widget_layout->images , GtkWidget * , i ) = select_image;
            }
        }
        else
        {
            for ( guint i = start_index ; i < end_index ; i++ )
            {
                GtkWidget * select_image = g_array_index( widget_layout->images , GtkWidget * , i + 1 );
                gtk_layout_move( GTK_LAYOUT( widget_layout->layout ) , select_image , 
                    i%( config_object.LineCardNumber )*config_object.CardWidth , i/( config_object.LineCardNumber )*config_object.CardHeight );
                g_array_index( widget_layout->images , GtkWidget * , i ) = select_image;
            }
        }
        gtk_layout_move( GTK_LAYOUT( widget_layout->layout ) , start_image , 
                end_index%( config_object.LineCardNumber )*config_object.CardWidth , end_index/( config_object.LineCardNumber )*config_object.CardHeight );
        g_array_index( widget_layout->images , GtkWidget * , end_index ) = start_image;
        widget_layout->select_target = FALSE;
    }
    return TRUE;
}

static gboolean button_press_handle( GtkWidget * widget , GdkEventMotion * event , gpointer data )
{
    ( void )widget;
    struct ImagesLayout * widget_layout = ( struct ImagesLayout * )data;
    int x, y;
    GdkModifierType state;
    gdk_window_get_device_position( event->window , event->device , &x , &y , &state );
    if ( x > config_object.CardWidth*config_object.LineCardNumber || y > config_object.CardHeight*widget_layout->height )
        return TRUE;
    widget_layout->select_x = x;
    widget_layout->select_y = y;
    widget_layout->dx = widget_layout->select_x - x/config_object.CardWidth*config_object.CardWidth;
    widget_layout->dy = widget_layout->select_y - y/config_object.CardWidth*config_object.CardWidth;
    widget_layout->select_target = TRUE;

    return TRUE;
}
