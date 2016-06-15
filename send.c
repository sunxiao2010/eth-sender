#define DEVICE_NAME "em1"
#ifndef SEND_INTERVAL
#error "请先在makefile文件中设置SEND_INTERVAL"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_ether.h>

typedef unsigned short UINT16;
typedef short INT16;

#define DEBUG(fmt,args...) 
#define OFFSET -1
#define _P(fmt,args...) printf(fvm,##args)
char   packagebuf[2048];
//char   packagebuf1[2048];
struct sockaddr addr_to;
int count;

int readinfile(char* name,char* start)
{
    unsigned char ch,cl;
    FILE *fp = fopen(name, "r");
    if (!fp)  perror("fail open file");
    count = OFFSET;
    while (ch=(unsigned char)fgetc(fp))
    {
        if(ch>0x7a) goto end;
        cl=(unsigned char)fgetc(fp);
        if(cl==0x7a) goto end;
        if(ch<0x40) 
        {
            ch=ch-'0';
            goto next;
        }
        if(ch>0x39) 
        {
            ch=tolower(ch)-0x61+0xa;
        }
next:
        if(cl<0x40) 
        {
            cl=cl-'0';
            goto next1;
        }
        if(cl>0x39) 
        {
            cl=tolower(cl)-0x61+0xa;
        }    
next1:
        *start=(ch<<4) + cl;
        start ++;
        count ++;
    }
end:
    fclose(fp);
    return 0;
}

typedef struct tagPsuedoHdr  {
    struct in_addr stSrcAddr;
    struct in_addr stDstAddr;
    unsigned char cPlaceHolder;
    unsigned char cProtocol;
    unsigned short length;
} pseudohdr_t;

INT16 InChkSum(UINT16 *addr,INT16 len)
{
    register int sum = 0;
    UINT16 answer = 0;
    register u_short *w = addr;
    register int nleft = len;
    if(NULL == addr)
    {
        DEBUG("InChkSum:NULL\n");
        return -1;
    }
    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }
    /* mop up an odd byte, if necessary */
    if (nleft == 1)
    {
        *(u_char *)(&answer) = *(u_char *)w ;
        sum += answer;
    }
    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = ~sum;                          /* truncate to 16 bits */
    return(answer);
}

int main()
{
    int g_Socket;
    int file;
    int ret;
    short int on = 1;
    pseudohdr_t psdhdr;
    char pspacket[2048];
    if( (g_Socket = socket(AF_INET, SOCK_PACKET, ETH_P_IP)) < 0 )
    {
        return -1;
    }
    memset(&addr_to, 0, sizeof addr_to);
    strcpy(addr_to.sa_data,DEVICE_NAME);
    addr_to.sa_family = AF_PACKET;
    readinfile("packet",packagebuf);
    //readinfile("packet1",packagebuf1);

    memcpy(&psdhdr.stSrcAddr,packagebuf+0x1a,4);
    memcpy(&psdhdr.stDstAddr,packagebuf+0x1e,4);
    psdhdr.cPlaceHolder = 0;
    psdhdr.cProtocol = packagebuf[0x17];
    psdhdr.length =htons(count - 14 - 20);

    memset (pspacket,0,2048);
    *(UINT16*)(packagebuf + 0x18) = 0;
    *(UINT16*)(packagebuf + 0x32) = 0;
    memcpy(pspacket,&psdhdr,sizeof(psdhdr));
    memcpy(pspacket + sizeof(psdhdr), packagebuf+14+20 ,count-14-20);

    *(UINT16*)(packagebuf + 0x18) = (UINT16)InChkSum((UINT16*)packagebuf+7,20 );
    if(psdhdr.cProtocol==0x6)
    {
        *(UINT16*)(packagebuf + 0x32) = (UINT16)InChkSum((UINT16*)pspacket,(count-14-20 + sizeof(psdhdr)));
    }
    while (1)
    {
       ret = sendto(g_Socket, packagebuf, count, 0, &addr_to, sizeof addr_to);
       if(ret<0) perror("send");
       usleep(SEND_INTERVAL);
    }
}
