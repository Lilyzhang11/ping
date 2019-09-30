#include"ping.h"
/*对class 中的私有变量进行初始化，设置报文数据段的长度为56个byte，报文发送和接收为0，报文序列初始化为0*/
Ping::Ping()
{   m_maxPacketSize=4;
	m_datalen=56;
	m_nsend=0;
	m_nreceived=0;
	m_icmp_seq=0;
}
/*对已经填好的icmp报文进行校验，并将校验和返回，返回值填入icmp报头中的校验和*/
unsigned short Ping::getChksum(int len, unsigned short*addr)
{
	int sum =0,i=0;
	unsigned short *w=addr;
    unsigned short answer=0;
	for(;i<len;i++)
	{
       sum+=*w;
       w++;
       i=i+2;
	}
	if(i!=len)
	{
     sum+= *(unsigned char*)w;
	}
	sum = (sum>>16)+(sum&0xffff);
	sum+=(sum>>16);
	answer = ~sum;
	return answer;
	
}
/*填写目的地套接字的内容*/
bool Ping::getsockaddr(struct sockaddr_in* m_dest_addr,const char*hostOrIp)
{
	struct hostent *host;
	//gethostbyname 返回指向hostent的静态指针
	struct sockaddr_in dest_addr;
	unsigned long inaddr=0l;
	bzero(&dest_addr,sizeof(dest_addr));
	dest_addr.sin_family=AF_INET;
	if(inaddr = inet_addr(hostOrIp)==INADDR_NONE)//字符串不是点分十进制
	{ if((host=gethostbyname(hostOrIp))==NULL)
	   {
	   	printf("Your input address %s is false.\n",hostOrIp);
	   	return false;
	   }
	   else
	   {
	   	memcpy((char*)&dest_addr.sin_addr,host->h_addr,host->h_length);
	   }
    }
    else if(!inet_aton(hostOrIp,&dest_addr.sin_addr))
    {
    	return false;
    }
    *m_dest_addr = dest_addr;
    return true;
	
}
/*先把icmp报头的内容填好，然后将报头加入数据段进行校验，校验和填入报头相应成员*/
int Ping::packIcmp(struct icmp*icmp,int pack_seq)
{
      struct icmp* picmp=icmp;
      int packsize;
      struct timeval* tval;
      packsize = 8+m_datalen;// icmp报头长度为8
      picmp->icmp_type = ICMP_ECHO;
      picmp->icmp_code=0;
      picmp->icmp_cksum=0;
      picmp->icmp_seq=pack_seq;
      picmp->icmp_id = m_pid;
      tval = (struct timeval*)picmp->icmp_data;//将发送时间填入icmp报头中
      gettimeofday(tval,NULL);
      picmp->icmp_cksum=getChksum(packsize,(unsigned short*)icmp);
      return packsize; 
      
}

bool Ping::sendPacket()
{
	size_t packetsize;
	for(;m_nsend<m_maxPacketSize;m_nsend++)
	{
		m_icmp_seq++;
		packetsize = packIcmp((struct icmp*)m_sendpacket,m_icmp_seq);
		if(sendto(m_sockfd,m_sendpacket, packetsize, 0, (struct sockaddr *) &m_dest_addr, sizeof(m_dest_addr)) < 0  )
		{
			perror("sendto error");
			continue;
		}

    }
    return true;
}
//确保是我发的icmp的回应,发过来的是ip报文，需要对其进行解包
bool Ping::unpackIcmp(char*buff,int len,IcmpEchoReply& icmpEchoReply)
{
 int iphdrlen;
 struct ip* ip;
 struct icmp*icmp;
 struct timeval *tvsend,tvrecv,tvresult;
 ip = (struct ip*)buff;
 iphdrlen = ip->ip_hl<<2;
 icmp = (struct icmp*)(buff+iphdrlen);
 len-=iphdrlen;
 if(len<8)
 {
 	printf("ICMP packets\'s length is less than 8\n");
		return false;
	
 }
 if((icmp->icmp_type==ICMP_ECHOREPLY)&&(icmp->icmp_id==m_pid))
 {
    tvsend=(struct timeval*)icmp->icmp_data;
    gettimeofday(&tvrecv,NULL);
    tvresult=tvsub(tvrecv,*tvsend);
    double rtt = tvresult.tv_sec*1000+tvresult.tv_usec/1000;
    icmpEchoReply.rtt=rtt;
    icmpEchoReply.icmpSeq=icmp->icmp_seq;
    icmpEchoReply.ipTtl=ip->ip_ttl;
    icmpEchoReply.icmpLen = len;
    return true;
 }
 	else
 	{
 		return false;
 	}



}
//确保是从目的IP发送过来的响应
bool Ping::recvPacket(PingResult&pingResult)
{   
   struct IcmpEchoReply icmpEchoReply;
   int nfd;
   int len;
   extern int errno;
   fd_set readyset;
   FD_ZERO(&readyset);
   socklen_t from_len = sizeof(m_from_addr);
   struct timeval timeout;
   timeout.tv_sec=4;
   timeout.tv_usec=0;
   for(int recvCount=0;recvCount<m_maxPacketSize;recvCount++)
   { 
   	FD_SET(m_sockfd,&readyset);
   	if((nfd=select(m_sockfd+1,&readyset,NULL,NULL,&timeout))==-1)
   	 {

      perror("select error");
      continue;   

     }
     if(nfd==0)//timeout
     {
     	icmpEchoReply.isReply=false;
     	pingResult.icmpEchoReplys.push_back(icmpEchoReply);
     	continue;
     }
     if(FD_ISSET(m_sockfd,&readyset))
     {
     	if((len=recvfrom(m_sockfd,m_recvpacket,sizeof(m_recvpacket),0,(struct sockaddr*)&m_from_addr,&from_len))<0)
     	{
     		if(errno==EINTR)
     		{
     			continue;
     		}
     		perror("recvfrom error");
     		continue;
     	}
     	icmpEchoReply.fromAddr=inet_ntoa(m_from_addr.sin_addr);//numtostring
        if(icmpEchoReply.fromAddr!=pingResult.ip)
        {
        	recvCount--;
        	continue;
        }
    }
        if(!unpackIcmp(m_recvpacket,len,icmpEchoReply))
        {
        	recvCount--;
        	continue;
        }
        icmpEchoReply.isReply=true;
        pingResult.icmpEchoReplys.push_back(icmpEchoReply);
        m_nreceived++;
   	
   }

  return true;
}
//用当前的时间减去发送的时间
struct timeval Ping::tvsub(struct timeval timeval1,struct timeval timeval2)
{
   struct timeval result=timeval1;
   if(result.tv_usec<timeval2.tv_usec)
   {
       result.tv_sec--;
       result.tv_usec+=1000000;

   }
result.tv_sec = result.tv_sec-timeval2.tv_sec;
result.tv_usec = result.tv_usec-timeval2.tv_usec;
return result;
}

bool Ping::ping(char*hostOrIp,PingResult& pingResult)
{
	return ping(hostOrIp,1,pingResult);
}

bool Ping::ping(char*hostOrIp,int count,PingResult& pingResult)
{
   struct protoent* protocol;
   int size = 50*1024;
   pingResult.icmpEchoReplys.clear();
   pingResult.dataLen=m_datalen;
   m_nreceived=0;
   m_nsend=0;
   m_maxPacketSize= count;
   m_pid =getpid();
   if((protocol=getprotobyname("icmp"))==NULL)
   {
   	perror("getprotobyname");
   	return false;
   }
   //需要在root权限下才能生成额套接字
   if((m_sockfd=socket(AF_INET,SOCK_RAW,protocol->p_proto) )<0)
   {
   	extern int errno;
   	pingResult.error = strerror(errno);
   	return false;

   }
   setsockopt(m_sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));//扩大套接字缓冲地址
   if(!getsockaddr(&m_dest_addr,hostOrIp))//建立目的地套接字地址失败
   {
   	std::string s = hostOrIp;
   	pingResult.error="unhostname "+s; 
   	return false;
   }
   pingResult.ip = inet_ntoa(m_dest_addr.sin_addr);
   sendPacket();
   recvPacket(pingResult);
   pingResult.nsend=m_nsend;
   pingResult.nreceived=m_nreceived;
   close(m_sockfd);
   return true;
}