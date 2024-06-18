#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "amcom.h"
#include "amcom_packets.h"

uint8_t playerNumber;
uint8_t numberOfPlayers;
uint32_t gameTime=0;
uint8_t size;
uint8_t numberOfFoods=0;
bool playerFound=false;

struct Player{
    uint8_t playerNo;
    uint16_t hp;
    float x;
    float y;

};
struct Food{
    uint16_t foodNo;
    uint8_t state;
    float x;
    float y;
};

struct Player player[8];
struct Food food[100];

void amPacketHandler(const AMCOM_Packet* packet, void* userContext) {
    uint8_t buf[AMCOM_MAX_PACKET_SIZE];
    size_t toSend = 0;

    //static AMCOM_MoveRequestPayload movePayload;

    switch (packet->header.type) {
        case AMCOM_IDENTIFY_REQUEST:
            printf("Got IDENTIFY.request. Responding with IDENTIFY.response\n");
            AMCOM_IdentifyResponsePayload identifyResponse;
            sprintf(identifyResponse.playerName, "Michal&Kuba");
            toSend = AMCOM_Serialize(AMCOM_IDENTIFY_RESPONSE, &identifyResponse, sizeof(identifyResponse), buf);
            break;


        case AMCOM_NEW_GAME_REQUEST:
            printf("Got NEW_GAME.request. Responding with NEW_GAME.response\n");
            AMCOM_NewGameResponsePayload newgameResponse;
            playerNumber = packet->payload[0];
            numberOfPlayers = packet->payload[1];
            numberOfFoods = numberOfPlayers*6;

            sprintf(newgameResponse.helloMessage, "Elooo!");
            toSend = AMCOM_Serialize(AMCOM_NEW_GAME_RESPONSE, &newgameResponse, sizeof(newgameResponse), buf);
            break;


        case AMCOM_FOOD_UPDATE_REQUEST:
            printf("Got FOOD_UPDATE.request.\n");
            for(int i = 0; i < packet->header.length; i += 11)
            {
                uint16_t Num = ((uint16_t)packet->payload[i] | ((uint16_t)packet->payload[i+1] << 8));
                food[Num].state = packet->payload[i+2];

                if(gameTime <= 0)
                {
                    food[Num].foodNo = Num;

                    memcpy(&food[Num].x, &packet->payload[i+3], sizeof(float));
                    memcpy(&food[Num].y, &packet->payload[i+7], sizeof(float));
                }
            }
            break;

        case AMCOM_PLAYER_UPDATE_REQUEST:
            printf("Got PLAYER_UPDATE.request. Responding with PLAYER_UPDATE.response\n");
            
            for(int i = 0; i < packet->header.length; i += 11)
            {
                uint8_t Num = packet->payload[i];
                if(gameTime == 0)
                {
                    player[Num].playerNo = Num;
                }

                uint16_t hp = packet->payload[i+1] | (packet->payload[i+2] << 8);
                player[Num].hp = hp;

                memcpy(&player[Num].x, &packet->payload[i+3], sizeof(float));
                memcpy(&player[Num].y, &packet->payload[i+7], sizeof(float));
            }
            break;


        case AMCOM_MOVE_REQUEST:
            printf("Got MOVE.request. Responding with MOVE.response\n");
            AMCOM_MoveResponsePayload moveResponse;
            AMCOM_MoveRequestPayload *getTime = (AMCOM_MoveRequestPayload *)packet->payload;
            gameTime = getTime->gameTime;

            float minResult=1000;
            float tmp=0;
            float minX=0;
            float minY=0;
            float tempX=player[playerNumber].x;
            float tempY=player[playerNumber].y;
            float PlayerX=0;
            float PlayerY=0;
            if(gameTime<60) {
                for (uint8_t i = 0; i < numberOfFoods; i++) {
                    if (food[i].state == 1) {
                        tmp = sqrt(pow(food[i].x - tempX, 2) +
                                   pow(food[i].y - tempY, 2));
                        if (tmp < minResult) {
                            minResult = tmp;
                            minX = food[i].x;
                            minY = food[i].y;

                        }
                    }
                }
            }else{
                for(uint8_t i=0; i<numberOfPlayers; i++){
                    if(player[i].playerNo == playerNumber) continue;
                    else if(player[i].hp==0) continue;
                    else{
                        if(player[playerNumber].hp>player[i].hp){
                            tmp = sqrtf(powf(player[playerNumber].x - player[i].x,2) + powf(player[playerNumber].y - player[i].y,2));
                            if(tmp<minResult){
                                minResult = tmp;
                                PlayerX = player[i].x;
                                PlayerY = player[i].y;
                                playerFound=true;
                            }

                        }
                    }
                }
                if(playerFound==true) {
                    moveResponse.angle = atan2f(PlayerY - player[playerNumber].y, PlayerX - player[playerNumber].x);
                }
                else if(playerFound==false){
                    for (uint8_t i = 0; i < numberOfFoods; i++) {
                        if (food[i].state == 1) {
                            tmp = sqrt(pow(food[i].x - tempX, 2) +
                                       pow(food[i].y - tempY, 2));
                            if (tmp < minResult) {
                                minResult = tmp;
                                minX = food[i].x;
                                minY = food[i].y;

                            }
                        }
                    }
                    moveResponse.angle = atan2f(PlayerY - player[playerNumber].y, PlayerX - player[playerNumber].x) + M_PI;
                }
                else{
                    for(uint8_t i=0; i<numberOfPlayers; i++){
                        if(player[i].playerNo == playerNumber) continue;
                        else if(player[i].hp==0) continue;
                        else{
                            if(player[playerNumber].hp>player[i].hp){
                                tmp = sqrtf(powf(player[playerNumber].x - player[i].x,2) + powf(player[playerNumber].y - player[i].y,2));
                                if(tmp<minResult){
                                    minResult = tmp;
                                    PlayerX = player[i].x;
                                    PlayerY = player[i].y;
//                                    playerFound=true;
                                }

                            }
                        }
                    }
                    moveResponse.angle = atan2f(PlayerY - player[playerNumber].y, PlayerX - player[playerNumber].x) + M_PI;
                }
            }

            moveResponse.angle= atan2(minY-tempY, minX-tempX);
            toSend = AMCOM_Serialize(AMCOM_MOVE_RESPONSE, &moveResponse, sizeof(moveResponse), buf);
            break;

        case AMCOM_GAME_OVER_REQUEST:
            printf("Got GAME_OVER.request. Responding with GAME_OVER.response\n");
            AMCOM_GameOverResponsePayload gameOverResponse;
            sprintf(gameOverResponse.endMessage, "Miło było");
            toSend = AMCOM_Serialize(AMCOM_GAME_OVER_RESPONSE, &gameOverResponse, sizeof(gameOverResponse), buf);
            break;

        case AMCOM_NO_PACKET:
            printf("Got NO_PACKET.request. Responding with NO_PACKET.response\n");
            break;
    }
    // retrieve socket from user context
    SOCKET ConnectSocket  = *((SOCKET*)userContext);
    // send response if any
    int bytesSent = send(ConnectSocket, (const char*)buf, toSend, 0 );
    if (bytesSent == SOCKET_ERROR) {
        printf("Socket send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        return;
    }
}


#define GAME_SERVER "localhost"
#define GAME_SERVER_PORT "2001"

int main(int argc, char **argv) {
    printf("This is mniAM player. Let's eat some transistors! \n");

    WSADATA wsaData;
    int iResult;

    // Initialize Winsock library (windows sockets)
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    // Prepare temporary data
    SOCKET ConnectSocket  = INVALID_SOCKET;
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
    int iSendResult;
    char recvbuf[512];
    int recvbuflen = sizeof(recvbuf);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the game server address and port
    iResult = getaddrinfo(GAME_SERVER, GAME_SERVER_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    printf("Connecting to game server...\n");
    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    // Free some used resources
    freeaddrinfo(result);

    // Check if we connected to the game server
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to the game server!\n");
        WSACleanup();
        return 1;
    } else {
        printf("Connected to game server\n");
    }

    AMCOM_Receiver amReceiver;
    AMCOM_InitReceiver(&amReceiver, amPacketHandler, &ConnectSocket);

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 ) {
            AMCOM_Deserialize(&amReceiver, recvbuf, iResult);
        } else if ( iResult == 0 ) {
            printf("Connection closed\n");
        } else {
            printf("recv failed with error: %d\n", WSAGetLastError());
        }

    } while( iResult > 0 );

    // No longer need the socket
    closesocket(ConnectSocket);
    // Clean up
    WSACleanup();

    return 0;
}
