#include"ping.h"
#define STDIN 0
void showResult(PingResult& pingResult)
{
	for(int i=0;i<pingResult.icmpEchoReplys.size();i++)
	{
		IcmpEchoReply icmpEchoReply = pingResult.icmpEchoReplys.at(i);
		if(icmpEchoReply.isReply)
		{
			printf("%d bytes from %s : icmp_seq=%d ttl=%d time = %.3f ms\n",
				icmpEchoReply.icmpLen,
				icmpEchoReply.fromAddr.c_str(),
				icmpEchoReply.icmpSeq,
				icmpEchoReply.ipTtl,//经过多少个转发点
				icmpEchoReply.rtt);//接受时间-发送时间
		}
		else
		{
			printf("request timeout\n");
		}
	}

}
int main(int argc,char*argv[])
{   
	if(argc!=2)
	{
		printf("Please input the order correctly!");
		return 0;
	}
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN,&fds);
	PingResult pingResult;
	char* hostOrIp = argv[1];
	Ping ping = Ping();
	int nsend=0,nreceived=0;
	bool ret;
	int i=1;
	struct timeval tv;
	tv.tv_sec = 0;
    tv.tv_usec = 1000*1;
    //键入’q‘+enter 终止循环
	while(1)
	{   
		int nfd = select(STDIN+1,&fds,NULL,NULL,&tv);
		if(nfd<0)
		{   
			printf("select error!\n");
			return 0;
			//检测到有键入
		}
		if(FD_ISSET(STDIN,&fds))
		{
			char buf[100];
			read(0,buf,sizeof(buf)-1);
			char c=buf[0];
			if(c=='q')
			{
				break;
			}
		}

   
		ret = ping.ping(hostOrIp,1,pingResult);
		if(i==1)
		{
			printf("PING %s (%s) %d bytes of data\n",hostOrIp,pingResult.ip.c_str(),pingResult.dataLen);

		}
		if(!ret)
		{
			printf("%s\n",pingResult.error.c_str());
			break;
		}
		showResult(pingResult);
		nsend += pingResult.nsend;
		nreceived += pingResult.nreceived;
        i++;
        FD_ZERO(&fds);
        FD_SET(0,&fds);
        
	}
	if(ret)//打印最后的接受包的比例
	{
		printf("%d packets transmitted %d packets recieved %%%d packets lost\n",nsend,nreceived,
			(nsend-nreceived)/nsend*100);

	}
	return 0;
}
