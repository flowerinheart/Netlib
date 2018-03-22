#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#define SIZE_LINE_NORMAL 2048

unsigned short api_checksum16(unsigned short *buffer, int size)  
{  
	unsigned int cksum = 0;  
  
	if ( !buffer ) {   
		perror("NULL\n");  
		return 0;  
	}	 
  
	while ( size > 1 ) {   
		//printf("1. Cksum: 0x%08x + 0x%04x\n", cksum, *buffer);  
		cksum += *buffer++;  
		size -= sizeof(unsigned short);  
	}	 
  
	if ( size ) {   
		cksum += *(unsigned char *)buffer;  
	}	 
  
	//printf("2. Cksum: 0x%08x\n", cksum);  
  
	/* 32 bit change to 16 bit */  
	while ( cksum >> 16 ) {   
		cksum = (cksum >> 16) + (cksum & 0xFFFF);  
		//printf("3. Cksum: 0x%08x\n", cksum);  
	}	 
  
	return (unsigned short)(~cksum);  
}  

void gen_icmp_packet(struct icmp *icmp_packet, int type, int seq)  
{  
	icmp_packet->icmp_type  = type;  
	icmp_packet->icmp_code  = 0;  
	icmp_packet->icmp_cksum = 0;  
	icmp_packet->icmp_id  = htons(getpid());  
	icmp_packet->icmp_seq = htons(seq);  
  
	gettimeofday((struct timeval *)icmp_packet->icmp_data, NULL);  
	icmp_packet->icmp_cksum = api_checksum16((unsigned short *)icmp_packet, sizeof(struct icmp));  
}  

void ping(char *dst_ip)  
{  
	int ret = 0;  
	int sd = 0;  
  
	char buf[SIZE_LINE_NORMAL] = {0};  
	  
	struct ip *ip = NULL;  
	struct sockaddr_in dst_addr = {0};  
	struct icmp icmp_packet = {0};  
	  
	struct timeval tm = {.tv_sec = 1, .tv_usec = 0};  
	struct timeval begin,end;
	  
	fd_set rdfds;  
  
	if ( !dst_ip ) {  
		perror("NULL\n");  
		return;
	}  
  
	sd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);  
	if ( sd < 0 ) {  
		perror("socket\n");  
		return;  
	}  
  
	dst_addr.sin_family = AF_INET;  
	dst_addr.sin_addr.s_addr = inet_addr(dst_ip);  
	  
	gen_icmp_packet(&icmp_packet, 8, 1);  

	ret = sendto(sd, &icmp_packet, sizeof(struct icmp), 0,  
			(struct sockaddr *)&dst_addr, sizeof(struct sockaddr_in));  
	if ( ret < 0 ) {  
		perror("sendto\n");  
		return; 
	}  
	printf("Send ping sucess!\n");  

	gettimeofday(&begin,NULL);
  
	/* Timeout 1s to recv icmp */  
	FD_ZERO(&rdfds);  
	FD_SET(sd, &rdfds);  
  
	ret = select(sd + 1, &rdfds, NULL, NULL, &tm);  
	if ( -1 == ret && EINTR != errno ) {  
		/* if serial error */  
		perror("select fail\n");  
		return; 
	}  
	else if ( 0 == ret ) {	  
		/* timeout */  
		perror("recv timeout\n"); 
		return;
	}	 
	if ( FD_ISSET(sd, &rdfds) ) {  
		ret = recv(sd, buf, sizeof(buf), 0);  
		if ( ret <= 0 ) {  
			perror("recv\n");  
			return; 
		}
		gettimeofday(&end,NULL);
		unsigned long us = 1000000*(end.tv_sec-begin.tv_sec) + (end.tv_usec-begin.tv_usec);
		ip = (struct ip *)buf;
		printf("from: %s\n", inet_ntoa(ip->ip_src));  
		printf("  to: %s\n", inet_ntoa(ip->ip_dst)); 
		printf(" rtt: %.2lfms\n", us/1000.0);
	}  
  
}  


int main(int argc, char* argv[]){
	if(argc==2)ping(argv[1]);
	return 0;
}


