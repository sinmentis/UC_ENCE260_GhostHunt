/** @file   display_image.h
    @author Shun and Mike
    @date   19 Oct 2017
    @brief  This function will display bitmap[].
*/
#include "display_image.h"
#include "system.h"
#include "ledmat.h"

/** display the given bitmap*/
void display_image(const uint8_t bitmap[])
{
    ledmat_display_column(bitmap[0], 0);
    ledmat_display_column(bitmap[1], 1);
    ledmat_display_column(bitmap[2], 2);
    ledmat_display_column(bitmap[3], 3);
    ledmat_display_column(bitmap[4], 4);
}

