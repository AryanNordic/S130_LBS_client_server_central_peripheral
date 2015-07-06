/* Copyright (C) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#ifndef DEVICE_CONFIG_H__
#define DEVICE_CONFIG_H__

#define LBS_UUID_BASE {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}
#define LBS_UUID_SERVICE 0x1523
#define LBS_UUID_LED_CHAR 0x1525
#define LBS_UUID_BUTTON_CHAR 0x1524

#define LBS_PEER_UUID_BASE            {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x33, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}
#define LBS_PEER_SERVICE_UUID         0x1533
#define LBS_PEER_LED_CHAR_UUID        0x1535
#define LBS_PEER_BUTTON_CHAR_UUID     0x1534
                                      /**< Peripheral characterisctics UUID. */

#endif // DEVICE_CONFIG_H__
