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
#include <pthread.h>

struct message{
    int type;
    char* info;
};

int game_over = 0;
/*
Proccesses messages
Processes the message by setting the type and the information

Input: received string and a pointer to an empty message structure
Output: pointer to message structure
*/
void * process_message(char* message, struct message *m){
    // fprintf(stderr, "Processing message...\n");

    char t = message[0];
    if (t == 0xFF) {
        m->type = 255;
    } else if (t == 0x01){
        m->type = 1;
    } else if (t == 0x02){
        m->type = 2;
    } else if (t == 0x03){
        m->type = 3;
    } else if (t == 0x04){
        m->type =  4;
    } else if (t == 0x05){
        m->type = 5;
    } else if (t == 0x06){
        m->type = 6;
    } else {
        fprintf(stderr, "ERROR: Message type not recognized: %c\n", t);
        return NULL;
    }
    m->info = &message[1];
    // fprintf(stderr, "... message processed\n");

    return m;
}

/*
Process TXT type messages
Prints received message

input: pointer to message structure
output: success or not
*/
int txt(struct message *m){
    fprintf(stderr, "%s\n", m->info);
    return 0;
}

/*
Process FYI type messages
Prints board

input: pointer to message structure
output: success or not
*/
int fyi(struct message *m){
    int n = (int) m->info[0];
    char *buf = m->info;
    int board[3][3] = {0};
    //int *board_info = &m->info[1];
    // Iterate over each filled position
    //fprintf(stderr, "n: %d\n",n);
    int player, col, row;
    for (int i = 0; i < n; i++) {
        //fprintf(stderr, "play: %d\n",i);
        player = *(buf + 1 +(i*3));
        //fprintf(stderr, "player: %d\n",player);
        col = *(buf + 2 +(i*3));
        //fprintf(stderr, "col: %d\n",col);
        row = *(buf + 3 +(i*3));
        //fprintf(stderr, "row: %d\n",row);
        if (col >= 0 && col < 3 && row >= 0 && row < 3) {
            board[row][col] = player;
            //fprintf(stderr, "player: %d, col: %d, row: %d\n", player, col, row);
        }
    }
    fprintf(stderr, "\n");
    // Print the board
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            if (board[r][c] == 0) {
                fprintf(stdout, " ");
            } else if (board[r][c] == 1) {
                fprintf(stdout, "X");
            } else if (board[r][c] == 2) {
                fprintf(stdout, "O");
            }
            if (c < 2) {
                fprintf(stdout, "|");
            }
        }
        fprintf(stdout, "\n");
        if (r < 2) {
            fprintf(stdout, "-+-+-\n");
        }
    }

    return 0;
}

/*
Process MYM type messages
Asks client for a move, checks is valid, sends MOV message to server

input: pointer to message structure, sockfd, server address
output: success or not
*/
int mym(struct message *m, int sockfd, struct sockaddr *dest){
    size_t size = 100;
    unsigned char *line = malloc(size);

    if (!line) {
        fprintf(stderr, "ERROR: Failed to allocate memory for line: %s\n", strerror(errno));
        return 1;
    }

    while (1) {
        fprintf(stdout, "Enter your move (column and row separated by space): ");
        int msg_len = getline((char **)&line, &size, stdin);
        if (msg_len == -1) {
            fprintf(stderr, "ERROR: Failed to read line: %s\n", strerror(errno));
            free(line);
            return 1;
        }

        // Trim newline character if present
        if (line[msg_len - 1] == '\n') {
            line[msg_len - 1] = '\0';
        }

        int row, col;
        if (sscanf((char *)line, "%d %d", &col, &row) != 2 || row < 0 || row > 2 || col < 0 || col > 2) {
            fprintf(stderr, "ERROR: Invalid input. Please enter valid row and column numbers (0, 1, or 2).\n");
            continue; // prompt for input again
        }

        unsigned char final_msg[3];
        final_msg[0] = 0x05; // MOV message type
        final_msg[1] = col;
        final_msg[2] = row;

        int s = sendto(sockfd, final_msg, sizeof(final_msg), 0, dest, sizeof(*dest));
        if (s == -1) {
            fprintf(stderr, "ERROR: Failed to send message: %s\n", strerror(errno));
            close(sockfd);
            free(line);
            return 1;
        }

        break; // exit loop after successful input and send
    }

    free(line);
    return 0;
}

/*
Process END type messages
Prints winner of the game

input: pointer to message structure
output: success or not
*/
int end(struct message *m){
    if (m->info[0] == 0){
        fprintf(stderr, "Game has ended: Draw");
    } else if ((m->info[0] == 1) || (m->info[0] == 2)) {
        fprintf(stderr, "Game has ended: the winner is player %u\n", m->info[0]);
    } else {
         fprintf(stderr, "ERROR: unexpected winner\n");
         return 1;
    }
    game_over = 1;
    return 0;
}

/*
Gets message
Waits to get a message, processes it and calls the necessary function

input: sockfd, line to receive message, length of line, pointer to server address
output: success or not
*/
int get_message(int sockfd, char* line, int len, struct sockaddr *dest){
    socklen_t addr_len = sizeof(dest);
    // fprintf(stderr, "Waiting for message...\n");
    int msg_len = recvfrom(sockfd, line, len, 0, (struct sockaddr *) dest, &addr_len);
    //fprintf(stderr, "Received: %s\n", line);
    if (msg_len > 0) {
        line[msg_len] = '\0'; // Null-terminate the received string
        struct message m;
        process_message(line, &m);

        //fprintf(stderr, "[R] [%d] (%d bytes)\n", m.type, msg_len);
        if (m.type == 255) {
            fprintf(stderr, "Game is full\n");
            close(sockfd);
            free(line);
            return 0;
        } else if (m.type == 1){
            // fprintf(stderr, "FYI message\n");
            if(!fyi(&m)){
                return 1;
            }
        } else if (m.type == 2){

            if(!mym(&m, sockfd, dest)){
                return 1;
            }
        } else if (m.type == 3){
            // fprintf(stderr, "END message\n");
            if (!end(&m)){
                return 1;
            }
            return 0;
        } else if (m.type == 4){
            // fprintf(stderr, "TXT message\n");

            if (!txt(&m)){
                return 1;
            }
        } else {
            fprintf(stderr, "ERROR: Unexpected message type\n");
            close(sockfd);
            free(line);
            return 5;
        }
    } else {
        fprintf(stderr, "ERROR: Failed to receive message: %s\n", strerror(errno));
        return 9;
    }
    return 0;
}


int main(int argc, char* argv[]) {
    // fprintf(stderr, "Successfully running\n");
    if (argc < 3){
        fprintf(stderr, "ERROR: Missing argument. Please enter Port Number: %s\n", strerror(errno));
        return 1;
    }

	char *ip_addr = argv[1];

    int port;
    if (sscanf(argv[2], "%d", &port) != 1) {
        fprintf(stderr, "ERROR: Invalid Port: %s\n", strerror(errno));
        return 3;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM,0);
    if (sockfd == -1){
        fprintf(stderr, "ERROR: Socket creation failed: %s\n", strerror(errno));
        return 4;
    // } else {
    //     fprintf(stderr, "Socket created.\n");
    }

	struct sockaddr_in dest;
    struct in_addr *dst_addr = malloc(sizeof(struct in_addr));

    if (! inet_pton(AF_INET, ip_addr, dst_addr)){
        fprintf(stderr, "ERROR: Could not turn IPaddress string to network address: %s\n", strerror(errno));
        close(sockfd);
        free(dst_addr);
        return 2;
    }
	dest.sin_addr = *dst_addr;
    dest.sin_port = htons(port);
    dest.sin_family = AF_INET;

    // Send Hello message
    char msg[] = {0x04, 'H', 'e', 'l', 'l', 'o', '\0'};
    int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));
    if (s == -1){
        fprintf(stderr, "ERROR: Failed to send message: %s\n", strerror(errno));
        close(sockfd);
        free(dst_addr);
        return 4;
    }
    // fprintf(stderr, "Hello sent\n");

    // Receive messages (Start playing)
    char *line = malloc(1000);
    while(1){
        // fprintf(stderr, "Loop entered\n");
        if (!get_message(sockfd, line, 1000, (struct sockaddr *)&dest)){
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            free(dst_addr);
            free(line);
            close(sockfd);
            return 1;
        }
        if(game_over){
            break;
        }
    }
    free(line);
    close(sockfd);
    free(dst_addr);
    return 0;
}
