#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

int nclients = 0;
int sockfd;
struct sockaddr_in player1;
struct sockaddr_in player2;
char *moves;
char *last;
int board[3][3] = {0};
int winner = 3;
socklen_t clilen;

int send_mym(int);

struct message{
    int type;
    char* info;
    struct sockaddr_in client_addr;
};

/*
Proccesses messages
Processes the message by setting the type and the information

Input: received string and a pointer to an empty message structure
Output: pointer to message structure
*/
void * process_message(char* message, struct message *m){
    // fprintf(stderr, "Processing message...\n");

    char t = message[0];
    if (t == 0x04){
        m->type =  4;
    } else if (t == 0x05){
        m->type = 5;
    } else {
        fprintf(stderr, "ERROR: Message type not recognized: %c\n", t);
        return NULL;
    }
    m->info = &message[1];
    // fprintf(stderr, "... message processed\n");
    return m;
}

/*
Checks if game is over
Updates winner as necessary

Input: 
Output:
*/
void check() {
    // Check rows and columns for a winner
    for (int i = 0; i < 3; i++) {
        // Check rows
        if ((board[i][0] == board[i][1]) && (board[i][1] == board[i][2]) && (board[i][0] != 0)) {
            winner = board[i][0];
            return;
        }
        // Check columns
        if ((board[0][i] == board[1][i]) && (board[1][i] == board[2][i]) && (board[0][i] != 0)) {
            winner = board[0][i];
            return;
        }
    }

    // Check diagonals for a winner
    if ((board[0][0] == board[1][1]) && (board[1][1] == board[2][2]) && (board[0][0] != 0)) {
        winner = board[0][0];
        return;
    }
    if ((board[0][2] == board[1][1]) && (board[1][1] == board[2][0]) && (board[0][2] != 0)) {
        winner = board[0][2];
        return;
    }

    // Check for draw
    int draw = 1;  // Assume draw initially
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == 0) {
                draw = 0;  // Found an empty cell, not a draw
                break;
            }
        }
        if (!draw) {
            break;
        }
    }

    if (draw) {
        winner = 0;  // Indicate draw
    }
}

/*
Process TXT type messages
Sends Welcome message if game not full,
Sends END message if game is full

input: pointer to message structure
output: success or not
*/
int txt(struct message *m){
    fprintf(stderr, "[R] [TXT] %s\n", m->info);

    // Check if the received message is "Hello"
    char hello[] = {'H', 'e', 'l', 'l', 'o', '\0'};
    if (strcmp(m->info, hello) == 0) {
        //fprintf(stderr, "Received hello\n");
        if (nclients == 0){
            player1 = m->client_addr;
            char *msg = malloc(500);
            if (!msg) {
                fprintf(stderr, "ERROR: Memory allocation failed\n");
                close(sockfd);
                return 4;
            }
            msg[0] = 4;
            strcpy((char*)&msg[1], "Welcome! You are player 1 in game 0, you play with X.");
            //fprintf(stderr, "sending: %s\n", msg);
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&player1, clilen);
            if (s == -1){
                fprintf(stderr, "ERROR: Failed to send message\n");
                close(sockfd);
                return 4;
                }
            fprintf(stderr, "[S] [TXT] [1] %s\n", msg+1);
            nclients = 1;
            //fprintf(stderr, "Welcomed player 1\n");

        } else if (nclients == 1){
            player2 = m->client_addr;
            char *msg = malloc(500);
            if (!msg) {
                fprintf(stderr, "ERROR: Memory allocation failed\n");
                close(sockfd);
                return 4;
            }
            msg[0] = 4;
            strcpy((char*)&msg[1], "Welcome! You are player 2 in game 0, you play with O.");
            //fprintf(stderr, "sending: %s\n", msg);
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&player2, clilen);
            if (s == -1){
                fprintf(stderr, "ERROR: Failed to send message\n");
                close(sockfd);
                return 4;
            }
            fprintf(stderr, "[S] [TXT] [2] %s\n", msg+1);
            nclients = 2;
            //fprintf(stderr, "Welcomed player 2\n");
        } else {
            char msg[] = {0x03, 0xFF};
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&( m->client_addr), sizeof(m->client_addr));
            if (s == -1){
                fprintf(stderr, "ERROR: Failed to send message\n");
                close(sockfd);
                return 4;
            }
            fprintf(stderr, "[S] [TXT] [] %s\n", msg+1);
            //fprintf(stderr, "Blocked player, game full\n");
        }
    } else {
        fprintf(stderr, "ERROR: receveived unexpected TXT message\n");
        return 1;
    }

    return 0;
}


/*
Process MOV type messages
Checks if move is valid. If it isn't resends MYM message
Updates moves and board

input: pointer to message structure, player
output: success or not
*/
int mov(struct message *m, int player){

    if (board[(int) m->info[1]][(int) m->info[0]] != 0){
        char* msg = malloc(20);
        msg[0] = 4;
        strcpy((char*)&msg[1], "Invalid move.");
        if (player == 1){
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&(player1), clilen);
            if (s == -1){
                fprintf(stderr, "ERROR: Failed to send message\n");
                close(sockfd);
                return 4;
            }
        } else {
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&(player2), clilen);
            if (s == -1){
                fprintf(stderr, "ERROR: Failed to send message\n");
                close(sockfd);
                return 4;
            }
        }
        if (send_mym(player)){
            return 1;
        }

        return 0;
    }
    moves[0] = (char) (((int) moves[0]) +1);
    *(last+1) =(char) player;
    *(last+2) = (char) m->info[0];
    *(last+3) = (char) m->info[1];
    board[(int) m->info[1]][(int) m->info[0]] = player;

    last = last+3;
    check();
    return 0;
}

/*
Send FYI type messages
Sends FYI message

input: player
output: success or not
*/
int send_fyi(int player){
    char *msg = malloc(500);
    msg[0] = 1;
    int l = ((int)moves[0])*3 +1;
    memcpy((char*)&msg[1], moves, l);
    //fprintf(stderr, "%s\n",msg);
    int len = (moves[0]*3) + 2;
    if (player == 1){
        int s = sendto(sockfd, msg, len, 0, (struct sockaddr *)&(player1), clilen);
        if (s == -1){
            fprintf(stderr, "ERROR: Failed to send message\n");
            close(sockfd);
            return 4;
        }
        fprintf(stderr, "[S] [FYI] [%d]\n",player);
    } else {
        int s = sendto(sockfd, msg, len, 0, (struct sockaddr *)&(player2), clilen);
        if (s == -1){
            fprintf(stderr, "ERROR: Failed to send message\n");
            close(sockfd);
            return 4;
        }
        fprintf(stderr, "[S] [FYI] [%d]\n", player);
    }
    free(msg);
    return 0;
}

/*
Send MYM type messages
Sends MYM message and waits to receive MOV message, checks received message is MOV
Calls MOV on the processed received message

input: player
output: success or not
*/
int send_mym(int player){
    // Send MYM

    char *msg = malloc(1);
    msg[0] = 2;
    if (player == 1){
        int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&(player1), clilen);
        if (s == -1){
            fprintf(stderr, "ERROR: Failed to send message\n");
            close(sockfd);
            return 4;
        }
        fprintf(stderr, "[S] [MYM] [%d]\n", player);
    } else {
        int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&(player2), clilen);
        if (s == -1){
            fprintf(stderr, "ERROR: Failed to send message\n");
            close(sockfd);
            return 4;
        }
        fprintf(stderr, "[S] [MYM] [%d]\n", player);
    }

    // Receive MOV
    char *line = malloc(10);
    int len = 10;
    int msg_len;
    if (player == 1){
        msg_len = recvfrom(sockfd, line, len, (unsigned int) 0, (struct sockaddr *) &player1, &clilen);
    } else {
        msg_len = recvfrom(sockfd, line, len, (unsigned int) 0, (struct sockaddr *) &player2, &clilen);
    }
    if (msg_len != 0){
        line[msg_len] = '\0'; // Null-terminate the received string
        struct message m;
        process_message(line, &m);
        if (m.type == 5){
            fprintf(stderr, "[R] [MOV] [%d] %s\n", player, m.info);
            mov(&m, player);
        } else {
            fprintf(stderr, "ERROR: Received wrong message type\n");
            return 5;
        }
    }
    return 0;
}

/*
Send END type messages
Sends END message to each player

input:
output: success or not
*/
int send_end(){
    char msg[] = {0x03, winner};
    int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&player1, clilen);
    if (s == -1){
        fprintf(stderr, "ERROR: Failed to send message\n");
        close(sockfd);
        return 1;
    }
    fprintf(stderr, "[S] [END] [1] %d\n", winner);
    s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&player2, clilen);
    if (s == -1){
        fprintf(stderr, "ERROR: Failed to send message\n");
        close(sockfd);
        return 1;
    }
    fprintf(stderr, "[S] [END] [2] %d\n", winner);
    return 0;
}

int main(int argc, char* argv[]) {
//        fprintf(stderr, "Successfully running\n");

        // Check Program was called correctly
        if (argc < 2){
                fprintf(stderr, "ERROR: Missing argument. Please enter Port Number.\n");
                return 1;
        }

        int port;
        if (sscanf(argv[1], "%d", &port) != 1) {
                fprintf(stderr, "ERROR: Invalid Port.\n");
                return 1;
                }

        // Build and Bind Socket
        sockfd = socket(AF_INET, SOCK_DGRAM,0);
        if (sockfd == -1){
                fprintf(stderr, "ERROR: Socket creation failed");
        	return 2;
	// } else {
        //        fprintf(stderr, "Socket created.\n");
        }

	struct sockaddr_in addr_server;
	memset(&addr_server, 0, sizeof(addr_server)); // Initialize to zero

	addr_server.sin_port = htons(port);
   	addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = INADDR_ANY;
    if (-1 == bind(sockfd, (struct sockaddr*)&addr_server, sizeof(struct sockaddr_in))){
            fprintf(stderr, "ERROR: Could not bind sockets.\n");
            close(sockfd);
            return 3;
    }
	// fprintf(stderr, "Successful bind.\n");

    // Waits to get 2 players
    while(nclients<2){
        char *line = malloc(100);
        int len = 100;
        struct sockaddr_in addr_client;
        clilen = sizeof(addr_client);
        int msg_len = recvfrom(sockfd, line, len, (unsigned int) 0, (struct sockaddr *) &addr_client, &clilen);
		if (msg_len != 0){
            line[msg_len] = '\0'; // Null-terminate the received string
            struct message m;
            m.client_addr = addr_client;
            process_message(line, &m);
            if (m.type == 4){
                txt(&m);
            }
        }
        free(line);
    }
    //Starts Game
    fprintf(stderr, "STARTING GAME\n");
    moves = malloc(1000);
    last = moves;
    while(1){
        if(send_fyi(1)){
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            return 1;
        }

        if(send_mym(1)){
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            return 1;
        }
        if (winner != 3){
            break;
        }
        if (send_fyi(2)){
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            return 1;
        }

        if(send_mym(2)){
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            return 1;
        }
        if (winner != 3){
            break;
        }
    }
    fprintf(stderr, "GAME OVER\n");

    if (send_end()){
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
        return 1;
    }
    return 0;
}