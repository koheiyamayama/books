/**********************************************************************
 *  ARP�e�[�u���X�V�v���O���� (arpupdate.c)
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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#ifndef __linux
#include <sys/ioctl.h>
#include <net/bpf.h>
#include <net/if.h>
#include <fcntl.h>
#endif
#include <netinet/if_ether.h>
//#include <net/if_arp.h>

#define CHKADDRESS(_saddr_) \
        {\
          unsigned char *p = (unsigned char *) &(_saddr_);\
          if ((p[0] == 127)\
           || (p[0] == 10)\
           || (p[0] == 172 && 16 <= p[1] && p[1] <= 31)\
           || (p[0] == 192 && p[1] == 168))\
            ;\
          else {\
            fprintf(stderr, "IP address error.\n");\
            exit(EXIT_FAILURE);\
          }\
        }

#define MAXSIZE 8192
#define CMAX    256

enum {CMD_NAME, IFNAME, DST_IP, MAC_ADDR, OPTION};
enum {NORMAL, REPLY, REQUEST};

#ifndef __linux
int open_bpf(char *ifname, int *bufsize);
#endif

void make_ethernet(struct ether_header *eth, unsigned char *ether_dhost,
                   unsigned char *ether_shost, u_int16_t ether_type);
void make_arp(struct ether_arp *arp, int op, unsigned char *arp_sha,
              unsigned char *arp_spa, unsigned char *arp_tha,
              unsigned char *arp_tpa);
void print_ethernet(struct ether_header *eth);
void print_arp(struct ether_arp *arp);
char *mac_ntoa(unsigned char d[]);
void help(char *cmd);

int main(int argc, char *argv[])
{
  int s;               /* �\�P�b�g�f�B�X�N���v�^          */
  u_char mac_addr[6];  /* MAC�A�h���X�i�[�p               */
  int tmp[6];          /* MAC�A�h���X�i�[�p(�ꎞ���p)     */
  int flag;            /* �t���O (REPLY�AREQUEST�ANORMAL) */
  int i;               /* ���[�v�ϐ�                      */
  u_int32_t dst_ip;    /* �^�[�Q�b�gIP�A�h���X            */
  char ifname[CMAX];   /* �C���^�t�F�[�X��                */
#ifndef __linux
  struct bpf_hdr *bp;  /* BPF�w�b�_�\����                 */
  int bpf_len;         /* BPF�ł̎�M�f�[�^�̒���         */
  int bufsize;         /* BPF�����̃o�b�t�@�T�C�Y         */
#else
  struct sockaddr sa;  /* �\�P�b�g�A�h���X�\����          */
#endif

  flag = NORMAL;
  if (argc == 5) {
    if (strcmp(argv[OPTION], "reply") == 0) 
      flag = REPLY;
    else if (strcmp(argv[OPTION], "request") == 0)
      flag = REQUEST;
    else {
      help(argv[CMD_NAME]);
      exit(EXIT_FAILURE);
    }
  } else if (argc != 4) {
    help(argv[CMD_NAME]);
    exit(EXIT_FAILURE);
  }

  snprintf(ifname, sizeof ifname, "%s", argv[IFNAME]);
  dst_ip = inet_addr(argv[DST_IP]);

  if (sscanf(argv[MAC_ADDR], "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1],
             &tmp[2], &tmp[3], &tmp[4], &tmp[5]) != 6) {
    printf("MAC address error %s\n", argv[MAC_ADDR]);
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < 6; i++)
    mac_addr[i] = (char) tmp[i];

  CHKADDRESS(dst_ip);

#ifdef __linux
  if ((s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&sa, 0, sizeof sa);
  sa.sa_family = PF_PACKET;
  snprintf(sa.sa_data, sizeof sa.sa_data, "%s", ifname);

  if (bind(s, &sa, sizeof sa) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }
#else
  if ((s = open_bpf(ifname, &bufsize)) < 0)
    exit(EXIT_FAILURE);
  bpf_len = 0;
#endif

  while (1) {
    struct ether_header *eth;  /* Ethernet�w�b�_�\����                      */
    char recv_buff[MAXSIZE];   /* ��M�o�b�t�@                              */
    char send_buff[MAXSIZE];   /* ���M�o�b�t�@                              */
    char *rp;                  /* �w�b�_�̐擪��\����Ɨp�|�C���^(��M�p)  */
    char *rp0;                 /* �p�P�b�g�̐擪��\���|�C���^(��M�p)      */
    char *sp;                  /* �w�b�_�̐擪��\����Ɨp�|�C���^(���M�p)  */
    int len;                   /* �e��̒���(�ꎞ���p)                      */

#ifndef __linux
    if (bpf_len <= 0) {
      if ((bpf_len = read(s, recv_buff, bufsize)) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
      }
      bp = (struct bpf_hdr *) recv_buff;
    } else {
      bp = (struct bpf_hdr *) ((void *) bp
          + BPF_WORDALIGN(bp->bh_hdrlen + bp->bh_caplen));
    }
    rp = rp0 = (char *) bp + bp->bh_hdrlen;
    len = bp->bh_caplen;
#ifdef DEBUG
    printf("bpf_len=%d,",  bpf_len);
    printf("hdrlen=%d,",   bp->bh_hdrlen);
    printf("caplen=%d,",   bp->bh_caplen);
    printf("datalen=%d\n", bp->bh_datalen);
#endif
    bpf_len -= BPF_WORDALIGN(bp->bh_hdrlen + bp->bh_caplen);
#else
    if ((len = read(s, recv_buff, MAXSIZE)) < 0) {
      perror("read");
      exit(EXIT_FAILURE);
    }
    rp = rp0 = recv_buff;
#endif
    eth = (struct ether_header *) rp;
    rp = rp + sizeof (struct ether_header);

    /* arpupdate�����M�����p�P�b�g�y�т��̕Ԏ��͖������� */
    if ( memcmp(eth->ether_dhost, mac_addr, 6) != 0 
      && memcmp(eth->ether_shost, mac_addr, 6) != 0
      && ntohs(eth->ether_type) == ETHERTYPE_ARP) {
      struct ether_arp *arp;  /* ARP�p�P�b�g�\���� */

      arp = (struct ether_arp *) rp;

      if (dst_ip == *(int *) (arp->arp_spa)) {
        static u_char zero[6]; /* Ethernet NULL MAC�A�h���X */
        static u_char one[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 
                               /* Ethernet�u���[�h�L���X�gMAC�A�h���X */

        printf("Hit****************\n");
        print_ethernet(eth);
        print_arp(arp);

        sp = send_buff + sizeof (struct ether_header);

        switch (flag) {
          case REPLY:
            make_arp((struct ether_arp *) sp, ARPOP_REPLY, mac_addr, 
                     arp->arp_tpa, arp->arp_sha, arp->arp_spa);
            make_ethernet((struct ether_header *) send_buff, arp->arp_sha,
                          mac_addr, ETHERTYPE_ARP);
            break;
          case REQUEST:
            make_arp((struct ether_arp *) sp, ARPOP_REQUEST, mac_addr,
                     arp->arp_spa, zero, arp->arp_tpa);
            make_ethernet((struct ether_header *) send_buff, one, mac_addr,
                          ETHERTYPE_ARP);
            break;
          default:
            make_arp((struct ether_arp *) sp, ARPOP_REQUEST, mac_addr,
                     arp->arp_tpa, zero, arp->arp_spa);
            make_ethernet((struct ether_header *) send_buff,
                          arp->arp_sha, mac_addr, ETHERTYPE_ARP);
            break;
        }
        len = sizeof (struct ether_header) + sizeof (struct ether_arp);

        /* �p�P�b�g��M��A0.5�b�҂��Ă���p�P�b�g�𑗐M */
        usleep(500 * 1000);
#ifndef __linux
        if (write(s, send_buff, len) < 0) {
          perror("write");
          exit(EXIT_FAILURE);
        }
#else
        if (sendto(s, send_buff, len, 0, &sa, sizeof sa) < 0) {
          perror("sendto");
          exit(EXIT_FAILURE);
        }
#endif
        printf("SEND---------------------------\n");
        print_ethernet((struct ether_header *) send_buff);
        print_arp((struct ether_arp *) sp);
      }
    }
  }
  return 0;
}

/*
 * void make_ethernet(struct ether_header *eth, unsigned char *ether_dhost,
 *                    unsigned char *ether_shost, u_int16_t ether_type)
 * �@�\
 *     Ethernet�w�b�_�̍쐬
 * ������ 
 *     struct ether_header *eth;    �쐬����w�b�_Ethernet�w�b�_
 *     unsigend char *ether_dhost;  �I�_MAC�A�h���X
 *     unsigend char *ether_shost;  �n�_MAC�A�h���X
 *     u_int16_t ether_type;        �^�C�v
 * �߂�l
 *     �Ȃ�
 */
void make_ethernet(struct ether_header *eth, unsigned char *ether_dhost,
                   unsigned char *ether_shost, u_int16_t ether_type)
{
  memcpy(eth->ether_dhost, ether_dhost, 6);
  memcpy(eth->ether_shost, ether_shost, 6);
  eth->ether_type = htons(ether_type);
}

/*
 * void make_arp(struct ether_arp *arp, int op, unsigned char *arp_sha,
 *               unsigned char *arp_spa, unsigned char *arp_tha,
 *               unsigned char *arp_tpa);
 * �@�\
 *     ARP�p�P�b�g�̍쐬
 * ������ 
 *     struct ether_arp *arp;   �쐬����ARP�w�b�_
 *     int op;                  �I�y���[�V����
 *     unsigned char *arp_sha;  �n�_MAC�A�h���X
 *     unsigned *arp_spa;       �n�_IP�A�h���X
 *     unsigned *arp_tha;       �^�[�Q�b�gMAC�A�h���X
 *     unsigned *arp_tpa;       �^�[�Q�b�gIP�A�h���X
 * �߂�l
 *     �Ȃ�
 */
void make_arp(struct ether_arp *arp, int op, unsigned char *arp_sha,
              unsigned char *arp_spa, unsigned char *arp_tha,
              unsigned char *arp_tpa)
{
  arp->arp_hrd  = htons(1);
  arp->arp_pro  = htons(ETHERTYPE_IP);
  arp->arp_hln  = 6;
  arp->arp_pln  = 4;
  arp->arp_op   = htons(op);
  memcpy(arp->arp_sha, arp_sha, 6);
  memcpy(arp->arp_spa, arp_spa, 4);
  memcpy(arp->arp_tha, arp_tha, 6);
  memcpy(arp->arp_tpa, arp_tpa, 4);
}

/*
 * char *mac_ntoa(unsigned char *d);
 * �@�\
 *    �z��Ɋi�[����Ă���MAC�A�h���X�𕶎���ɕϊ�
 *    static�ϐ��𗘗p���邽�߁A�񃊃G���g�����g�֐�
 * ������
 *    unsigned char *d;  MAC�A�h���X���i�[����Ă���̈�̐擪�A�h���X
 * �߂�l
 *    ������ɕϊ����ꂽMAC�A�h���X
 */
char *mac_ntoa(unsigned char *d)
{
#define MACSTRLEN 50
  static char str[MACSTRLEN];  /* ������ɕϊ�����MAC�A�h���X��ۑ� */

  snprintf(str, MACSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
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
  int type = ntohs(eth->ether_type); /* Ethernet�^�C�v */

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
    printf("| Ethernet Typ:     0x%04x|\n", type);
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

  if (op <= 0 || ARP_OP_MAX < op)
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
 * void help(char *cmd);
 * �@�\
 *     arpupdate�̎g�p���@�̕\��
 * ������
 *     char *cmd;    �R�}���h�� (arpupdate)
 * �߂�l
 *     �Ȃ�
 */
void help(char *cmd)
{
  fprintf(stderr, "usage: %s ifname dst_ip mac_addr [reply|request]\n", cmd);
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
  char buf[CMAX];    /* ������i�[�p  */
  int bpfd;          /* �t�@�C���f�B�X�N���v�^  */
  struct ifreq ifr;  /* �C���^�t�F�[�X�����\����  */
  int i;             /* ���[�v�ϐ�    */

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

#ifdef BIOCSHDRCMPLT
  /* ���M��MAC�A�h���X���㏑������Ȃ��悤�ɂ��� */
  i = 1;
  if (ioctl(bpfd, BIOCSHDRCMPLT, &i) < 0) {
    perror("ioctl(BIOCSHDRCMPLT)");
    return -1;
  }
#endif

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
