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
uint8_t numberOfFoods;
uint32_t gameTime=0;
uint16_t foodsNo[48];
uint16_t minNo=0;
uint8_t NumberOfFoodsDependsOfPlayers[8] = {6, 12, 18, 24, 30, 36, 42, 48};
uint8_t it=0;
uint8_t numOfPackets=1;
uint8_t cnt=0;
uint8_t limit=0;
uint8_t size;
uint32_t foodUp;
uint16_t temp;


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

struct Food food[200];
struct Food food2[16];
struct Food food3[16];
struct Player player[8];


void actualizeFood(struct Food* food, uint8_t No, uint8_t state, float x, float y){

    food->foodNo = No;
    food->state = state;
    food->x=x;
    food->y=y;
}

static struct Food* headFood=NULL;
void addFood(struct Food* food,uint8_t No, uint8_t state, float x, float y){

    food->foodNo = No;
    food->state = state;
    food->x=x;
    food->y=y;

    if(headFood==NULL){
        headFood = food;
        headFood->next = NULL;
    }

    struct Food* current = headFood;
    while(current->next != NULL){
        current = current->next;
    }

    current->next = food;
    food->next = NULL;
}

bool returnNo( uint16_t No){

    struct Food* current = headFood;
    while(current!=NULL){
        if(current->foodNo==No) return true;
        current = current->next;
    }
    return false;
}

float getAngleToClosestFood(float PlayerX, float PlayerY){

    float minResult = 1001;
    float tmp=0;
    float angle=0;
    float foodX=0, foodY=0;
    uint16_t No;

    if(headFood==NULL) return 0;

    struct Food* current = headFood;

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
    if (headFood == NULL) return; // Lista jest pusta, nic do usunięcia

    struct Food* current = headFood;
    struct Food* prev = NULL;

    // Specjalny przypadek dla głowy listy
    if (headFood->foodNo == No) {
        if (headFood != NULL) {
            headFood = headFood->next;

            return;
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


uint16_t UpdateFoods(AMCOM_FoodUpdateRequestPayload* foodUpdate, uint32_t time, uint8_t iterator, uint8_t j) {

            for (uint8_t i = 0; i < j; i++) {
                if (returnNo(foodUpdate->foodState[i].foodNo) == false) {
                    addFood(&food[iterator], foodUpdate->foodState[i].foodNo, foodUpdate->foodState[i].state,
                                  foodUpdate->foodState[i].x, foodUpdate->foodState[i].y);

//                    addFood(&food[i]);
                    printf("Dodalem jedzznie o nr %d\n", foodUpdate->foodState[i].foodNo);
                    iterator++;
                }
//                printf("ITERATORY: %d, %d\n", i, iterator);
//
                else{
                    if(foodUpdate->foodState[i].state==0){
                        deleteFood(foodUpdate->foodState[i].foodNo);
                    }
                }
            }
    return iterator;
}


void amPacketHandler(const AMCOM_Packet* packet, void* userContext) {
    uint8_t buf[AMCOM_MAX_PACKET_SIZE];
    size_t toSend = 0;

    switch (packet->header.type) {
        case AMCOM_IDENTIFY_REQUEST:
            printf("Got IDENTIFY.request. Responding with IDENTIFY.response\n");
            AMCOM_IdentifyResponsePayload identifyResponse;
            sprintf(identifyResponse.playerName, "Michal&Kuba");
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
            if(numberOfPlayers<3) limit=1;
            else if(numberOfPlayers>2 && numberOfPlayers<6) limit =2;
            else limit=3;
            numberOfFoods = numberOfPlayers*6;
            printf("%d", numberOfPlayers);
            sprintf(newGameResponse.helloMessage, "Elooo");


            toSend = AMCOM_Serialize(AMCOM_NEW_GAME_RESPONSE, &newGameResponse, sizeof(newGameResponse), buf);


            break;


        case AMCOM_FOOD_UPDATE_REQUEST:

            printf("Got FOOD_UPDATE.request. Responding with FOOD_UPDATE.response\n");
            AMCOM_FoodUpdateRequestPayload *foodUpdates = (AMCOM_FoodUpdateRequestPayload *) packet->payload;
            printf("LEN: %d\n",packet->header.length);

            size = packet->header.length/11;
            printf("SIZE: %d\n", size);

            temp = UpdateFoods(foodUpdates, foodUp, it, size);
            it = temp;
            printf("ITERATOR: %d\n", it);



            break;

        case AMCOM_PLAYER_UPDATE_REQUEST:
            printf("Got PLAYER_UPDATE.request. Responding with PLAYER_UPDATE.response\n");
            AMCOM_PlayerUpdateRequestPayload *playerUpdate = (AMCOM_PlayerUpdateRequestPayload *) packet->payload;


            for (uint8_t i = 0; i <numberOfPlayers; i++) {
                player[playerUpdate->playerState[i].playerNo].playerNo = playerUpdate->playerState[i].playerNo;
                player[playerUpdate->playerState[i].playerNo].hp = playerUpdate->playerState[i].hp;
                player[playerUpdate->playerState[i].playerNo].x = playerUpdate->playerState[i].x;
                player[playerUpdate->playerState[i].playerNo].y = playerUpdate->playerState[i].y;
                printf("[%d, %d, %f, %f]", player[i].playerNo, player[i].hp, player[i].x, player[i].y);

            }



            break;


        case AMCOM_MOVE_REQUEST:
            printf("Got MOVE.request. Responding with MOVE.response\n");
            AMCOM_MoveResponsePayload moveResponse;
             AMCOM_MoveRequestPayload *getTime = (AMCOM_MoveRequestPayload *)packet->payload;
            gameTime = getTime->gameTime;
            printf("GETTIME: %d", gameTime);

            float minResult = 1001;
            float tmp=0;
            float PlayerX=0, PlayerY=0;
            bool playerFound=false;

            if(gameTime<60) {
                moveResponse.angle = getAngleToClosestFood(player[playerNumber].x, player[playerNumber].y);
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