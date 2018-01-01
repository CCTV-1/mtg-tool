#include <inttypes.h>
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
    gboolean targetrootdirectory_nofree;
    gchar * TargetDirectory;
    gchar * ImageRootDirectory;
    gchar * ImageSuffix;
    gboolean imagesuffix_nofree;
    gchar * DeckFileDirectory;
    gboolean deckfiledirectory_nofree;
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

enum CardLocal
{
    MAIN = 0,
    SIDEBOARD = 1,
    UNKNOWN_LOCAL = G_MAXINT32,
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

gint32 process_deck( const gchar * deckfilename );

void tool_destroy( void );

//do main
static gboolean remove_directory( const gchar * dir );

static gboolean make_directory( const gchar * dir );

//do tool_inital
static json_int_t get_integer_node( json_t * root , const gchar * nodename );

static gchar * get_string_node( json_t * root, const gchar * nodename );

static gint get_boolean_node( json_t * root , const gchar * nodename );

//do process_deck
static void preview_init( const gchar * deckfilename );

static void preview_add_title( enum CardLocal card_local , gint32 * postion_count );

static void preview_add_card( struct CardObject * card , gint32 * postion_count );

static void preview_display( void );

static void preview_destroy( void );

static gboolean copy_file( const gchar * source_path , const gchar * destination_path );

static GSList * get_cardlist( const gchar * deckfilename , GSList * cardlist );

static struct CardObject * allocate_cardobject( void );

static void free_cardobject( struct CardObject * card );

static void remove_forwardslash( gchar * str );

static gboolean makeimagefilepath( gchar ** imagefilepath , const gchar * cardname , const gchar * cardseries );

static gboolean maketargetfilepath( gchar ** targetfilepath , const gchar * cardname , const gchar * cardseries , gsize retry_count );

static gboolean get_deckpreview( GtkWidget * window );

static gboolean on_key_press( GtkWidget * widget , GdkEventKey * event , gpointer data );

gint main ( gint argc, gchar * argv[] )
{
    gtk_init( &argc, &argv );
    tool_inital();

    gchar * deckfile_fullname;
    const gchar * deckfile_shortname;
    GDir * dir_ptr;
    if ( g_file_test( config_object.DeckFileDirectory , G_FILE_TEST_EXISTS ) != TRUE )
    {
        g_message( "%s not exitst\n" , config_object.DeckFileDirectory );
        return EXIT_FAILURE;
    }
    if ( g_file_test( config_object.DeckFileDirectory , G_FILE_TEST_IS_DIR ) != TRUE )
    {
        g_message( "%s not is directory\n" , config_object.DeckFileDirectory );
        return EXIT_FAILURE;
    }
    dir_ptr = g_dir_open( config_object.DeckFileDirectory , 0 , NULL );
    while( ( deckfile_shortname = g_dir_read_name ( dir_ptr ) ) != NULL )
    {
        deckfile_fullname = g_strdup_printf( "%s%s" , config_object.DeckFileDirectory , deckfile_shortname );
        if ( g_file_test( deckfile_fullname , G_FILE_TEST_IS_DIR ) == TRUE )
            continue;
        config_object.TargetDirectory = g_strdup_printf( "%s%s" , config_object.TargetRootDirectory , deckfile_shortname );
        gsize targetdirectory_len = strnlen( config_object.TargetDirectory , BUFFSIZE );
        //****.dck --> ****/$(NUL)"
        config_object.TargetDirectory[targetdirectory_len-4] = '/';
        config_object.TargetDirectory[targetdirectory_len-3] = '\0';
        if ( remove_directory( config_object.TargetDirectory ) != TRUE )
            continue;
        if ( make_directory( config_object.TargetDirectory  ) == FALSE )
            continue;
        gint32 copy_success_count = process_deck( deckfile_fullname );
        if ( config_object.CopyFile == TRUE )
            g_message( "deck:\"%s\" successfully copied %"PRId32" card images \n" , deckfile_fullname , copy_success_count );
        else
            g_message( "set to do not copy files,deck:\"%s\"\n" , deckfile_fullname );
        g_free( deckfile_fullname );
        g_free( config_object.TargetDirectory );
    }
    g_dir_close( dir_ptr );
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
    config_object.imagesuffix_nofree = FALSE;
    config_object.DeckFileDirectory = get_string_node( root, "DeckFileDirectory" );
    config_object.deckfiledirectory_nofree = FALSE;
    config_object.TargetRootDirectory = get_string_node( root , "TargetRootDirectory" );
    config_object.targetrootdirectory_nofree = FALSE;
    //runtime get
    config_object.TargetDirectory = NULL;
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
        config_object.ImageSuffix = ".jpg";
        config_object.imagesuffix_nofree = TRUE;
    }
    if ( config_object.DeckFileDirectory == NULL )
    {
        g_message( "get configuration:DeckFileDirectory faliure,use default DeckFileDirectory:\"./\"\n" );
        config_object.DeckFileDirectory = "./";
        config_object.deckfiledirectory_nofree = TRUE;
    }
    if ( config_object.TargetRootDirectory == NULL )
    {
        g_message( "get configuration:TargetRootDirectory faliure,use default TargetRootDirectory:\"./\"\n" );
        config_object.TargetRootDirectory = "./";
        config_object.targetrootdirectory_nofree = TRUE;
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

gint32 process_deck( const gchar * deckfilename )
{
    gint32 copy_success_count = 0;
    gint32 postion_count = 0;
    preview_init( deckfilename );

    //NULL is empty GSList
    GSList * cardlist = get_cardlist( deckfilename , NULL );
    if ( cardlist == NULL )
    {
        g_message( "deck \"%s\" format unknown,parsing stop\n" , deckfilename );
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
            gchar * imagefilepath = NULL;
            gchar * targetfilepath = NULL;
            gsize rename_count = 1;
            makeimagefilepath( &imagefilepath , card->cardname , card->cardseries );
            maketargetfilepath( &targetfilepath , card->cardname , card->cardseries , card->cardnumber );
            while (  g_file_test( targetfilepath , G_FILE_TEST_EXISTS ) == TRUE )
            {
                g_free( targetfilepath );
                maketargetfilepath( &targetfilepath , card->cardname , card->cardseries , card->cardnumber + rename_count*100 );
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
    g_slist_free_full( cardlist , ( GDestroyNotify )free_cardobject );

    return copy_success_count;
}

void tool_destroy( void )
{
    free( config_object.ImageRootDirectory );
    if ( config_object.imagesuffix_nofree != TRUE )
        free( config_object.ImageSuffix );
    if ( config_object.deckfiledirectory_nofree != TRUE )
        free( config_object.DeckFileDirectory );
    if ( config_object.targetrootdirectory_nofree != TRUE )
        free( config_object.TargetRootDirectory );
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
    gsize string_len = strnlen( onlyread_string , BUFFSIZE ) + 1;
    gchar * return_string = ( gchar * )malloc( string_len*sizeof( gchar ) );
    if ( return_string == NULL )
    {
        perror( "allocate configuration:" );
        return NULL;
    }
    g_strlcpy( return_string, onlyread_string, string_len );
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

static void preview_init( const gchar * deckfilename )
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
        gtk_window_set_title( GTK_WINDOW( preview_object.window ) , deckfilename );
    gtk_window_set_default_size( GTK_WINDOW( preview_object.window ), ( gint )config_object.WindowWidth , ( gint )config_object.WindowHeight );
    g_signal_connect( G_OBJECT( preview_object.window ), "delete_event", G_CALLBACK( get_deckpreview ) , preview_object.window );
    preview_object.scrolled = gtk_scrolled_window_new( NULL, NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( preview_object.scrolled ), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
    preview_object.grid = gtk_grid_new();
    if ( config_object.AutomaticScaling == TRUE )
    {
        gtk_grid_set_row_homogeneous( GTK_GRID( preview_object.grid ) , TRUE );
        gtk_grid_set_column_homogeneous( GTK_GRID( preview_object.grid ) , TRUE );
    }
    gtk_container_add( GTK_CONTAINER( preview_object.scrolled ), preview_object.grid );
    gtk_container_add( GTK_CONTAINER( preview_object.window ), preview_object.scrolled );
}

static void preview_add_title( enum CardLocal card_local , gint32 * postion_count )
{
    gchar * title_label_buff = NULL;
    GtkWidget * title_label = gtk_label_new( NULL );
    if ( card_local == MAIN )
        title_label_buff = g_strdup_printf(  "<span font='%"PRId64"'>main</span>" , config_object.TitleFontSize );
    else if ( card_local == SIDEBOARD )
        title_label_buff = g_strdup_printf( "<span font='%"PRId64"'>sideboard</span>" , config_object.TitleFontSize );
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
    gchar * imagefilepath = NULL;
    gint32 cardnumber = card->cardnumber;
    makeimagefilepath( &imagefilepath , card->cardname , card->cardseries );
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

static GSList * get_cardlist( const gchar * deckfilename , GSList * cardlist )
{
    gchar line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        g_message( "open deck:\"%s\" faliure\n" , deckfilename );
        return NULL;
    }

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
            gchar * cardnumber_str = words[1];
            gint32 cardnumber = 0;
            for ( gsize i = 0 ; i < strnlen( cardnumber_str , BUFFSIZE ) ; i++ )
                cardnumber = 10*cardnumber + line_buff[i] - '0';
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
            gchar * cardnumber_str = words[1];
            gint32 cardnumber = 0;
            for ( gsize i = 0 ; i < strnlen( cardnumber_str , BUFFSIZE ) ; i++ )
                cardnumber = 10*cardnumber + line_buff[i] - '0';
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
            gchar * cardnumber_str = words[1];
            gchar * cardid_str = words[4];
            gint32 cardnumber = 0;
            //G_MAXINT32 is unknown id
            gint32 cardid = G_MAXINT32;
            for ( gsize i = 0 ; i < strnlen( cardnumber_str , BUFFSIZE ) ; i++ )
                cardnumber = 10*cardnumber + line_buff[i] - '0';
            for ( gsize i = 0 ; i < strnlen( cardid_str , BUFFSIZE ) ; i++ )
                cardid = 10*cardid + line_buff[i] - '0';
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

        continue;

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

static gboolean makeimagefilepath( gchar ** imagefilepath , const gchar * cardname , const gchar * cardseries  )
{
    if ( cardname == NULL )
        return FALSE;

    const gchar * forgeimagesuffix = ".full"; 
    const gchar * basiclandnamelist[] = 
    {
        "Plains","Island","Swamp","Mountain","Forest",
        "Snow-Covered Plains","Snow-Covered Island",
        "Snow-Covered Swamp","Snow-Covered Mountain",
        "Snow-Covered Forest","Wastes"
    };

    for ( gsize i = 0 ; i < ( sizeof( basiclandnamelist )/sizeof( basiclandnamelist[0] ) ) ; i++ )
        if ( strncmp( cardname , basiclandnamelist[i] , BUFFSIZE ) == 0 )
            forgeimagesuffix = "1.full";

    if ( cardseries != NULL )
        *imagefilepath = g_strdup_printf( "%s%s/%s%s%s" , 
            config_object.ImageRootDirectory , cardseries , cardname , forgeimagesuffix , config_object.ImageSuffix );
    else
        *imagefilepath = g_strdup_printf( "%s%s%s%s" , 
            config_object.ImageRootDirectory , cardname , forgeimagesuffix , config_object.ImageSuffix );

    return TRUE;
}

static gboolean maketargetfilepath( gchar ** targetfilepath , const gchar * cardname , const gchar * cardseries , gsize retry_count )
{
    if ( cardname == NULL )
        return FALSE;

    if ( cardseries != NULL )
        *targetfilepath = g_strdup_printf( "%s%s.%s%"PRIu64"%s" ,
            config_object.TargetDirectory , cardname , cardseries , retry_count , config_object.ImageSuffix );
    else
        *targetfilepath = g_strdup_printf( "%s%s%"PRIu64"%s" ,
            config_object.TargetDirectory , cardname , retry_count , config_object.ImageSuffix );
    return TRUE;
}

static gboolean get_deckpreview( GtkWidget * window )
{
    GdkWindow * gdk_window = gtk_widget_get_window( GTK_WIDGET( window ) );
    GdkPixbuf * deckpreview = gdk_pixbuf_get_from_window( GDK_WINDOW( gdk_window ) , 0 , 0 , config_object.WindowWidth , config_object.WindowHeight );
    if ( deckpreview == NULL )
    {
        g_message( "get deck preview buff faliure\n" );
    }
    else
    {
        gchar * previewname = g_strdup_printf( "%sdeckpreview.jpg" , config_object.TargetDirectory );
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
