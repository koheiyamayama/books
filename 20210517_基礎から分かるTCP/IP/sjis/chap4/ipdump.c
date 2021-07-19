/**********************************************************************
 *  TCP/IP�p�P�b�g���j�^�����O�v���O���� (ipdump.c)
 *        Ver 2.3 2019�N 5�� 1��
 *                                ����E���� ���R���� (Yukio Murayama)
 *
 *  �g�p������
 *    �{�v���O�����́ATCP/IP�v���g�R���̊w�K�A�y�сA�l�b�g���[�N�v��
 *    �O���~���O�̋Z�\�����コ���邽�߂ɂ̂݁A���̂܂܁A�܂��́A�C��
 *    ���Ďg�p���邱�Ƃ��ł��܂��B�{�v���O�����ɂ��āA�@���ŋ֎~��
 *    ��Ă��邩�A�܂��́A�����Ǒ��ɔ�����悤�ȉ����A�y�сA�g�p����
 *    �~���܂��B�{�v���O�����͖��ۏ؂ł��B����҂͖{�v���O�����ɂ��
 *    �Ĕ������������Ȃ鑹�Q�ɂ��Ă��ӔC����邱�Ƃ͂ł��܂���B
 **********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#ifndef __linux
#include <sys/ioctl.h>
#include <net/bpf.h>
#include <net/if.h>
#include <fcntl.h>
#endif
#include <netinet/if_ether.h>

#define MAXSIZE 8192
#define CMAX    256
#define OPTNUM  9
#define ON      1
#define OFF     0
#define DEF_IF  "en0"   /* Mac OS X�̃f�t�H���g�C���^�t�F�[�X�� */

enum {ETHER, ARP, IP, TCP, UDP, ICMP, DUMP, ALL, TIME};
enum {IP_ADDR, PORT};

/* �p�P�b�g�t�B���^�p�}�N�� */
#define FILTER_ARP(_p) (filter.flg[IP_ADDR] ?\
                       ((*(int *) (_p)->arp_spa == filter.ip.s_addr\
                         || *(int *) (_p)->arp_tpa == filter.ip.s_addr) ?\
                        1 : 0)\
                       : 1)
#define FILTER_IP(_p) (filter.flg[IP_ADDR] ?\
                       (((_p)->ip_src.s_addr == filter.ip.s_addr\
                         || (_p)->ip_dst.s_addr == filter.ip.s_addr) ?\
                        1 : 0)\
                       : 1)
#define FILTER_TCP(_p) (filter.flg[PORT] ?\
                        (((_p)->th_sport == filter.port\
                          || (_p)->th_dport == filter.port) ?\
                         1 : 0)\
                        : 1)
#define FILTER_UDP(_p) (filter.flg[PORT] ?\
                        (((_p)->uh_sport == filter.port\
                          || (_p)->uh_dport == filter.port) ?\
                         1 : 0)\
                        : 1)

struct filter {
  struct in_addr ip;  /* IP�A�h���X     */
  u_int16_t port;     /* �|�[�g�ԍ�     */
  int flg[2];         /* �t�B���^�t���O */
};

#ifndef __linux
int open_bpf(char *ifname, int *bufsize);
#endif

void print_ethernet(struct ether_header *eth);
void print_arp(struct ether_arp *arp);
void print_ip(struct ip *ip);
void print_icmp(struct icmp *icmp);
void print_tcp(struct tcphdr *tcp);
void print_tcp_mini(struct tcphdr *tcp);
void print_udp(struct udphdr *udp);
void dump_packet(unsigned char *buff, int len);
char *mac_ntoa(u_char *d);
char *tcp_ftoa(int flag);
char *ip_ttoa(int flag);
char *ip_ftoa(int flag);
void help(char *cmd);

int main(int argc, char **argv)
{
  int s;                   /* �\�P�b�g�f�B�X�N���v�^    */
  int c;                   /* getopt()�Ŏ擾��������    */
  char ifname[CMAX] = "";  /* �C���^�t�F�[�X��          */
  int opt[OPTNUM];         /* �\���I�v�V�����̃t���O    */
  extern int optind;       /* getopt()�̃O���[�o���ϐ�  */
#ifndef __linux
  struct bpf_hdr *bp;      /* BPF�w�b�_�\����           */
  int bpf_len;             /* BPF�ł̎�M�f�[�^�̒���   */
  int bufsize;             /* BPF�����̃o�b�t�@�T�C�Y   */
#endif 
  struct filter filter;    /* �t�B���^������          */

  /* �\������p�P�b�g(�I�v�V����)�̏����l */
  opt[ETHER] = OFF;
  opt[ARP]   = ON;
  opt[IP]    = ON;
  opt[TCP]   = ON;
  opt[UDP]   = ON;
  opt[ICMP]  = ON;
  opt[DUMP]  = OFF;
  opt[ALL]   = OFF;
  opt[TIME]  = OFF;
  /* �t�B���^�̏����l */
  filter.flg[IP_ADDR] = OFF;
  filter.flg[PORT]    = OFF;

  /* �R�}���h���C���I�v�V�����̌��� */
  while ((c = getopt(argc, argv, "aei:p:f:dhft")) != EOF) {
    switch (c) {
      case 'a':  /* all */
        opt[ALL]  = ON;
        break;
      case 'e':  /* ethernet */
        opt[ETHER]= ON;
        break;
      case 'd':  /* dump */
        opt[DUMP] = ON;
        break;
      case 't':  /* time */
        opt[TIME] = ON;
        break;
      case 'i':  /* if name */
        snprintf(ifname, sizeof ifname, "%s", optarg);
        break;
      case 'p':  /* protocol */
        opt[ARP]  = OFF;
        opt[IP]   = OFF;
        opt[TCP]  = OFF;
        opt[UDP]  = OFF;
        opt[ICMP] = OFF;
        optind--;
        while (argv[optind] != NULL && argv[optind][0] != '-') {
          if (strcmp(argv[optind], "arp") == 0)
            opt[ARP]  = ON;
          else if (strcmp(argv[optind], "ip") == 0)
            opt[IP]   = ON;
          else if (strcmp(argv[optind], "tcp") == 0)
            opt[TCP]  = ON;
          else if (strcmp(argv[optind], "udp") == 0)
            opt[UDP]  = ON;
          else if (strcmp(argv[optind], "icmp") == 0)
            opt[ICMP] = ON;
          else if (strcmp(argv[optind], "other") == 0)
            ;
          else {
            help(argv[0]);
            exit(EXIT_FAILURE);
          }
          optind++;
        }
        break;
      case 'f':  /* filter */
        optind--;
        while (argv[optind] != NULL && argv[optind][0] != '-') {
          if (strcmp(argv[optind], "ip") == 0 && argv[optind+1] != NULL) {
            filter.flg[IP_ADDR] = ON;
            filter.ip.s_addr = inet_addr(argv[++optind]);
          } else if (strcmp(argv[optind], "port")==0 && argv[optind+1]!=NULL) {
            filter.flg[PORT] = ON;
            filter.port = htons(atoi(argv[++optind]));
          } else {
            help(argv[0]);
            exit(EXIT_FAILURE);
          }
          optind++;
        }
        break;
      case 'h':  /* help */
      case '?':
      default:
        help(argv[0]);
        exit(EXIT_FAILURE);
        break;
    }
  }

  if (optind < argc) {
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
    help(argv[0]);
    exit(EXIT_FAILURE);
  }

  if (filter.flg[IP_ADDR])
    printf("filter ip   = %s\n", inet_ntoa(filter.ip));
  if (filter.flg[PORT])
    printf("filter port = %d\n", htons(filter.port));

#ifdef __linux
  if ((s = socket(AF_INET, SOCK_PACKET, htons(ETH_P_ALL))) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (strcmp(ifname, "") != 0) {
    struct sockaddr sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_family = AF_INET;
    snprintf(sa.sa_data, sizeof sa.sa_data, "%s", ifname);
    if (bind(s, &sa, sizeof sa) < 0) {
      perror("bind");
      exit(EXIT_FAILURE);
    }
  }
#else
  if (strcmp(ifname, "") == 0)
    strcpy(ifname, DEF_IF);

  if ((s = open_bpf(ifname, &bufsize)) < 0)
    exit(EXIT_FAILURE);
  bpf_len = 0;
#endif

  while (1) {
    struct ether_header *eth;  /* Ethernet�w�b�_�\����              */
    struct ether_arp *arp;     /* ARP�p�P�b�g�\����                 */
    struct ip *ip;             /* IP�w�b�_�\����                    */
    struct icmp *icmp;         /* ICMP�p�P�b�g�\����                */
    struct tcphdr *tcp;        /* TCP�w�b�_�\����                   */
    struct udphdr *udp;        /* UDP�w�b�_�\����                   */
    char buff[MAXSIZE];        /* �f�[�^��M�o�b�t�@                */
    void *p;                   /* �w�b�_�̐擪��\����Ɨp�|�C���^  */
    void *p0;                  /* �p�P�b�g�̐擪��\���|�C���^      */
    int len;                   /* ��M�����f�[�^�̒���              */
    int disp;                  /* ��ʂɏo�͂������ǂ����̃t���O    */
    struct timeval tv;         /* �p�P�b�g���_���v��������          */
    struct tm tm;              /* localtime�ł̎����\��             */

#ifndef __linux
    /*  BPF����̓��� */
    if (bpf_len <= 0) {
      /* �����̃p�P�b�g���ꊇ���o�� */
      if ((bpf_len = read(s, buff, bufsize)) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
      }
      bp = (struct bpf_hdr *) buff;
    } else {
      /* BPF�̎��̃p�P�b�g�փ|�C���^���ړ� */
      bp = (struct bpf_hdr *) ((void *) bp
           + BPF_WORDALIGN(bp->bh_hdrlen + bp->bh_caplen));
    }
    /* �p�P�b�g�_���v�̎������Z�b�g */
    memcpy(&tv, &(bp->bh_tstamp), sizeof tv);
    localtime_r((time_t *) &tv.tv_sec, &tm);
    /* Ethernet�w�b�_�̐擪�Ƀ|�C���^���Z�b�g */
    p = p0 = (char *) bp + bp->bh_hdrlen;
    len = bp->bh_caplen;
#ifdef DEBUG
    /* BPF�w�b�_�\���̒l��\�� */
    printf("bpf_len=%d,",  bpf_len);
    printf("hdrlen=%d,",   bp->bh_hdrlen);
    printf("caplen=%d,",   bp->bh_caplen);
    printf("datalen=%d\n", bp->bh_datalen);
#endif
    /* ����while���[�v�̂��߂̏��� */
    bpf_len -= BPF_WORDALIGN(bp->bh_hdrlen + bp->bh_caplen);
#else
    /* Linux SOCK_PACKET����̓��� */
    if ((len = read(s, buff, MAXSIZE)) < 0) {
      perror("read");
      exit(EXIT_FAILURE);
    }
    /* �p�P�b�g�_���v�̎������Z�b�g */
    gettimeofday(&tv, (struct timezone *) 0);
    localtime_r((time_t *) &tv.tv_sec, &tm);
    /* Ethernet�w�b�_�̐擪�Ƀ|�C���^���Z�b�g */
    p = p0 = buff;
#endif

    /*
     * �p�P�b�g��̓��[�`��
     */
    disp = OFF;  /* ��ʂɏo�͂��邩�ǂ����t���O */

    /* Ethernet�w�b�_�\���̂̐ݒ� */
    eth = (struct ether_header *) p;
    p += sizeof (struct ether_header);

    if (ntohs(eth->ether_type) == ETHERTYPE_ARP) {
      arp = (struct ether_arp *) p;
      if (opt[ARP] == ON && FILTER_ARP(arp))
        disp = ON;
    } else if (ntohs(eth->ether_type) == ETHERTYPE_IP) {
      ip = (struct ip *) p;
      p += ((int) (ip->ip_hl) << 2);

      if (!FILTER_IP(ip))
        continue;
      if (opt[IP] == ON && opt[TCP] != ON && opt[UDP] != ON && opt[ICMP] != ON)
        disp = ON;

      switch (ip->ip_p) {
        case IPPROTO_TCP:
          tcp = (struct tcphdr *) p;
          p += ((int) (tcp->th_off) << 2);
          if (!FILTER_TCP(tcp))
            continue;
          if (opt[TCP] == ON)
            disp = ON;
          break;
        case IPPROTO_UDP:
          udp = (struct udphdr *) p;
          p += sizeof (struct udphdr);
          if (!FILTER_UDP(udp))
            continue;
          if (opt[UDP] == ON)
            disp = ON;
          break;
        case IPPROTO_ICMP:
          icmp = (struct icmp *) p;
          p = icmp->icmp_data;
          if (opt[ICMP] == ON)
            disp = ON;
          break;
        default:
          if (opt[ALL] == ON)
            disp = ON;
          break;
      }
    } else if (opt[ETHER] == ON && opt[ALL] == ON)
      disp = ON;

    /*
     * �p�P�b�g�\�����[�`��
     */
    if (disp == ON || opt[ALL] == ON) {
      if (opt[TIME] == ON)
        printf("Time: %02d:%02d:%02d.%06d\n", tm.tm_hour, tm.tm_min, tm.tm_sec,
               (int) tv.tv_usec);
      if (opt[ETHER] == ON)
        print_ethernet(eth);
      if (ntohs(eth->ether_type) == ETHERTYPE_ARP) {
        if (opt[ARP] == ON)
          print_arp(arp);
      } else if (ntohs(eth->ether_type) == ETHERTYPE_IP) {
        if (opt[IP] == ON)
          print_ip(ip);
        if (ip->ip_p == IPPROTO_TCP && opt[TCP] == ON)
          print_tcp(tcp);
        else if (ip->ip_p == IPPROTO_UDP && opt[UDP] == ON)
          print_udp(udp);
        else if (ip->ip_p == IPPROTO_ICMP && opt[ICMP] == ON)
          print_icmp(icmp);
        else if (opt[ALL] == ON)
          printf("Protocol: unknown\n");
      } else if (opt[ALL] == ON)
        printf("Protocol: unknown\n");
      if (opt[DUMP] == ON)
        dump_packet(p0, len);
      printf("\n");
    }
  }
  return 0;
}

/*
 * char *mac_ntoa(u_char *d);
 * �@�\
 *    �z��Ɋi�[����Ă���MAC�A�h���X�𕶎���ɕϊ�
 *    static�ϐ��𗘗p���邽�߁A�񃊃G���g�����g�֐�
 * ������
 *    u_char *d;  MAC�A�h���X���i�[����Ă���̈�̐擪�A�h���X
 * �߂�l
 *    ������ɕϊ����ꂽMAC�A�h���X
 */
char *mac_ntoa(u_char *d)
{
#define MAX_MACSTR 50
  static char str[MAX_MACSTR];  /* ������ɕϊ�����MAC�A�h���X��ۑ� */

  snprintf(str, MAX_MACSTR, "%02x:%02x:%02x:%02x:%02x:%02x",
           d[0], d[1], d[2], d[3], d[4], d[5]);

  return str;
}

/*
 * void print_ethernet(struct ether_header *eth);
 * �@�\
 *     Ethernet�w�b�_�̕\��
 * ������
 *     struct ether_header *eth;  Ethernet�w�b�_�\���̂ւ̃|�C���^
 * �߂�l
 *     �Ȃ�
 */
void print_ethernet(struct ether_header *eth)
{
  int type = ntohs(eth->ether_type);  /* Ethernet�^�C�v */

  if (type <= 1500)
    printf("IEEE 802.3 Ethernet Frame:\n");
  else
    printf("Ethernet Frame:\n");

  printf("+-------------------------+-------------------------"
         "+-------------------------+\n");
  printf("| Destination MAC Address:                          "
         "         %17s|\n", mac_ntoa(eth->ether_dhost));
  printf("+-------------------------+-------------------------"
         "+-------------------------+\n");
  printf("| Source MAC Address:                               "
         "         %17s|\n", mac_ntoa(eth->ether_shost));
  printf("+-------------------------+-------------------------"
         "+-------------------------+\n");
  if (type < 1500)
    printf("| Length:            %5u|\n", type);
  else 
    printf("| Ethernet Type:    0x%04x|\n", type);
  printf("+-------------------------+\n");
}

/*
 * void print_arp(struct ether_arp *arp);
 * �@�\
 *     ARP�p�P�b�g�̕\��
 * ������
 *     struct ether_arp *arp;  ARP�p�P�b�g�\���̂ւ̃|�C���^
 * �߂�l
 *     �Ȃ�
 */
void print_arp(struct ether_arp *arp)
{
  static char *arp_op_name[] = {
    "Undefine",
    "(ARP Request)",
    "(ARP Reply)",
    "(RARP Request)",
    "(RARP Reply)"
  };   /* �I�y���[�V�����̎�ނ�\�������� */
#define ARP_OP_MAX (sizeof arp_op_name / sizeof arp_op_name[0])
  int op = ntohs(arp->ea_hdr.ar_op);  /* ARP�I�y���[�V���� */

  if (op < 0 || ARP_OP_MAX < op)
    op = 0;

  printf("Protocol: ARP\n");
  printf("+-------------------------+-------------------------+\n");
  printf("| Hard Type: %2u%-11s| Protocol:0x%04x%-9s|\n",
         ntohs(arp->ea_hdr.ar_hrd),
         (ntohs(arp->ea_hdr.ar_hrd)==ARPHRD_ETHER)?"(Ethernet)":"(Not Ether)",
         ntohs(arp->ea_hdr.ar_pro),
         (ntohs(arp->ea_hdr.ar_pro)==ETHERTYPE_IP)?"(IP)":"(Not IP)");
  printf("+------------+------------+-------------------------+\n");
  printf("| HardLen:%3u| Addr Len:%2u| OP: %4d%16s|\n",
         arp->ea_hdr.ar_hln, arp->ea_hdr.ar_pln, ntohs(arp->ea_hdr.ar_op),
         arp_op_name[op]);
  printf("+------------+------------+-------------------------"
         "+-------------------------+\n");
  printf("| Source MAC Address:                               "
         "         %17s|\n", mac_ntoa(arp->arp_sha));
  printf("+---------------------------------------------------"
         "+-------------------------+\n");
  printf("| Source IP Address:                 %15s|\n",
         inet_ntoa(*(struct in_addr *) &arp->arp_spa));
  printf("+---------------------------------------------------"
         "+-------------------------+\n");
  printf("| Destination MAC Address:                          "
         "         %17s|\n", mac_ntoa(arp->arp_tha));
  printf("+---------------------------------------------------"
         "+-------------------------+\n");
  printf("| Destination IP Address:            %15s|\n",
         inet_ntoa(*(struct in_addr *) &arp->arp_tpa));
  printf("+---------------------------------------------------+\n");
}

/*
 * void print_ip(struct ip *ip);
 * �@�\
 *     IP�w�b�_�̕\��
 * ������
 *     struct ip *ip;  IP�w�b�_�\���̂ւ̃|�C���^
 * �߂�l
 *     �Ȃ�
 */
void print_ip(struct ip *ip)
{
  printf("Protocol: IP\n");
  printf("+-----+------+------------+-------------------------+\n");
  printf("| IV:%1u| HL:%2u| T: %8s| Total Length: %10u|\n",
         ip->ip_v, ip->ip_hl, ip_ttoa(ip->ip_tos), ntohs(ip->ip_len));
  printf("+-----+------+------------+-------+-----------------+\n");
  printf("| Identifier:        %5u| FF:%3s| FO:        %5u|\n",
         ntohs(ip->ip_id), ip_ftoa(ntohs(ip->ip_off)),
         ntohs(ip->ip_off) &IP_OFFMASK);
  printf("+------------+------------+-------+-----------------+\n");
  printf("| TTL:    %3u| Pro:    %3u| Header Checksum:   %5u|\n",
         ip->ip_ttl, ip->ip_p, ntohs(ip->ip_sum));
  printf("+------------+------------+-------------------------+\n");
  printf("| Source IP Address:                 %15s|\n",
         inet_ntoa(*(struct in_addr *) &(ip->ip_src)));
  printf("+---------------------------------------------------+\n");
  printf("| Destination IP Address:            %15s|\n",
         inet_ntoa(*(struct in_addr *) &(ip->ip_dst)));
  printf("+---------------------------------------------------+\n");
}

/*
 * char *ip_ftoa(int flag);
 * �@�\
 *     IP�w�b�_�̃t���O�����g�r�b�g�𕶎���ɕϊ�
 *     static�ϐ����g�p���Ă��邽�߁A�񃊃G���g�����g�֐�
 * ������
 *     int flag;  �t���O�����g�t�B�[���h�̒l
 * �߂�l
 *     char *     �ϊ����ꂽ������
 */
char *ip_ftoa(int flag)
{
  static int  f[] = {'R', 'D', 'M'};  /* �t���O�����g�t���O��\������ */
#define IP_FLG_MAX (sizeof f / sizeof f[0])
  static char str[IP_FLG_MAX + 1];    /* �߂�l���i�[����o�b�t�@     */
  unsigned int mask = 0x8000;         /* �}�X�N                       */
  int i;                              /* ���[�v��                     */

  for (i = 0; i < IP_FLG_MAX; i++) {
    if (((flag << i) & mask) != 0)
      str[i] = f[i];
    else
      str[i] = '0';
  }
  str[i] = '\0';

  return str;
}

/*
 * char *ip_ttoa(int flag);
 * �@�\
 *     IP�w�b�_��TOS�t�B�[���h�𕶎���ɕϊ�
 *     static�ϐ����g�p���Ă��邽�߁A�񃊃G���g�����g�֐�
 * ������
 *     int flag;  TOS�t�B�[���h�̒l
 * �߂�l
 *     char *     �ϊ����ꂽ������
 */
char *ip_ttoa(int flag)
{
  static int  f[] = {'1', '1', '1', 'D', 'T', 'R', 'C', 'X'}; 
                                 /* TOS�t�B�[���h��\������       */
#define TOS_MAX (sizeof f / sizeof f[0])
  static char str[TOS_MAX + 1];  /* �߂�l���i�[����o�b�t�@      */
  unsigned int mask = 0x80;      /* TOS�t�B�[���h�����o���}�X�N */
  int i;                         /* ���[�v�ϐ�                    */

  for (i = 0; i < TOS_MAX; i++) {
    if (((flag << i) & mask) != 0)
      str[i] = f[i];
    else
      str[i] = '0';
  }
  str[i] = '\0';

  return str;
}

/*
 * void print_icmp(struct icmp *icmp);
 * �@�\
 *     ICMP�w�b�_�E�f�[�^�̕\��
 * ������
 *     struct icmp *icmp;  ICMP�w�b�_�\����
 * �߂�l
 *     �Ȃ�
 */
void print_icmp(struct icmp *icmp)
{
  static char *type_name[] = {
    "Echo Reply",               /* Type  0 */
    "Undefine",                 /* Type  1 */
    "Undefine",                 /* Type  2 */
    "Destination Unreachable",  /* Type  3 */
    "Source Quench",            /* Type  4 */
    "Redirect (change route)",  /* Type  5 */
    "Undefine",                 /* Type  6 */
    "Undefine",                 /* Type  7 */
    "Echo Request",             /* Type  8 */
    "Undefine",                 /* Type  9 */
    "Undefine",                 /* Type 10 */
    "Time Exceeded",            /* Type 11 */
    "Parameter Problem",        /* Type 12 */
    "Timestamp Request",        /* Type 13 */
    "Timestamp Reply",          /* Type 14 */
    "Information Request",      /* Type 15 */
    "Information Reply",        /* Type 16 */
    "Address Mask Request",     /* Type 17 */
    "Address Mask Reply",       /* Type 18 */
    "Unknown"                   /* Type 19 */
  };                            /* ICMP�̃^�C�v��\�������� */
#define ICMP_TYPE_MAX (sizeof type_name / sizeof type_name[0])
  int type = icmp->icmp_type;  /* ICMP�^�C�v */

  if (type < 0 || ICMP_TYPE_MAX <= type)
    type = ICMP_TYPE_MAX - 1;

  printf("Protocol: ICMP (%s)\n", type_name[type]);

  printf("+------------+------------+-------------------------+\n");
  printf("| Type:   %3u| Code:   %3u| Checksum:          %5u|\n",
         icmp->icmp_type, icmp->icmp_code, ntohs(icmp->icmp_cksum));
  printf("+------------+------------+-------------------------+\n");

  if (icmp->icmp_type == 0 || icmp->icmp_type == 8) {
      printf("| Identification:    %5u| Sequence Number:   %5u|\n",
             ntohs(icmp->icmp_id), ntohs(icmp->icmp_seq));
      printf("+-------------------------+-------------------------+\n");
  } else if (icmp->icmp_type == 3) {
    if (icmp->icmp_code == 4) {
      printf("| void:          %5u| Next MTU:          %5u|\n",
             ntohs(icmp->icmp_pmvoid), ntohs(icmp->icmp_nextmtu));
      printf("+-------------------------+-------------------------+\n");
    } else {
      printf("| Unused:                                 %10lu|\n",
             (unsigned long) ntohl(icmp->icmp_void));
      printf("+-------------------------+-------------------------+\n");
    }
  } else if (icmp->icmp_type == 5) {
    printf("| Router IP Address:                 %15s|\n",
           inet_ntoa(*(struct in_addr *) &(icmp->icmp_gwaddr)));
    printf("+---------------------------------------------------+\n");
  } else if (icmp->icmp_type == 11) {
    printf("| Unused:                                 %10lu|\n",
           (unsigned long) ntohl(icmp->icmp_void));
    printf("+---------------------------------------------------+\n");
  }

  /* ICMP�̌��ɁAIP�w�b�_�ƃg�����X�|�[�g�w�b�_�������ꍇ�̏��� */
  if (icmp->icmp_type == 3 || icmp->icmp_type == 5 || icmp->icmp_type == 11) {
    struct ip *ip = (struct ip *) icmp->icmp_data;    /* IP�w�b�_             */
    char *p = (char *) ip + ((int) (ip->ip_hl) << 2); /* �g�����X�|�[�g�w�b�_ */

    print_ip(ip);
    switch (ip->ip_p) {
      case IPPROTO_TCP:
        print_tcp_mini((struct tcphdr *) p);
        break;
      case IPPROTO_UDP:
        print_udp((struct udphdr *) p);
        break;
    }
  }
}

/*
 * void print_tcp_mini(struct tcphdr *tcp);
 * �@�\
 *     TCP�w�b�_�̐擪��64�r�b�g�̕\��(ICMP�ŕԑ�����镔��)
 * ������
 *     struct tcphdr *tcp;  TCP�w�b�_�\����
 * �߂�l
 *     �Ȃ�
 */
void print_tcp_mini(struct tcphdr *tcp)
{
  printf("Protocol: TCP\n");
  printf("+-------------------------+-------------------------+\n");
  printf("| Source Port:       %5u| Destination Port:  %5u|\n",
         ntohs(tcp->th_sport), ntohs(tcp->th_dport));
  printf("+-------------------------+-------------------------+\n");
  printf("| Sequence Number:                        %10lu|\n",
         (unsigned long) ntohl(tcp->th_seq));
  printf("+---------------------------------------------------+\n");
}

/*
 * void print_tcp(struct tcphdr *tcp);
 * �@�\
 *     TCP�w�b�_�̕\��
 * ������
 *     struct tcphdr *tcp;  TCP�w�b�_�\����
 * �߂�l
 *     �Ȃ�
 */
void print_tcp(struct tcphdr *tcp)
{
  printf("Protocol: TCP\n");
  printf("+-------------------------+-------------------------+\n");
  printf("| Source Port:       %5u| Destination Port:  %5u|\n",
         ntohs(tcp->th_sport), ntohs(tcp->th_dport));
  printf("+-------------------------+-------------------------+\n");
  printf("| Sequence Number:                        %10lu|\n",
         (unsigned long) ntohl(tcp->th_seq));
  printf("+---------------------------------------------------+\n");
  printf("| Acknowledgement Number:                 %10lu|\n",
         (unsigned long) ntohl(tcp->th_ack));
  printf("+------+---------+--------+-------------------------+\n");
  printf("| DO:%2u| Reserved|F:%6s| Window Size:       %5u|\n",
         tcp->th_off, tcp_ftoa(tcp->th_flags), ntohs(tcp->th_win));
  printf("+------+---------+--------+-------------------------+\n");
  printf("| Checksum:          %5u| Urgent Pointer:    %5u|\n",
         ntohs(tcp->th_sum), ntohs(tcp->th_urp));
  printf("+-------------------------+-------------------------+\n");
}

/*
 * char *tcp_ftoa(int flag);
 * �@�\
 *     TCP�w�b�_�̃R���g���[���t���O�𕶎���ɕϊ�
 * ������
 *     int flag;      TCP�̃R���g���[���t���O
 * �߂�l
 *     char *         �ϊ����ꂽ������
 */
char *tcp_ftoa(int flag)
{
  static int  f[] = {'U', 'A', 'P', 'R', 'S', 'F'}; /* TCP�t���O��\������  */
#define TCP_FLG_MAX (sizeof f / sizeof f[0])
  static char str[TCP_FLG_MAX + 1];            /* �߂�l���i�[����o�b�t�@  */
  unsigned int mask = 1 << (TCP_FLG_MAX - 1);  /* �t���O�����o���}�X�N    */
  int i;                                       /* ���[�v�ϐ�                */

  for (i = 0; i < TCP_FLG_MAX; i++) {
    if (((flag << i) & mask) != 0)
      str[i] = f[i];
    else
      str[i] = '0';
  }
  str[i] = '\0';

  return str;
}

/*
 * void print_udp(struct udphdr *udp);
 * �@�\
 *     UDP�w�b�_��\��
 * ������
 *     struct udphdr *udp;  UDP�w�b�_�\���̂ւ̃|�C���^
 * �߂�l
 *     �Ȃ�
 */
void print_udp(struct udphdr *udp)
{
  printf("Protocol: UDP\n");
  printf("+-------------------------+-------------------------+\n");
  printf("| Source Port:       %5u| Destination Port:  %5u|\n",
         ntohs(udp->uh_sport), ntohs(udp->uh_dport));
  printf("+-------------------------+-------------------------+\n");
  printf("| Length:            %5u| Checksum:          %5u|\n",
         ntohs(udp->uh_ulen), ntohs(udp->uh_sum));
  printf("+-------------------------+-------------------------+\n");
}

/*
 * void dump_packet(unsigned char *buff, int len);
 * �@�\
 *     Ethernet�t���[���擪����16�i���_���v(�A�X�L�[�����\��)
 * ������
 *     unsigned char *buff;  �_���v����f�[�^�̐擪�A�h���X
 *     int len;              �_���v����o�C�g��
 * �߂�l
 *     �Ȃ�
 */
void dump_packet(unsigned char *buff, int len)
{
  int i, j;  /* ���[�v�ϐ� */

  printf("Frame Dump:\n");
  for (i = 0; i < len; i += 16) {
    /* 16�i���_���v */
    for (j = i; j < i + 16 && j < len; j++) {
      printf("%02x", buff[j]);
      if (j % 2)
        printf(" ");
    }

    /* �Ō�̍s�̒[���𐮗� */
    if (j == len && len % 16 != 0)
      for (j = 0; j < 40 - (len % 16) * 2.5; j++)
        printf(" ");
    printf(": ");

    /* �A�X�L�[�����\�� */
    for (j = i; j < i + 16 && j < len; j++) {
      if ((buff[j] >= 0x20) && (buff[j] <= 0x7e))
        putchar(buff[j]);
      else
        printf(".");
    }

    printf("\n");
  }
  fflush(stdout);
}

#ifndef __linux
/*
 * int open_bpf(char *ifname, int *bufsize);
 * �@�\
 *     BPF���I�[�v������
 * ������
 *     char *ifname;  �C���^�t�F�[�X��
 *     int *bufsize;  BPF�����̃o�b�t�@�T�C�Y
 * �߂�l
 *     int            �t�@�C���f�B�X�N���v�^
 */
int open_bpf(char *ifname, int *bufsize)
{
  char buf[CMAX];    /* ������i�[�p              */
  int bpfd;          /* �t�@�C���f�B�X�N���v�^    */
  struct ifreq ifr;  /* �C���^�t�F�[�X�����\����  */
  int i;             /* ���[�v�ϐ�                */

  /* BPF�f�o�C�X�t�@�C���̃I�[�v�� */
  for (i = 0; i < 256; i++) { 
    snprintf(buf, CMAX, "/dev/bpf%d", i);
    if ((bpfd = open(buf, O_RDWR, 0)) > 0)
      break;
  }

  if (i >= 256) {
    fprintf(stderr, "cannot open BPF\n");
    return -1;
  }

  /* BPF�����̃o�b�t�@�T�C�Y�̐ݒ� */
  *bufsize = MAXSIZE;
  if (ioctl(bpfd, BIOCSBLEN, bufsize) < 0) {
    snprintf(buf, CMAX, "ioctl(BIOCSBLEN, %d)", *bufsize);
    perror(buf);
    return -1;
  }

  /* �C���^�t�F�[�X���̐ݒ� */
  snprintf(ifr.ifr_name, sizeof ifr.ifr_name, "%s", ifname);
  if (ioctl(bpfd, BIOCSETIF, &ifr) < 0) {
    snprintf(buf, CMAX, "ioctl(BIOCSETIF, '%s')", ifname);
    perror(buf);
    return -1;
  }
  fprintf(stderr, "BPF read from '%s' (%s)\n", ifr.ifr_name, buf);

  /* promiscuous���[�h */
  if (ioctl(bpfd, BIOCPROMISC, NULL) < 0) {
    perror("ioctl(BIOCPROMISC)");
    return -1;
  }

  /* �������[�h */
  i = 1;
  if (ioctl(bpfd, BIOCIMMEDIATE, &i) < 0) {
    perror("ioctl(BIOCIMMEDIATE)");
    return -1;
  }

  return bpfd;
}
#endif

void help(char *cmd)
{
  fprintf(stderr, "usage: %s [-aedht] [-i ifname] [-p protocols] [-f filters]\n", cmd);
  fprintf(stderr, "protocols: arp ip icmp tcp udp other\n");
  fprintf(stderr, "filters: ip <ip addr> port <port number>\n");
#ifdef __linux
  fprintf(stderr, "default: %s -p arp ip icmp tcp udp\n", cmd);
#else
  fprintf(stderr, "default: %s -i %s -p arp ip icmp tcp udp\n", cmd, DEF_IF);
#endif
}
