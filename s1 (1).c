#include <stdio.h>
#include <sys/socket.h> //for socket(), connect(), send(), recv()
#include <arpa/inet.h>  // different address structures are declared
#include <stdlib.h>     // atoi() which convert string to integer
#include <string.h>
#include <unistd.h> // close() function
#include <stdbool.h>
#define MAXPENDING 5
#define BUFLEN 512 // Max length of buffer
#define PORT 6969
#define drop_packet 10 // x% packets dropped

typedef struct packet
{
    int sq_no;
    char data[BUFLEN];
} DATA;

bool discard()
{
    return (rand() % 10 == 0);
}

void die(char *s)
{
    perror(s);
    exit(1);
}
int i = 0;
int main()
{
    char ch;
    struct sockaddr_in si_me, si_other;

    int s, i, slen = sizeof(si_other), recv_len;
    char buf[BUFLEN];
    DATA send_pkt, rcv_pkt;
    // create a UDP socket
    if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        die("Can't create Socket");
    }
    memset((char *)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
    {
        die("Can't bind the connection");
    }

    printf("Binding successful\n");

    int temp1 = listen(s, MAXPENDING);

    printf("listening\n");

    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);

    FILE *fp;
    fp = fopen("temp.txt", "w");
    fprintf(fp, "0");
    fclose(fp);
    FILE *list;
    int childpid = fork();

    if (childpid == 0)
    {
        int clientsocket = accept(s, (struct sockaddr *)&client_addr, &client_len);
        printf("accepted client %s\n", inet_ntoa(client_addr.sin_addr));
        int state = 0;
        while (1)
        {
            switch (state)
            {
            case 0:
                fp = fopen("temp.txt", "r");
                if (fp == NULL)
                {
                    die("File open error");
                    return 1;
                }
                ch = fgetc(fp);
                // fseek(fp, 0, SEEK_SET);
                fclose(fp);

                if (ch == '0')
                {
                    if (recv(clientsocket, &rcv_pkt, sizeof(rcv_pkt), 0) == -1)
                    {
                        die("Data not received");
                    }
                    list = fopen("list.txt", "a");
                    fprintf(list, "%s,", rcv_pkt.data);
                    fclose(list);
                    if (!discard())
                    {
                        if (rcv_pkt.sq_no == 0)
                        {
                            send_pkt.sq_no = 0;
                            if (send(clientsocket, &send_pkt, sizeof(send_pkt), 0) == -1)
                            {
                                die("Data not received");
                            }
                            state = 1;

                            // sh = 1;

                            fp = fopen("temp.txt", "w");
                            if (fp == NULL)
                            {
                                die("File open error");
                            }

                            fprintf(fp, "1");
                            fclose(fp);
                        }
                        else
                        {
                            send_pkt.sq_no = 1;
                            if (send(clientsocket, &send_pkt, sizeof(send_pkt), 0) == -1)
                            {
                                die("Data not received");
                            }
                        }
                    }
                    else
                    {
                        printf("Packet Dropped\n");
                    }
                }
                else
                {
                    if ((recv_len = recv(clientsocket, &rcv_pkt, sizeof(rcv_pkt), 0)) == -1)
                    {
                        die("Data not received");
                    }
                }
                break;

            case 1:
                fp = fopen("temp.txt", "r");
                if (fp == NULL)
                {
                    die("File open error");
                    return 1;
                }
                ch = fgetc(fp);
                // fseek(fp, 0, SEEK_SET)
                fclose(fp);

                if (ch == '0')
                {
                    if ((recv_len = recv(clientsocket, &rcv_pkt, sizeof(rcv_pkt), 0)) == -1)
                    {
                        die("Data not received");
                    }
                    list = fopen("list.txt", "a");
                    fprintf(list, "%s,", rcv_pkt.data);
                    fclose(list);
                    if (!discard())
                    {
                        if (rcv_pkt.sq_no == 1)
                        {
                            send_pkt.sq_no = 1;
                            if (send(clientsocket, &send_pkt, sizeof(send_pkt), 0) == -1)
                            {
                                die("Data not received");
                            }
                            state = 0;
                            // sh = 1;

                            fp = fopen("temp.txt", "w");
                            if (fp == NULL)
                            {
                                die("File open error");
                                return 1;
                            }

                            fprintf(fp, "1");
                            fclose(fp);
                            // fprintf(fp, "1");
                        }
                        else
                        {
                            send_pkt.sq_no = 0;
                            if (send(clientsocket, &send_pkt, sizeof(send_pkt), 0) == -1)
                            {
                                die("Data not received");
                            }
                        }
                    }
                    else
                    {
                        printf("Packet Dropped\n");
                    }
                }
                else
                {
                    if ((recv_len = recv(clientsocket, &rcv_pkt, sizeof(rcv_pkt), 0)) == -1)
                    {
                        die("Data not received");
                    }
                }
            }
        }
    }
    else
    {
        int clientsocket = accept(s, (struct sockaddr *)&client_addr, &client_len);

        printf("accepted client %s\n", inet_ntoa(client_addr.sin_addr));

        int state = 0;
        while (1)
        {
            switch (state)
            {
            case 0:
                fp = fopen("temp.txt", "r");
                if (fp == NULL)
                {
                    die("File open error");
                    return 1;
                }
                ch = fgetc(fp);
                // fseek(fp, 0, SEEK_SET);
                fclose(fp);

                if (ch == '0')
                {
                    if ((recv_len = recv(clientsocket, &rcv_pkt, sizeof(rcv_pkt), 0)) == -1)
                    {
                        die("Data not received");
                    }
                }
                else
                {
                    if ((recv_len = recv(clientsocket, &rcv_pkt, sizeof(rcv_pkt), 0)) == -1)
                    {
                        die("Data not received");
                    }
                    list = fopen("list.txt", "a");
                    fprintf(list, "%s\n", rcv_pkt.data);
                    fclose(list);
                    if (!discard())
                    {
                        if (rcv_pkt.sq_no == 0)
                        {
                            send_pkt.sq_no = 0;
                            if (send(clientsocket, &send_pkt, sizeof(send_pkt), 0) == -1)
                            {
                                die("Data not received");
                            }
                            state = 1;
                            // sh = 0;
                            fp = fopen("temp.txt", "w");
                            if (fp == NULL)
                            {
                                die("File open error");
                                return 1;
                            }

                            fprintf(fp, "0");
                            fclose(fp);
                        }
                        else
                        {
                            send_pkt.sq_no = 1;
                            if (send(clientsocket, &send_pkt, sizeof(send_pkt), 0) == -1)
                            {
                                die("Data not received");
                            }
                        }
                    }
                    else
                    {
                        printf("Packet Dropped\n");
                    }
                }
                break;
            case 1:

                fp = fopen("temp.txt", "r");
                if (fp == NULL)
                {
                    die("File open error");
                    return 1;
                }
                ch = fgetc(fp);
                // fseek(fp, 0, SEEK_SET);
                fclose(fp);

                if (ch == '0')
                {
                    if ((recv_len = recv(clientsocket, &rcv_pkt, sizeof(rcv_pkt), 0)) == -1)
                    {
                        die("Data not received");
                    }
                }
                else
                {
                    if ((recv_len = recv(clientsocket, &rcv_pkt, sizeof(rcv_pkt), 0)) == -1)
                    {
                        die("Data not received");
                    }
                    list = fopen("list.txt", "a");
                    fprintf(list, "%s\n", rcv_pkt.data);
                    fclose(list);
                    if (!discard())
                    {
                        if (rcv_pkt.sq_no == 1)
                        {
                            send_pkt.sq_no = 1;
                            if (send(clientsocket, &send_pkt, sizeof(send_pkt), 0) == -1)
                            {
                                die("Data not received");
                            }
                            state = 0;
                            // sh = 0;
                            fp = fopen("temp.txt", "w");
                            if (fp == NULL)
                            {
                                die("File open error");
                                return 1;
                            }

                            fprintf(fp, "0");
                            fclose(fp);
                            // fprintf(fp, "0");
                        }
                        else
                        {
                            send_pkt.sq_no = 0;
                            if (send(clientsocket, &send_pkt, sizeof(send_pkt), 0) == -1)
                            {
                                die("Data not received");
                            }
                        }
                    }
                    else
                    {
                        printf("Packet Dropped\n");
                    }
                }
            }
        }
    }
    return 0;
}
