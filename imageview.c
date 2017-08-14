#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include <gtk/gtk.h>

int main ( int argc , char * argv[] )
{
    gtk_init( &argc , &argv );
    
    GtkWidget * window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title( GTK_WINDOW( window ) , "series images view" );
    gtk_window_set_default_size( GTK_WINDOW( window ) , 600 , 400 );
    g_signal_connect( G_OBJECT( window ) , "delete_event" , G_CALLBACK( gtk_main_quit ) , NULL );

    GtkWidget * scrolled = gtk_scrolled_window_new( NULL , NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ) , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

    //traverse the destination folder display all image
    GtkWidget * grid = gtk_grid_new();
    DIR *dirptr;
    struct dirent *entry;
    size_t count = 0;
    
    //gtk_init parses some standard command line options,argc and argv are adjusted accordingly
    //argv[0] is executable file name,if argc = 1 the program does nothing
    if ( argc == 1 )
        return EXIT_FAILURE;
    const char * dirname = argv[1];
    for( int i = 1 ; i < argc ; dirname = argv[i] , i++ )
    {
        if ( ( dirptr = opendir( dirname ) ) == NULL )
        {
            g_print( "can't open directory %s\n" , dirname );
            continue;
        }
    
        chdir( dirname );
        while( ( entry = readdir( dirptr ) ) != NULL )
        {
            if ( strncmp( entry->d_name , ".." , 4 ) == 0 || strncmp( entry->d_name , "." , 4 ) == 0 )
            {
                g_print( "%s is directory\n" , entry->d_name );
                continue;
            }
            else
            {
                GtkWidget * image = gtk_image_new_from_file( entry->d_name );
                if ( image == NULL )
                {
                    g_print( "%s not is image file\n" , entry->d_name );
                    continue;
                }
                gtk_grid_attach( GTK_GRID( grid ) , image , count%5 , count/5 ,  1 , 1);
                count++;
            }
        }
        chdir( ".." );
        closedir( dirptr );
    }

    gtk_container_add( GTK_CONTAINER( scrolled ) , grid );
    gtk_container_add( GTK_CONTAINER( window ) , scrolled );


    gtk_widget_show_all( scrolled );
    gtk_widget_show_all( window );

    gtk_main();
    return EXIT_SUCCESS;
}
