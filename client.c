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

void * process_message(char* message){
    fprintf(stderr, "Processing message...\n");

    char t;
    char* info;
    sscanf(message, "%c %s", &t, &info);
    struct message *m;
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
    m->info = info;
    fprintf(stderr, "... message processed\n");

    return m;
}


int txt(struct message *m){
    fprintf(stderr, "%s\n", m->info);
    return 0;
}

int fyi(struct message *m){
    fprintf(stderr, "%c|%c|%c\n-+-+-\n%c|%c|%c\n-+-+-\n%c|%c|%c\n", m->info[0],m->info[1],m->info[2],m->info[3],m->info[4],m->info[5],m->info[6],m->info[7],m->info[8]);
    return 0;
}

int mym(struct message *m, int sockfd, char* line, struct sockaddr *dest){
    size_t size = 10; 
    int msg_len = getline(&line, &size, stdin);
    /*
    int *row;
    int *col;
    
    if (sscanf(line, "%d %d", &row, &col) < 0){
        fprintf(stderr, "Error parsing input\n");
        return 1;
    }
    if (!(((0 <= row) && (row <= 2)) && ((0 <= col) && (col <= 2))) ){
        fprintf(stderr, "Invalid location\n");
        return 2;
    }
    */
    line = strcat("MOV ", line);
    int s = sendto(sockfd, line, strlen(line), 0, (struct sockaddr *)&dest, sizeof(dest));
    if (s == -1){
        fprintf(stderr, "Failed to send message\n");
        close(sockfd);
        return 1;
    }
    return 0;

}

int end(struct message *m){
    fprintf(stderr, "Winner: player %s\n", m->info);
    return 0;
}


int get_message(int sockfd, char* line, int len, struct sockaddr *dest){
    socklen_t addr_len = sizeof(dest);
    fprintf(stderr, "Waiting for message...\n");
    int msg_len = recvfrom(sockfd, line, len, 0, (struct sockaddr *) &dest, &addr_len);
    fprintf(stderr, "Received: %s\n", line);
    if (msg_len > 0) {
        line[msg_len] = '\0'; // Null-terminate the received string
        struct message *m = process_message(line);
        if (m->type == 255) {
            fprintf(stderr, "Game is full\n");
            close(sockfd);
            free(line);
            return 0;
        } else if (m->type == 1){
            if(!fyi(m)){
                return 1;
            }
        } else if (m->type == 2){
            if(!mym(m, sockfd, line, dest)){
                return 1;
            }
        } else if (m->type == 3){
           if (!end(m)){
            return 1;
           }
        } else if (m->type == 4){
            if (!txt(m)){
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
    fprintf(stderr, "Successfully running\n");
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
    } else {
        fprintf(stderr, "Socket created.\n");
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
    char msg[] = {0x04, 'H', 'e', 'l', 'l', 'o', '\0'};
    int s = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));
    if (s == -1){
        fprintf(stderr, "Failed to send message\n");
        close(sockfd);
        return 4;
    }
    fprintf(stderr, "Hello sent\n");
    // Receive Welcome message
    char *line = malloc(100);
    while(1){
        fprintf(stderr, "Loop entered\n");
        if (!get_message(sockfd, line, 100, (struct sockaddr *)&dest)){
            fprintf(stderr, "Error encountered\n");
            return 1;
        }
    }
    free(line);
    close(sockfd);
    return 0;
}
