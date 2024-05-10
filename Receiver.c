#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
/*
Compile the code on gcc with the command below, on other 
compilers you can use pragma
gcc Receiver.c -lws2_32 -o Receiver

be sure to trun off windows defender and folder protection as this will delete your binaries
first four bytes is a sequence number 
*/ 
char* getFilename();
int getLocalPort();
int getMode();
long getModeParamter();
int receiveFile();
void setFilename(char* fileName);
int setLocalPort(int port);
int setMode(int mode);
int setModeParameter(long n);

struct sockaddr source;
int slen =  sizeof(source);

/*"Private Methods"*/
unsigned int bytesToInt(unsigned char* bytes){
    unsigned int seq = 0;
    seq+= bytes[0];
    seq+= (bytes[1] << 8);
    seq+= (bytes[2] << 16);
    seq+= (bytes[3] << 24);
    return seq;
} 
unsigned char* intToBytes(unsigned int n){
    unsigned char* b = malloc(4*sizeof(unsigned char));
    b[3] = (n >> 24);
    b[2] = (n >> 16);
    b[1] = (n >> 8);
    b[0] = n;
    return b;
}
static int getSeqNumAndShift(unsigned char* packet){//returns the sequence number removes the sequence number from the front of the packet
    int seqNum = 0;//sequence numbers start at 1
    seqNum = bytesToInt(packet);
    /*now shift the memory down*/ 
    memmove(packet, packet+4, 252);
    return seqNum;
}

static char* fileName;
static int port;
static int mode;
static long modeParameter;
struct sockaddr_in receiverAddr, slidingAddr;
int sockfd,slidingSocket;
int rc;
static long timeout;
int wasInit = -1;
int slidingLen = sizeof(slidingAddr);

static void init(){//acts as a main funciton to initialize and bind socket 
WSADATA wsa; //this is for windows socket API womp womp 
if (WSAStartup(MAKEWORD(2,2),&wsa) != 0){
printf("WSA STARTUP FAILED");
}
sockfd = socket(AF_INET, SOCK_DGRAM, 0);
if(sockfd < 0 ){
    printf("socket creation error");
}
receiverAddr.sin_family = AF_INET;
receiverAddr.sin_port = htons(8080);
receiverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
rc = bind(sockfd, (const struct sockaddr*)&receiverAddr, sizeof(receiverAddr));
if(rc < 0){
    printf( "bind failed: %d", WSAGetLastError());
}
wasInit = 0;
}


char* getFilename(){
    return fileName;
}
int getLocalPort(){
    return port;
}
int getMode(){
    return mode;
}
long getModeParameter(){
    return modeParameter;
}
void setFilename(char* name){
    fileName = name;
}
int setLocalPort(int localPort){
    if(localPort > 0 && localPort <65566){
        port =  localPort;
        return 1;
    }else{
        return -1;
    }
}
int setMode(int newMode){
    if(newMode == 1 || newMode ==0){
        mode = newMode;
        return 1;
    }else{
        return -1;
    }
}
int setModeParameter(long n){
    if(n <= 1024 && n > 0){
        modeParameter = n;
        return 1;
    }else{
        return -1;
    }
}



static int sendACK(int seq){
    unsigned char *bytes = intToBytes(seq);
    int send = sendto(sockfd, (char *)bytes, 4,0,  (SOCKADDR *)&source, slen);
    return send;
}
static int sendACKSliding(int seq){
    unsigned char *bytes = intToBytes(seq);
    int send = sendto(sockfd, (char *)bytes, 4,0,  (SOCKADDR *)&source, slen);
    return send;
}
static int stopAndWait(FILE* file){
    printf("\nTesting stop and wait\n");
    if(wasInit == -1){
    init();
    }
    char *buffer = calloc(256, sizeof(char));
    int read,seq,prev;
    seq = 0;
    read = -1;
    prev = -1;
    while(1){ 
    read = recvfrom(sockfd, (char *)buffer , 256, 0 , (SOCKADDR *)&source , &slen);
    if(read > 0){
    seq = getSeqNumAndShift(buffer);
    fprintf(file,"%.252s",buffer);
    if(seq == prev +1){
        prev++;
        sendACK(seq);
    }else if(seq == -1){
        sendACK(seq);
        free(buffer);
        printf("\nStop and Wait Success!!!\n");
        return 0;
    }else{
        fprintf(stderr,"\nInvalid sequence number\n");
        free(buffer);
        return -1;
    }
    }
    }
    return -1;
}

static int slidingWindow(FILE* file){
    printf("\nTesting sliding window\n");
    char output[10000] = {0};
    if(wasInit == -1){
        init();
    }
    char *buffer = calloc(256, sizeof(char));
    int read,seq,prev;
    seq = 0;
    read = -1;
    prev = -1;
    while(1){ 
    read = recvfrom(sockfd, (char *)buffer , 256, 0 , (SOCKADDR *)&source , &slen);
    if(read > 0){
    seq = getSeqNumAndShift(buffer);
    if(seq == -1){
        printf("\nSliding Window Success !!!");
        sendACKSliding(seq);
        output[prev] = '\0';
        fprintf(file, "%s",output);
        return 0;
    }else{
    prev+=252;
    memmove(output + seq*252, buffer,252);
    sendACKSliding(seq);
    }
    

    }
    }
    return -1;
}



int receiveFile(){
    int a = 0;
    if(mode == 0){//stop and wait
    }else if(mode == 1){//sliding window
    //a = slidingWindow();
    }else{
        printf( "invlaid mode @ receiveFile");
        return -1;
    }
}


int main(){
FILE *fp = fopen("stopAndWait.txt","w");
FILE *fp2 = fopen("slidingWindow.txt", "w");
stopAndWait(fp);
slidingWindow(fp2);
fclose(fp);
fclose(fp2);
close(sockfd);
return 0;
}