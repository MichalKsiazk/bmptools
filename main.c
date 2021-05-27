#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

//#include "bmp_tools.h"
//#include "file_io.h"

//#include "commands.h"


#define BLUE_SELECTOR        0
#define GREEN_SELECTOR       1 
#define RED_SELECTOR         2

#define PIXEL_SIZE           3

#define FSIZE_INFO_SHIFT     2
#define OFFSET_INFO_SHIFT   10
#define COLS_INFO_SHIFT     18
#define ROWS_INFO_SHIFT     22

typedef enum FILE_STATUS
{
    ERROR  = 0,
    SUCCES = 1
}FILE_STATUS;

char* read_file(FILE_STATUS* open_status, char* filename);
void save_file(char* filename, char* file);

uint32_t get_header_value_32(char* file, uint32_t shift);
void set_header_value_32(char* file, uint32_t shift, uint32_t value);

void set_pixel(char* file, uint64_t pixel_pos, uint8_t b, uint8_t g, uint8_t r);
void set_pixel_component(char* file, uint64_t pixel_pos, uint8_t color_selector, uint8_t color);
void set_all_components(char* file, uint8_t color_selector, uint8_t color);
uint8_t get_pixel_component(char* file, uint64_t pixel_pos, uint8_t color_selector);
void set_all_components(char* file, uint8_t color_selector, uint8_t color);
void set_all_pixels(char* file, uint8_t b, uint8_t g, uint8_t r);

uint8_t additional_bytes(char* file, uint64_t pixel_pos);

void fade_up(char* file, uint64_t pixel_pos , uint8_t b, uint8_t g, uint8_t r);
void fade_down(char* file, uint64_t pixel_pos , uint8_t b, uint8_t g, uint8_t r);

bool compare_str(char* a, uint64_t a_start, uint64_t a_lenght, char* b);

uint32_t get_argument(char* command, uint8_t argument_pos, bool* err);

bool is_range_valid(char* file, uint32_t x, uint32_t y);

void cmd_ssp(char* file, char* command);
void cmd_ssc(char* file, char* command);
void cmd_sap(char* file, char* command);
void cmd_sac(char* file, char* command);
void cmd_bsp(char* file, char* command);
void cmd_dsp(char* file, char* command);


char* read_file(FILE_STATUS* open_status, char* filename)
{
    FILE *file_pointer;

    file_pointer = fopen(filename, "rb");




    if(file_pointer == NULL)
    {
        *open_status = ERROR;

        printf("BLAD W TRAKCIE OTWIERANIA PLIKU: %s\n ", filename);
        return NULL;
    }
    else
    {
        fseek(file_pointer, 0, SEEK_END);

        long fsize = ftell(file_pointer);

        fseek(file_pointer, 0, SEEK_SET);
        char* buffer = (char*) malloc(fsize + 1);

        fread(buffer, 1, fsize, file_pointer);

        buffer[fsize] = '\0';

        fclose(file_pointer); 
        *open_status = SUCCES; 
 
        return buffer;
    }

    *open_status = ERROR;
    return NULL;
}


void save_file(char* filename, char* file)
{
    FILE *file_pointer;
    file_pointer = fopen(filename, "wb");


    uint64_t saved_bytes = fwrite(file, 1, (size_t) get_header_value_32(file, FSIZE_INFO_SHIFT), file_pointer);
    printf("%d\n", saved_bytes);
    fclose(file_pointer);
}





uint32_t get_header_value_32(char* file, uint32_t shift)
{
    uint32_t retval = 0;
    for(int i = 0; i < 4; i++)
    {
        uint32_t b = *(file + shift + i) & 0xff;
        uint32_t b_shifted = b << (8 * i); 
        retval += b_shifted; 
    }
    return retval;
}

void set_header_value_32(char* file, uint32_t shift, uint32_t value)
{
    for(int i = 0; i < 4; i++)
    {
        uint8_t b = (uint8_t)  (value >> (8 * i));
        *(file + shift + i) = b;
    }
}


void set_pixel(char* file, uint64_t pixel_pos, uint8_t b, uint8_t g, uint8_t r)
{

    uint8_t extra_bytes = additional_bytes(file, pixel_pos);
    uint32_t data_offset = get_header_value_32(file, OFFSET_INFO_SHIFT);
    uint64_t absolute_pos = data_offset + pixel_pos * PIXEL_SIZE + extra_bytes;

    file[ absolute_pos + BLUE_SELECTOR  ]   = (char) b;
    file[ absolute_pos + GREEN_SELECTOR ]   = (char) g;
    file[ absolute_pos + RED_SELECTOR   ]   = (char) r;

}



void set_pixel_component(char* file, uint64_t pixel_pos, uint8_t color_selector, uint8_t color)
{
    uint8_t extra_bytes = additional_bytes(file, pixel_pos);
    uint32_t data_offset = get_header_value_32(file, OFFSET_INFO_SHIFT);
    uint64_t absolute_pos = data_offset + pixel_pos * PIXEL_SIZE + extra_bytes;

    file[absolute_pos + color_selector] = (char) color;
}


uint8_t get_pixel_component(char* file, uint64_t pixel_pos, uint8_t color_selector)
{
    uint8_t extra_bytes = additional_bytes(file, pixel_pos);
    uint32_t data_offset = get_header_value_32(file, OFFSET_INFO_SHIFT);
    uint64_t absolute_pos = data_offset + pixel_pos * PIXEL_SIZE + extra_bytes;

    uint8_t retval = (uint8_t) file[absolute_pos + color_selector];
    return retval;
}
 

void set_all_components(char* file, uint8_t color_selector, uint8_t color)
{

    uint32_t cols = get_header_value_32(file, COLS_INFO_SHIFT);
    uint32_t rows = get_header_value_32(file, ROWS_INFO_SHIFT);

    uint64_t data_size = cols * rows;

    for(uint64_t i = 0; i < data_size; i++)
    {
        set_pixel_component(file, i, color_selector, color);
    }
}


void set_all_pixels(char* file, uint8_t b, uint8_t g, uint8_t r)
{
    uint32_t cols = get_header_value_32(file, COLS_INFO_SHIFT);
    uint32_t rows = get_header_value_32(file, ROWS_INFO_SHIFT);

    uint64_t data_size = cols * rows;

    for(uint64_t i = 0; i < data_size; i++)
    {
        set_pixel(file, i, b, g, r);
    }
}

uint8_t additional_bytes(char* file, uint64_t pixel_pos)
{

    uint32_t row_size = get_header_value_32(file, COLS_INFO_SHIFT) * PIXEL_SIZE;
    uint8_t additional_bytes = 0;
    while(row_size % 4 != 0)
    {
        additional_bytes++;
        row_size++;
    }
    additional_bytes *= pixel_pos / get_header_value_32(file, COLS_INFO_SHIFT);
    return additional_bytes;
}


void fade_up(char* file, uint64_t pixel_pos , uint8_t b, uint8_t g, uint8_t r)
{
    uint8_t curr_b = get_pixel_component(file, pixel_pos, BLUE_SELECTOR);
    uint8_t curr_g = get_pixel_component(file, pixel_pos, GREEN_SELECTOR);
    uint8_t curr_r = get_pixel_component(file, pixel_pos, RED_SELECTOR);

    uint8_t out_b = curr_b + b > 255 ? 255 : curr_b + b;
    uint8_t out_g = curr_g + g > 255 ? 255 : curr_g + g;
    uint8_t out_r = curr_r + r > 255 ? 255 : curr_r + r;

    set_pixel(file, pixel_pos, out_b, out_g, out_r);
}

void fade_down(char* file, uint64_t pixel_pos , uint8_t b, uint8_t g, uint8_t r)
{

    uint8_t curr_b = get_pixel_component(file, pixel_pos, BLUE_SELECTOR);
    uint8_t curr_g = get_pixel_component(file, pixel_pos, GREEN_SELECTOR);
    uint8_t curr_r = get_pixel_component(file, pixel_pos, RED_SELECTOR);

    int16_t out_b = curr_b - b < 0 ? 0 : curr_b - b;
    int16_t out_g = curr_g - g < 0 ? 0 : curr_g - g;
    int16_t out_r = curr_r - r < 0 ? 0 : curr_r - r;

    set_pixel(file, pixel_pos, (uint8_t) out_b, (uint8_t) out_g, (uint8_t) out_r);
}


bool compare_str(char* a, uint64_t a_start, uint64_t a_lenght, char* b)
{
    uint64_t j = 0;

    for(uint64_t i = a_start; i < a_start + a_lenght; i++)
    {
        if(a[i] != b[j])
        {
            return false;
        }
        j++;
    }

    return true;
}


uint32_t get_argument(char* command, uint8_t argument_pos, bool* err)
{
    uint32_t arg_start = 0;
    uint32_t cnt_args = 0;

    uint32_t len = strlen(command);

    for(uint32_t i = 0; i < len; i++)
    {
        if(command[i] == '<')
        {
            cnt_args++;
        }
        if(cnt_args == argument_pos)
        {
            arg_start = i;
            break;
        }
    }
    if(arg_start == 0)
    {
        *err = true;
        return 0;
    }
    uint32_t arg_end = 0;
    for(uint32_t i = arg_start; i < len; i++)
    {
        if(command[i] == '>')
        {
            arg_end = i;
            break;
        }
    }
    if(arg_end == 0)
    {
        *err = true;
        return 0;
    }
    uint32_t arg_len = arg_end - arg_start - 1;
    char argument[arg_len + 1];
    memcpy(argument, &command[arg_start + 1], arg_len);
    argument[arg_len] = '\0';

    uint32_t retval = strtol(argument, NULL, 10);
    return retval;
}






bool is_range_valid(char* file, uint32_t x, uint32_t y)
{
    uint32_t max_x = get_header_value_32(file, COLS_INFO_SHIFT);
    uint32_t max_y = get_header_value_32(file, ROWS_INFO_SHIFT);
    return x < max_x && y < max_y;
}


void cmd_ssp(char* file, char* command)
{
    bool errc = false;

    uint32_t x = get_argument(command, 1, &errc);
    uint32_t y = get_argument(command, 2, &errc);

    uint8_t b = (uint8_t) get_argument(command, 3, &errc);
    uint8_t g = (uint8_t) get_argument(command, 4, &errc);
    uint8_t r = (uint8_t) get_argument(command, 5, &errc);

    if(!errc)
    {
        bool valid_range = is_range_valid(file, x, y);

        if(valid_range)
        {
            uint64_t pixel_pos = y * get_header_value_32(file, COLS_INFO_SHIFT) + x;
            set_pixel(file, pixel_pos, b, g, r);
        }
        else
        {
            printf("podane argumenty przekraczaja rozmiar bitmapy\n");
        }     
    }
}

void cmd_ssc(char* file, char* command)
{

    bool errc = false;

    uint32_t x = get_argument(command, 1, &errc);
    uint32_t y = get_argument(command, 2, &errc);

    uint8_t color = (uint8_t) get_argument(command, 3, &errc);
    uint8_t color_selector = (uint8_t) get_argument(command, 4, &errc);


    printf("%d\n", x);
    printf("%d\n", y);
    printf("%d\n", color);
    printf("%d\n", color_selector);


    if(!errc)
    {
        bool valid_range = is_range_valid(file, x, y);

        if(valid_range)
        {
            uint64_t pixel_pos = y * get_header_value_32(file, COLS_INFO_SHIFT) + x;
            set_pixel_component(file, pixel_pos, color_selector, color);
        }
        else
        {
            printf("niepoprawne argumenty\n");
        }     
    }
}


void cmd_sap(char* file, char* command)
{
    bool errc = false;

    uint8_t b = (uint8_t) get_argument(command, 1, &errc);
    uint8_t g = (uint8_t) get_argument(command, 2, &errc);
    uint8_t r = (uint8_t) get_argument(command, 3, &errc); 

    if(!errc)
    {
        set_all_pixels(file, b, g, r);
    }
}

void cmd_sac(char* file, char* command)
{
    bool errc = false;

    uint8_t color = (uint8_t) get_argument(command, 1, &errc);
    uint8_t color_selector = (uint8_t) get_argument(command, 2, &errc);

    if(!errc)
    {
        set_all_components(file, color_selector, color);
    }
}

void cmd_bsp(char* file, char* command)
{
    bool errc = false;

    uint32_t x = get_argument(command, 1, &errc);
    uint32_t y = get_argument(command, 2, &errc);

    uint8_t b = (uint8_t) get_argument(command, 3, &errc);
    uint8_t g = (uint8_t) get_argument(command, 4, &errc);
    uint8_t r = (uint8_t) get_argument(command, 5, &errc); 
    
    if(!errc)
    {

        bool valid_range = is_range_valid(file, x, y);

        if(valid_range)
        {
            uint64_t pixel_pos = y * get_header_value_32(file, COLS_INFO_SHIFT) + x;
            fade_up(file, pixel_pos, b, g, r);
        }
        else
        {
            printf("niepoprawne argumenty\n");
        }     
    }
}

void cmd_dsp(char* file, char* command)
{
    bool errc = false;

    uint32_t x = get_argument(command, 1, &errc);
    uint32_t y = get_argument(command, 2, &errc);

    uint8_t b = (uint8_t) get_argument(command, 3, &errc);
    uint8_t g = (uint8_t) get_argument(command, 4, &errc);
    uint8_t r = (uint8_t) get_argument(command, 5, &errc); 
    
    if(!errc)
    {

        bool valid_range = is_range_valid(file, x, y);

        if(valid_range)
        {
            uint64_t pixel_pos = y * get_header_value_32(file, COLS_INFO_SHIFT) + x;
            fade_down(file, pixel_pos, b, g, r);
        }
        else
        {
            printf("niepoprawne argumenty\n");
        }     
    }
}

void print_help()
{
    printf("\n");
    printf("SSP <kolumna> <rzad> (autokorekta) <b> <g> <r> (0-255) - zmien pojedynczy piksel\n\n");
    printf("SSC <kolumna> <rzad> (autokorekta) <wartosc> (0-255) <wybor skladowej> (0-b,1-g,2-r) - zmien pojedyncza skladowa \n\n");
    printf("SAP <r> <g> <b> - zamien wszystkie piksele \n\n");
    printf("SAC <wartosc> (0-255) <wybor skladowej> (0-b,1-g,2-r) - zmien wszystkie skladowe \n\n");
    printf("BSP <kolumna> <rzad> (autokorekta) <r> <g> <b> (0-255) - rozjasnij pojedynczy piksel o wartosc \n\n");
    printf("DSP <kolumna> <rzad> (autokorekta) <r> <g> <b> (0-255) - zciemnij pojedynczy piksel o wartosc \n\n");
    printf("SAVE - zapisz plik \n\n");
    printf("ESC - zamknij program \n\n");

}


void command_interface()
{
    printf("KONSOLOWY EDYTOR PLIKOW .BMP\n wpisz HELP aby wyswietlic liste komend\n");

    char* save_filename;

    char* file;

    while(1)
    {
        char input[256];
        printf("podaj nazwe pliku do edycji: ");
        gets(input);
        printf("\n");

        FILE_STATUS status;

        file = read_file(&status, input);

        if(status == SUCCES)
        {
            printf("plik zostal otworzony i wczytany!\n");
            break;
        }
        else
        {
            printf("nie udalo sie otworzyc pliku!\n");
        }     
    }

        char input[256];
        printf("podaj nazwe pliku do zapisu: ");
        gets(input);
        printf("\n");

        save_filename = input;


        printf("wpisz HELP aby wyswietlic liste polecen\n");

    while(1)
    {

        uint32_t max_x = get_header_value_32(file, COLS_INFO_SHIFT);
        uint32_t max_y = get_header_value_32(file, ROWS_INFO_SHIFT);

        printf("rozmiar bitmapy - x(0-%d)  ", max_x - 1);
        printf("y(0-%d)\n", max_y - 1);


        char input[256];
        printf("komenda: ");
        gets(input);
        printf("\n");

        if(compare_str(input, 0, 4, "HELP"))
        { print_help(); }
        else if(compare_str(input, 0, 3, "SSP"))
        { cmd_ssp(file, input); } 
        else if (compare_str(input, 0, 3, "SSC"))
        { cmd_ssc(file, input); } 
        else if (compare_str(input, 0, 3, "SAP"))
        { cmd_sap(file, input); }
        else if(compare_str(input, 0, 3, "SAC"))
        { cmd_sac(file, input); }
        else if(compare_str(input, 0, 3, "BSP"))
        { cmd_bsp(file, input); }
        else if(compare_str(input, 0, 3, "DSP"))
        {  cmd_dsp(file, input); }
        else if(compare_str(input, 0, 4, "SAVE"))
        {  save_file(save_filename, file); }
        else if(compare_str(input, 0, 3, "ESC"))
        { 
            save_file(save_filename, file);
            return; 
        }    

        
        
    }
}



int main()
{
    command_interface(); 
}
