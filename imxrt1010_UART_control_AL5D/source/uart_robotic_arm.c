/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "fsl_lpuart.h"
#include "fsl_iomuxc.h"
#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LPUART_PC_BOARD            	LPUART1
#define LPUART_BOARD_ARM	   		LPUART4
#define DEMO_LPUART_CLK_FREQ   	 	BOARD_DebugConsoleSrcFreq()
#define DEMO_LPUART_IRQn         	LPUART1_IRQn
#define DEMO_LPUART_IRQHandler   	LPUART1_IRQHandler

/*! @brief Ring buffer size (Unit: Byte). */
#define DEMO_RING_BUFFER_SIZE 	 	16
#define SIZE_BUFFER 		 	 	DEMO_RING_BUFFER_SIZE
/*! @brief Ring buffer to save received data. */

#define MAX_SIZE_ERROR_BUFFER       100
#define MAX_SIZE_INIT_BUFFER		50
#define NB_SERVO 					6
#define LIMIT_HIGH_GRIPPER			2000
#define LIMIT_LOW_GRIPPER			800
#define LIMIT_HIGH_WRIST			2000
#define LIMIT_LOW_WRIST				800
#define LIMIT_HIGH_ELBOW			2000
#define LIMIT_LOW_ELBOW				800
#define LIMIT_HIGH_SHOULDER			2000
#define LIMIT_LOW_SHOULDER			800
#define LIMIT_HIGH_BASE				2000
#define LIMIT_LOW_BASE				800
#define LIMIT_LOW_ROTATION			800
#define LIMIT_HIGH_ROTATION			2000
#define DISTANCE					50
#define DATA_STORE_SIZE				100


#define ERROR_SIG_HIGH				1u
#define ERROR_SIG_LOW				2u
#define ERROR_SIG_KEY				3u
#define NO_ERROR					4u

#define NO_STORE					0u
#define STORE_SPI					1u
#define READ_SPI					2u
typedef enum {
	BASE, SHOULDER, ELBOW, WRIST, GRIPPER, ROTATION
}SERVO_ID;

typedef enum {
	FLAG_INVALIDE_DATA,
	FLAG_VALIDE_DATA,
	FLAG_CONTROLE_DATA,
	NO_DATA
} flag_receive_data_t;
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
flag_receive_data_t  mapper_for_ARM(uint8_t c, uint8_t *sequence, uint8_t *sizeSequence);
void convert_pos_toString(uint16_t pos, uint8_t *posBuffer, uint8_t* bufferSize);
void send_info_seq();
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*
  Ring buffer for data input and output, in this example, input data are saved
  to ring buffer in IRQ handler. The main function polls the ring buffer status,
  if there are new data, then send them out.
  Ring buffer full: (((rxIndex + 1) % DEMO_RING_BUFFER_SIZE) == txIndex)
  Ring buffer empty: (rxIndex == txIndex)
*/
uint8_t demoRingBuffer[DEMO_RING_BUFFER_SIZE];
uint8_t sequenceBuffer[DEMO_RING_BUFFER_SIZE];
uint8_t dataStore[DATA_STORE_SIZE][DEMO_RING_BUFFER_SIZE];
uint8_t sizeSequence[DATA_STORE_SIZE];
volatile uint16_t txIndex; /* Index of the data to send out. */
volatile uint16_t rxIndex; /* Index of the memory to save new arrived data. */
uint16_t rxIndex_pred;
volatile uint16_t storeIndex;
uint16_t actual_pos[NB_SERVO];
uint16_t request_pos[NB_SERVO];
uint8_t error_signal = NO_ERROR;

/*******************************************************************************
 * Code
 ******************************************************************************/
void add_on_LPUART_Write_StringNonBlocking(LPUART_Type *base, const uint8_t *data) {
	const uint8_t *dataAdress = data;
	while (*dataAdress != '\0') {
		while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(base)));
		LPUART_WriteByte(base, *(dataAdress++));
	}
}
void DEMO_LPUART_IRQHandler(void)
{
    uint8_t data;
    uint16_t tmprxIndex = rxIndex;
    uint16_t tmptxIndex = txIndex;


    /* If new data arrived. */
    if ((kLPUART_RxDataRegFullFlag)&LPUART_GetStatusFlags(LPUART_PC_BOARD))
   {
        data = LPUART_ReadByte(LPUART_PC_BOARD);

        /* If ring buffer is not full0, add data to ring buffer. */
        /*if (((tmprxIndex + 1) % DEMO_RING_BUFFER_SIZE) != tmptxIndex)*/
        if ((rxIndex - storeIndex) != DEMO_RING_BUFFER_SIZE)
        {
            demoRingBuffer[(rxIndex % DEMO_RING_BUFFER_SIZE)] = data;
            rxIndex++;
            //rxIndex %= DEMO_RING_BUFFER_SIZE;
            //mapper_for_ARM(data, sequenceBuffer, &sizeARMSequence);
        }
    }
    SDK_ISR_EXIT_BARRIER;
}

void BOARD_InitPins_UART4(void) {
  CLOCK_EnableClock(kCLOCK_Iomuxc);

  IOMUXC_SetPinMux(
		  IOMUXC_GPIO_AD_02_LPUART4_TXD,
		  0U);
  IOMUXC_SetPinMux(
		  IOMUXC_GPIO_AD_01_LPUART4_RXD,
		  0U);

  IOMUXC_SetPinConfig(
		  IOMUXC_GPIO_AD_02_LPUART4_TXD,
		  0x10A0U);

  IOMUXC_SetPinConfig(
		  IOMUXC_GPIO_AD_01_LPUART4_RXD,
		  0x10A0U);
}

// cmd of type #1P1500\r
void initSeq_ARM(void) {
	uint8_t i;
	char temp;
	uint8_t initBuffer[SIZE_BUFFER];

	initBuffer[0] = '#';
	for(i = 0; i < NB_SERVO + 1; i++) {
		temp = i + '0';
		initBuffer[1] = temp; /**convertir en char*/
		initBuffer[2] = 'P';
		initBuffer[3] = '1';
		initBuffer[4] = '5';
		initBuffer[5] = '0';
		initBuffer[6] = '0';
		initBuffer[7] = '\r';
		actual_pos[i] = 1500;
		uint8_t k  = 0;
		rxIndex   = 0;
		txIndex    = 0;
		storeIndex = 0;
		for (k = 0; k < 8 ; k++) {
			while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_BOARD_ARM)));
			LPUART_WriteByte(LPUART_BOARD_ARM, initBuffer[k]);

			while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_PC_BOARD)));
			LPUART_WriteByte(LPUART_PC_BOARD, initBuffer[k]);
		}
	}
}

flag_receive_data_t  mapper_for_ARM(uint8_t c, uint8_t *sequence, uint8_t *sizeSequence) {
	uint8_t error_string[MAX_SIZE_ERROR_BUFFER] = "limit ";
	char id_servo;
	uint16_t position;
	uint8_t buffer_pos[16];
	flag_receive_data_t data_flag = FLAG_VALIDE_DATA;

	uint8_t id_sequence = 0;
	switch(c) {
	case 'q': /*BASE LEFT/ or HIGH*/

		request_pos[BASE] = (actual_pos[BASE] - DISTANCE);
		if (request_pos[BASE] > LIMIT_LOW_BASE) {
			actual_pos[BASE] = request_pos[BASE];
			position = actual_pos[BASE];
			id_servo = BASE + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}

		break;
	case 'd':
		request_pos[BASE] = (actual_pos[BASE] + DISTANCE);
		if (request_pos[BASE] < LIMIT_HIGH_BASE) {
			actual_pos[BASE] = request_pos[BASE];
			position = actual_pos[BASE];
			id_servo = BASE + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
	case 'z': /*SHOULDER LEFT/ or HIGH*/
		request_pos[SHOULDER] = (actual_pos[SHOULDER] + DISTANCE);
		if (request_pos[SHOULDER] < LIMIT_HIGH_SHOULDER) {
			actual_pos[SHOULDER] = request_pos[SHOULDER];
			position = actual_pos[SHOULDER];
			id_servo = SHOULDER + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
	case 's': /*SHOULDER LEFT/ or HIGH*/
		request_pos[SHOULDER] = (actual_pos[SHOULDER] - DISTANCE);
		if (request_pos[SHOULDER] > LIMIT_LOW_SHOULDER) {
			actual_pos[SHOULDER] = request_pos[SHOULDER];
			position = actual_pos[SHOULDER];
			id_servo = SHOULDER + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
	case 'a':  /*WRIST OPEN */

		request_pos[GRIPPER] = actual_pos[GRIPPER] - DISTANCE;
		if (request_pos[GRIPPER] > LIMIT_LOW_GRIPPER) {
			actual_pos[GRIPPER] = request_pos[GRIPPER];
			position = actual_pos[GRIPPER];
			id_servo = GRIPPER + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
	case 'e':		/*WRIST CLOSE */
		request_pos[GRIPPER] = actual_pos[GRIPPER] + DISTANCE;
		if (request_pos[GRIPPER] < LIMIT_HIGH_GRIPPER) {
			actual_pos[GRIPPER]  = request_pos[GRIPPER];
			position = actual_pos[GRIPPER];
			id_servo = GRIPPER + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
	case 'i':
		request_pos[WRIST] = actual_pos[WRIST] + DISTANCE;
		if (request_pos[WRIST] != LIMIT_HIGH_WRIST) {
			actual_pos[WRIST] = request_pos[WRIST];
			position = actual_pos[WRIST];
			id_servo = WRIST + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
	case 'k':
		request_pos[WRIST] = actual_pos[WRIST] - DISTANCE;
		if (request_pos[WRIST] != LIMIT_HIGH_WRIST) {
			actual_pos[WRIST] = request_pos[WRIST];
			position = actual_pos[WRIST];
			id_servo = WRIST + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
	case 'r':
			request_pos[ROTATION] = (actual_pos[ROTATION] - DISTANCE);
			if (request_pos[ROTATION] > LIMIT_LOW_ROTATION) {
				actual_pos[ROTATION] = request_pos[ROTATION];
				position = actual_pos[ROTATION];
				id_servo = ROTATION + '0';
			} else {
				error_signal = ERROR_SIG_LOW;
				data_flag = FLAG_INVALIDE_DATA;
				return data_flag;
			}
			break;
	case 't':
			request_pos[ROTATION] = (actual_pos[ROTATION] + DISTANCE);
			if (request_pos[ROTATION] < LIMIT_HIGH_ROTATION) {
				actual_pos[ROTATION] = request_pos[ROTATION];
				position = actual_pos[ROTATION];
				id_servo = ROTATION + '0';
			} else {
				error_signal = ERROR_SIG_HIGH;
				data_flag = FLAG_INVALIDE_DATA;
				return data_flag;
			}
			break;
	case 'j':
		request_pos[ELBOW] = (actual_pos[ELBOW] - DISTANCE);
		if (request_pos[ELBOW] > LIMIT_LOW_ROTATION) {
			actual_pos[ELBOW] = request_pos[ELBOW];
			position = actual_pos[ELBOW];
			id_servo = ELBOW + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
		break;
	case 'l':
		request_pos[ELBOW] = (actual_pos[ELBOW] + DISTANCE);
		if (request_pos[ELBOW] < LIMIT_HIGH_ROTATION) {
			actual_pos[ELBOW] = request_pos[ELBOW];
			position = actual_pos[ELBOW];
			id_servo = ELBOW + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			data_flag = FLAG_INVALIDE_DATA;
			return data_flag;
		}
		break;
	case '\r':
		data_flag = FLAG_CONTROLE_DATA;
		initSeq_ARM();
		break;

	default:
		error_signal = ERROR_SIG_KEY;
		return data_flag;

	}
	if (data_flag == FLAG_VALIDE_DATA) {
		//data_flag = FLAG_VALIDE_DATA;
		sequence[id_sequence++] = '#';
		sequence[id_sequence++] = id_servo;
		sequence[id_sequence++] = 'P';
		uint8_t bufferSize;
		convert_pos_toString(position, buffer_pos, &bufferSize);
		for(uint8_t i = 0; i < bufferSize; i++) {
			sequence[i + id_sequence] = buffer_pos[i];
		}
		id_sequence += bufferSize;
		sequence[id_sequence] = '\r';
		id_sequence ++;
		*sizeSequence = id_sequence;
	}
    return data_flag;
}

/*!
 * @brief Concert decimal position (as 1800) into string buffer for send sequence to AL5D
 * uint16_t pos  				(Input) pos to convert
 * uint8_t *posBuffer			(Ouput) Buffer so store the converted position
 * uint8_t* bufferSize          (Ouput) lenght of the string usefull to send the exact sequence to AL5D
 */

void convert_pos_toString(uint16_t pos, uint8_t *posBuffer, uint8_t* bufferSize) {
	uint16_t temp 	 = pos;
	char tempChar;
	uint8_t idBuffer = 0;
	uint8_t buffSize;
	uint16_t pivot = 1000;
	buffSize = ((pos >= 1000) ? 4 :
					(pos >= 100) ? 3 :
							(pos >= 10) ? 2 :
									(pos > 0) ? 1 : 0);
	*bufferSize = buffSize;
	while ((temp > 0) && (idBuffer < buffSize)) {
		if (temp >= pivot) {
			tempChar = (temp / pivot) + '0';
			posBuffer[idBuffer++] = tempChar;
			temp %= pivot;
			pivot /= 10;
			//idBuffer ++;
			if(temp == 0) {
				for(uint8_t i = idBuffer; i < buffSize; i++) {
					posBuffer[idBuffer++] = '0';
					//idBuffer++;
				}
			}
		} else { /* temp <= pivot */
			pivot /= 10;
			if (idBuffer != 0) {
				posBuffer[idBuffer++] = '0';
			}
		}
	}
}
void save_sequence_dataStore(uint8_t *data, uint8_t dataSize, uint8_t dataStore[DEMO_RING_BUFFER_SIZE][DEMO_RING_BUFFER_SIZE]/*, uint16_t *indexToStore*/) {
	uint16_t tmpstoreIndex = storeIndex;
	for(uint8_t i = 0; i < dataSize; i++) {
		dataStore[tmpstoreIndex][i] = data[i];
	}
	sizeSequence[tmpstoreIndex] = dataSize;
	storeIndex = storeIndex + 1;
}

void send_info_seq() {

	uint8_t nb_line = 6;
	uint8_t offset  = 0;
	uint8_t* sequence_init1 = "		UART based C server :\r\n";
	uint8_t* sequence_init2 = " please control the robot arm using your keyboard \r\n";
	uint8_t* sequence_init3 = " (a;e) =>  Gripper open / close  \r\n";
	uint8_t* sequence_init4 = " (r;t) =>  Gripper rotation \r\n";
	uint8_t* sequence_init5 = " (q;d) =>  Base rotation \r\n";
	uint8_t* sequence_init6 = " (z;s) =>  Shoulder rotation\r\n";
	uint8_t* sequence_init7 = " (i;k) =>  Wrist rotation \r\n";
	uint8_t* sequence_init8 = " (j;l) =>  Elbow rotation \r\n";
	uint8_t* sequence_init9 = " ENTER =>  memorisation of a sequence \r\n";

	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init1);
	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init2);
	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init3);
	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init4);
	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init5);
	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init6);
	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init7);
	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init8);
	add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, sequence_init8);
}
/*!
    Error handler
 *
 */

void error_management(/*uint16_t *rxIndex, uint16_t* storeIndex,
					  uint16_t *txIndex*/) {
	   if (error_signal != NO_ERROR) {
		   storeIndex = 0;
		   txIndex    = 0;
		   rxIndex    = 0;
	   }
	   uint8_t *error_high_string =  "limit high reached\r\n";
	   uint8_t *error_low_string  =  "limit low reached\r\n";
	   uint8_t * indication_string = "(a;e), (q;d), (z;s), (i;k), \r\n";
	   uint8_t *error_inv_string  = "invalide key please try the following combinaison: \r\n";
	   switch (error_signal) {
	   case ERROR_SIG_HIGH:
		   add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, error_high_string);
		   break;
	   case ERROR_SIG_LOW:
		   add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, error_low_string);
		   break;
	   case ERROR_SIG_KEY:
		   add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, error_inv_string);
		   add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, indication_string);
		   break;
	   default: /* Last time error was detected and user did not type a correct info yet so e still wait */
		   break;
	   }
	   /*flag_receive_data_pred = flag_receive_data;*/
}
/*!
 * @brief consumer function : translate the command (in case of accurate one) into sequence for AL5D

 */
uint8_t consumer_fn(uint8_t cmd/*, uint16_t *rxIndex, uint16_t* storeIndex, uint16_t *txIndex*/) {
	flag_receive_data_t flag_receive_data;
	uint8_t sizeARMSequence;
	error_signal = NO_ERROR;
	uint16_t tmprxIndex    = rxIndex;
	uint16_t tmpstoreIndex = storeIndex;
	uint16_t tmptxIndex    = txIndex;

    if (tmprxIndex > tmpstoreIndex) { /* Donne reçus et pas encore stockées*/
    	flag_receive_data = mapper_for_ARM(cmd, sequenceBuffer, &sizeARMSequence);                   /* Pour une donne on construit la sequence */
    	if (flag_receive_data == FLAG_VALIDE_DATA) {
    		save_sequence_dataStore(sequenceBuffer, sizeARMSequence, dataStore);		  /* Pour une donne reçus on stocke la sequence */
    	}
    }
    /*tmprxIndex_pred   = g_HostHidKeyboard.add_on_rxIndex;*/
    if (flag_receive_data == FLAG_VALIDE_DATA) {
        if (tmptxIndex < storeIndex) { 													/* Donne stockés et pas encore envoyés*/
        	/* Transmit to robotic arm */
        	while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_BOARD_ARM)));
        	LPUART_WriteBlocking(LPUART_BOARD_ARM,dataStore[tmptxIndex] , sizeSequence[tmptxIndex]);/*sizeof(dataStore[tmptxIndex]) / sizeof(dataStore[tmptxIndex][0])*/
        	/* Transmit to debug session  */
        	uint8_t new_line = '\n';
        	while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_PC_BOARD)));
        	LPUART_WriteBlocking(LPUART_PC_BOARD,&new_line , 1);
        	while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_PC_BOARD)));
        	while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_PC_BOARD)));
            LPUART_WriteBlocking(LPUART_PC_BOARD,dataStore[tmptxIndex] , sizeSequence[tmptxIndex]);
            txIndex = txIndex + 1;
        }
    } else if (flag_receive_data == FLAG_INVALIDE_DATA) {  /* Incorrect value of data */
    		error_management(/*rxIndex, storeIndex, txIndex*/);
       }
	return flag_receive_data;
}
/*!
 * @brief main function
 *
 */

uint8_t control_arm_AL5D(/*uint16_t *rxIndex , uint16_t* storeIndex, uint16_t *txIndex*/) {
	uint8_t cmd;
	uint8_t main_flag;
	if (rxIndex > storeIndex) {
		cmd = demoRingBuffer[((storeIndex) % DEMO_RING_BUFFER_SIZE)];
		main_flag = consumer_fn(cmd/*, rxIndex, storeIndex, txIndex*/);
	}

	return main_flag;
}
/*!
 * @brief Main function
 */
int main(void)
{
    lpuart_config_t config;
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitPins_UART4();

    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;
    LPUART_Init(LPUART_PC_BOARD, &config, DEMO_LPUART_CLK_FREQ);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;
    LPUART_Init(LPUART_BOARD_ARM, &config , DEMO_LPUART_CLK_FREQ);
    LPUART_EnableInterrupts(LPUART_PC_BOARD, kLPUART_RxDataRegFullInterruptEnable);
    EnableIRQ(DEMO_LPUART_IRQn);
    initSeq_ARM();
    send_info_seq();
    rxIndex    = 0;
    storeIndex = 0;
    txIndex    = 0;
    while (1)
    {
    control_arm_AL5D(/*&rxIndex , &storeIndex, &txIndex*/);
    }
}
