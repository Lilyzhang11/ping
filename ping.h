#ifndef PING_H
#define PING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <string>
#include <vector>
#define PACKET_SIZE     4096//发送，接受内容区域的大小
#define MAX_WAIT_TIME   5  
#define MAX_NO_PACKETS  3
struct IcmpEchoReply
{
	int icmpSeq;
	int icmpLen;
	int ipTtl; //ip报文被转发的次数
	double  rtt; //发送到接收，中间经过的时间
	std:: string fromAddr;
	bool isReply;

};

struct PingResult
{
   int dataLen;
   int nreceived;
   int nsend;
   std::string ip;
   std::string error;
   std::vector<IcmpEchoReply>icmpEchoReplys;
};
class Ping
{
public: 
	Ping();
	bool ping(char*hostOrIp,int count,PingResult& pingResult);
	bool ping(char*hostOrIp,PingResult& pingResult);
private:
	unsigned short getChksum(int len, unsigned short*addr);
	int packIcmp(struct icmp*icmp,int pack_seq);
	bool unpackIcmp(char*buff,int len,IcmpEchoReply& icmpEchoReply);
	struct timeval tvsub(struct timeval timeval1,struct timeval timeval2);
	bool sendPacket();
	bool recvPacket(PingResult& pingResult);
	bool getsockaddr(struct sockaddr_in* m_dest_addr,const char*hostOrIp);

private:
	char m_sendpacket[PACKET_SIZE];
    char m_recvpacket[PACKET_SIZE];
    int m_maxPacketSize;
    int m_sockfd;
    int m_nsend;
    int m_nreceived;
    int m_datalen;
    int m_icmp_seq;
    struct sockaddr_in m_dest_addr;
    struct sockaddr_in m_from_addr;
    pid_t m_pid;



};
#endif