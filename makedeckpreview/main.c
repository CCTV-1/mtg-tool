#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <jansson.h>

#define CONFIG_FILE "config.json"
#define BUFFSIZE 1024

struct ConfigObject
{
    char * TargetRootDirectory;
    bool targetrootdirectory_nofree;
    char * TargetDirectory;
    char * ImageRootDirectory;
    char * ImageSuffix;
    bool imagesuffix_nofree;
    char * DeckFileDirectory;
    bool deckfiledirectory_nofree;
    json_int_t WindowWidth;
    json_int_t WindowHeight;
    json_int_t CardWidth;
    json_int_t CardHeight;
    json_int_t LineCardNumber;
    json_int_t TitleFontSize;
    guint CloseWindowKeyValue;
    int AutomaticScaling;
    int HideTitleBar;
    int CopyFile;
}config_object;

enum CardLocal
{
    MAIN = 0,
    SIDEBOARD = 1,
    UNKNOWN_LOCAL = INT32_MAX,
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
    char * cardname;
    char * cardseries;
    int32_t cardid;
    int32_t cardnumber;
};

void tool_inital( void );

int32_t process_deck( const char * deckfilename );

void tool_destroy( void );

//do main
static bool remove_directory( const char * dir );

static bool make_directory( const char * dir );

//do tool_inital
static json_int_t get_integer_node( json_t * root , const char * nodename );

static char * get_string_node( json_t * root, const char * nodename );

static int get_boolean_node( json_t * root , const char * nodename );

//do process_deck
static void preview_init( const char * deckfilename );

static void preview_add_title( enum CardLocal card_local , int32_t * postion_count );

static void preview_add_card( struct CardObject * card , int32_t * postion_count );

static void preview_display( void );

static void preview_destroy( void );

static bool copy_file( const char * source_path , const char * destination_path );

static GSList * get_cardlist( const char * deckfilename , GSList * cardlist );

static struct CardObject * allocate_cardobject( void );

static void free_cardobject( struct CardObject * card );

static void remove_forwardslash( char * str );

static bool makeimagefilepath( char * imagefilepath , size_t path_maxlen , const char * cardname , const char * cardseries );

static bool maketargetfilepath( char * targetfilepath , size_t path_maxlen , const char * cardname , const char * cardseries , size_t retry_count );

static bool get_deckpreview( GtkWidget * window );

static bool on_key_press( GtkWidget * widget , GdkEventKey * event , gpointer data );

int main ( int argc, char * argv[] )
{
    gtk_init( &argc, &argv );
    tool_inital();
    char deckfilename[BUFFSIZE];
    DIR * dir_ptr = opendir( config_object.DeckFileDirectory );
    struct dirent * dirent_ptr;
    struct stat dir_stat;
    struct stat deckfile_stat;

    if ( access( config_object.DeckFileDirectory , F_OK ) != 0 )
    {
        fprintf( stderr , "%s not exitst,errno:%d\n" , config_object.DeckFileDirectory , errno );
        return EXIT_FAILURE;
    }
    
    if ( stat( config_object.DeckFileDirectory , &dir_stat ) < 0 )
    {
        perror( "get deckfiledirectory stat:" );
        return EXIT_FAILURE;
    }

    if ( !S_ISDIR( dir_stat.st_mode ) )
    {
        fprintf( stderr , "%s not is directory\n" , config_object.DeckFileDirectory );
        return EXIT_FAILURE;
    }
    else
    {
        dir_ptr = opendir( config_object.DeckFileDirectory );
        while( ( dirent_ptr = readdir( dir_ptr ) ) != NULL )
        {
            if ( ( strncmp( "." , dirent_ptr->d_name , 1  ) == 0 ) || ( strncmp( ".." , dirent_ptr->d_name , 2 ) == 0 ) )
                continue;
            else
            {
                sprintf( deckfilename , "%s%s" , config_object.DeckFileDirectory , dirent_ptr->d_name );
                if ( stat( deckfilename , &deckfile_stat ) < 0 )
                    continue;
                if ( S_ISDIR( deckfile_stat.st_mode ) )
                    continue;
                else
                {
                    sprintf( config_object.TargetDirectory , "%s%s" , config_object.TargetRootDirectory , dirent_ptr->d_name );
                    size_t targetdirectory_len = strnlen( config_object.TargetDirectory , BUFFSIZE );
                    //****.dck --> ****/$(NUL)"
                    config_object.TargetDirectory[targetdirectory_len-4] = '/';
                    config_object.TargetDirectory[targetdirectory_len-3] = '\0';
                    if ( remove_directory( config_object.TargetDirectory ) != true )
                        continue;
                    if ( make_directory( config_object.TargetDirectory  ) == false )
                        continue;
                    int32_t copy_success_count = process_deck( deckfilename );
                    if ( config_object.CopyFile == true )
                        fprintf( stdout , "deck:\"%s\" successfully copied %"PRId32" card images \n" , deckfilename , copy_success_count );
                    else
                        fprintf( stdout , "set to do not copy files,deck:\"%s\"\n" , deckfilename );
                }
            }
        }
        closedir( dir_ptr );
    }
    tool_destroy( );
    return EXIT_SUCCESS;
}

void tool_inital( void )
{
    json_t * root;
    json_error_t error;

    root = json_load_file( CONFIG_FILE , 0 , &error );

    if( !root )
    {
        fprintf( stderr , "error: on line %d: %s\n", error.line , error.text );
        exit( EXIT_FAILURE );
    }

    config_object.ImageRootDirectory = get_string_node( root, "ImageRootDirectory" );
    config_object.ImageSuffix = get_string_node( root, "ImageSuffix" );
    config_object.imagesuffix_nofree = false;
    config_object.DeckFileDirectory = get_string_node( root, "DeckFileDirectory" );
    config_object.deckfiledirectory_nofree = false;
    config_object.TargetRootDirectory = get_string_node( root , "TargetRootDirectory" );
    config_object.targetrootdirectory_nofree = false;
    config_object.TargetDirectory = ( char * )calloc( BUFFSIZE , sizeof( char ) );
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
        fprintf( stderr , "get configuration:ImageRootDirectory faliure,no exitst default configuration,programs exit\n" );
        json_decref( root );
        exit( EXIT_FAILURE );
    }
    if ( config_object.ImageSuffix == NULL )
    {
        fprintf( stderr , "get configuration:ImageSuffix faliure,use default ImageSuffix:\".jpg\"\n" );
        config_object.ImageSuffix = ".jpg";
        config_object.imagesuffix_nofree = true;
    }
    if ( config_object.DeckFileDirectory == NULL )
    {
        fprintf( stderr , "get configuration:DeckFileDirectory faliure,use default DeckFileDirectory:\"./\"\n" );
        config_object.DeckFileDirectory = "./";
        config_object.deckfiledirectory_nofree = true;
    }
    if ( config_object.TargetRootDirectory == NULL )
    {
        fprintf( stderr , "get configuration:TargetRootDirectory faliure,use default TargetRootDirectory:\"./\"\n" );
        config_object.TargetRootDirectory = "./";
        config_object.targetrootdirectory_nofree = true;
    }
    if ( config_object.TargetDirectory == NULL )
    {
        fprintf( stderr , "make target directory path faliure,programs exit\n" );
        json_decref( root );
        exit( EXIT_FAILURE );
    }
    if ( config_object.WindowWidth == 0 ) 
    {
        fprintf( stderr , "get configuration:WindowWidth faliure,use default WindowWidth:1050\n" );
        config_object.WindowWidth = 1050;
    }
    if ( config_object.WindowHeight == 0 )
    {
        fprintf( stderr , "get configuration:WindowHeight faliure,use default WindowHeight:600\n" );
        config_object.WindowHeight = 600;
    }
    if ( config_object.CardWidth == 0 )
    {
        fprintf( stderr , "get configuration:CardWidth faliure,use default CardWidth:70\n" );
        config_object.CardWidth = 70;
    }
    if ( config_object.CardHeight == 0 )
    {
        fprintf( stderr , "get configuration:CardHeight faliure,use default CardHeight:100\n" );
        config_object.CardHeight = 100;
    }
    if ( config_object.LineCardNumber == 0 )
    {
        fprintf( stderr , "get configuration:LineCardNumber faliure,use default LineCardNumber:15\n" );
        config_object.LineCardNumber = 15;
    }
    if ( config_object.TitleFontSize == 0 )
    {
        fprintf( stderr , "get configuration:LineCardNumber faliure,use default LineCardNumber:20\n" );
        config_object.TitleFontSize = 20;
    }
    if ( config_object.CloseWindowKeyValue == 0 )
    {
        fprintf( stderr , "get configuration:CloseWindowKeyValue faliure,use default CloseWindowKeyValue:32(space)\n" );
        config_object.CloseWindowKeyValue = 32;
    }
    if ( config_object.AutomaticScaling == -1 )
    {
        fprintf( stderr , "get configuration:AutomaticScaling faliure,use default AutomaticScaling:false\n" );
        config_object.AutomaticScaling = false;
    }
    if ( config_object.HideTitleBar == -1 )
    {
        fprintf( stderr , "get configuration:AutomaticScaling faliure,use default HideTitleBar:false\n" );
        config_object.HideTitleBar = false;
    }
    if ( config_object.CopyFile == -1 )
    {
        fprintf( stderr , "get configuration:AutomaticScaling faliure,use default CopyFile:false\n" );
        config_object.CopyFile = true;
    }

    json_decref( root );
}

int32_t process_deck( const char * deckfilename )
{
    int32_t copy_success_count = 0;
    int32_t postion_count = 0;
    preview_init( deckfilename );

    //NULL is empty GSList
    GSList * cardlist = get_cardlist( deckfilename , NULL );
    if ( cardlist == NULL )
    {
        fprintf( stderr , "deck \"%s\" format unknown,parsing stop\n" , deckfilename );
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
        if ( config_object.CopyFile == true )
        {
            char imagefilepath[BUFFSIZE];
            char targetfilepath[BUFFSIZE];
            size_t rename_count = 1;
            makeimagefilepath( imagefilepath , BUFFSIZE , card->cardname , card->cardseries );
            maketargetfilepath( targetfilepath , BUFFSIZE , card->cardname , card->cardseries , card->cardnumber );
            while ( access( targetfilepath , F_OK ) == 0 )
            {
                maketargetfilepath( targetfilepath , BUFFSIZE , card->cardname , card->cardseries , card->cardnumber + rename_count*100 );
                rename_count++;
            }
            if ( copy_file( imagefilepath , targetfilepath ) == true )
                copy_success_count++;
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
    if ( config_object.imagesuffix_nofree != true )
        free( config_object.ImageSuffix );
    if ( config_object.deckfiledirectory_nofree != true )
        free( config_object.DeckFileDirectory );
    if ( config_object.targetrootdirectory_nofree != true )
        free( config_object.TargetDirectory );
}

static bool remove_directory( const char * dir )
{
    char dir_name[BUFFSIZE];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;

    if ( access( dir , F_OK ) != 0 )
        return true;

    if ( stat( dir , &dir_stat ) < 0 )
    {
        perror("get directory stat error");
        return false;
    }

    if ( S_ISREG( dir_stat.st_mode ) )
    {
        remove( dir );
    }
    else if ( S_ISDIR( dir_stat.st_mode ) )
    {
        dirp = opendir( dir );
        while ( ( dp = readdir( dirp ) ) != NULL )
        {
            if ( (  strncmp( "." , dp->d_name , 2 ) == 0 ) || ( strncmp( ".." , dp->d_name , 3 ) == 0 ) )
                continue;
            sprintf( dir_name , "%s/%s" , dir, dp->d_name );
            remove_directory( dir_name ); 
        }
        closedir( dirp );
        rmdir( dir ); 
    } 
    else
    {
        perror( "unknow file type" );
        return false;
    }
    return true;
}

static bool make_directory( const char * dir )
{
    GError * g_error;
    GFile * targetdirectory = g_file_new_for_path( dir );
    if ( g_file_make_directory_with_parents( targetdirectory , NULL , &g_error ) == FALSE )
    {
        g_object_unref( targetdirectory );
        if ( g_error_matches( g_error , G_IO_ERROR_EXISTS, G_IO_ERROR_FAILED ) )
        {
            fprintf( stderr , "directory %s already exists\n" , dir );
            return false;
        }
        else if ( g_error_matches( g_error , G_IO_ERROR_NOT_SUPPORTED , G_IO_ERROR_FAILED ) )
        {
            fprintf( stderr , "file system does not support creating directories\n" );
            return false;
        }
        else
        {
            fprintf( stderr , "create directory %s unknown error\n" , dir );
            return false;
        }
    }
    g_object_unref( targetdirectory );
    return true;
}

static char * get_string_node( json_t * root, const char * nodename )
{
    json_t * node = json_object_get( root, nodename );
    if( !json_is_string( node ) )
    {
        fprintf( stderr , "error: %s is not a string node\n", nodename );
        return NULL;
    }
    const char * onlyread_string = json_string_value( node );
    size_t string_len = strnlen( onlyread_string , BUFFSIZE ) + 1;
    char * return_string = ( char * )malloc( string_len*sizeof( char ) );
    if ( return_string == NULL )
    {
        perror( "allocate configuration:" );
        return NULL;
    }
    strncpy( return_string, onlyread_string, string_len );
    return return_string;
}

static json_int_t get_integer_node( json_t * root , const char * nodename )
{
    json_t * node = json_object_get( root , nodename );
    if ( !json_is_integer( node ) )
    {
        fprintf( stderr , "error: %s is not a integer node\n" , nodename );
    }
    //if not integer node json_integer_value return 0
    return json_integer_value( node );
}

static int get_boolean_node( json_t * root , const char * nodename )
{
    json_t * node = json_object_get( root , nodename );
    if ( !json_is_boolean( node ) )
    {
        fprintf( stderr , "error: %s is not a boolean node\n" , nodename );
        //identify get value error
        return -1;
    }
    //consistent with the standard boolean behavior
    return json_boolean_value( node );
}

static bool copy_file( const char * source_path , const char * destination_path )
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
            fprintf( stderr , "%s not exist\n" , source_path );
            return false;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_EXISTS , G_IO_ERROR_FAILED ) )
        {
            fprintf( stderr , "%s exist\n" , destination_path );
            return false;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_IS_DIRECTORY , G_IO_ERROR_FAILED ) )
        {
            fprintf( stderr , "%s is directory\n" , destination_path );
            return false;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_WOULD_MERGE , G_IO_ERROR_FAILED ) )
        {
            fprintf( stderr , "%s and %s\n" , source_path , destination_path );
            return false;
        }
        if ( g_error_matches( g_error , G_IO_ERROR_WOULD_RECURSE , G_IO_ERROR_FAILED ) )
        {
            fprintf( stderr , "%s is directory or \n" , source_path );
            return false;
        }
    }
    g_object_unref( source );
    g_object_unref( destination );
    return true;
}

static void preview_init( const char * deckfilename )
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
    if ( config_object.AutomaticScaling == true )
    {
        gtk_grid_set_row_homogeneous( GTK_GRID( preview_object.grid ) , TRUE );
        gtk_grid_set_column_homogeneous( GTK_GRID( preview_object.grid ) , TRUE );
    }
    gtk_container_add( GTK_CONTAINER( preview_object.scrolled ), preview_object.grid );
    gtk_container_add( GTK_CONTAINER( preview_object.window ), preview_object.scrolled );
}

static void preview_add_title( enum CardLocal card_local , int32_t * postion_count )
{
    char title_label_buff[BUFFSIZE];
    GtkWidget * title_label = gtk_label_new( NULL );
    if ( card_local == MAIN )
        snprintf( title_label_buff , BUFFSIZE , "<span font='%"PRId64"'>main</span>" , config_object.TitleFontSize );
    else if ( card_local == SIDEBOARD )
        snprintf( title_label_buff , BUFFSIZE , "<span font='%"PRId64"'>sideboard</span>" , config_object.TitleFontSize );
    else if ( card_local == UNKNOWN_LOCAL )
        return ;
    gtk_label_set_markup( GTK_LABEL( title_label ) , title_label_buff );
    GtkWidget * title_box = gtk_event_box_new();
    gtk_container_add( GTK_CONTAINER( title_box ) , title_label );
    *postion_count += config_object.LineCardNumber - *postion_count%config_object.LineCardNumber;
    gtk_grid_attach( GTK_GRID( preview_object.grid ), title_box , *postion_count%config_object.LineCardNumber , 
        *postion_count/config_object.LineCardNumber ,  config_object.LineCardNumber , 1 );
    *postion_count += config_object.LineCardNumber;
}

static void preview_add_card( struct CardObject * card , int32_t * postion_count )
{
    char imagefilepath[BUFFSIZE];
    int32_t cardnumber = card->cardnumber;
    makeimagefilepath( imagefilepath , BUFFSIZE , card->cardname , card->cardseries );
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

static GSList * get_cardlist( const char * deckfilename , GSList * cardlist )
{
    char line_buff[BUFFSIZE];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        fprintf( stderr , "open deck:\"%s\" faliure\n" , deckfilename );
        return NULL;
    }

    int32_t card_local = UNKNOWN_LOCAL;
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
            char ** words = g_regex_split( regex , line_buff , 0 );
            char * cardnumber_str = words[1];
            int32_t cardnumber = 0;
            for ( size_t i = 0 ; i < strnlen( cardnumber_str , BUFFSIZE ) ; i++ )
                cardnumber = 10*cardnumber + line_buff[i] - '0';
            card->card_local = card_local;
            card->cardnumber = cardnumber;
            strncat( card->cardname , words[2] , BUFFSIZE );
            strncat( card->cardseries , words[3] , BUFFSIZE );
            //INT32_MAX is unknown id
            card->cardid = INT32_MAX;
            remove_forwardslash( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( char ** )words );
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
            char ** words = g_regex_split( regex , line_buff , 0 );
            char * cardnumber_str = words[1];
            int32_t cardnumber = 0;
            for ( size_t i = 0 ; i < strnlen( cardnumber_str , BUFFSIZE ) ; i++ )
                cardnumber = 10*cardnumber + line_buff[i] - '0';
            card->card_local = card_local;
            card->cardnumber = cardnumber;
            strncat( card->cardname , words[2] , BUFFSIZE );
            card->cardseries = NULL;
            //INT32_MAX is unknown id
            card->cardid = INT32_MAX;
            remove_forwardslash( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( char ** )words );
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
            char ** words = g_regex_split( regex , line_buff , 0 );
            char * cardnumber_str = words[1];
            char * cardid_str = words[4];
            int32_t cardnumber = 0;
            //INT32_MAX is unknown id
            int32_t cardid = INT32_MAX;
            for ( size_t i = 0 ; i < strnlen( cardnumber_str , BUFFSIZE ) ; i++ )
                cardnumber = 10*cardnumber + line_buff[i] - '0';
            for ( size_t i = 0 ; i < strnlen( cardid_str , BUFFSIZE ) ; i++ )
                cardid = 10*cardid + line_buff[i] - '0';
            card->card_local = card_local;
            card->cardnumber = cardnumber;
            strncat( card->cardname , words[2] , BUFFSIZE );
            strncat( card->cardseries , words[3] , BUFFSIZE );
            card->cardid = cardid;
            remove_forwardslash( card->cardname );
            cardlist = g_slist_append( cardlist , ( gpointer )card );
            //if match succee,continue other test
            g_strfreev( ( char ** )words );
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
    card->cardname = calloc( 1 , BUFFSIZE*sizeof( char ) );
    if ( card->cardname == NULL )
    {
        free( card );
        return NULL;
    }
    card->cardseries = calloc( 1 , BUFFSIZE*sizeof( char ) );
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

static void remove_forwardslash( char * str )
{
    //fast check
    int is_exists = strrchr( str , '/' ) == NULL;
    if ( is_exists == true )
        return ;
    size_t str_len = strnlen( str, BUFFSIZE );
    char * pos_ptr = strstr( str , " // " );
    while( pos_ptr )
    {
        for ( size_t i = pos_ptr - str ; i <= str_len ; i++ )
            str[i] = str[i+4];
        pos_ptr = strstr( str , " // " );
    }
}

#define CHECKLEN(X) ( ( path_maxlen ) > ( X ) ? true : false ) 

static bool makeimagefilepath( char * imagefilepath , size_t path_maxlen , const char * cardname , const char * cardseries  )
{
    if ( imagefilepath == NULL || cardname == NULL )
        return false;
    size_t imagerootdirectory_len = strnlen( config_object.ImageRootDirectory , BUFFSIZE );
    size_t imagesuffix_len = strnlen( config_object.ImageSuffix , BUFFSIZE );
    size_t cardseries_len = 0;
    size_t cardname_len = strnlen( cardname , BUFFSIZE );

    if ( cardseries != NULL )
        cardseries_len = strnlen( cardseries , BUFFSIZE );
    if ( !CHECKLEN( imagerootdirectory_len + imagesuffix_len + cardname_len + cardseries_len + 10 ) )
        return false;

    const char * forgeimagesuffix = ".full"; 
    const char * basiclandnamelist[] = 
    {
        "Plains","Island","Swamp","Mountain","Forest",
        "Snow-Covered Plains","Snow-Covered Island",
        "Snow-Covered Swamp","Snow-Covered Mountain",
        "Snow-Covered Forest","Wastes"
    };

    for ( size_t i = 0 ; i < ( sizeof( basiclandnamelist )/sizeof( basiclandnamelist[0] ) ) ; i++ )
        if ( strncmp( cardname , basiclandnamelist[i] , BUFFSIZE ) == 0 )
            forgeimagesuffix = "1.full";

    if ( cardseries != NULL )
        snprintf( imagefilepath , path_maxlen , "%s%s/%s%s%s" , 
            config_object.ImageRootDirectory , cardseries , cardname , forgeimagesuffix , config_object.ImageSuffix );
    else
        snprintf( imagefilepath , path_maxlen , "%s%s%s%s" , 
            config_object.ImageRootDirectory , cardname , forgeimagesuffix , config_object.ImageSuffix );

    return true;
}

static bool maketargetfilepath( char * targetfilepath , size_t path_maxlen ,  const char * cardname , const char * cardseries , size_t retry_count )
{
    if ( targetfilepath == NULL || cardname == NULL )
        return false;
    size_t targetfilepath_len = strnlen( config_object.TargetDirectory , BUFFSIZE );
    size_t imagesuffix_len = strnlen( config_object.ImageSuffix , BUFFSIZE );
    size_t cardseries_len = 0;
    size_t cardname_len = strnlen( cardname , BUFFSIZE );
    
    if ( cardseries != NULL )
        cardseries_len = strnlen( cardseries , BUFFSIZE );
    if ( !CHECKLEN( targetfilepath_len + imagesuffix_len + cardname_len + cardseries_len + 10 ) )
        return false;

    if ( cardseries != NULL )
        snprintf( targetfilepath , path_maxlen , "%s%s.%s%"PRIu64"%s" ,
            config_object.TargetDirectory , cardname , cardseries , retry_count , config_object.ImageSuffix );
    else
        snprintf( targetfilepath , path_maxlen , "%s%s%"PRIu64"%s" ,
            config_object.TargetDirectory , cardname , retry_count , config_object.ImageSuffix );

    return true;
#undef CHECKLEN
}

static bool get_deckpreview( GtkWidget * window )
{
    GdkWindow * gdk_window = gtk_widget_get_window( GTK_WIDGET( window ) );
    GdkPixbuf * deckpreview = gdk_pixbuf_get_from_window( GDK_WINDOW( gdk_window ) , 0 , 0 , config_object.WindowWidth , config_object.WindowHeight );
    if ( deckpreview == NULL )
    {
        fprintf( stderr , "get deck preview buff faliure\n" );
    }
    else
    {
        char previewname[BUFFSIZE];
        snprintf( previewname , BUFFSIZE , "%sdeckpreview.jpg" , config_object.TargetDirectory );
        gdk_pixbuf_save( deckpreview , previewname , "jpeg" , NULL , "quality" , "100" , NULL );
    }
    gtk_main_quit();
    return TRUE;
}

static bool on_key_press( GtkWidget * widget , GdkEventKey * event , gpointer data )
{
    ( void )widget;
    ( void )data;
    if( event->keyval == config_object.CloseWindowKeyValue )
        gtk_main_quit();
    return FALSE;
}
