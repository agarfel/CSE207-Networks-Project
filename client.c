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
        fprintf(stderr, "Message type not recognized: %c\n", t);
        return 0;
    }
    m->info = strdup(&message[1]);
    // fprintf(stderr, "... message processed\n");

    return m;
}


int txt(struct message *m){
    fprintf(stderr, "%s\n", m->info);
    return 0;
}

int fyi(struct message *m){
    int n = m->info[0];
    // fprintf(stderr, "N: %d\n", n);
    int board[3][3] = {0}; // 3x3 board initialized to 0 (no moves)
    fprintf(stderr, "Board: %s\n", m->info);
    // Iterate over each filled position
    for (int i = 0; i < n; i++) {
        int player = m->info[1 + (i * 3)];
        int col = m->info[2 + (i * 3)];
        int row = m->info[3 + (i * 3)];
        fprintf(stderr, "Move: player %d, col %d, row %d\n",player,col,row);
        // Store the player number in the board
        fprintf(stderr, "i : %d\n", i);
        if (col >= 0 && col < 3 && row >= 0 && row < 3) {
            board[row][col] = player;
        }
    }

    // Print the board
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            if (board[r][c] == 0) {
                fprintf(stderr, " ");
            } else {
                fprintf(stderr, "%d", board[r][c]);
            }
            if (c < 2) {
                fprintf(stderr, "|");
            }
        }
        fprintf(stderr, "\n");
        if (r < 2) {
            fprintf(stderr, "-+-+-\n");
        }
    }

    return 0;
}

int mym(struct message *m, int sockfd, struct sockaddr *dest){
    size_t size = 100;
    char *line = malloc(size);

    if (!line) {
        fprintf(stderr, "Failed to allocate memory for line\n");
        return 1;
    }

    int msg_len = getline(&line, &size, stdin);
    if (msg_len == -1) {
        fprintf(stderr, "Failed to read line\n");
        free(line);
        return 1;
    }

    // Trim newline character if present
    if (line[msg_len - 1] == '\n') {
        line[msg_len - 1] = '\0';
    }

    int row, col;
    sscanf(line, "%d %d", &row, &col);
    fprintf(stderr, "MYM row: %d, col: %d\n", row, col);
    // Allocate a buffer for the final message
    unsigned char final_msg[4];
    final_msg[0] = 0x05;
    switch(row){
        case 0:
            final_msg[1] = 0;
            break;
        case 1:
            final_msg[1] = 1;
            break;
        case 2:
            final_msg[1] = 2;
            break;
    }
    switch(col){
        case 0:
            final_msg[2] = 0;
            break;
        case 1:
            final_msg[2] = 1;
            break;
        case 2:
            final_msg[2] = 2;
            break;
    }
    final_msg[3] = '\0';  // Null-terminate for safety

    int s = sendto(sockfd, final_msg, strlen(final_msg), 0, dest, sizeof(*dest));
    if (s == -1) {
        fprintf(stderr, "Failed to send message\n");
        close(sockfd);
        free(line);
        return 1;
    }

    free(line);
    return 0;

    /*
    size_t size = 100; 
    char *line = malloc(100);
    char *msg = "MOV";
    int msg_len = getline(&line, &size, stdin);
    fprintf(stderr, "Read line:%s\n%s\n", line,msg);
    strcat(msg, line);
    fprintf(stderr, "Sending: %s\n", msg);
    int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));
    if (s == -1){
        fprintf(stderr, "Failed to send message\n");
        close(sockfd);
        free(line);
        return 1;
    }
    free(line);
    return 0;
*/
}

int end(struct message *m){
    fprintf(stderr, "Winner: player %s\n", m->info);
    return 0;
}


int get_message(int sockfd, char* line, int len, struct sockaddr *dest){
    socklen_t addr_len = sizeof(dest);
    // fprintf(stderr, "Waiting for message...\n");
    int msg_len = recvfrom(sockfd, line, len, 0, (struct sockaddr *) &dest, &addr_len);
    // fprintf(stderr, "Received: %s\n", line);
    if (msg_len > 0) {
        line[msg_len] = '\0'; // Null-terminate the received string
        struct message m;
        process_message(line, &m);
        // fprintf(stderr, "Message processed\n");
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
            fprintf(stderr, "MYM message\n");

            if(!mym(&m, sockfd, &dest)){
                return 1;
            }
        } else if (m.type == 3){
            // fprintf(stderr, "END message\n");
            if (!end(&m)){
                return 1;
            }
        } else if (m.type == 4){
            // fprintf(stderr, "TXT message\n");

            if (!txt(&m)){
                return 1;
            }
        } else {
            fprintf(stderr, "Unexpected message type\n");
            close(sockfd);
            free(line);
            return 5;
        }
    } else {
        perror("Failed to receive message");
        return 9;
    }
    return 0;
}


int main(int argc, char* argv[]) {
    // fprintf(stderr, "Successfully running\n");
    if (argc < 3){
        fprintf(stderr, "Missing argument. Please enter Port Number.\n");
        return 1;
    }

	char *ip_addr = argv[1];

    int port;
    if (sscanf(argv[2], "%d", &port) != 1) {
        fprintf(stderr, "Invalid Port.\n");
        return 3;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM,0);
    if (sockfd == -1){
        fprintf(stderr, "Socket creation failed");
    // } else {
    //     fprintf(stderr, "Socket created.\n");
    }

	struct sockaddr_in dest;
    struct in_addr *dst_addr = malloc(sizeof(struct in_addr));

    if (! inet_pton(AF_INET, ip_addr, dst_addr)){
        fprintf(stderr, "Could not turn IPaddress string to network address.\n");
        close(sockfd);
        return 2;
    }
	dest.sin_addr = *dst_addr;
    dest.sin_port = htons(port);
    dest.sin_family = AF_INET;

    // Send Hello message
    unsigned char msg[] = {0x04, 'H', 'e', 'l', 'l', 'o', '\0'};
    int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));
    if (s == -1){
        fprintf(stderr, "Failed to send message\n");
        close(sockfd);
        return 4;
    }
    // fprintf(stderr, "Hello sent\n");
    // Receive Welcome message
    char *line = malloc(1000);
    while(1){
        // fprintf(stderr, "Loop entered\n");
        if (!get_message(sockfd, line, 1000, (struct sockaddr *)&dest)){
            fprintf(stderr, "Error encountered\n");
            return 1;
        }
    }
    free(line);
    close(sockfd);
    return 0;
}
