#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include <gtk/gtk.h>

int main ( int argc , char * argv[] )
{
    //初始化gtk
    gtk_init( &argc , &argv );
    
    //创建窗口、设置标题窗口缺省大小、绑定delete_event信号的处理函数
    GtkWidget * window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title( GTK_WINDOW( window ) , "series images view" );
    gtk_window_set_default_size( GTK_WINDOW( window ) , 600 , 400 );
    g_signal_connect( G_OBJECT( window ) , "delete_event" , G_CALLBACK( gtk_main_quit ) , NULL );

    //创建滚动条，横向永不显示，纵向按需显示
    GtkWidget * scrolled = gtk_scrolled_window_new( NULL , NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ) , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

    //遍历目标文件夹将图片填入方格中
    GtkWidget * grid = gtk_grid_new();
    const char * dirname;
    DIR *dirptr;
    struct dirent *entry;
    size_t count = 0;
    
    if ( argc > 1 )
        dirname = argv[1];

    for( int i = 1 ; i < argc ; i++ , dirname = argv[i] )
    {
        if ( ( dirptr = opendir( dirname ) ) == NULL )
        {
            g_print( "can`t open directory %s\n" , dirname );
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
    //将方格、滚动条加入窗口
    gtk_container_add( GTK_CONTAINER( scrolled ) , grid );
    gtk_container_add( GTK_CONTAINER( window ) , scrolled );

    //使窗口、滚动条以及其中的内容显示
    gtk_widget_show_all( scrolled );
    gtk_widget_show_all( window );

    gtk_main();
    return EXIT_SUCCESS;
}

