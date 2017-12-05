#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
#endif

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <pthread.h>

#include <gtk/gtk.h>
#include <jansson.h>

#define CONFIG_FILE "config.json"
#define BUFFSIZE 1024

struct config_object
{
    char * TargetRootDirectory;
    char * TargetDirectory;
    char * ImageRootDirectory;
    char * ImageSuffix;
    char * DeckFileDirectory;
    json_int_t WindowWidth;
    json_int_t WindowHeight;
};

static void cp( const char * to, const char * from );

static bool remove_directory( const char * dir );

static void remove_forwardslash( char * str );

static int process( const char * deckfilename , struct config_object * cfg_ptr );

static json_int_t get_integer_node( json_t * root , const char * nodename );

static char * get_string_node( json_t * root, const char * nodename );

static struct config_object * tool_inital( void );

static void tool_destroy( struct config_object * cfg_ptr );

static bool makeimagefilepath( char * imagefilepath , size_t path_maxlen , const char * cardname , const char * cardseries ,  struct config_object * cfg_ptr );

static bool maketargetfilepath( char * targetfilepath , size_t path_maxlen , const char * cardname , const char * cardseries , struct config_object * cfg_ptr , size_t retry_counter );

int main ( int argc, char * argv[] )
{
    gtk_init( &argc, &argv );
    struct config_object * cfg_ptr = tool_inital();
    char deckfilename[BUFFSIZE];
    DIR * dir_ptr = opendir( cfg_ptr->DeckFileDirectory );
    struct dirent * dirent_ptr;
    struct stat dir_stat;
    struct stat deckfile_stat;

    if ( access( cfg_ptr->DeckFileDirectory , F_OK ) != 0 )
    {
        fprintf( stderr , "%s not exitst,errno:%d\n" , cfg_ptr->DeckFileDirectory , errno );
        return EXIT_FAILURE;
    }
    
    if ( stat( cfg_ptr->DeckFileDirectory , &dir_stat ) < 0 )
    {
        perror( "get DeckFileDirectory stat:" );
        return EXIT_FAILURE;
    }

    if ( !S_ISDIR( dir_stat.st_mode ) )
    {
        fprintf( stderr , "%s not is directory\n" , cfg_ptr->DeckFileDirectory );
        return EXIT_FAILURE;
    }
    else
    {
        dir_ptr = opendir( cfg_ptr->DeckFileDirectory );
        while( ( dirent_ptr = readdir( dir_ptr ) ) != NULL )
        {
            if ( ( strncmp( "." , dirent_ptr->d_name , 1  ) == 0 ) || ( strncmp( ".." , dirent_ptr->d_name , 2 ) == 0 ) )
                continue;
            else
            {
                sprintf( deckfilename , "%s%s" , cfg_ptr->DeckFileDirectory , dirent_ptr->d_name );
                if ( stat( deckfilename , &deckfile_stat ) < 0 )
                    continue;
                if ( S_ISDIR( deckfile_stat.st_mode ) )
                    continue;
                else
                {
                    sprintf( cfg_ptr->TargetDirectory , "%s%s" , cfg_ptr->TargetRootDirectory , dirent_ptr->d_name );
                    size_t targetdirectory_len = strlen( cfg_ptr->TargetDirectory );
                    //****.dck --> ****/$(NUL)"
                    cfg_ptr->TargetDirectory[targetdirectory_len-4] = '/';
                    cfg_ptr->TargetDirectory[targetdirectory_len-3] = '\0';
                    if ( remove_directory( cfg_ptr->TargetDirectory ) != true )
                        continue;
#ifdef _WIN32
                    char * temp = cfg_ptr->TargetDirectory;
                    while( *temp++ )
                        if ( *temp == '/' )
                            *temp = '\\';
                    if ( _mkdir( cfg_ptr->TargetDirectory ) != 0 && errno == ENOENT )
                    {
                        fprintf( stderr , "%s parent directory not exitst\n" , cfg_ptr->TargetDirectory );
                        return EXIT_FAILURE;
                    }
#elif defined __unix__
                    mkdir( cfg_ptr->TargetDirectory , 0755 );
#endif
                    process( deckfilename , cfg_ptr );
                }
            }
        }
        closedir( dir_ptr );
    }
    tool_destroy( cfg_ptr );
    return EXIT_SUCCESS;
}

static int process( const char * deckfilename , struct config_object * cfg_ptr )
{
    int ret_code;
    char line_buff[BUFFSIZE];
    char err_buff[BUFFSIZE];
    regex_t regex;
    regmatch_t pmatch[4];
    FILE * deckfile = fopen( deckfilename , "r" );
    if ( deckfile == NULL )
    {
        fprintf( stderr, "open deck file faliure\n" );
        return false;
    }

    GtkWidget * window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title( GTK_WINDOW( window ), deckfilename );
    gtk_window_set_default_size( GTK_WINDOW( window ), ( gint )cfg_ptr->WindowWidth , ( gint )cfg_ptr->WindowHeight );
    g_signal_connect( G_OBJECT( window ), "delete_event", G_CALLBACK( gtk_main_quit ), NULL );

    GtkWidget * scrolled = gtk_scrolled_window_new( NULL, NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

    GtkWidget * grid = gtk_grid_new();
    size_t count = 0;
    while ( fgets( line_buff, BUFFSIZE, deckfile ) != NULL )
    {
        if ( ( strncmp( line_buff , "[Main]" , 6 ) == 0 ) || ( strncmp( line_buff , "[main]" , 6 ) == 0 ) )
        {
            GtkWidget * mainlabel = gtk_button_new_with_label( "main" );
            count += 15 - count%15;
            gtk_grid_attach( GTK_GRID( grid ), mainlabel, count%15, count/15,  15 , 1 );
            count += 15;
            continue;
        }
        else if ( ( strncmp( line_buff , "[Sideboard]" , 11 ) == 0 ) || ( strncmp( line_buff , "[sideboard]" , 11 ) == 0 ) )
        {
            GtkWidget * sideboardlabel = gtk_button_new_with_label( "Sideboard" );
            count += 15 - count%15;
            gtk_grid_attach( GTK_GRID( grid ), sideboardlabel, count%15, count/15,  15 , 1 );
            count += 15;
            continue;
        }
        if ( ( ret_code = regcomp( &regex, "^([0-9]+) ([^|]+)\\|([^|0-9]+)", REG_EXTENDED | REG_NEWLINE ) ) != 0 )
        {
            regerror( ret_code, &regex, err_buff, BUFFSIZE );
            fprintf( stderr, "error:%s\n", err_buff );
            return false;
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
            ret_code = makeimagefilepath( imagefilepath , BUFFSIZE , cardname , cardseries , cfg_ptr );
            while ( cardnumber-- )
            {
                GtkWidget * image = gtk_image_new_from_file( imagefilepath );
                if ( image == NULL )
                {
                    g_print( "%s not is image file or not found\n", imagefilepath );
                    cardnumber = 0;
                    goto newregex_clean;
                }
                GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
                if ( pixbuf == NULL )
                {
                    g_print( "%s not is image file or not found\n", imagefilepath );
                    cardnumber = 0;
                    goto newregex_clean;
                }
                pixbuf = gdk_pixbuf_scale_simple( GDK_PIXBUF( pixbuf ), 70, 100, GDK_INTERP_BILINEAR );
                gtk_image_set_from_pixbuf( GTK_IMAGE( image ), pixbuf );
                gtk_grid_attach( GTK_GRID( grid ), image, count%15, count/15,  1, 1);
                count++;

                char targetfilepath[BUFFSIZE];
                ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , cardseries , cfg_ptr , cardnumber );
                size_t rename_count = 1;
                while ( access( targetfilepath , F_OK ) == 0 )
                {
                    ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , cardseries , cfg_ptr , cardnumber + rename_count*100 );
                    rename_count++;
                }
                cp( targetfilepath , imagefilepath );
            }
newregex_clean:
            ;
        }
        else
        {
            regfree( &regex );
            if ( ( ret_code = regcomp( &regex, "^([0-9]+) ([^|\r]+)", REG_EXTENDED | REG_NEWLINE ) ) != 0 )
            {
                regerror( ret_code, &regex, err_buff, BUFFSIZE );
                fprintf( stderr, "error:%s\n", err_buff );
                return false;
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
                ret_code = makeimagefilepath( imagefilepath , BUFFSIZE , cardname , NULL , cfg_ptr );
                while ( cardnumber-- )
                {
                    GtkWidget * image = gtk_image_new_from_file( imagefilepath );
                    if ( image == NULL )
                    {
                        g_print( "%s not is image file or not found\n", imagefilepath );
                        cardnumber = 0;
                        goto oldregex_clean;
                    }
                    GdkPixbuf * pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( image ) );
                    if ( pixbuf == NULL )
                    {
                        g_print( "%s not is image file or not found\n", imagefilepath );
                        cardnumber = 0;
                        goto oldregex_clean;
                    }
                    pixbuf = gdk_pixbuf_scale_simple( GDK_PIXBUF( pixbuf ), 70, 100, GDK_INTERP_BILINEAR );
                    gtk_image_set_from_pixbuf( GTK_IMAGE( image ), pixbuf );
                    gtk_grid_attach( GTK_GRID( grid ), image, count%15, count/15,  1, 1);
                    count++;

                    char targetfilepath[BUFFSIZE];
                    ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , NULL , cfg_ptr , cardnumber );
                    size_t rename_count = 1;
                    while ( access( targetfilepath , F_OK ) == 0 )
                    {
                        ret_code = maketargetfilepath( targetfilepath , BUFFSIZE , cardname , NULL , cfg_ptr , cardnumber + rename_count*100 );
                        rename_count++;
                    }
                    cp( targetfilepath , imagefilepath );
                }
oldregex_clean:
                ;
            }
        }
        regfree( &regex );
    }

    gtk_container_add( GTK_CONTAINER( scrolled ), grid );
    gtk_container_add( GTK_CONTAINER( window ), scrolled );

    gtk_widget_show_all( scrolled );
    gtk_widget_show_all( window );

    gtk_main();
    return true;
}

static struct config_object * tool_inital( void )
{

    struct config_object * cfg_ptr = ( struct config_object * )calloc( sizeof( struct config_object ) , 1 );

    if ( cfg_ptr == NULL )
    {
        perror( "allocate configuration object:" );
        exit( EXIT_FAILURE );
    }

    json_t * root;
    json_error_t error;

    FILE * jsonfile = fopen( CONFIG_FILE, "r" );
    if ( jsonfile == NULL )
    {
        fprintf( stderr, "open config file faliure\n" );
        exit( EXIT_FAILURE );
    }

    root = json_loadf( jsonfile, 0, &error );

    if( !root )
    {
        fprintf( stderr, "error: on line %d: %s\n", error.line, error.text );
        exit( EXIT_FAILURE );
    }

    cfg_ptr->ImageRootDirectory = get_string_node( root, "ImageRootDirectory" );
    cfg_ptr->ImageSuffix = get_string_node( root, "ImageSuffix" );
    cfg_ptr->DeckFileDirectory = get_string_node( root, "DeckFileDirectory" );
    cfg_ptr->TargetRootDirectory = get_string_node( root , "TargetRootDirectory" );
    cfg_ptr->TargetDirectory = ( char * )calloc( BUFFSIZE , sizeof( char ) );
    cfg_ptr->WindowWidth = get_integer_node( root , "WindowWidth" );
    cfg_ptr->WindowHeight = get_integer_node( root , "WindowHeight" );

    if ( cfg_ptr->ImageRootDirectory == NULL )
        goto initial_faliure;
    if ( cfg_ptr->ImageSuffix == NULL )
        goto initial_faliure;
    if ( cfg_ptr->DeckFileDirectory == NULL )
        goto initial_faliure;
    if ( cfg_ptr->TargetRootDirectory == NULL )
        goto initial_faliure;
    if ( cfg_ptr->TargetDirectory == NULL )
    {
        perror( "initial TargetDirectory:" );
        exit( EXIT_FAILURE );
    }
    if ( ( cfg_ptr->WindowWidth == 0 ) || ( cfg_ptr->WindowHeight == 0 ) )
        goto initial_faliure;

    fclose( jsonfile );
    json_decref(root);
    return cfg_ptr;

initial_faliure:
    fprintf( stderr , "configuration options format or value error\n" );
    tool_destroy( cfg_ptr );
    exit( EXIT_FAILURE );
}

static void tool_destroy( struct config_object * cfg_ptr )
{
    free( cfg_ptr->ImageRootDirectory );
    free( cfg_ptr->ImageSuffix );
    free( cfg_ptr->DeckFileDirectory );
    free( cfg_ptr->TargetDirectory );
    free( cfg_ptr );
}

static bool makeimagefilepath( char * imagefilepath , size_t path_maxlen , const char * cardname , const char * cardseries ,  struct config_object * cfg_ptr )
{
#define CHECKLEN(X) ( ( path_maxlen ) > ( X ) ? true : false ) 

    size_t imagefilepath_len = strlen( cfg_ptr->ImageRootDirectory );
    size_t imagesuffix_len = strlen( cfg_ptr->ImageSuffix );
    size_t cardseries_len = 0;
    size_t cardname_len = strlen( cardname );

    if ( !CHECKLEN( imagefilepath_len + imagesuffix_len + cardname_len + strlen( cardseries ) + 10 ) )
        return false;

    strncpy( imagefilepath, cfg_ptr->ImageRootDirectory, imagefilepath_len + 1 );
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
    strncpy( imagefilepath + imagefilepath_len, cfg_ptr->ImageSuffix, imagesuffix_len + 1 );
    imagefilepath_len += imagesuffix_len;

    return true;
#undef CHECKLEN
}

static bool maketargetfilepath( char * targetfilepath , size_t path_maxlen ,  const char * cardname , const char * cardseries , struct config_object * cfg_ptr , size_t retry_counter )
{
#define CHECKLEN(X) ( ( path_maxlen ) > ( X ) ? true : false )

    size_t targetfilepath_len = strlen( cfg_ptr->TargetDirectory );
    size_t imagesuffix_len = strlen( cfg_ptr->ImageSuffix );
    size_t cardseries_len = 0;
    size_t cardname_len = strlen( cardname );
    
    if ( !CHECKLEN( targetfilepath_len + imagesuffix_len + cardname_len + strlen( cardseries ) + 10 ) )
        return false;

    strncpy( targetfilepath , cfg_ptr->TargetDirectory , targetfilepath_len + 1 );
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
    strncpy( targetfilepath + targetfilepath_len , cfg_ptr->ImageSuffix , imagesuffix_len + 1 );
    targetfilepath_len += imagesuffix_len;

    return true;
#undef CHECKLEN
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

static void cp ( const char * to , const char * from )
{
    char command_buff[BUFFSIZE];
#ifdef _WIN32
    sprintf( command_buff , "copy \"%s\" \"%s\"" , from , to );
    char * temp = command_buff;
    while( *temp++ )
        if ( *temp == '/' )
            *temp = '\\';
#elif defined __unix__
    sprintf( command_buff , "cp \"%s\" \"%s\"" , from , to );
#endif
    size_t ret_code = system( command_buff );
    ( void )ret_code;
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

