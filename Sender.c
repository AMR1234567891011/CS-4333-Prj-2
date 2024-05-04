#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <string.h>
#include <errno.h>

static char mash[] = "I was working in the lab, late one night\nWhen my eyes beheld an eerie sight\nFor my monster from his slab, began to rise\nAnd suddenly to my surprise\nHe did the monster mash\n(The monster mash) It was a graveyard smash\n(He did the mash) It caught on in a flash\n(He did the mash) He did the monster mash\nFrom my laboratory in the castle east\nTo the master bedroom where the vampires feast\nThe ghouls all came from their humble abodes\nTo get a jolt from my electrodes\nThey did the monster mash\n(The monster mash) It was a graveyard smash\n(They did the mash) It caught on in a flash\n(They did the mash) They did the monster mash\nThe zombies were having fun (Wa hoo, tennis shoe)\nThe party had just begun (Wa hoo, tennis shoe)\nThe guests included Wolfman, Dracula and his son\nThe scene was rockin', all were digging the sounds\nIgor on chains, backed by his baying hounds\nThe coffin-bangers were about to arrive\nWith their vocal group, 'The Crypt-Kicker Five'\nThey played the monster mash\n(The monster mash) It was a graveyard smash\n(They played the mash) It caught on in a flash\n(They played the mash) They played the monster mash\nOut from his coffin, Drac's voice did ring\nSeems he was troubled by just one thing\nHe opened the lid and shook his fist and said\nWhatever happened to my Transylvania Twist?\nIt's now the monster mash\n(The monster mash) And it's a graveyard smash\n(It's now the mash) It's caught on in a flash\n(It's now the mash) It's now the monster mash\nNow everything's cool, Drac's a part of the band\nAnd my Monster Mash is the hit of the land\nFor you, the living, this mash was meant too\nWhen you get to my door, tell them Boris sent you\nThen you can monster mash\n(The monster mash) And do my graveyard smash\n(Then you can mash) You'll catch on in a flash\n(Then you can mash) Then you can monster mash\nEasy Igor, you impetuous young boy (Wa hoo, monster mash)\n(Wa hoo, monster mash)\n(Wa hoo, monster mash)\n(Wa hoo, monster mash)\n(Wa hoo, monster mash)\r\n";

/*
Compile the code on gcc with the command below, on other 
compilers you can use pragma
gcc Receiver.c -lws2_32 -o Receiver

Also make sure to disable windows defender and folder proteciton because that will delte your binaries
*/ 
char* getFilename();
int getLocalPort();
int getMode();
long getModeParameter();
int getReceiver();//probably a socket struct
long getTimeout();
int sendFile();
void setFileName();
int setLocalPort();
int setMode();
int setModeParameter();
int setReceiver();
int setTimeout();

/*"Private Methods"*/
unsigned char* intToBytes(int n){
    unsigned char* b = malloc(4*sizeof(unsigned char));
    b[3] = (n >> 24);
    b[2] = (n >> 16);
    b[1] = (n >> 8);
    b[0] = n;
    return b;
}
unsigned int bytesToInt(unsigned char* bytes){
    unsigned int seq = 0;
    seq+= bytes[0];
    seq+= (bytes[1] << 8);
    seq+= (bytes[2] << 16);
    seq+= (bytes[3] << 24);
    return seq;
} 
static int getSeqNumAndShift(unsigned char* packet){//returns the sequence number removes the sequence number from the front of the packet
    int seqNum = 0;//sequence numbers start at 1
    seqNum = bytesToInt(packet);
    /*now shift the memory down*/ 
    memmove(packet, packet+4, 252);
    return seqNum;
}

struct sockaddr_in serverAddr, slidingAddr;
int sockfd, sockfdSliding;
int rc;
int serverAddrLen;
int wasInit = -1;
static void init(){//initializes sockets needed and windows api
WSADATA wsa;
if (WSAStartup(MAKEWORD(2,2),&wsa) != 0){
printf("WSA STARTUP FAILED");
}

sockfd = socket(AF_INET, SOCK_DGRAM, 17);
if(sockfd < 0){
    printf("socket create failed womp womp");
}
serverAddr.sin_family = AF_INET;
serverAddr.sin_port = htons(8080);
serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
DWORD timeout = 1000;
setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char const*)&timeout, sizeof(timeout));
serverAddrLen = sizeof(serverAddr);
wasInit = 0;
}


static void addSeqToPacket(char * buffer, int seq){//adds a sequence number to the begining of a buffer. converts int into 4 bytes then prepends the 4 bytes
    unsigned char *bytes  = intToBytes((unsigned int)seq);
    memmove(buffer+sizeof(bytes), buffer, strlen(buffer));
    memmove(buffer, (char *) bytes, sizeof(bytes));
}

static int getFormattedString(char* buffer, char* string, int seq){//sets buffer equal to the 252 bytes of string specified by seq, if seq is the final packet then returns -1
    int last = seq;
    int readLength = 252;
    if(strlen(string) < (readLength*seq)){
        readLength = (strlen(string) - (252* (seq-1)));
        last = -1;
    }
    memmove(buffer, string+(seq*252), readLength);
    return last;
    
}static int sendSlidingWindow(int seq, char *string){//sends a packet with a sequence number retruns if send was true;
    char buffer[256] = {0};
    seq = getFormattedString(buffer, string, seq);
    addSeqToPacket(buffer,seq);
    int send  =  sendto(sockfd, buffer, 256, 0,(SOCKADDR *)&serverAddr, sizeof(serverAddr));
    if(send < 0){
        fprintf(stderr,"\n error sending packet\n");
    }
    return seq;

}


static int slidingWindow(char *string){//Preforms the sliding window protocol
    if(wasInit == -1){
        init();
    }
    int back =  strlen(string) / 252;
    int Ack = 0;
    int max = 0;
    int idx = 0;
    while(1){
        while(Ack < 5){
            if(idx-1 > back){
                break;
            }
            max = sendSlidingWindow(idx, mash);
            printf("\nSent Packet: %d", max);
            idx++;
            Ack++;
        }
        unsigned char receive[4];
        int read = 0;
        while(Ack > 0){
        read = recvfrom(sockfd, receive, 4, 0, (SOCKADDR *)&serverAddr, &serverAddrLen);
        if(read > 0){
            int ackSeqNum = bytesToInt(receive);
            printf("\nACK Received: %d",ackSeqNum);
            Ack--;
            if(ackSeqNum == -1){
                printf("\nSliding Window Succes!!!\n");
                return 0;
            }
        }
        }
    }
    return 0;
    
}

static int sendStopAndWait(int seq,char *string ){//returns zero on success sent and -1 on error, sends individual packet for stop and wait 
init();
char* buffer = calloc(256, sizeof(char));
seq = getFormattedString(buffer, string, seq);
addSeqToPacket(buffer, seq);
buffer[256] = '\0';
while(1){
    unsigned char receive[4];
    int addrLen = sizeof(serverAddr);
    int timeOuts = 0;

    while(timeOuts < 3){
        printf("\nSent Packet: %d", seq);
        int send = sendto(sockfd, buffer, 256, 0, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
    if(send < 0){
        fprintf(stderr, "error sending");
    }
    int read =  recvfrom(sockfd, receive, 4, 0, (SOCKADDR *)&serverAddr, &addrLen);
        if(read > 0){
            int ackSeqNum = bytesToInt(receive);
            printf("\nACK Received: %d",ackSeqNum);
            if(ackSeqNum == -1){
                printf("\nStop and Wait Success!!!");
            }
            free(buffer);
            return 0;
        }else{//the read function timed out if this block is being executed
            timeOuts ++;
        }
    }
    free(buffer);
    fprintf(stderr, "\nsocket timed out waiting for acknoledgement\n");
    return -1;

}
free(buffer);
}
int stopAndWait(char *string){//preforms stop and wait protocol
    int val = 0;
    int last =  strlen(string) / 252;
    for(int i = 0; i <= last+1; i++){
        val = sendStopAndWait(i,string);
        if(val < 0){
            break;
        }
    }
    if(val < 0){
        fprintf(stderr, "\n** socket timed out waiting for an acknowledgement **\n");
        return -1;
    }
    return 0;
}

int main(){
printf("Stop and Wait Testing Now !!!");
stopAndWait(mash);
printf("\n\n\nSliding Window Testing Now!!!");
slidingWindow(mash);
close(sockfd);
return 0;
}