#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex.h>

#include <gtk/gtk.h>
#include <jansson.h>

#define CONFIG_FILE "config.json"
#define BUFFSIZE 1024

struct config_object
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
    json_int_t LineCardNumber;
    json_int_t CardWidth;
    json_int_t CardHeight;
}config_object;

void tool_inital( void );

size_t parse_deck( const char * deckfilename );

void tool_destroy( void );

//do tool_inital
static json_int_t get_integer_node( json_t * root , const char * nodename );

static char * get_string_node( json_t * root, const char * nodename );

//do main
static bool remove_directory( const char * dir );

static bool make_directory( const char * dir );

static bool copy_file( const char * source_path , const char * destination_path );

//do parse_deck
static size_t do_forge_deck( GtkWidget * grid , const char * line_buff , size_t * count );

static size_t do_goldfish_deck( GtkWidget * grid , const char * line_buff , size_t * count );

static size_t do_mtga_deck( GtkWidget * grid , const char * line_buff , size_t * count );

static void remove_forwardslash( char * str );

static bool makeimagefilepath( char * imagefilepath , size_t path_maxlen , const char * cardname , const char * cardseries );

static bool maketargetfilepath( char * targetfilepath , size_t path_maxlen , const char * cardname , const char * cardseries , size_t retry_counter );

static void get_deckpreview( GtkWidget * window );

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
                    size_t targetdirectory_len = strlen( config_object.TargetDirectory );
                    //****.dck --> ****/$(NUL)"
                    config_object.TargetDirectory[targetdirectory_len-4] = '/';
                    config_object.TargetDirectory[targetdirectory_len-3] = '\0';
                    if ( remove_directory( config_object.TargetDirectory ) != true )
                        continue;
                    if ( make_directory( config_object.TargetDirectory  ) == false )
                        continue;
                    size_t copy_success_count = parse_deck( deckfilename );
                    fprintf( stdout , "deck:\"%s\" successfully copied %zd card images \n" , deckfilename , copy_success_count );
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

    FILE * jsonfile = fopen( CONFIG_FILE, "r" );
    if ( jsonfile == NULL )
    {
        fprintf( stderr, "open config file faliure\n" );
        exit( EXIT_FAILURE );
    }

    root = json_loadf( jsonfile , 0 , &error );

    if( !root )
    {
        fprintf( stderr, "error: on line %d: %s\n", error.line , error.text );
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
    config_object.LineCardNumber = get_integer_node( root , "LineCardNumber" );
    config_object.CardWidth = get_integer_node( root , "CardWidth" );
    config_object.CardHeight = get_integer_node( root , "CardHeight" );

    if ( config_object.ImageRootDirectory == NULL )
    {
        fprintf( stderr , "get configuration:ImageRootDirectory faliure,no exitst default configuration,programs exit\n" );
        fclose( jsonfile );
        json_decref( root );
        exit( EXIT_FAILURE );
    }
    if ( config_object.ImageSuffix == NULL )
    {
        fprintf( stderr , "get configuration:ImageSuffix faliure,use default configuration\n" );
        config_object.ImageSuffix = ".jpg";
        config_object.imagesuffix_nofree = false;
    }
    if ( config_object.DeckFileDirectory == NULL )
    {
        fprintf( stderr , "get configuration:DeckFileDirectory faliure,use default configuration\n" );
        config_object.DeckFileDirectory = "./";
        config_object.deckfiledirectory_nofree = false;
    }
    if ( config_object.TargetRootDirectory == NULL )
    {
        fprintf( stderr , "get configuration:TargetRootDirectory faliure,use default configuration\n" );
        config_object.TargetRootDirectory = "./";
        config_object.targetrootdirectory_nofree = false;
    }
    if ( config_object.TargetDirectory == NULL )
    {
        fprintf( stderr , "make target directory path faliure,programs exit\n" );
        fclose( jsonfile );
        json_decref( root );
        exit( EXIT_FAILURE );
    }
    if ( config_object.WindowWidth == 0 ) 
    {
        fprintf( stderr , "get configuration:WindowWidth faliure,use default configuration\n" );
        config_object.WindowWidth = 1050;
    }
    if ( config_object.WindowHeight == 0 )
    {
        fprintf( stderr , "get configuration:WindowHeight faliure,use default configuration\n" );
        config_object.WindowHeight = 600;
    }
    if ( config_object.LineCardNumber == 0 )
    {
        fprintf( stderr , "get configuration:LineCardNumber faliure,use default configuration\n" );
        config_object.LineCardNumber = 15;
    }
    if ( config_object.CardWidth == 0 )
    {
        fprintf( stderr , "get configuration:CardWidth faliure,use default configuration\n" );
        config_object.CardWidth = 70;
    }
    if ( config_object.CardHeight == 0 )
    {
        fprintf( stderr , "get configuration:CardHeight faliure,use default configuration\n" );
        config_object.CardHeight = 100;
    }

    fclose( jsonfile );
    json_decref( root );
}

size_t parse_deck( const char * deckfilename )
{
    int ret_code;
    char line_buff[BUFFSIZE];
    size_t copy_success_count = 0;
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        fprintf( stderr, "open deck file faliure\n" );
        return 0;
    }

    GtkWidget * window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    GtkWidget * window_icon = gtk_image_new_from_file( "logo.ico" );
    if ( window_icon == NULL )
    {
        g_print( "%s not is image file or not found\n", "logo.ico" );
    }
    else
    {
        GdkPixbuf * icon_pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( window_icon ) );
        if ( icon_pixbuf == NULL )
            g_print( "%s not is image file or not found\n", "logo.ico" );
        else
            gtk_window_set_icon( GTK_WINDOW( window ) , icon_pixbuf );
    }
    gtk_window_set_title( GTK_WINDOW( window ), deckfilename );
    gtk_window_set_default_size( GTK_WINDOW( window ), ( gint )config_object.WindowWidth , ( gint )config_object.WindowHeight );
    g_signal_connect( G_OBJECT( window ), "delete_event", G_CALLBACK( get_deckpreview ), window );

    GtkWidget * scrolled = gtk_scrolled_window_new( NULL, NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

    GtkWidget * grid = gtk_grid_new();
    size_t count = 0;
    while ( fgets( line_buff, BUFFSIZE, deckfile ) != NULL )
    {
        if ( ( strncmp( line_buff , "[Main]" , 6 ) == 0 ) || ( strncmp( line_buff , "[main]" , 6 ) == 0 ) )
        {
            GtkWidget * mainlabel = gtk_button_new_with_label( "main" );
            count += config_object.LineCardNumber - count%config_object.LineCardNumber;
            gtk_grid_attach( GTK_GRID( grid ), mainlabel, count%config_object.LineCardNumber, count/config_object.LineCardNumber,  config_object.LineCardNumber , 1 );
            count += config_object.LineCardNumber;
            continue;
        }
        else if ( ( strncmp( line_buff , "[Sideboard]" , 11 ) == 0 ) || ( strncmp( line_buff , "[sideboard]" , 11 ) == 0 ) )
        {
            GtkWidget * sideboardlabel = gtk_button_new_with_label( "Sideboard" );
            count += config_object.LineCardNumber - count%config_object.LineCardNumber;
            gtk_grid_attach( GTK_GRID( grid ), sideboardlabel, count%config_object.LineCardNumber, count/config_object.LineCardNumber,  config_object.LineCardNumber , 1 );
            count += config_object.LineCardNumber;
            continue;
        }
        if ( ( ret_code = do_forge_deck( grid , line_buff , &count ) ) != 0 )
            copy_success_count += ret_code;
        else if ( ( ret_code = do_mtga_deck( grid , line_buff , &count ) ) != 0 )
            copy_success_count += ret_code;
        else if ( ( ret_code = do_goldfish_deck( grid , line_buff , &count ) ) != 0 )
            copy_success_count += ret_code;
        else
            continue;
    }

    gtk_container_add( GTK_CONTAINER( scrolled ), grid );
    gtk_container_add( GTK_CONTAINER( window ), scrolled );

    gtk_widget_show_all( scrolled );
    gtk_widget_show_all( window );

    gtk_main();

    return copy_success_count;
}

void tool_destroy( void )
{
    free( config_object.ImageRootDirectory );
    if ( config_object.imagesuffix_nofree == true )
        free( config_object.ImageSuffix );
    if ( config_object.deckfiledirectory_nofree == true )
        free( config_object.DeckFileDirectory );
    if ( config_object.targetrootdirectory_nofree == true )
        free( config_object.TargetDirectory );
}

static char * get_string_node( json_t * root, const char * nodename )
{
    json_t * node = json_object_get( root, nodename );
    if( !json_is_string( node ) )
    {
        fprintf( stderr, "error: %s is not a string node\n", nodename );
        return NULL;
    }
    const char * onlyread_string = json_string_value( node );
    size_t string_len = strlen( onlyread_string ) + 1;
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

static bool remove_directory( const char * dir )
{
    char cur_dir[] = ".";
    char up_dir[] = "..";
    char dir_name[BUFFSIZE];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;

    if ( 0 != access(dir, F_OK) )
        return true;

    if ( 0 > stat(dir, &dir_stat) )
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
            if ( ( 0 == strcmp( cur_dir , dp->d_name ) ) || ( 0 == strcmp( up_dir , dp->d_name ) ) )
                continue;
            sprintf( dir_name , "%s/%s" , dir, dp->d_name );
            remove_directory( dir_name ); 
        }
        closedir( dirp );
        rmdir( dir ); 
    } 
    else
    {
        perror( "unknow file type!" );
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

static size_t do_forge_deck( GtkWidget * grid , const char * line_buff , size_t * count )
{
    char err_buff[BUFFSIZE];
    int ret_code;
    size_t copy_success_count = 0;
    regex_t regex;
    regmatch_t pmatch[4];

    if ( ( ret_code = regcomp( &regex, "^([0-9]+) ([^|]+)\\|([^|]+)", REG_EXTENDED | REG_NEWLINE ) ) != 0 )
    {
        regerror( ret_code, &regex, err_buff, BUFFSIZE );
        fprintf( stderr, "error:%s\n", err_buff );
        return 0;
    }
    if ( ( ret_code = regexec( &regex, line_buff, 4, pmatch, 0 ) ) == 0 )
    {
        size_t series_len = pmatch[3].rm_eo - pmatch[3].rm_so;
        if ( *( line_buff + pmatch[3].rm_eo - 1 ) == '\n' )
            series_len -= 1;
        if ( *( line_buff + pmatch[3].rm_eo - 2 ) == '\r' )
            series_len -= 1;
        char cardseries[BUFFSIZE];
        strncpy( cardseries, line_buff + pmatch[3].rm_so, series_len );
        cardseries[series_len] = '\0';
        size_t name_len = pmatch[2].rm_eo - pmatch[2].rm_so;
        char cardname[BUFFSIZE];
        strncpy( cardname, line_buff + pmatch[2].rm_so, name_len );
        cardname[name_len] = '\0';
        remove_forwardslash( cardname );
        size_t cardnumber = 0;
        for ( int i = 0 ; i <  pmatch[1].rm_eo - pmatch[1].rm_so ; i++ )
            cardnumber = 10*cardnumber + line_buff[i] - '0';
        char imagefilepath[BUFFSIZE];
        ret_code = makeimagefilepath( imagefilepath , BUFFSIZE , cardname , cardseries );
        while ( cardnumber-- )
        {
            GtkWidget * image = gtk_image_new_from_file( imagefilepath );
            if ( image == NULL )
            {
                g_print( "%s not is image file or not found\n", imagefilepath );
                return copy_success_count;
            }
            GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
            if ( pixbuf == NULL )
            {
                g_print( "%s not is image file or not found\n", imagefilepath );
                return copy_success_count;
            }
            pixbuf = gdk_pixbuf_scale_simple( GDK_PIXBUF( pixbuf ), config_object.CardWidth, config_object.CardHeight, GDK_INTERP_BILINEAR );
            gtk_image_set_from_pixbuf( GTK_IMAGE( image ), pixbuf );
            gtk_grid_attach( GTK_GRID( grid ), image, *count%config_object.LineCardNumber, *count/config_object.LineCardNumber,  1, 1);
            *count += 1;
            char targetfilepath[BUFFSIZE];
            ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , cardseries , cardnumber );
            size_t rename_count = 1;
            while ( access( targetfilepath , F_OK ) == 0 )
            {
                ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , cardseries , cardnumber + rename_count*100 );
                rename_count++;
            }
            ret_code = copy_file( imagefilepath , targetfilepath );
            if ( ret_code == true )
                copy_success_count++;
        }
        regfree( &regex );
        return copy_success_count;
    }
    return 0;
}

static size_t do_goldfish_deck( GtkWidget * grid , const char * line_buff , size_t * count )
{
    char err_buff[BUFFSIZE];
    int ret_code;
    size_t copy_success_count = 0;
    regex_t regex;
    regmatch_t pmatch[3];
    if ( ( ret_code = regcomp( &regex , "^([0-9]+) ([^|\r]+)", REG_EXTENDED | REG_NEWLINE ) ) != 0 )
    {
        regerror( ret_code, &regex, err_buff, BUFFSIZE );
        fprintf( stderr, "error:%s\n", err_buff );
        return 0;
    }
    if ( ( ret_code = regexec( &regex, line_buff, 3 , pmatch, 0 ) ) == 0 )
    {
        size_t name_len = pmatch[2].rm_eo - pmatch[2].rm_so;
        if ( *( line_buff + pmatch[2].rm_eo - 1 ) == '\n' )
            name_len -= 1;
        if ( *( line_buff + pmatch[2].rm_eo - 2 ) == '\r' )
            name_len -= 1;
        char cardname[BUFFSIZE];
        strncpy( cardname, line_buff + pmatch[2].rm_so, name_len );
        cardname[name_len] = '\0';
        remove_forwardslash( cardname );
        size_t cardnumber = 0;
        for ( int i = 0 ; i <  pmatch[1].rm_eo - pmatch[1].rm_so ; i++ )
            cardnumber = 10*cardnumber + line_buff[i] - '0';
        char imagefilepath[BUFFSIZE];
        ret_code = makeimagefilepath( imagefilepath , BUFFSIZE , cardname , NULL );
        while ( cardnumber-- )
        {
            GtkWidget * image = gtk_image_new_from_file( imagefilepath );
            if ( image == NULL )
            {
                g_print( "%s not is image file or not found\n", imagefilepath );
                return copy_success_count;
            }
            GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
            if ( pixbuf == NULL )
            {
                g_print( "%s not is image file or not found\n", imagefilepath );
                return copy_success_count;
            }
            pixbuf = gdk_pixbuf_scale_simple( GDK_PIXBUF( pixbuf ), config_object.CardWidth, config_object.CardHeight, GDK_INTERP_BILINEAR );
            gtk_image_set_from_pixbuf( GTK_IMAGE( image ), pixbuf );
            gtk_grid_attach( GTK_GRID( grid ), image, *count%config_object.LineCardNumber, *count/config_object.LineCardNumber,  1, 1);
            *count += 1;
            char targetfilepath[BUFFSIZE];
            ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , NULL , cardnumber );
            size_t rename_count = 1;
            while ( access( targetfilepath , F_OK ) == 0 )
            {
                ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , NULL , cardnumber + rename_count*100 );
                rename_count++;
            }
            ret_code = copy_file( imagefilepath , targetfilepath );
            if ( ret_code == true )
                copy_success_count++;
        }
        regfree( &regex );
        return copy_success_count;
    }
    return 0;
}

static size_t do_mtga_deck( GtkWidget * grid , const char * line_buff , size_t * count )
{
    char err_buff[BUFFSIZE];
    int ret_code;
    size_t copy_success_count = 0;
    regex_t regex;
    regmatch_t pmatch[5];

    if ( ( ret_code = regcomp( &regex, "^(^[0-9]+) ([^\\(\\)]+) \\(([^ ]+)\\) ([0-9]+)", REG_EXTENDED | REG_NEWLINE ) ) != 0 )
    {
        regerror( ret_code, &regex, err_buff, BUFFSIZE );
        fprintf( stderr, "error:%s\n", err_buff );
        return 0;
    }
    if ( ( ret_code = regexec( &regex, line_buff, 5 , pmatch, 0 ) ) == 0 )
    {
        size_t series_len = pmatch[3].rm_eo - pmatch[3].rm_so;
        if ( *( line_buff + pmatch[3].rm_eo - 1 ) == '\n' )
            series_len -= 1;
        if ( *( line_buff + pmatch[3].rm_eo - 2 ) == '\r' )
            series_len -= 1;
        char cardseries[BUFFSIZE];
        strncpy( cardseries, line_buff + pmatch[3].rm_so, series_len );
        cardseries[series_len] = '\0';
        size_t name_len = pmatch[2].rm_eo - pmatch[2].rm_so;
        char cardname[BUFFSIZE];
        strncpy( cardname, line_buff + pmatch[2].rm_so, name_len );
        cardname[name_len] = '\0';
        remove_forwardslash( cardname );
        size_t cardnumber = 0;
        for ( int i = 0 ; i <  pmatch[1].rm_eo - pmatch[1].rm_so ; i++ )
            cardnumber = 10*cardnumber + line_buff[i] - '0';
        size_t cardid = 0;
        for ( int i = 0 ; i <  pmatch[4].rm_eo - pmatch[4].rm_so ; i++ )
            cardid = 10*cardid + line_buff[i] - '0';
        char imagefilepath[BUFFSIZE];
        ret_code = makeimagefilepath( imagefilepath , BUFFSIZE , cardname , cardseries );
        while ( cardnumber-- )
        {
            GtkWidget * image = gtk_image_new_from_file( imagefilepath );
            if ( image == NULL )
            {
                g_print( "%s not is image file or not found\n", imagefilepath );
                return copy_success_count;
            }
            GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
            if ( pixbuf == NULL )
            {
                g_print( "%s not is image file or not found\n", imagefilepath );
                return copy_success_count;
            }
            pixbuf = gdk_pixbuf_scale_simple( GDK_PIXBUF( pixbuf ) , config_object.CardWidth , config_object.CardHeight , GDK_INTERP_BILINEAR );
            gtk_image_set_from_pixbuf( GTK_IMAGE( image ) , pixbuf );
            gtk_grid_attach( GTK_GRID( grid ) , image , *count%config_object.LineCardNumber, *count/config_object.LineCardNumber, 1 , 1 );
            *count += 1;
            char targetfilepath[BUFFSIZE];
            ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , cardseries , cardnumber );
            size_t rename_count = 1;
            while ( access( targetfilepath , F_OK ) == 0 )
            {
                ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , cardseries , cardnumber + rename_count*100 );
                rename_count++;
            }
            ret_code = copy_file( imagefilepath , targetfilepath );
            if ( ret_code == true )
                copy_success_count++;
        }
        regfree( &regex );
        return copy_success_count;
    }
    return 0;
}

static void remove_forwardslash( char * str )
{
    //fast check
    int is_exists = strrchr( str , '/' ) == NULL;
    if ( is_exists == true )
        return ;
    size_t str_len = strlen( str );
    char * pos_ptr = strstr( str , " // " );
    while( pos_ptr )
    {
        for ( size_t i = pos_ptr - str ; i <= str_len ; i++ )
            str[i] = str[i+4];
        pos_ptr = strstr( str , " // " );
    }
}

static bool makeimagefilepath( char * imagefilepath , size_t path_maxlen , const char * cardname , const char * cardseries  )
{
#define CHECKLEN(X) ( ( path_maxlen ) > ( X ) ? true : false ) 

    size_t imagefilepath_len = strlen( config_object.ImageRootDirectory );
    size_t imagesuffix_len = strlen( config_object.ImageSuffix );
    size_t cardseries_len = 0;
    size_t cardname_len = strlen( cardname );

    if ( !CHECKLEN( imagefilepath_len + imagesuffix_len + cardname_len + strlen( cardseries ) + 10 ) )
        return false;

    strncpy( imagefilepath, config_object.ImageRootDirectory, imagefilepath_len + 1 );
    if ( cardseries != NULL )
    {
        cardseries_len = strlen( cardseries );
        strncpy( imagefilepath + imagefilepath_len, cardseries , cardseries_len );
        imagefilepath_len += cardseries_len;
        strncpy( imagefilepath + imagefilepath_len, "/", 2 );
        imagefilepath_len += 1;
    }
    strncpy( imagefilepath + imagefilepath_len, cardname, cardname_len );
    imagefilepath_len += cardname_len;
    if ( strncmp( cardname , "Plains" , 6 ) == 0 )
    {
        strncpy( imagefilepath + imagefilepath_len , "1" , 2 );
        imagefilepath_len += 1;
    }
    if ( strncmp( cardname , "Island" , 6 ) == 0 )
    {
        strncpy( imagefilepath + imagefilepath_len , "1" , 2 );
        imagefilepath_len += 1;
    }
    if ( strncmp( cardname , "Swamp" , 5 ) == 0 )
    {
        strncpy( imagefilepath + imagefilepath_len , "1" , 2 );
        imagefilepath_len += 1;
    }
    if ( strncmp( cardname , "Mountain" , 8 ) == 0 )
    {
        strncpy( imagefilepath + imagefilepath_len , "1" , 2 );
        imagefilepath_len += 1;
    }
    if ( strncmp( cardname , "Forest" , 6 ) == 0 )
    {
        strncpy( imagefilepath + imagefilepath_len , "1" , 2 );
        imagefilepath_len += 1;
    }
    strncpy( imagefilepath + imagefilepath_len, ".full", 6 );
    imagefilepath_len += 5;
    strncpy( imagefilepath + imagefilepath_len, config_object.ImageSuffix, imagesuffix_len + 1 );
    imagefilepath_len += imagesuffix_len;

    return true;
#undef CHECKLEN
}

static bool maketargetfilepath( char * targetfilepath , size_t path_maxlen ,  const char * cardname , const char * cardseries , size_t retry_counter )
{
#define CHECKLEN(X) ( ( path_maxlen ) > ( X ) ? true : false )

    size_t targetfilepath_len = strlen( config_object.TargetDirectory );
    size_t imagesuffix_len = strlen( config_object.ImageSuffix );
    size_t cardseries_len = 0;
    size_t cardname_len = strlen( cardname );
    
    if ( cardseries == NULL )
    {
        if ( !CHECKLEN( targetfilepath_len + imagesuffix_len + cardname_len + 10 ) )
            return false;
    }
    else
        if ( !CHECKLEN( targetfilepath_len + imagesuffix_len + cardname_len + strlen( cardseries ) + 10 ) )
            return false;

    strncpy( targetfilepath , config_object.TargetDirectory , targetfilepath_len + 1 );
    strncpy( targetfilepath + targetfilepath_len , cardname ,  cardname_len + 1 );
    targetfilepath_len +=  cardname_len;
    if ( cardseries != NULL )
    {
        cardseries_len = strlen( cardseries );
        strncpy( targetfilepath + targetfilepath_len , "." , 2 );
        targetfilepath_len += 1;
        strncpy( targetfilepath + targetfilepath_len ,  cardseries ,  cardseries_len + 1 );
        targetfilepath_len +=  cardseries_len;
    }
    char retry_buff[BUFFSIZE];
    sprintf( retry_buff , "%zd" , retry_counter );
    strncpy( targetfilepath + targetfilepath_len , retry_buff , strlen( retry_buff ) + 1 );
    targetfilepath_len += strlen( retry_buff );
    strncpy( targetfilepath + targetfilepath_len , config_object.ImageSuffix , imagesuffix_len + 1 );
    targetfilepath_len += imagesuffix_len;

    return true;
#undef CHECKLEN
}

void get_deckpreview( GtkWidget * window )
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
        gdk_pixbuf_save ( deckpreview , previewname , "jpeg" , NULL , "quality" , "100" , NULL );
    }
    gtk_main_quit();
}
