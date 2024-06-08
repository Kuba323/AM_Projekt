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
uint32_t gameTime;
uint16_t foodsNo[48];
uint16_t minNo=0;


//
struct Player{
    uint8_t playerNo;
    uint16_t hp;
    float x;
    float y;
    struct Player* next;
};

struct Food{
    uint16_t foodNo;
    uint8_t state;
    float x;
    float y;
    struct Food* next;
};
//
//struct Player players[AMCOM_MAX_PLAYER_UPDATES];
//struct Food food[AMCOM_MAX_FOOD_UPDATES];

//AMCOM_PlayerState players[AMCOM_MAX_PLAYER_UPDATES];
//AMCOM_FoodState food[AMCOM_MAX_FOOD_UPDATES];

struct Food food[AMCOM_MAX_FOOD_UPDATES];
struct Player players[AMCOM_MAX_PLAYER_UPDATES];

//float minResult=1000;
//float tmp=0;
//float minX=0;
//float minY=0;
//float tempX=0;
//float tempY=0;

void actualizeFood(struct Food* food, uint8_t No, uint8_t state, float x, float y){

    food->foodNo = No;
    food->state = state;
    food->x=x;
    food->y=y;
}

static struct Food* head=NULL;
void addFood(struct Food* food){

    if(head==NULL){
        head = food;
        head->next = NULL;
    }

    struct Food* current = head;
    while(current->next != NULL){
        current = current->next;
    }

    current->next = food;
    food->next = NULL;
}

float getAngleToClosestFood(float PlayerX, float PlayerY){

    float minResult = 1001;
    float tmp=0;
    float angle=0;
    float foodX=0, foodY=0;
    uint16_t No;

    if(head==NULL) return 0;

    struct Food* current = head;

    while(current){

        tmp = sqrtf(powf(current->x - PlayerX,2) + powf(current->y - PlayerY,2));
        if(tmp<minResult){
            minResult = tmp;
            foodX = current->x;
            foodY = current->y;
        }
        current = current->next;
    }

    angle = atan2(foodY-PlayerY, foodX-PlayerX);
    printf("ANGLE: %f\n", angle);
    return angle;
}

void deleteFood(uint16_t No) {
    if (head == NULL) return; // Lista jest pusta, nic do usunięcia

    struct Food* current = head;
    struct Food* prev = NULL;

    // Specjalny przypadek dla głowy listy
    if (head->foodNo == No) {
        if (head != NULL) {
            head = head->next;
        }
    }

    // Przechodzenie przez listę w poszukiwaniu elementu do usunięcia
    while (current != NULL) {
        if (current->foodNo == No) {
            prev->next = current->next;
            return;
        }
        prev = current;
        current = current->next;
    }
}

void amPacketHandler(const AMCOM_Packet* packet, void* userContext) {
    uint8_t buf[AMCOM_MAX_PACKET_SIZE];
    size_t toSend = 0;

    switch (packet->header.type) {
        case AMCOM_IDENTIFY_REQUEST:
            printf("Got IDENTIFY.request. Responding with IDENTIFY.response\n");
            AMCOM_IdentifyResponsePayload identifyResponse;
            sprintf(identifyResponse.playerName, "mniAM player");
            toSend = AMCOM_Serialize(AMCOM_IDENTIFY_RESPONSE, &identifyResponse, sizeof(identifyResponse), buf);
            break;


        case AMCOM_NEW_GAME_REQUEST:
            printf("Got NEW_GAME.request. Responding with NEW_GAME.response\n");
            AMCOM_NewGameResponsePayload newGameResponse;
            AMCOM_NewGameRequestPayload *newGameRequest = (AMCOM_NewGameRequestPayload *) packet->payload;
            printf("%d", packet->payload);
            playerNumber = newGameRequest->playerNumber;
            printf("%d", playerNumber);
            numberOfPlayers = newGameRequest->numberOfPlayers;
            printf("%d", numberOfPlayers);
            sprintf(newGameResponse.helloMessage, "Elooo");


            toSend = AMCOM_Serialize(AMCOM_NEW_GAME_RESPONSE, &newGameResponse, sizeof(newGameResponse), buf);


            break;


        case AMCOM_FOOD_UPDATE_REQUEST:

            printf("Got FOOD_UPDATE.request. Responding with FOOD_UPDATE.response\n");
            AMCOM_FoodUpdateRequestPayload *foodUpdate = (AMCOM_FoodUpdateRequestPayload *) packet->payload;

            if(gameTime==0){
                for(uint8_t i=0; i<6; i++){
                    actualizeFood(&food[i], foodUpdate->foodState[i].foodNo, foodUpdate->foodState[i].state,
                                  foodUpdate->foodState[i].x, foodUpdate->foodState[i].y);

                    addFood(&food[i]);
                }
            }
            else{
                for(uint8_t i=0; i<6; i++){
                    if(foodUpdate->foodState[i].state==0){
                        deleteFood(foodUpdate->foodState->foodNo);
                        printf("Delete\n");
                        continue;
                    }
                    actualizeFood(&food[i], foodUpdate->foodState[i].foodNo, foodUpdate->foodState[i].state,
                                  foodUpdate->foodState[i].x, foodUpdate->foodState[i].y);
                }
            }

            break;

        case AMCOM_PLAYER_UPDATE_REQUEST:
            printf("Got PLAYER_UPDATE.request. Responding with PLAYER_UPDATE.response\n");
            AMCOM_PlayerUpdateRequestPayload *playerUpdate = (AMCOM_PlayerUpdateRequestPayload *) packet->payload;


            for (uint8_t i = 0; i < 1; i++) {
                players[playerUpdate->playerState[i].playerNo].playerNo = playerUpdate->playerState[i].playerNo;
                players[playerUpdate->playerState[i].playerNo].hp = playerUpdate->playerState[i].hp;
                players[playerUpdate->playerState[i].playerNo].x = playerUpdate->playerState[i].x;
                players[playerUpdate->playerState[i].playerNo].y = playerUpdate->playerState[i].y;
                printf("[%d, %d, %f, %f]", players[i].playerNo, players[i].hp, players[i].x, players[i].y);

            }


            break;


        case AMCOM_MOVE_REQUEST:
            printf("Got MOVE.request. Responding with MOVE.response\n");
            AMCOM_MoveResponsePayload moveResponse;
             AMCOM_MoveRequestPayload *getTime = (AMCOM_MoveRequestPayload *)packet->payload;
            gameTime = getTime->gameTime;
            printf("GETTIME: %d", gameTime);

            moveResponse.angle= getAngleToClosestFood(players[playerNumber].x, players[playerNumber].y);


            toSend = AMCOM_Serialize(AMCOM_MOVE_RESPONSE, &moveResponse, sizeof(moveResponse), buf);
            break;

        case AMCOM_GAME_OVER_REQUEST:
            printf("Got GAME_OVER.request. Responding with GAME_OVER.response\n");
            AMCOM_GameOverResponsePayload gameOverResponse;
            sprintf(gameOverResponse.endMessage, "Nara");
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