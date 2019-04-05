#pragma once
#ifndef PREVIEWCONFIG_H
#define PREVIEWCONFIG_H

#include "commontypes.h"

struct ConfigObject
{
    enum ImageNameFormat image_name_format;
    char * target_root_directory;
    char * image_root_directory;
    char * image_suffix;
    char * deck_file_directory;
    int window_width;
    int window_height;
    int card_width;
    int card_height;
    int line_card_number;
    int title_font_size;
    int hide_title_bar;
    int copy_file;
    int use_network_image;
};

extern struct ConfigObject config_object;
struct ConfigObject config_object;

void config_inital( void );
void config_destroy( void );

#endif