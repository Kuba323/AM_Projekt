#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "amcom.h"

/// Start of packet character
const uint8_t  AMCOM_SOP         = 0xA1;
const uint16_t AMCOM_INITIAL_CRC = 0xFFFF;

static uint16_t AMCOM_UpdateCRC(uint8_t byte, uint16_t crc)
{
    byte ^= (uint8_t)(crc & 0x00ff);
    byte ^= (uint8_t)(byte << 4);
    return ((((uint16_t)byte << 8) | (uint8_t)(crc >> 8)) ^ (uint8_t)(byte >> 4) ^ ((uint16_t)byte << 3));
}


void AMCOM_InitReceiver(AMCOM_Receiver* receiver, AMCOM_PacketHandler packetHandlerCallback, void* userContext) {
    // TODO

    receiver->receivedPacketState=AMCOM_PACKET_STATE_EMPTY;
    receiver->packetHandler = packetHandlerCallback;
    receiver->userContext = userContext;

}

size_t AMCOM_Serialize(uint8_t packetType, const void* payload, size_t payloadSize, uint8_t* destinationBuffer) {
    // TODO
    if(destinationBuffer==NULL) return 0;

    destinationBuffer[0] = AMCOM_SOP;
    destinationBuffer[1] = packetType;
    destinationBuffer[2] = payloadSize;
    uint16_t crc = AMCOM_INITIAL_CRC;
    if(payloadSize == 0){

        crc = AMCOM_UpdateCRC(packetType, crc);
        crc = AMCOM_UpdateCRC(payloadSize, crc);
    }else{

        crc = AMCOM_UpdateCRC(packetType, crc);
        crc = AMCOM_UpdateCRC(payloadSize, crc);
        const uint8_t* payloadBytes = (const uint8_t*)payload;
        for(size_t i = 0; i < payloadSize; i++) {
            crc = AMCOM_UpdateCRC(payloadBytes[i], crc);
        }
    }
    destinationBuffer[3] = crc;
    destinationBuffer[4] = crc>>8;

    if(payload != NULL && payloadSize>0){
        const uint8_t* payloadPtr = (const uint8_t*)payload;
        for(size_t i=0; i<payloadSize; i++){
            destinationBuffer[i+5] = payloadPtr[i];
        }
        return payloadSize+5;
    }
    else{
        return 5;
    }


}

void AMCOM_Deserialize(AMCOM_Receiver* receiver, const void* data, size_t dataSize) {
    // TODO



    const uint8_t* buffer = (const uint8_t *)data;

    for(uint8_t i=0; i<dataSize; i++){

        uint8_t byte = buffer[i];

        switch(receiver->receivedPacketState){

            case AMCOM_PACKET_STATE_EMPTY:
                if(byte==AMCOM_SOP){
                    receiver->receivedPacket.header.sop=byte;
                    receiver->receivedPacketState=AMCOM_PACKET_STATE_GOT_SOP;
                }
                break;

            case AMCOM_PACKET_STATE_GOT_SOP:
                receiver->receivedPacket.header.type = byte;
                receiver->receivedPacketState=AMCOM_PACKET_STATE_GOT_TYPE;
                break;

            case AMCOM_PACKET_STATE_GOT_TYPE:
                if(byte>200) receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                else{

                    receiver->payloadCounter=0;
                    receiver->receivedPacket.header.length = byte;
                    receiver->receivedPacketState=AMCOM_PACKET_STATE_GOT_LENGTH;
                }
                break;

            case AMCOM_PACKET_STATE_GOT_LENGTH:
                receiver->receivedPacket.header.crc = byte;
                receiver->receivedPacketState=AMCOM_PACKET_STATE_GOT_CRC_LO;
                break;

            case AMCOM_PACKET_STATE_GOT_CRC_LO:
                receiver->receivedPacket.header.crc |= (uint16_t)byte<<8;
                if(receiver->receivedPacket.header.length==0){
                    receiver->receivedPacketState=AMCOM_PACKET_STATE_GOT_WHOLE_PACKET;
                }
                else{
                    receiver->receivedPacketState=AMCOM_PACKET_STATE_GETTING_PAYLOAD;
                }
                break;
            case AMCOM_PACKET_STATE_GETTING_PAYLOAD:
                receiver->receivedPacket.payload[receiver->payloadCounter++] = byte;

                if(receiver->payloadCounter>=receiver->receivedPacket.header.length){
                    receiver->receivedPacketState=AMCOM_PACKET_STATE_GOT_WHOLE_PACKET;
                }
                break;
        }
    }
    if(receiver->receivedPacketState==AMCOM_PACKET_STATE_GOT_WHOLE_PACKET){
        uint16_t crc = AMCOM_INITIAL_CRC;
        if(receiver->receivedPacket.header.length == 0){

            crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.type, crc);
            crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.length, crc);
        }else{

            crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.type, crc);
            crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.length, crc);
            for(size_t i = 0; i < receiver->receivedPacket.header.length; i++) {
                crc = AMCOM_UpdateCRC(receiver->receivedPacket.payload[i], crc);
            }
        }

        if(crc == receiver->receivedPacket.header.crc){
            receiver->packetHandler(&receiver->receivedPacket, receiver->userContext);
        }
        receiver->receivedPacketState=AMCOM_PACKET_STATE_EMPTY;
    }






}
