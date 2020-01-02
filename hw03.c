#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define MATCH_MAX 40960
char match_string[MATCH_MAX][100];  // Calculate how much match ip (string form)
int match_cnt[MATCH_MAX]; // Calculate how much match ip count corresponding to 'matching_string'
int idx = 0; // Record total kinds of 'match_string'.

void got_packet(u_char *param, const struct pcap_pkthdr *header, const u_char *data);

int main(int argc, char *argv[]) {
    char *device;
    char error_buffer[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    int timeout_limit = 10;

    if (argc >2){
        if (strcmp(argv[1],"-r")==0){
            handle = pcap_open_offline(argv[2], error_buffer); // open a pcap file.
        }
    }
    else {
        device = pcap_lookupdev(error_buffer);
        handle = pcap_open_live(device,BUFSIZ,0,timeout_limit,error_buffer);
        if (!device)
        {
            printf("Couldn't find default device: %s\n", error_buffer);
            exit(1);
        }
        if (!handle)
        {
            printf("Couldn't open device: %s\n", error_buffer);
            exit(1);
        }
    }
    printf("type\t\t time\t\t\t address\t\t\t length\n");
    memset(match_string, 0, sizeof(match_string));
    memset(match_cnt, 0, sizeof(match_cnt));
    pcap_loop(handle,0, got_packet, NULL);

    for(int i=0; i<idx; i++){
        printf("IP_address %s\ntimes: %d\n",match_string[i], match_cnt[i]);
    }

    return 0;
}


/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518
/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14
/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6

/* Ethernet header */
struct sniff_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
        #define IP_RF 0x8000            /* reserved fragment flag */
        #define IP_DF 0x4000            /* dont fragment flag */
        #define IP_MF 0x2000            /* more fragments flag */
        #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
    	u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct sniff_tcp {
        u_short th_sport;               /* source port */
        u_short th_dport;               /* destination port */
        tcp_seq th_seq;                 /* sequence number */
        tcp_seq th_ack;                 /* acknowledgement number */
        u_char  th_offx2;               /* data offset, rsvd */
#define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
        u_char  th_flags;
        #define TH_FIN  0x01
        #define TH_SYN  0x02
        #define TH_RST  0x04
        #define TH_PUSH 0x08
        #define TH_ACK  0x10
        #define TH_URG  0x20
        #define TH_ECE  0x40
        #define TH_CWR  0x80
        #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
        u_short th_win;                 /* window */
        u_short th_sum;                 /* checksum */
        u_short th_urp;                 /* urgent pointer */
};

void got_packet(u_char* param, const struct pcap_pkthdr* header, const u_char* data)
{
    const struct sniff_ethernet *ethernet;  /* The ethernet header [1] */
	const struct sniff_ip *ip;              /* The IP header */
	const struct sniff_tcp *tcp;            /* The TCP header */

    //  IP
	ip = (struct sniff_ip*)(data + SIZE_ETHERNET);
    // TCP	
    int size_ip = IP_HL(ip)*4;
    tcp = (struct sniff_tcp*)(data + SIZE_ETHERNET + size_ip);

	switch(ip->ip_p) {
		case IPPROTO_TCP:
			printf("TCP\t");
			break;
		case IPPROTO_UDP:
			printf("UDP\t");
			break;
		case IPPROTO_ICMP:
			printf("ICMP\t");
			break;
		case IPPROTO_IP:
			printf("IP\t");
			break;
		case 89:
			printf("OSPF\t");
			break;
        	case 91:
			printf("LARP\t");
			break;
        	case 54:
			printf("NARP\t");
			break;
		default:
			printf("IP\t");
			break;
		
	}

    //  print time
    struct tm* ltime;
    char timestr[30];
    ltime = localtime(&header->ts.tv_sec);
    strftime(timestr, sizeof(timestr), "%Y/%m/%d %H:%M:%S", ltime);
    printf("[%s.%.6d]  ", timestr, (int)header->ts.tv_usec);


    printf("%s.%d ----> ", inet_ntoa(ip->ip_src),ntohs(tcp->th_sport));
	printf("%s.%d\t", inet_ntoa(ip->ip_dst),ntohs(tcp->th_dport));
	printf ("[length: %d]\n" ,header->len);
    	
    ///// Calculate how much match ip count corresponding to 'matching_string' /////
    
    if(idx == MATCH_MAX){
        printf("Out of buffer range!\n");
        return ;
    }
    char tmp[100];
    int flag = 0;
    memset(tmp, 0, 100);
    
    sprintf(tmp, "%s -> ", inet_ntoa(ip->ip_src));
    strcat(tmp,inet_ntoa(ip->ip_dst));
    
    for (int i = 0; i < idx; i++)
    {
        if(strcmp(match_string[i], tmp) == 0){
            match_cnt[i]++;
            flag = 1;
            break;
        }
    }
    if (flag == 0)
    {
        strcpy(match_string[idx], tmp);
        match_cnt[idx]++;
        idx += 1;
    }

}

