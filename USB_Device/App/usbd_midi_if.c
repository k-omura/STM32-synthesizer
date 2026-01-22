/**
 ******************************************************************************
 * @file           : usbd_midi_if.c
 * @brief          :
 ******************************************************************************

 (CC at)2016 by D.F.Mac. @TripArts Music

 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_midi_if.h"

#include <stm32_synth.h>

// basic midi rx/tx functions
static uint16_t MIDI_DataRx(uint8_t *msg, uint16_t length);
static uint16_t MIDI_DataTx(uint8_t *msg, uint16_t length);

USBD_MIDI_ItfTypeDef USBD_Interface_fops_FS = { MIDI_DataRx, MIDI_DataTx };

static uint16_t MIDI_DataRx(uint8_t *msg, uint16_t length) {
	uint16_t cnt;
	uint16_t msgs = length / 4;
	uint16_t chk = length % 4;
	uint8_t *midi_message_p = msg;

	if ((chk == 0) && (msgs > 0))
	{
		for (cnt = 0; cnt < msgs; cnt++)
		{
			//stm32synth_inputMIDI(midi_message_p);
			stm32synth_midi_writebuff(midi_message_p);
			midi_message_p += 4;
		}
	}
	return 0;
}

void sendMidiMessage(uint8_t *msg, uint16_t size) {
	if (size == 4) {
		MIDI_DataTx(msg, size);
	}
}

static uint16_t MIDI_DataTx(uint8_t *msg, uint16_t length) {
	uint32_t i = 0;
	while (i < length) {
		APP_Rx_Buffer[APP_Rx_ptr_in] = *(msg + i);
		APP_Rx_ptr_in++;
		i++;
		if (APP_Rx_ptr_in == APP_RX_DATA_SIZE) {
			APP_Rx_ptr_in = 0;
		}
	}
	return USBD_OK;
}

void sendNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
	uint8_t buffer[4];

	buffer[0] = 0x09;
	buffer[1] = 0x90 | ch;
	buffer[2] = 0x7f & note;
	buffer[3] = 0x7f & vel;
	sendMidiMessage(buffer, 4);
}

void sendNoteOff(uint8_t ch, uint8_t note) {
	uint8_t buffer[4];

	buffer[0] = 0x08;
	buffer[1] = 0x80 | ch;
	buffer[2] = 0x7f & note;
	buffer[3] = 0;
	sendMidiMessage(buffer, 4);
}

void sendCtlChange(uint8_t ch, uint8_t num, uint8_t value) {
	uint8_t buffer[4];

	buffer[0] = 0x0b;
	buffer[1] = 0xb0 | ch;
	buffer[2] = 0x7f & num;
	buffer[3] = 0x7f & value;
	sendMidiMessage(buffer, 4);
}

void processMidiMessage() {
	// Rx
	stm32synth_midi_buff2input();

	// Tx
	USBD_MIDI_SendPacket();
}

