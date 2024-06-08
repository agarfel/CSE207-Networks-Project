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
int winner = 0;

struct message{
    int type;
    char* info;
    struct sockaddr_in client_addr;
};

void * process_message(char* message, struct message *m){
    // fprintf(stderr, "Processing message...\n");

    char t = message[0];
    if (t == 0x04){
        m->type =  4;
    } else if (t == 0x05){
        m->type = 5;
    } else {
        fprintf(stderr, "Message type not recognized: %c\n", t);
        return 0;
    }
    m->info = &message[1];
    // fprintf(stderr, "... message processed\n");

    return m;
}

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
        winner = 255;  // Indicate draw
    }
}

int txt(struct message *m){
    fprintf(stderr, "[R] [TXT] %s\n", m->info);

    // Check if the received message is "Hello"
    unsigned char hello[] = {'H', 'e', 'l', 'l', 'o', '\0'};
    if (strcmp(m->info, hello) == 0) {
        //fprintf(stderr, "Received hello\n");
        if (nclients == 0){
            player1 = m->client_addr;
            unsigned char *msg = malloc(500);
            if (!msg) {
                fprintf(stderr, "Memory allocation failed\n");
                close(sockfd);
                return 4;
            }
            msg[0] = 4;
            strcpy((char*)&msg[1], "Welcome! You are player 1 in game 0, you play with X.");
            //fprintf(stderr, "sending: %s\n", msg);
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&player1, sizeof(player1));
            if (s == -1){
                fprintf(stderr, "Failed to send message\n");
                close(sockfd);
                return 4;
                }
            nclients = 1;
            //fprintf(stderr, "Welcomed player 1\n");

        } else if (nclients == 1){
            player2 = m->client_addr;
            unsigned char *msg = malloc(500);
            if (!msg) {
                fprintf(stderr, "Memory allocation failed\n");
                close(sockfd);
                return 4;
            }
            msg[0] = 4;
            strcpy((char*)&msg[1], "Welcome! You are player 2 in game 0, you play with O.");
            //fprintf(stderr, "sending: %s\n", msg);
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&player2, sizeof(player2));
            if (s == -1){
                fprintf(stderr, "Failed to send message\n");
                close(sockfd);
                return 4;
            }
            nclients = 2;
            fprintf(stderr, "Welcomed player 2\n");
        } else {
            unsigned char msg[] = {0x03, 0xFF};
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&( m->client_addr), sizeof(m->client_addr));
            if (s == -1){
                fprintf(stderr, "Failed to send message\n");
                close(sockfd);
                return 4;
            }
            fprintf(stderr, "Blocked player, game full\n");
        }
    }

    return 0;
}

int mov(struct message *m, int player){
    moves[0] = (char) (((int) moves[0]) +1);
    *(last+1) =(char) player;
    *(last+2) = (char) m->info[0];
    *(last+3) = (char) m->info[1];
    if (board[m->info[1]][ m->info[0]] != 0){
        char* msg = msg[0] = 4;
        strcpy((char*)&msg[1], "Invalid move.");
        if (player == 1){
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&(player1), sizeof(player1));
            if (s == -1){
                fprintf(stderr, "Failed to send message\n");
                close(sockfd);
                return 4;
            }
        } else {
            int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&(player2), sizeof(player2));
            if (s == -1){
                fprintf(stderr, "Failed to send message\n");
                close(sockfd);
                return 4;
            }
        send_mym(player);
        }
        return 0;
    }
    board[m->info[1]][ m->info[0]] = player;

    last = last+3;
    check();
    return 0;
}

int send_fyi(int player){
    char *msg = malloc(500);
    msg[0] = 1;
    int l = ((int)moves[0])*3 +1;
    memcpy((char*)&msg[1], moves, l);
    fprintf(stderr, "%s\n",msg);
    int len = (moves[0]*3) + 2;
    if (player == 1){
        int s = sendto(sockfd, msg, len, 0, (struct sockaddr *)&(player1), sizeof(player1));
        if (s == -1){
            fprintf(stderr, "Failed to send message\n");
            close(sockfd);
            return 4;
        }
    } else {
        int s = sendto(sockfd, msg, len, 0, (struct sockaddr *)&(player2), sizeof(player2));
        if (s == -1){
            fprintf(stderr, "Failed to send message\n");
            close(sockfd);
            return 4;
        }
    }
    free(msg);
}

int send_mym(int player){
    // Send FYI
    send_fyi(player);

    // Send MYM

    unsigned char *msg = malloc(1);
    msg[0] = 2;
    if (player == 1){
        int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&(player1), sizeof(player1));
        if (s == -1){
            fprintf(stderr, "Failed to send message\n");
            close(sockfd);
            return 4;
        }
    } else {
        int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&(player2), sizeof(player2));
        if (s == -1){
            fprintf(stderr, "Failed to send message\n");
            close(sockfd);
            return 4;
        }
    }

    // Receive MOV
    char *line = malloc(10);
    int len = 10;
    int msg_len;
    if (player == 1){
        msg_len = recvfrom(sockfd, line, len, (unsigned int) 0, (struct sockaddr *) &player1, sizeof(player1));
    } else {
        msg_len = recvfrom(sockfd, line, len, (unsigned int) 0, (struct sockaddr *) &player2, sizeof(player2));
    }
    if (msg_len != 0){
        line[msg_len] = '\0'; // Null-terminate the received string
        struct message m;
        process_message(line, &m);
        if (m.type == 5){
            mov(&m, player);
        } else {
            fprintf(stderr, "Received wrong message type\n");
            return 5;
        }
    }
}

void send_end(){
    unsigned char msg[] = {0x03, winner};
    int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&player1, sizeof(player1));
    if (s == -1){
        fprintf(stderr, "Failed to send message\n");
        close(sockfd);
        return 4;
    }
    s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&player2, sizeof(player2));
    if (s == -1){
        fprintf(stderr, "Failed to send message\n");
        close(sockfd);
        return 4;
    }
}

int main(int argc, char* argv[]) {
//        fprintf(stderr, "Successfully running\n");

        // Check Program was called correctly
        if (argc < 2){
                fprintf(stderr, "Missing argument. Please enter Port Number.\n");
                return 1;
        }

        int port;
        if (sscanf(argv[1], "%d", &port) != 1) {
                fprintf(stderr, "Invalid Port.\n");
                return 1;
                }

        // Build and Bind Socket
        sockfd = socket(AF_INET, SOCK_DGRAM,0);
        if (sockfd == -1){
                fprintf(stderr, "Socket creation failed");
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
            fprintf(stderr, "Could not bind sockets.\n");
            close(sockfd);
            return 3;
    }
	// fprintf(stderr, "Successful bind.\n");

    while(nclients<2){
        char *line = malloc(100);
        int len = 100;
        struct sockaddr_in addr_client;
        socklen_t clilen = sizeof(addr_client);
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
    fprintf(stderr, "Starting Game\n");
    moves = malloc(1000);
    last = moves;
    while(1){
        send_mym(1);
        if (winner != 0){
            break;
        }
        send_mym(2);
        if (winner != 0){
            break;
        }
    }

    send_end();
    return 0;
}