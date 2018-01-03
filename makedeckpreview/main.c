#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <jansson.h>

#define CONFIG_FILE "config.json"
#define BUFFSIZE 1024

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
    guint CloseWindowKeyValue;
    gint AutomaticScaling;
    gint HideTitleBar;
    gint CopyFile;
}config_object;

struct DeckObject
{
    gchar * deckfullname;
    gchar * targetdirectory;
};

enum CardLocal
{
    MAIN = 0,
    SIDEBOARD,
    UNKNOWN_LOCAL,
};

struct PreviewObject
{
    GtkWidget * window;
    GtkWidget * window_icon;
    GdkPixbuf * icon_pixbuf;
    GtkWidget * scrolled;
    GtkWidget * grid;
}preview_object;

struct CardObject
{
    enum CardLocal card_local;
    gchar * cardname;
    gchar * cardseries;
    gint32 cardid;
    gint32 cardnumber;
};

void tool_inital( void );

gint32 process_deck( struct DeckObject * );

void tool_destroy( void );

//do main
static gboolean remove_directory( const gchar * dir );

static gboolean make_directory( const gchar * dir );

//do tool_inital
static json_int_t get_integer_node( json_t * root , const gchar * nodename );

static gchar * get_string_node( json_t * root, const gchar * nodename );

static gint get_boolean_node( json_t * root , const gchar * nodename );

//do process_deck
static void preview_init( struct DeckObject * deck );

static void preview_add_title( enum CardLocal card_local , gint32 * postion_count );

static void preview_add_card( struct CardObject * card , gint32 * postion_count );

static void preview_display( void );

static void preview_destroy( void );

static gboolean copy_file( const gchar * source_path , const gchar * destination_path );

static GSList * get_cardlist( const gchar * deckfilename );

static void delete_cardlist( GSList * cardlist );

static struct CardObject * allocate_cardobject( void );

static void free_cardobject( struct CardObject * card );

static void remove_forwardslash( gchar * str );

static gchar * make_imagefile_path( const gchar * cardname , const gchar * cardseries );

static gchar * make_targetfile_path( const char * targetdirectory , const gchar * cardname , const gchar * cardseries , gsize retry_count );

static gboolean get_deckpreview( GtkWidget * window , GdkEvent * event , gpointer data );

static gboolean on_key_press( GtkWidget * widget , GdkEventKey * event , gpointer data );

int main ( int argc, char * argv[] )
{
    gtk_init( &argc, &argv );
    tool_inital();

    GSList * decklist = NULL;
    GtkWindow * window = GTK_WINDOW( gtk_window_new( GTK_WINDOW_TOPLEVEL ) );
    GtkFileChooserDialog * dialog = GTK_FILE_CHOOSER_DIALOG( gtk_file_chooser_dialog_new( 
        "select deck file" , window , GTK_FILE_CHOOSER_ACTION_OPEN , "cancel" ,
        GTK_RESPONSE_CANCEL , "confirm" , GTK_RESPONSE_ACCEPT , NULL ) );
    gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER( dialog ) , TRUE );
    gtk_file_chooser_set_uri( GTK_FILE_CHOOSER( dialog ) , config_object.DeckFileDirectory );
    gint ret_code = gtk_dialog_run( GTK_DIALOG( dialog ) );
    if ( ret_code == GTK_RESPONSE_ACCEPT )
    {
        decklist = gtk_file_chooser_get_filenames( GTK_FILE_CHOOSER( dialog ) );
    }
    if ( decklist == NULL )
        return EXIT_FAILURE;
    GSList * temp_ptr = decklist;
    gtk_widget_destroy( GTK_WIDGET( dialog ) );
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
            g_message( "deck:\"%s\" successfully copied %"G_GINT32_FORMAT" card images \n" , deckfile_fullname , copy_success_count );
        else
            g_message( "set to do not copy files,deck:\"%s\"\n" , deckfile_fullname );
        g_free( targetdirectory );
        temp_ptr = g_slist_next( temp_ptr );
    }
    g_slist_free_full( decklist , ( GDestroyNotify )g_free );
    //delete_deck_list( decklist );
    tool_destroy();
    return EXIT_SUCCESS;
}

void tool_inital( void )
{
    json_t * root;
    json_error_t error;

    root = json_load_file( CONFIG_FILE , 0 , &error );

    if( !root )
    {
        g_message( "error: on line %d: %s\n", error.line , error.text );
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
    config_object.CloseWindowKeyValue = ( guint )get_integer_node( root , "CloseWindowKeyValue" );
    config_object.AutomaticScaling = get_boolean_node( root , "AutomaticScaling" );
    config_object.HideTitleBar = get_boolean_node( root , "HideTitleBar" );
    config_object.CopyFile = get_boolean_node( root , "CopyFile" );

    if ( config_object.ImageRootDirectory == NULL )
    {
        g_message( "get configuration:ImageRootDirectory faliure,no exitst default configuration,programs exit\n" );
        json_decref( root );
        exit( EXIT_FAILURE );
    }
    if ( config_object.ImageSuffix == NULL )
    {
        g_message( "get configuration:ImageSuffix faliure,use default ImageSuffix:\".jpg\"\n" );
        config_object.ImageSuffix = g_strdup_printf( "%s" , ".jpg" );
    }
    if ( config_object.DeckFileDirectory == NULL )
    {
        g_message( "get configuration:DeckFileDirectory faliure,use default DeckFileDirectory:\"./\"\n" );
        config_object.DeckFileDirectory = g_strdup_printf( "%s" , "./");
    }
    if ( config_object.TargetRootDirectory == NULL )
    {
        g_message( "get configuration:TargetRootDirectory faliure,use default TargetRootDirectory:\"./\"\n" );
        config_object.TargetRootDirectory = g_strdup_printf( "%s" , "./");
    }
    if ( config_object.WindowWidth == 0 ) 
    {
        g_message( "get configuration:WindowWidth faliure,use default WindowWidth:1050\n" );
        config_object.WindowWidth = 1050;
    }
    if ( config_object.WindowHeight == 0 )
    {
        g_message( "get configuration:WindowHeight faliure,use default WindowHeight:600\n" );
        config_object.WindowHeight = 600;
    }
    if ( config_object.CardWidth == 0 )
    {
        g_message( "get configuration:CardWidth faliure,use default CardWidth:70\n" );
        config_object.CardWidth = 70;
    }
    if ( config_object.CardHeight == 0 )
    {
        g_message( "get configuration:CardHeight faliure,use default CardHeight:100\n" );
        config_object.CardHeight = 100;
    }
    if ( config_object.LineCardNumber == 0 )
    {
        g_message( "get configuration:LineCardNumber faliure,use default LineCardNumber:15\n" );
        config_object.LineCardNumber = 15;
    }
    if ( config_object.TitleFontSize == 0 )
    {
        g_message( "get configuration:LineCardNumber faliure,use default LineCardNumber:20\n" );
        config_object.TitleFontSize = 20;
    }
    if ( config_object.CloseWindowKeyValue == 0 )
    {
        g_message( "get configuration:CloseWindowKeyValue faliure,use default CloseWindowKeyValue:32(space)\n" );
        config_object.CloseWindowKeyValue = 32;
    }
    if ( config_object.AutomaticScaling == -1 )
    {
        g_message( "get configuration:AutomaticScaling faliure,use default AutomaticScaling:FALSE\n" );
        config_object.AutomaticScaling = FALSE;
    }
    if ( config_object.HideTitleBar == -1 )
    {
        g_message( "get configuration:AutomaticScaling faliure,use default HideTitleBar:FALSE\n" );
        config_object.HideTitleBar = FALSE;
    }
    if ( config_object.CopyFile == -1 )
    {
        g_message( "get configuration:AutomaticScaling faliure,use default CopyFile:FALSE\n" );
        config_object.CopyFile = TRUE;
    }

    json_decref( root );
}

void tool_destroy( void )
{
    g_free( config_object.ImageRootDirectory );
    g_free( config_object.ImageSuffix );
    g_free( config_object.DeckFileDirectory );
    g_free( config_object.TargetRootDirectory );
}

gint32 process_deck( struct DeckObject * deck )
{
    gint32 copy_success_count = 0;
    gint32 postion_count = 0;
    preview_init( deck );

    //NULL is empty GSList
    GSList * cardlist = get_cardlist( deck->deckfullname );
    if ( cardlist == NULL )
    {
        g_message( "deck \"%s\" format unknown,parsing stop\n" , deck->deckfullname );
        return 0;
    }

    enum CardLocal card_local = UNKNOWN_LOCAL;
    GSList * temp = cardlist;
    while( temp != NULL )
    {
        struct CardObject * card = ( struct CardObject * )temp->data;
        if ( card_local != card->card_local )
        {
            card_local = card->card_local;
            preview_add_title( card_local , &postion_count );
        }
        preview_add_card( card , &postion_count );
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
            g_message( "directory %s already exists\n" , dir );
            return FALSE;
        }
        else if ( g_error_matches( g_error , G_IO_ERROR_NOT_SUPPORTED , G_IO_ERROR_FAILED ) )
        {
            g_message( "file system does not support creating directories\n" );
            return FALSE;
        }
        else
        {
            g_message( "create directory %s unknown error\n" , dir );
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
        g_message( "error: %s is not a string node\n", nodename );
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
        g_message( "error: %s is not a integer node\n" , nodename );
    }
    //if not integer node json_integer_value return 0
    return json_integer_value( node );
}

static gint get_boolean_node( json_t * root , const gchar * nodename )
{
    json_t * node = json_object_get( root , nodename );
    if ( !json_is_boolean( node ) )
    {
        g_message( "error: %s is not a boolean node\n" , nodename );
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
            g_message( "%s not exist\n" , source_path );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_EXISTS , G_IO_ERROR_FAILED ) )
        {
            g_message( "%s exist\n" , destination_path );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_IS_DIRECTORY , G_IO_ERROR_FAILED ) )
        {
            g_message( "%s is directory\n" , destination_path );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_WOULD_MERGE , G_IO_ERROR_FAILED ) )
        {
            g_message( "%s and %s\n" , source_path , destination_path );
            return FALSE;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_WOULD_RECURSE , G_IO_ERROR_FAILED ) )
        {
            g_message( "%s is directory or \n" , source_path );
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
    preview_object.window_icon = gtk_image_new_from_file( "logo.ico" );
    if ( preview_object.window_icon == NULL )
    {
        g_print( "%s not is image file or not found\n" , "logo.ico" );
    }
    else
    {
        preview_object.icon_pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( preview_object.window_icon ) );
        if ( preview_object.icon_pixbuf == NULL )
            g_print( "%s not is image file or not found\n" , "logo.ico" );
        else
            gtk_window_set_icon( GTK_WINDOW( preview_object.window ) , preview_object.icon_pixbuf );
    }
    if ( config_object.HideTitleBar )
    {
        g_signal_connect( G_OBJECT( preview_object.window ) , "key-press-event" , G_CALLBACK( on_key_press ) , NULL );
        gtk_window_set_decorated( GTK_WINDOW( preview_object.window ) , FALSE );
    }
    else
        gtk_window_set_title( GTK_WINDOW( preview_object.window ) , deck->deckfullname );
    gtk_window_set_default_size( GTK_WINDOW( preview_object.window ) , ( gint )config_object.WindowWidth , ( gint )config_object.WindowHeight );
    g_signal_connect( G_OBJECT( preview_object.window ) , "delete-event", G_CALLBACK( get_deckpreview ) , deck->targetdirectory );
    preview_object.scrolled = gtk_scrolled_window_new( NULL , NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( preview_object.scrolled ) , GTK_POLICY_AUTOMATIC , GTK_POLICY_AUTOMATIC );
    preview_object.grid = gtk_grid_new();
    if ( config_object.AutomaticScaling == TRUE )
    {
        gtk_grid_set_row_homogeneous( GTK_GRID( preview_object.grid ) , TRUE );
        gtk_grid_set_column_homogeneous( GTK_GRID( preview_object.grid ) , TRUE );
    }
    gtk_container_add( GTK_CONTAINER( preview_object.scrolled ) , preview_object.grid );
    gtk_container_add( GTK_CONTAINER( preview_object.window ) , preview_object.scrolled );
}

static void preview_add_title( enum CardLocal card_local , gint32 * postion_count )
{
    gchar * title_label_buff = NULL;
    GtkWidget * title_label = gtk_label_new( NULL );
    if ( card_local == MAIN )
        title_label_buff = g_strdup_printf(  "<span font='%"G_GINT64_FORMAT"'>main</span>" , config_object.TitleFontSize );
    else if ( card_local == SIDEBOARD )
        title_label_buff = g_strdup_printf( "<span font='%"G_GINT64_FORMAT"'>sideboard</span>" , config_object.TitleFontSize );
    else if ( card_local == UNKNOWN_LOCAL )
        return ;
    gtk_label_set_markup( GTK_LABEL( title_label ) , title_label_buff );
    GtkWidget * title_box = gtk_event_box_new();
    gtk_container_add( GTK_CONTAINER( title_box ) , title_label );
    *postion_count += config_object.LineCardNumber - *postion_count%config_object.LineCardNumber;
    gtk_grid_attach( GTK_GRID( preview_object.grid ), title_box , *postion_count%config_object.LineCardNumber , 
        *postion_count/config_object.LineCardNumber ,  config_object.LineCardNumber , 1 );
    *postion_count += config_object.LineCardNumber;
    g_free( title_label_buff );
}

static void preview_add_card( struct CardObject * card , gint32 * postion_count )
{
    gchar * imagefilepath = make_imagefile_path( card->cardname , card->cardseries );;
    gint32 cardnumber = card->cardnumber;
    while ( cardnumber-- )
    {
        GtkWidget * image = gtk_image_new_from_file( imagefilepath );
        if ( image == NULL )
        {
            g_print( "%s not is image file or not found\n" , imagefilepath );
            return ;
        }
        GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
        if ( pixbuf == NULL )
        {
            g_print( "%s not is image file or not found\n" , imagefilepath );
            return ;
        }
        pixbuf = gdk_pixbuf_scale_simple( GDK_PIXBUF( pixbuf ) , config_object.CardWidth , config_object.CardHeight , GDK_INTERP_BILINEAR );
        gtk_image_set_from_pixbuf( GTK_IMAGE( image ) , pixbuf );
        gtk_grid_attach( GTK_GRID(  preview_object.grid ) , image , *postion_count%config_object.LineCardNumber , *postion_count/config_object.LineCardNumber , 1 , 1 );
        *postion_count += 1;
    }
    g_free( imagefilepath );
}

static void preview_display( void )
{
    gtk_widget_show_all( GTK_WIDGET( preview_object.window ) );
    gtk_main();
}

static void preview_destroy( void )
{
    gtk_widget_destroy( GTK_WIDGET( preview_object.window ) );
}

static GSList * get_cardlist( const gchar * deckfilename )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_message( "open deck:\"%s\" faliure\n" , deckfilename );
        return NULL;
    }

    GSList * cardlist = NULL;
    gint32 card_local = UNKNOWN_LOCAL;
    while ( fgets( line_buff , BUFFSIZE , deckfile ) != NULL )
    {
        if ( ( strncmp( line_buff , "[Main]" , 6 ) == 0 ) || ( strncmp( line_buff , "[main]" , 6 ) == 0 ) )
            card_local = MAIN;
        if ( ( strncmp( line_buff , "[Sideboard]" , 11 ) == 0 ) || ( strncmp( line_buff , "[sideboard]" , 11 ) == 0 ) )
            card_local = SIDEBOARD;

        GError * g_error = NULL;
        GMatchInfo * match_info;
        //forge deck format regex
        GRegex * regex = g_regex_new( "^([0-9]+)\\ ([^|]+)\\|([^|^\\r^\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
        if ( regex == NULL )
            goto parse_err;
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
            remove_forwardslash( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }

        g_error = NULL;
        //goldfish deck format regex
        regex = g_regex_new( "^([0-9]+)\\ ([^|\\r\\n]+)" , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
        if ( regex == NULL )
            goto parse_err;
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
            remove_forwardslash( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }

        g_error = NULL;
        //mtga deck format regex
        regex = g_regex_new( "^([0-9]+)\\ ([^\\(^\\)]+)\\ \\(([^\\ ]+)\\)\\ ([0-9^\\r^\\n]+)" , 
                            G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
        if ( regex == NULL )
            goto parse_err;
        if ( g_regex_match( regex , line_buff , 0 , &match_info ) == TRUE )
        {
            struct CardObject * card = allocate_cardobject();
            gchar ** words = g_regex_split( regex , line_buff , 0 );
            gint32 cardnumber = g_ascii_strtoll( words[1] , NULL , 10 );
            gint32 cardid = g_ascii_strtoll( words[4] , NULL , 10 );;
            card->card_local = card_local;
            card->cardnumber = cardnumber;
            g_strlcat( card->cardname , words[2] , BUFFSIZE );
            g_strlcat( card->cardseries , words[3] , BUFFSIZE );
            card->cardid = cardid;
            remove_forwardslash( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( gchar ** )words );
            g_regex_unref( regex );
            continue;
        }
    parse_err:
        if ( g_error != NULL )
        {
            g_printerr( "Error while matching: %s\n" , g_error->message );
            g_error_free( g_error );
            exit( EXIT_FAILURE );
        }
    }

    fclose( deckfile );
    return cardlist;
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
static void remove_forwardslash( gchar * str )
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
        g_message( "get deck preview buff faliure\n" );
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

static gboolean on_key_press( GtkWidget * widget , GdkEventKey * event , gpointer data )
{
    ( void )widget;
    ( void )data;
    if( event->keyval == config_object.CloseWindowKeyValue )
        gtk_main_quit();
    return FALSE;
}
