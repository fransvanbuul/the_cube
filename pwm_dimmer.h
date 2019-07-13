#include <avr/pgmspace.h>

#define DIMMER_STEPS 256

#define low8(x)  (uint8_t)(x & 0xFF)
#define high8(x)  (uint8_t)(x >> 8)

const uint16_t dimmer_values[DIMMER_STEPS] = {
    0, 4, 6, 7, 9, 11, 12, 14, 15, 17,
    19, 20, 22, 24, 26, 27, 29, 31, 33, 35,
    37, 39, 41, 43, 45, 47, 49, 51, 53, 55,
    58, 60, 62, 65, 67, 69, 72, 74, 77, 79,
    82, 85, 87, 90, 93, 95, 98, 101, 104, 107,
    110, 113, 116, 119, 122, 126, 129, 132, 136, 139,
    143, 146, 150, 153, 157, 161, 165, 168, 172, 176,
    180, 184, 189, 193, 197, 201, 206, 210, 215, 219,
    224, 229, 234, 238, 243, 248, 253, 259, 264, 269,
    275, 280, 286, 291, 297, 303, 309, 315, 321, 327,
    333, 340, 346, 353, 359, 366, 373, 380, 387, 394,
    401, 408, 416, 424, 431, 439, 447, 455, 463, 471,
    480, 488, 497, 506, 515, 524, 533, 542, 552, 561,
    571, 581, 591, 601, 611, 622, 632, 643, 654, 665,
    676, 688, 699, 711, 723, 735, 748, 760, 773, 786,
    799, 812, 825, 839, 853, 867, 881, 896, 910, 925,
    940, 956, 971, 987, 1003, 1019, 1036, 1053, 1070, 1087,
    1104, 1122, 1140, 1158, 1177, 1196, 1215, 1234, 1254, 1274,
    1294, 1315, 1336, 1357, 1379, 1400, 1423, 1445, 1468, 1491,
    1515, 1538, 1563, 1587, 1612, 1637, 1663, 1689, 1716, 1742,
    1770, 1797, 1825, 1854, 1883, 1912, 1942, 1972, 2002, 2033,
    2065, 2097, 2129, 2162, 2196, 2230, 2264, 2299, 2335, 2371,
    2407, 2444, 2482, 2520, 2559, 2598, 2638, 2679, 2720, 2761,
    2804, 2847, 2890, 2934, 2979, 3025, 3071, 3118, 3165, 3214,
    3263, 3313, 3363, 3414, 3466, 3519, 3573, 3627, 3682, 3738,
    3795, 3852, 3911, 3970, 4030, 4095};
