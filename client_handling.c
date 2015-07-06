/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

#include <string.h>
#include <stdbool.h>
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "ble_lbs.h"
#include "ble_db_discovery.h"
#include "client_handling.h"
#include "ble_srv_common.h"
#include "boards.h"

/**@brief Client states. */
typedef enum
{
    IDLE,                                           /**< Idle state. */
    STATE_SERVICE_DISC,                             /**< Service discovery state. */
    STATE_NOTIF_ENABLE,                             /**< State where the request to enable notifications is sent to the peer. . */
    STATE_RUNNING,                                  /**< Running state. */
    STATE_ERROR                                     /**< Error state. */
} client_state_t;

/**@brief Client context information. */
typedef struct
{
    ble_db_discovery_t           srv_db;            /**< The DB Discovery module instance associated with this client. */
    dm_handle_t                  handle;            /**< Device manager identifier for the device. */
    uint8_t                      button_char_index; /**< Client characteristics index in discovered service information. */
    uint8_t                      led_char_index;    /**< Client characteristics index in discovered service information. */
    uint8_t                      state;             /**< Client state. */
} client_t;


static client_t         m_client[MAX_CLIENTS];      /**< Client context information list. */
static uint8_t          m_client_count;             /**< Number of clients. */
static uint8_t          m_base_uuid_type;           /**< UUID type. */


/**@brief Function for finding client context information based on handle.
 *
 * @param[in] conn_handle  Connection handle.
 *
 * @return client context information or NULL upon failure.
 */
static uint32_t client_find(uint16_t conn_handle)
{
    uint32_t i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (m_client[i].srv_db.conn_handle == conn_handle)
        {
            return i;
        }
    }

    return MAX_CLIENTS;
}


/**@brief Function for service discovery.
 *
 * @param[in] p_client Client context information.
 */
static void service_discover(client_t * p_client)
{
    uint32_t   err_code;

    p_client->state = STATE_SERVICE_DISC;

    err_code = ble_db_discovery_start(&(p_client->srv_db),
                                      p_client->srv_db.conn_handle);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling enabling notifications.
 *
 * @param[in] p_client Client context information.
 */
static void notif_enable(client_t * p_client)
{
    uint32_t                 err_code;
    ble_gattc_write_params_t write_params;
    uint8_t                  buf[BLE_CCCD_VALUE_LEN];

    p_client->state = STATE_NOTIF_ENABLE;

    buf[0] = BLE_GATT_HVX_NOTIFICATION;
    buf[1] = 0;

    write_params.write_op = BLE_GATT_OP_WRITE_REQ;
    write_params.handle   = p_client->srv_db.services[0].charateristics[p_client->button_char_index].cccd_handle;
    write_params.offset   = 0;
    write_params.len      = sizeof(buf);
    write_params.p_value  = buf;

    err_code = sd_ble_gattc_write(p_client->srv_db.conn_handle, &write_params);
    APP_ERROR_CHECK(err_code);
}

static void db_discovery_evt_handler(ble_db_discovery_evt_t * p_evt)
{
    // Find the client using the connection handle.
    client_t * p_client;
    uint32_t   index;
    bool       is_valid_srv_found = false;

    index = client_find(p_evt->conn_handle);
    p_client = &m_client[index];

    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE)
    {
        uint8_t i;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            ble_db_discovery_char_t * p_characteristic;

            p_characteristic = &(p_evt->params.discovered_db.charateristics[i]);

            if ((p_characteristic->characteristic.uuid.uuid == LBS_PEER_BUTTON_CHAR_UUID)
                &&
                (p_characteristic->characteristic.uuid.type == m_base_uuid_type))
            {
                // Characteristic found. Store the information needed and break.

                p_client->button_char_index = i;
                is_valid_srv_found   = true;
            }

            if ((p_characteristic->characteristic.uuid.uuid == LBS_PEER_LED_CHAR_UUID)
                &&
                (p_characteristic->characteristic.uuid.type == m_base_uuid_type))
            {
                // Characteristic found. Store the information needed and break.

                p_client->led_char_index = i;
            }
        }
    }

    if (is_valid_srv_found)
    {
        // Enable notification.
        notif_enable(p_client);
    }
    else
    {
        p_client->state = STATE_ERROR;
    }
}


/**@brief Function for setting client to the running state once write response is received.
 *
 * @param[in] p_ble_evt Event to handle.
 */
static void on_evt_write_rsp(ble_evt_t * p_ble_evt, client_t * p_client)
{
    if ((p_client != NULL) && (p_client->state == STATE_NOTIF_ENABLE))
    {
        if (p_ble_evt->evt.gattc_evt.params.write_rsp.handle !=
            p_client->srv_db.services[0].charateristics[p_client->button_char_index].cccd_handle)
        {
            // Got response from unexpected handle.
            p_client->state = STATE_ERROR;
        }
        else
        {
            p_client->state = STATE_RUNNING;
        }
    }
}


/**@brief Function for toggling LEDS based on received notifications.
 *
 * @param[in] p_ble_evt Handle to event.
 * @param[in] p_client  Handle to client handling module.
 * @param[in] index     Client contex Index.
 */
static void on_evt_hvx(ble_evt_t * p_ble_evt, client_t * p_client, uint32_t index, uint32_t * user_data)
{
		 ble_lbs_t * p_lbs = (ble_lbs_t *)user_data;
	
	  (void)index;
    if ((p_client != NULL) && (p_client->state == STATE_RUNNING))
    {
        if ((p_ble_evt->evt.gattc_evt.params.hvx.handle ==
             p_client->srv_db.services[0].charateristics[p_client->button_char_index].characteristic.handle_value)
             &&
             (p_ble_evt->evt.gattc_evt.params.hvx.len == 1) && (p_lbs->led_write_handler != NULL ))
        {
					p_lbs->led_write_handler(p_ble_evt->evt.gattc_evt.params.hvx.data[0], p_ble_evt->header.evt_id);
        }
    }
}


/**@brief Function for handling timeout events.
 */
static void on_evt_timeout(ble_evt_t * p_ble_evt, client_t * p_client)
{
    APP_ERROR_CHECK_BOOL(p_ble_evt->evt.gattc_evt.params.timeout.src
                         == BLE_GATT_TIMEOUT_SRC_PROTOCOL);

    if (p_client != NULL)
    {
        p_client->state = STATE_ERROR;
    }
}


ret_code_t client_handling_dm_event_handler(const dm_handle_t    * p_handle,
                                              const dm_event_t     * p_event,
                                              const ret_code_t     event_result)
{
    client_t * p_client = &m_client[p_handle->connection_id];

    switch (p_event->event_id)
    {
       case DM_EVT_LINK_SECURED:
           // Attempt configuring CCCD now that bonding is established.
           if (event_result == NRF_SUCCESS)
           {
               notif_enable(p_client);
           }
           break;
       default:
           break;
    }

    return NRF_SUCCESS;
}


void client_handling_ble_evt_handler(ble_evt_t * p_ble_evt, uint32_t * user_data)
{
    client_t * p_client = NULL;

    uint32_t index = client_find(p_ble_evt->evt.gattc_evt.conn_handle);
    if (index != MAX_CLIENTS)
    {
       p_client = &m_client[index];
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_WRITE_RSP:
            if ((p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION) ||
                (p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INSUF_ENCRYPTION))
            {
                uint32_t err_code = dm_security_setup_req(&p_client->handle);
                APP_ERROR_CHECK(err_code);

            }
            on_evt_write_rsp(p_ble_evt, p_client);
            break;

        case BLE_GATTC_EVT_HVX:
            on_evt_hvx(p_ble_evt, p_client, index, user_data);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            on_evt_timeout(p_ble_evt, p_client);
            break;

        default:
            break;
    }


    if (p_client != NULL)
    {
        ble_db_discovery_on_ble_evt(&(p_client->srv_db), p_ble_evt);
    }
}


/**@brief Database discovery module initialization.
 */
static void db_discovery_init(void)
{
    uint32_t err_code = ble_db_discovery_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the client handling.
 */
void client_handling_init(void)
{
    uint32_t err_code;
    uint32_t i;

    ble_uuid128_t base_uuid = {LBS_PEER_UUID_BASE};

    err_code = sd_ble_uuid_vs_add(&base_uuid, &m_base_uuid_type);
    APP_ERROR_CHECK(err_code);

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        m_client[i].state  = IDLE;
    }

    m_client_count = 0;

    db_discovery_init();

    // Register with discovery module for the discovery of the service.
    ble_uuid_t uuid;

    uuid.type = m_base_uuid_type;
    uuid.uuid = LBS_PEER_SERVICE_UUID;

    err_code = ble_db_discovery_evt_register(&uuid,
                                             db_discovery_evt_handler);

    APP_ERROR_CHECK(err_code);
}

/**@brief Function for returning the current number of clients.
 */
uint8_t client_handling_count(void)
{
    return m_client_count;
}


/**@brief Function for creating a new client.
 */
uint32_t client_handling_create(const dm_handle_t * p_handle, uint16_t conn_handle)
{
    m_client[p_handle->connection_id].state              = STATE_SERVICE_DISC;
    m_client[p_handle->connection_id].srv_db.conn_handle = conn_handle;
                m_client_count++;
    m_client[p_handle->connection_id].handle             = (*p_handle);
    service_discover(&m_client[p_handle->connection_id]);

    return NRF_SUCCESS;
}


/**@brief Function for freeing up a client by setting its state to idle.
 */
uint32_t client_handling_destroy(const dm_handle_t * p_handle)
{
    uint32_t      err_code = NRF_SUCCESS;
    client_t    * p_client = &m_client[p_handle->connection_id];
    uint32_t      leds[] = LEDS_LIST;
	
    if (p_client->state != IDLE)
    {
            m_client_count--;
            p_client->state = IDLE;
            if(p_handle->connection_id < LEDS_NUMBER)
				 LEDS_OFF( 1 << leds[p_handle->connection_id] );
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }
    return err_code;
}

/**@brief Needed to get the value handle of peer led characteristic handle
 */
uint32_t client_handling_write_char_lbs(uint16_t conn_handle, uint8_t value)
{
    // Find the client using the connection handle.
    client_t * p_client;
    uint32_t   index;
    ble_gattc_write_params_t write_params = {0};
    
    index = client_find(conn_handle);
    p_client = &m_client[index];

    write_params.write_op = BLE_GATT_OP_WRITE_CMD;  // keeping it simple, no response
    write_params.handle   = p_client->srv_db.services[0].charateristics[p_client->led_char_index].characteristic.handle_value;
    write_params.len      = sizeof(value);
    write_params.p_value  = &value;
    
    return sd_ble_gattc_write(conn_handle, &write_params);
}
