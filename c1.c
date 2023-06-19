// ********** all libraries ********* //
#include <stdio.h>  
#include <string.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>   
#include <unistd.h> 
#include <sys/ipc.h>
#include <sys/shm.h>


// ************ all defines ********* //
#define SHARED_SIZE 1024
#define BUFFERLENGTH 2048 
#define PORT1 8080  
#define PORT2 8081
#define PDR 0.1 

#define DATA false
#define ACK true


// ************ all structs ********* //
typedef enum boolean
{
    false,
    true
} bool;

typedef struct
{
    bool type;        
    int id;            
    int seq_no;        
    int length;          
    int actual_size;          
    int last;          
    char data[BUFFERLENGTH];
} packet;

void die(char *s)
{
    perror(s);
    exit(1);
}

int main(void)
{
    int shmid;
    int *shvar, last_seq_no;
    key_t key;
    char *shm;

    // creating a shared memory space -- taken from gfg //
    // Generate a unique key for the shared memory segment
    key = ftok(".", 's');

    // Create the shared memory segment
    shmid = shmget(key, SHARED_SIZE, IPC_CREAT | 0666);

    // Attach to the shared memory segment
    shm = shmat(shmid, NULL, 0);

    // Initialize the shared variable
    shvar = (int *)shm;
    *shvar = 0;
    last_seq_no = 0; // keep track of the last sequence number received

    struct sockaddr_in s1, s2, s3;
    int s, i, slen = sizeof(s3), recieve_length;
    char bufferlength[BUFFERLENGTH];

    /*CREATE A TCP SOCKET 1*/

    int socketfd1 = socket(PF_INET, SOCK_STREAM, 0);
    if (socketfd1 < 0)
    {
        printf("Error while server socket creation");
        exit(0);
    }
    printf("Server Socket Created for client 1\n");

    /*CONSTRUCT LOCAL ADDRESS STRUCTURE FOR CLIENT 1*/

    struct sockaddr_in saddress1, caddress1;
    memset(&saddress1, 0, sizeof(saddress1));
    saddress1.sin_family = AF_INET;
    saddress1.sin_port = htons(PORT1);
    saddress1.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("Server 1 address assigned\n");

    /*BINDING FOR S1*/

    if (bind(socketfd1, (struct sockaddr *)&saddress1, sizeof(saddress1)) < 0)
    {
        printf("Error while binding\n");
        exit(0);
    }
    printf("Binding successful server 1\n");

    /*LISTENING FOR S1*/

    if (listen(socketfd1, 10) < 0)
    {
        printf("Error in listen");
        exit(0);
    }
    printf("Now Listening socketfd1\n\n");

    /*CREATE A TCP SOCKET 2*/

    int socketfd2 = socket(PF_INET, SOCK_STREAM, 0);
    if (socketfd2 < 0)
    {
        printf("Error while server socket creation");
        exit(0);
    }
    printf("Server Socket Created for client 2\n");

    /*CONSTRUCT LOCAL ADDRESS STRUCTURE FOR CLIENT 2*/

    struct sockaddr_in saddress2, caddress2;
    memset(&saddress2, 0, sizeof(saddress2));
    saddress2.sin_family = AF_INET;
    saddress2.sin_port = htons(PORT2);
    saddress2.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("Server 2 address assigned\n");

    /*BINDING FOR S2*/

    if (bind(socketfd2, (struct sockaddr *)&saddress2, sizeof(saddress2)) < 0)
    {
        printf("Error while binding\n");
        exit(0);
    }
    printf("Binding successful server 2\n");

    /*LISTENING FOR S2*/

    if (listen(socketfd2, 10) < 0)
    {
        printf("Error in listen");
        exit(0);
    }
    printf("Now Listening socketfd2\n\n");

    /*CONNECTING FOR S1*/

    int clientc1 = sizeof(caddress1);
    int csocket1 = accept(socketfd1, (struct sockaddr *)&caddress1, &clientc1);
    if (csocket1 < 0)
    {
        printf("Error in client socket");
        exit(0);
    }
    socketfd1 = csocket1;
    printf("c1 connected\n");

    /*CONNECTING FOR S2*/

    int clientc2 = sizeof(caddress2);
    int csocket2 = accept(socketfd2, (struct sockaddr *)&caddress2, &clientc2);
    if (csocket2 < 0)
    {
        printf("Error in client socket");
        exit(0);
    }
    socketfd2 = csocket2;
    printf("c2 connected\n");

    pid_t processid;
    processid = fork();
    if (processid < 0)
    {
        return -1;
    }

    int state = 0;
    while (1)
    {
        if (processid == 0)
        {
            switch (state)
            {
            case 0:

                if ((recieve_length = recv(socketfd2, bufferlength, BUFFERLENGTH, 0)) == -1)
                {
                    die("recv()");
                }

                packet *rcvd_pkt = (packet *)bufferlength;

                if (rcvd_pkt->length == 0)
                {
                    continue;
                }

                // calculate probability of dropping packet
                double num = rand() % 100;
                num = num / 100;

                if (num < PDR)
                {
                    printf("DROP PKT : ");
                    printf("Seq. No. = %d, actual_size = %d Bytes \n", rcvd_pkt->seq_no, rcvd_pkt->actual_size);
                    continue;
                }

                if (rcvd_pkt->id == 1)
                {
                    packet acked_pkt;
                    acked_pkt.type = ACK;
                    acked_pkt.id = rcvd_pkt->id;
                    acked_pkt.seq_no = rcvd_pkt->seq_no + rcvd_pkt->length; 
                    acked_pkt.length = 0;
                    acked_pkt.actual_size = 0;
                    acked_pkt.last = rcvd_pkt->last;
                    memset(acked_pkt.data, 0, BUFFERLENGTH);

                    if (send(socketfd2, &acked_pkt, sizeof(acked_pkt), 0) == -1)
                    {
                        die("send()");
                    }

                    continue;
                }

                if (*shvar == 0)
                {
                    continue;
                }

                FILE *file_out;
                file_out = fopen("list.txt", "a");
                if (file_out == NULL)
                {
                    die("fopen");
                }
                fwrite(rcvd_pkt->data, sizeof(char), rcvd_pkt->length, file_out);
                fflush(file_out);
                fclose(file_out);

                printf("RCVD PKT: ");
                printf("Seq. No. = %d, actual_size = %d Bytes\n ", rcvd_pkt->seq_no, rcvd_pkt->actual_size);
                last_seq_no = (rcvd_pkt->seq_no + rcvd_pkt->length > last_seq_no) ? rcvd_pkt->seq_no + rcvd_pkt->length : last_seq_no;

                // now reply the client with an ACK packet
                packet acked_pkt;
                acked_pkt.type = ACK;
                acked_pkt.id = 0;
                acked_pkt.seq_no = last_seq_no;     
                acked_pkt.length = 0;              
                acked_pkt.actual_size = 0;        
                acked_pkt.last = rcvd_pkt->last; 
                memset(acked_pkt.data, 0, BUFFERLENGTH); 
              
                printf("SENT ACK : ");
                printf("Seq. No. = %d \n", acked_pkt.seq_no);

                if (send(socketfd2, &acked_pkt, sizeof(acked_pkt), 0) == -1)
                {
                    die("send()");
                }
                state = 1;
                *shvar = 0;

                if (rcvd_pkt->last == 1)
                {
                    printf("File received successfully.\n");
                    break;
                }

                break;
            case 1:

                if ((recieve_length = recv(socketfd2, bufferlength, BUFFERLENGTH, 0)) == -1)
                {
                    die("recievev()");
                }
                rcvd_pkt = (packet *)bufferlength;
                if (rcvd_pkt->length == 0)
                {
                    continue;
                }

                // Drop packet with probability PDR
                num = rand() % 100;
                num = num / 100;

                if (num < PDR)
                {
                    printf("DROP PKT : ");
                    printf("Seq. No. = %d, actual_size = %d Bytes \n", rcvd_pkt->seq_no, rcvd_pkt->actual_size);
                    continue;
                }

                if (rcvd_pkt->id == 0)
                {
                    packet acked_pkt;
                    acked_pkt.type = ACK;
                    acked_pkt.id = rcvd_pkt->id;
                    acked_pkt.seq_no = rcvd_pkt->seq_no + rcvd_pkt->length;
                    acked_pkt.length = 0;                             
                    acked_pkt.actual_size = 0;                          
                    acked_pkt.last = rcvd_pkt->last;                 
                    memset(acked_pkt.data, 0, BUFFERLENGTH);                   
                    printf("SENT ACK : ");
                    printf("Seq. No. = %d \n", acked_pkt.seq_no);

                    if (send(socketfd2, &acked_pkt, sizeof(acked_pkt), 0) == -1)
                    {
                        die("send()");
                    }
                    continue;
                }

                if (*shvar == 0)
                {
                    continue;
                }

                file_out = fopen("list.txt", "a");
                if (file_out == NULL)
                {
                    die("fopen");
                }
                fwrite(rcvd_pkt->data, sizeof(char), rcvd_pkt->length, file_out);
                fflush(file_out);
                fclose(file_out);

                printf("RCVD DATA ");
                printf("Seq. No. = %d, actual_size = %d Bytes\n ", rcvd_pkt->seq_no, rcvd_pkt->actual_size);

                last_seq_no = (rcvd_pkt->seq_no + rcvd_pkt->length > last_seq_no) ? rcvd_pkt->seq_no + rcvd_pkt->length : last_seq_no;

                acked_pkt;
                acked_pkt.type = ACK;
                acked_pkt.id = 1;
                acked_pkt.seq_no = last_seq_no;         
                acked_pkt.length = 0;                   
                acked_pkt.actual_size = 0;                    
                acked_pkt.last = rcvd_pkt->last; 
                memset(acked_pkt.data, 0, BUFFERLENGTH);  
                
                printf("SENT ACK : ");
                printf("Seq. No. = %d \n", acked_pkt.seq_no);

                if (send(socketfd2, &acked_pkt, sizeof(acked_pkt), 0) == -1)
                {
                    die("send()");
                }
                state = 0;
                *shvar = 0;

                if (rcvd_pkt->last == 1)
                {
                    printf("File received successfully. (Check list.txt)\n  Press Ctrl+C to terminate.\n");
                    break;
                }

                break;
            default:
                break;
            }
        }
        else
        {
           if(state == -1)
           {
            break;
           }
            switch (state)
            {
            case 0:
               
                if ((recieve_length = recv(socketfd1, bufferlength, BUFFERLENGTH, 0)) == -1)
                {
                    die("recv()");
                    
                }

                packet *rcvd_pkt = (packet *)bufferlength;

                if (rcvd_pkt->length == 0)
                {
                    continue;
                }

                // Drop packet with probability PDR
                double num = rand() % 100;
                num = num / 100;

                if (num < PDR)
                {
                    printf("DROP PKT : ");
                    printf("Seq. No. = %d, actual_size = %d Bytes \n", rcvd_pkt->seq_no, rcvd_pkt->actual_size);
                    continue;
                }

                if (rcvd_pkt->id == 1)
                {
                    packet acked_pkt;
                    acked_pkt.type = ACK;
                    acked_pkt.id = rcvd_pkt->id;
                    acked_pkt.seq_no = rcvd_pkt->seq_no + rcvd_pkt->length; 
                    acked_pkt.length = 0;  
                    acked_pkt.actual_size = 0;   
                    acked_pkt.last = rcvd_pkt->last;    
                    memset(acked_pkt.data, 0, BUFFERLENGTH);   
                    printf("SENT ACK : ");
                    printf("Seq. No. = %d \n", acked_pkt.seq_no);

                    if (send(socketfd1, &acked_pkt, sizeof(acked_pkt), 0) == -1)
                    {
                        die("send()");
                    }
                    continue;
                }

                if (*shvar == 1)
                {
                    continue;
                }

                FILE *file_out;
                file_out = fopen("list.txt", "a");
                if (file_out == NULL)
                {
                    die("fopen");
                }
                fwrite(rcvd_pkt->data, sizeof(char), rcvd_pkt->length, file_out);
                fflush(file_out);
                fclose(file_out);

                printf("RCVD DATA : ");
                printf("Seq. No. = %d, actual_size = %d Bytes\n ", rcvd_pkt->seq_no, rcvd_pkt->actual_size);

                last_seq_no = (rcvd_pkt->seq_no + rcvd_pkt->length > last_seq_no) ? rcvd_pkt->seq_no + rcvd_pkt->length : last_seq_no;

                packet acked_pkt;
                acked_pkt.type = ACK;
                acked_pkt.seq_no = last_seq_no;       
                acked_pkt.actual_size = 0;
                acked_pkt.length = 0;
                acked_pkt.id = 0;        
                acked_pkt.last = rcvd_pkt->last; 
                memset(acked_pkt.data, 0, BUFFERLENGTH);   
                
                printf("SENT ACK : ");
                printf("Seq. No. = %d \n", acked_pkt.seq_no);

                if (send(socketfd1, &acked_pkt, sizeof(acked_pkt), 0) == -1)
                {
                    die("send()");
                }
                state = 1;
                *shvar = 1;

                if (rcvd_pkt->last == 1)
                {
                    break;
                }

                break;
            case 1:

                if ((recieve_length = recv(socketfd1, bufferlength, BUFFERLENGTH, 0)) == -1)
                {
                    die("recv()");
                }

                rcvd_pkt = (packet *)bufferlength;

                if (rcvd_pkt->length == 0)
                {
                    continue;
                }

                // Drop packet with probability PDR
                num = rand() % 100;
                num = num / 100;

                if (num < PDR)
                {
                    printf("DROP PKT : ");
                    printf("Seq. No. = %d, actual_size = %d Bytes \n", rcvd_pkt->seq_no, rcvd_pkt->actual_size);
                    continue;
                }

                if (rcvd_pkt->id == 0)
                {
                    packet acked_pkt;
                    acked_pkt.type = ACK;
                    acked_pkt.id = rcvd_pkt->id;
                    acked_pkt.seq_no = rcvd_pkt->seq_no + rcvd_pkt->length; 
                    acked_pkt.length = 0;                                     
                    acked_pkt.actual_size = 0;                                        
                    acked_pkt.last = rcvd_pkt->last;                             
                    memset(acked_pkt.data, 0, BUFFERLENGTH); 
                    printf("SENT ACK : ");
                    printf("Seq. No. = %d \n", acked_pkt.seq_no);

                    if (send(socketfd1, &acked_pkt, sizeof(acked_pkt), 0) == -1)
                    {
                        die("send()");
                    }
                    continue;
                }

                if (*shvar == 1)
                {
                    continue;
                }

                file_out;
                file_out = fopen("list.txt", "a");
                if (file_out == NULL)
                {
                    die("fopen");
                }
                fwrite(rcvd_pkt->data, sizeof(char), rcvd_pkt->length, file_out);
                fflush(file_out);
                fclose(file_out);

                printf("RCVD DATA : ");
                printf("Seq. No. = %d, actual_size = %d Bytes\n ", rcvd_pkt->seq_no, rcvd_pkt->actual_size);
                last_seq_no = (rcvd_pkt->seq_no + rcvd_pkt->length > last_seq_no) ? rcvd_pkt->seq_no + rcvd_pkt->length : last_seq_no;
                acked_pkt.type = ACK;
                acked_pkt.id = 1;
                acked_pkt.seq_no = last_seq_no;         
                acked_pkt.length = 0;               
                acked_pkt.actual_size = 0;           
                acked_pkt.last = rcvd_pkt->last;
                memset(acked_pkt.data, 0, BUFFERLENGTH);
        
                printf("SENT ACK : ");
                printf("Seq. No. = %d \n", acked_pkt.seq_no);

                if (send(socketfd1, &acked_pkt, sizeof(acked_pkt), 0) == -1)
                {
                    die("send()");
                }
                state = 0;
                *shvar = 1;

                if (rcvd_pkt->last == 1)
                {

                    break;
                }

                break;
            default:
                break;
            }
        }
    }

    if (shmdt(shvar) == -1)
    {
        die("shmdt");
    }
    close(s);
    return 0;
}