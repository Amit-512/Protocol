#include <stdio.h>
#include <sys/socket.h> //for socket(), connect(), send(), recv()
#include <arpa/inet.h>  // different address structures are declared
#include <stdlib.h>     // atoi() which convert string to integer
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h> // close() function
#define MAX_NAMES 100
#define MAX_NAME_LENGTH 50
#define BUFLEN 512 // Max length of buffer
#define PORT 6969
void die(char *s)
{
    perror(s);
    exit(1);
}

typedef struct packet
{
    int sq_no;
    char data[BUFLEN];
} DATA;
int state = 0;
DATA send_pkt;

struct sockaddr_in si_other;
int s, i, slen = sizeof(si_other);
char buf[BUFLEN];
char message[BUFLEN];

void sig_handler(int signum)
{
    printf("RE-TRANSMIT PKT: Seq. No. = 40, Size = 30 Bytes\n");
    if (send(s, &send_pkt, sizeof(send_pkt), 0) == -1)
    {
        die("Data not sent");
    }
    alarm(5);
}

int main()
{

    signal(SIGALRM, sig_handler);
    DATA rcv_pkt;

    /* CREATE A TCP SOCKET*/
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0)
    {
        die("Socket Not Created");
    }

    memset((char *)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    si_other.sin_addr.s_addr = inet_addr("172.18.122.2");

    if (connect(s, (struct sockaddr *)&si_other, sizeof(si_other)) < 0)
    {
        die("Connection not Established");
    }
    FILE *file = fopen("id.txt", "r"); // open the file in read mode

    if (file == NULL)
    {
        printf("Unable to open file\n");
        return 1;
    }

    char names[MAX_NAMES][MAX_NAME_LENGTH]; // array to store student names
    char c = fgetc(file);                   // read the first character from the file
    int nameIndex = 0;                      // index of current name in array
    int charIndex = 0;                      // index of current character in name

    while (c != EOF && nameIndex < MAX_NAMES)
    {
        if (c == ',')
        {
            names[nameIndex][charIndex] = '\0'; // terminate the name string
            nameIndex++;                        // move to the next name in the array
            charIndex = 0;                      // reset the character index for the new name
        }
        else if (c == '.')
        {
            break; // end of file
        }
        else
        {
            names[nameIndex][charIndex] = c; // add the character to the current name
            charIndex++;                     // move to the next character in the name
        }
        c = fgetc(file); // read the next character from the file
    }

    fclose(file); // close the file

    int i = 0;
    while (i <= nameIndex && 1)
    {
        switch (state)
        {
        case 0:
            strcpy(send_pkt.data, names[i]);
            printf("SENT PKT: Seq. No. = %ld, Size = %ld\n", i * sizeof(send_pkt), sizeof(send_pkt));
            // gets(send_pkt.data);
            send_pkt.sq_no = 0;
            if (send(s, &send_pkt, sizeof(send_pkt), 0) == -1)
            {
                die("Data not sent");
            }
            alarm(5);
            state = 1;
            break;
        case 1:
            printf("Waiting for ack 0 \n");
            if (recv(s, &rcv_pkt, sizeof(rcv_pkt), 0) == -1)
            {
                die("Data not received");
            }
            if (rcv_pkt.sq_no == 0)
            {
                printf("RCVD ACK: Seq. No. = 0\n");
                i++;
                state = 2;
            }
            else
            {
                break;
            }
            alarm(0);
            break;
        case 2:
            strcpy(send_pkt.data, names[i]);
            printf("SENT PKT: Seq. No. = %ld, Size = %ld\n", i * sizeof(send_pkt), sizeof(send_pkt));
            // gets(send_pkt.data);
            send_pkt.sq_no = 1;
            if (send(s, &send_pkt, sizeof(send_pkt), 0) == -1)
            {
                die("Data not sent");
            }
            state = 3;
            alarm(5);
            break;
        case 3:
            printf("Waiting for ack 1 \n");
            if (recv(s, &rcv_pkt, sizeof(rcv_pkt), 0) == -1)
            {
                die("Data not received");
            }
            if (rcv_pkt.sq_no == 1)
            {
                printf("RCVD ACK: Seq. No. = 1\n");

                i++;
                state = 0;
            }
            else
            {

                break;
            }
            alarm(0);
            break;
        }
    }
    close(s);
    return 0;
}