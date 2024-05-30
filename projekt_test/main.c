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
uint16_t gameTime;
uint16_t foodsNo[48];
float prevX=0;
float prevY=0;
float xPlayer=0;
float yPlayer=0;
float minDist=0;
float tmp=0;
float minX=0;
float minY=0;
float angle=0;


typedef struct {
    uint8_t playerNo;
    uint16_t hp;
    float x;
    float y;
}Player;

struct Food{
    uint16_t foodNo;
    uint8_t state;
    float x;
    float y;
    struct Food* next;
} ;

void createFood(struct Food* food, uint16_t No, uint8_t state ,float x, float y){
    food->foodNo=No;
    food->state = state;
    food->x=x;
    food->y=y;

    //food->next = NULL;
}

static struct Food* head=NULL;
void addFood(struct Food* food) {
    if (head == NULL) {
        head = food;
    } else {
        struct Food *current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = food;
    }
    food->next = NULL;
}


void actualizeFood(struct Food* food, uint16_t No, uint8_t state, float x, float y){
    food->foodNo=No;
    food->state=state;
    food->x=x;
    food->y=y;
}

uint16_t getNoMinDistance(float xPlayer, float yPlayer) {
    if (head == NULL) return 0;
    struct Food* current = head;
    float min = 1000;
    uint16_t noMinFood = 0;
    while (current) {
        if (current->state == 1) {
            float dist = sqrtf(powf(current->x - xPlayer, 2) + powf(current->y - yPlayer, 2));
            if (dist < min) {
                min = dist;
                noMinFood = current->foodNo;
            }
        }
        current = current->next;
    }
    return noMinFood;
}

float getAngle(uint16_t No, float xPlayer, float yPlayer){
    if(head==0)return 0;
    struct Food* current = head;
    float angle=0;
    while(current!=NULL){
        if(current->foodNo==No){
            angle = atan2(current->y-yPlayer, current->x-xPlayer);
            break;
        }else{
            angle =0;
        }
        current = current->next;
    }
    return angle;
}

void deleteFood(uint16_t No){
    if (head == NULL) return;
    struct Food* current = head;
    struct Food* prev=NULL;

    if(head->foodNo==No){
        head = head->next;
        free(current);
        return;
    }

    while(current){
        if(current->foodNo == No){
            if(prev==NULL) prev->next = current->next;
            free(current);
            return;
        }
        prev=current;
        current = current->next;

    }
}

Player players[8];
struct Food food[48];

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
            AMCOM_NewGameRequestPayload* newGameRequest = (AMCOM_NewGameRequestPayload*) packet->payload;
            playerNumber = newGameRequest->playerNumber;
            printf("%d", playerNumber);
            numberOfPlayers = newGameRequest->numberOfPlayers;
            printf("%d", numberOfPlayers);
            sprintf(newGameResponse.helloMessage, "Elooo");


            toSend = AMCOM_Serialize(AMCOM_NEW_GAME_RESPONSE, &newGameResponse, sizeof(newGameResponse), buf);


            for(uint8_t i=0; i<6; i++ ){
                createFood(&food[i],0,0,0,0);
                addFood(&food[i]);
            }

            break;

        case AMCOM_PLAYER_UPDATE_REQUEST:
            printf("Got PLAYER_UPDATE.request. Responding with PLAYER_UPDATE.response\n");
            AMCOM_PlayerUpdateRequestPayload* playerUpdate = (AMCOM_PlayerUpdateRequestPayload*) packet->payload;


            for(uint8_t i=0; i<1; i++){
                players[i].playerNo = playerUpdate->playerState[i].playerNo;
                players[i].hp = playerUpdate->playerState[i].hp;
                players[i].x = playerUpdate->playerState[i].x;
                players[i].y = playerUpdate->playerState[i].y;
            }

            break;
        case AMCOM_FOOD_UPDATE_REQUEST:

            printf("Got FOOD_UPDATE.request. Responding with FOOD_UPDATE.response\n");
            AMCOM_FoodUpdateRequestPayload *foodUpdate = (AMCOM_FoodUpdateRequestPayload*) packet->payload;

            for(uint8_t i=0; i<6; i++){
                foodsNo[i] = food[i].foodNo;
            }

            for(uint8_t i=0; i<6; i++){
                    createFood(&food[i], foodUpdate->foodState[i].foodNo, foodUpdate->foodState[i].state,
                               foodUpdate->foodState[i].x, foodUpdate->foodState[i].y);
                    if(food[i].state==0) deleteFood(foodsNo[i]);
                    else {
                        addFood(&food[i]); // Re-add the food item to the list if it's still available
                    }
                    //addFood(&food[i]);
//                food[i].foodNo = foodUpdate->foodState[i].foodNo;
//                food[i].state = foodUpdate->foodState[i].state;
//                food[i].x = foodUpdate->foodState[i].x;
//                food[i].y = foodUpdate->foodState[i].y;

            }



            break;

        case AMCOM_MOVE_REQUEST:
            printf("Got MOVE.request. Responding with MOVE.response\n");
            AMCOM_MoveResponsePayload moveResponse;

            uint16_t No = getNoMinDistance(players[playerNumber].x, players[playerNumber].y);

            moveResponse.angle = getAngle(No, players[playerNumber].x, players[playerNumber].y);
//            xPlayer=players[playerNumber].x;
//            yPlayer=players[playerNumber].y;
//            for(size_t i=0; i<6; i++){
//                if(food[i].foodNo!=0) {
//                    minDist = 1000;
//                    tmp = 0;
//                    minX = 0;
//                    minY = 0;
//                    tmp = sqrtf(powf(food[i].x - xPlayer, 2) + powf(food[i].y - yPlayer, 2));
//                    if (tmp < minDist) {
//                        minDist = tmp;
//                        minX = food[i].x;
//                        minY = food[i].y;
//                        angle = atan2(minY - yPlayer, minX - xPlayer);
//                    }
//                }
//            }
//            moveResponse.angle = angle;


//            char angleBuf[10];
//            sprintf(angleBuf, "%.1f" , moveResponse.angle);
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
