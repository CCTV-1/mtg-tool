#include <stdio.h>
#include <stdlib.h>

#include "previewconfig.h"
#include "deckprase.h"

#define BUFFSIZE 1024

#define MTGA_REGEX "^([0-9]+)\\ ([^\\(^\\)]+)\\ \\(([^\\ ]+)\\)\\ ([0-9A-Z^\\r^\\n]+)"
#define XMAGE_REGEX "^(SB:\\ )?(\\d+)\\ \\[([^:\\]]*):(\\d+)\\]\\ ([^\\r\\n]+)"
#define FORGE_REGEX "^([0-9]+)\\ ([^|]+)\\|([^|^\\r^\\n]+)"
#define GOLDFISH_REGEX "^([0-9]+)\\ ([^|\\r\\n]+)"

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

gchar * strreplace( const gchar * source_str , const gchar * from_str , const gchar * to_str )
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

static gchar * cardname_to_imagename( const gchar * cardname )
{
    gchar * new_str = NULL;

    if ( config_object.image_name_format == FORGE_NAME_FORMAT )
        new_str = strreplace( cardname , " // " , "" );
    else if ( config_object.image_name_format == XMAGE_NAME_FORMAT )
        new_str = strreplace( cardname , "//" , "-" );

    //file system name limmit replace:"\/:*?"<>|"

    return new_str;
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
    GRegex * forge_regex = g_regex_new( FORGE_REGEX , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( forge_regex == NULL )
        goto parse_err;

    GRegex * xmage_regex = g_regex_new( XMAGE_REGEX , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( xmage_regex == NULL )
        goto parse_err;

    GRegex * mtga_regex = g_regex_new( MTGA_REGEX ,
                        G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
    if ( mtga_regex == NULL )
        goto parse_err;

    GRegex * goldfish_regex = g_regex_new( GOLDFISH_REGEX , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
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
    enum CardLocal card_local = COMMAND_LOCAL;
    GError * g_error = NULL;
    GMatchInfo * match_info;
    GRegex * regex = g_regex_new( FORGE_REGEX , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
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

            gchar * image_name = cardname_to_imagename( card->cardname );
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
    GRegex * regex = g_regex_new( XMAGE_REGEX , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
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
            card->cardid = g_ascii_strtoll( words[4] , NULL , 10 );
            g_strlcat( card->cardname , words[5] , BUFFSIZE );

            gchar * image_name = cardname_to_imagename( card->cardname );
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
    enum CardLocal card_local = MAIN_LOCAL;
    GError * g_error = NULL;
    GMatchInfo * match_info;
    GRegex * regex = g_regex_new( MTGA_REGEX , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
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
            gint32 cardid = g_ascii_strtoll( words[4] , NULL , 10 );
            card->card_local = card_local;
            card->cardnumber = cardnumber;
            g_strlcat( card->cardname , words[2] , BUFFSIZE );
            //mtga format set "DOM" name is "DAR"
            if ( ( strncmp( "DAR" , words[3] , 4 ) == 0 ) || ( strncmp( "dar" , words[3] , 4 ) == 0 ) )
                g_strlcat( card->cardseries , "DOM" , BUFFSIZE );
            else
                g_strlcat( card->cardseries , words[3] , BUFFSIZE );
            card->cardid = cardid;

            gchar * image_name = cardname_to_imagename( card->cardname );
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
    enum CardLocal card_local = COMMAND_LOCAL;
    GError * g_error = NULL;
    GMatchInfo * match_info;
    GRegex * regex = g_regex_new( GOLDFISH_REGEX , G_REGEX_EXTENDED | G_REGEX_NEWLINE_ANYCRLF , 0 , &g_error );
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
            gchar * image_name = cardname_to_imagename( card->cardname );
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

GSList * get_cardlist( const gchar * deckfilename )
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
        default:
        {
            return NULL;
        }
    }

    return NULL;
}

void delete_cardlist( GSList * cardlist )
{
    g_slist_free_full( cardlist , ( GDestroyNotify )free_cardobject );
}
