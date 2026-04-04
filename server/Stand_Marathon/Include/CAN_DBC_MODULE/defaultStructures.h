#ifndef DEFAULT_STRUCTURES_H
#define DEFAULT_STRUCTURES_H	  

#include <stdint.h>

typedef enum
{
	CAN_TX_STATUS_NO_MAILBOX_DEFAULT = 0, /*!< can transmission no empty mailbox */
	CAN_TX_STATUS_SUCCESS_DEFAULT = 1 // else
} can_transmit_status_type_default;

typedef struct
{
	uint32_t message_id;                  /*!< specifies the 11 bits standard identifier.
	                                            this parameter can be a value between 0 to 0x7FF. */

	uint8_t dlc;                           /*!< specifies frame data length that will be transmitted.
	                                            this parameter can be a value between 0 to 8 */

	uint8_t data[8];                       /*!< contains the transmit data. it ranges from 0 to 0xFF. */

} dbc_can_tx_message_type;

typedef struct
{
	uint32_t message_id;                /*!< specifies the 11 bits standard identifier
	                                          this parameter can be a value between 0 to 0x7FF. */

	uint8_t dlc;                         /*!< specifies the frame data length that will be received.
	                                          this parameter can be a value between 0 to 8 */

	uint8_t data[8];                     /*!< contains the receive data. it ranges from 0 to 0xFF.*/

} dbc_can_rx_message_type;

#endif