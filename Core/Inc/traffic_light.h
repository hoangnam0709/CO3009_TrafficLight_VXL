/*
 * traffic_light.h
 *
 *  Created on: 6 thg 11, 2026
 *      Author: cuong
 */

#ifndef INC_TRAFFIC_LIGHT_H_
#define INC_TRAFFIC_LIGHT_H_

#include <stdint.h>

#define ROAD_1   1
#define ROAD_2   2

typedef enum {
    LIGHT_OFF   = 0,
    LIGHT_RED   = 1,
    LIGHT_GREEN = 2,
    LIGHT_AMBER = 3
} light_color_t;

#define OFF     LIGHT_OFF
#define RED     LIGHT_RED
#define GREEN   LIGHT_GREEN
#define AMBER   LIGHT_AMBER

void    traffic_set(uint8_t road, uint8_t color);
uint8_t traffic_get(uint8_t road);   /* last colour set on that road */

#endif /* INC_TRAFFIC_LIGHT_H_ */
