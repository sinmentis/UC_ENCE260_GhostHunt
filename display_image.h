/** @file   display_image.h
    @author Shun and Mike
    @date   19 Oct 2017
    @brief  This function will display bitmap[].
*/

#include "system.h"

/* write bitmap before you use it.
 * example:
 * static const uint8_t bitmap[] = {0x21, 0x2C, 0x08, 0x2A, 0x00};
 * static const uint8_t ghost_map[] = {0x38, 0x57, 0x41, 0x57, 0x38};
 * static const uint8_t human_map[] = {0x3E, 0x55, 0x45, 0x55, 0x3E};
 */
 
void 
display_image(const uint8_t bitmap[]);
