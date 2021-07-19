/**********************************************************************
 *  ���[�g�X�L�����v���O���� (scanroute.c)
 *        Ver 2.1 2013�N 7�� 10��
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
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#define __FAVOR_BSD
#include <netinet/udp.h>
#include <arpa/inet.h>

#define BUFSIZE 512

enum {CMD_NAME, DST_IP};
enum {ON, OFF};

struct packet_udp {
  struct ip ip;
  struct udphdr udp;
};

void make_ip_header(struct ip *ip, int srcip, int dstip, int iplen);
void make_udp_header(struct udphdr *udp);
void tvsub(struct timeval *out, struct timeval *in);

int main(int argc, char *argv[])
{
  struct packet_udp sendpacket;  /* ���M����UDP/IP�p�P�b�g          */
  struct sockaddr_in send_sa;    /* �I�_�̃A�h���X                  */
  int send_sd;                   /* ���M�p�\�P�b�g�f�B�X�N���v�^    */
  int recv_sd;                   /* ��M�p�\�P�b�g�f�B�X�N���v�^    */
  int len;                       /* ���M����IP�p�P�b�g��            */
  int ttl;                       /* TTL�̒l                         */
  u_char buff[BUFSIZE];          /* ��M�o�b�t�@                    */
  struct icmp *icmp;             /* ICMP�\����                      */
  struct ip *ip;                 /* IP�\����                        */
  struct in_addr ipaddr;         /* IP�A�h���X�\����                */
  int on = 1;                    /* ON                              */
  int dns_flg = ON;              /* �z�X�g�̃h���C�������������邩  */

  /* ������-n�I�v�V�����̃`�F�b�N */
  if (argc == 3 && strcmp(argv[1], "-n") == 0) {
    dns_flg = OFF;
    argv[1] = argv[2];
    argv[2] = NULL;
    argc = 2;
  }

  /* �������̃`�F�b�N */
  if (argc != 2) {
    fprintf(stderr, "usage: %s [-n] dst_ip\n", argv[CMD_NAME]);
    exit(EXIT_FAILURE);
  }

  memset(&send_sa, 0, sizeof send_sa);
  send_sa.sin_family = AF_INET;

  /* �h���C��������IP�A�h���X�ւ̕ϊ� */
  if ((send_sa.sin_addr.s_addr = inet_addr(argv[DST_IP])) == INADDR_NONE) {
    struct hostent *he;  /* �z�X�g��� */

    if ((he = gethostbyname(argv[DST_IP])) == NULL) {
      fprintf(stderr, "unknown host %s\n", argv[DST_IP]);
      exit(EXIT_FAILURE);
    }
    send_sa.sin_family = he->h_addrtype;
    memcpy((char *) &(send_sa.sin_addr), he->h_addr, sizeof he->h_length);
  }

  /* UDP/IP���M�praw�\�P�b�g�̃I�[�v�� */
  if ((send_sd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
    perror("socket(SOCK_RAW)");
    exit(EXIT_FAILURE);
  }
  /* IP�w�b�_�����O�ō쐬���� */
  if (setsockopt(send_sd, IPPROTO_IP, IP_HDRINCL, &on, sizeof on) < 0) {
    perror("setsockopt(IPPROTO_IP, IP_HDRINCL)");
    exit(EXIT_FAILURE);
  }

  /* ICMP��M�praw�\�P�b�g�̃I�[�v�� */
  if ((recv_sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    perror("socket(SOCK_RAW)");
    exit(EXIT_FAILURE);
  }

  /* UDP/IP�p�P�b�g�̍쐬 */
  len = sizeof (struct packet_udp);
  memset(&sendpacket, 0, sizeof sendpacket);
  make_udp_header(&(sendpacket.udp));
  make_ip_header(&(sendpacket.ip), 0, send_sa.sin_addr.s_addr, len);

  /*
   * �X�L�������[�g���C�����[�`��
   */
  printf("scanroute %s\n", inet_ntoa(send_sa.sin_addr));
  for (ttl = 1; ttl <= 64; ttl++) {
    struct timeval tvm0;  /* �p�P�b�g���M����     */
    struct timeval tvm1;  /* �p�P�b�g��M����     */
    struct timeval tv;    /* select�̃^�C���A�E�g */
    int i;                /* ���[�v�ϐ�           */

    printf("%2d: ", ttl);
    fflush(stdout);

    sendpacket.ip.ip_ttl = ttl;

    for (i = 0; i < 3; i++) {
      /* UDP�p�P�b�g�̑��M */
      if (sendto(send_sd, (char *) &sendpacket, len, 0,
                 (struct sockaddr *) &send_sa, sizeof send_sa) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
      }
      /* ���M�����̋L�^ */
      gettimeofday(&tvm0, (struct timezone *) 0);

      /* select�̃^�C���A�E�g�̐ݒ� */
      tv.tv_sec  = 3;
      tv.tv_usec = 0;

      /* �p�P�b�g��M���[�v */
      while (1) {
        fd_set readfd;  /* select�Ō�������f�B�X�N���v�^ */
        int n;          /* select�̖߂�l                 */
        int ihlen;      /* �w�b�_��                       */

        /* select�Ńp�P�b�g����M����܂ő҂� */
        FD_ZERO(&readfd);
        FD_SET(recv_sd, &readfd);
        if ((n = select(recv_sd + 1, &readfd, NULL, NULL, &tv)) < 0) {
          perror("select");
          exit(EXIT_FAILURE);
        }
        if (n == 0) {
          printf("? ");
          fflush(stdout);
          break;
        }

        /* ICMP�p�P�b�g�̎�M */
        if (recvfrom(recv_sd, buff, BUFSIZE, 0, NULL, NULL) < 0) {
          perror("recvfrom");
          exit(EXIT_FAILURE);
        }
        /* �p�P�b�g��M�����̋L�^ */
        gettimeofday(&tvm1, (struct timezone *) 0);

        /* �֌W����p�P�b�g���ǂ��������� */
        ip = (struct ip *) buff;
        ihlen = ip->ip_hl << 2;
        icmp = (struct icmp *) (buff + ihlen);
        if ((icmp->icmp_type == ICMP_TIMXCEED
             && icmp->icmp_code == ICMP_TIMXCEED_INTRANS)
            || icmp->icmp_type == ICMP_UNREACH)
          goto exit_loop;
      }
    }
   exit_loop:
    if (i < 3) {
      struct hostent *host;          /* �z�X�g���              */
      char hostip[INET_ADDRSTRLEN];  /* IP�A�h���X��\��������  */

      /* IP�A�h���X�ƃh���C�����̕\�� */
      memcpy(&ipaddr, &(ip->ip_src.s_addr), sizeof ipaddr);
      snprintf(hostip, INET_ADDRSTRLEN, "%s", inet_ntoa(*(struct in_addr *)
                                                        &(ip->ip_src.s_addr)));
      if (dns_flg == OFF)
        printf("%-15s", hostip);
      else if ((host = gethostbyaddr((char *) &ipaddr, 4, AF_INET)) == NULL)
        printf("%-15s (%s) ", hostip, hostip);
      else
        printf("%-15s (%s) ", hostip, host->h_name);

      /* �������Ԃ̕\�� */
      tvsub(&tvm1, &tvm0);
      printf(": RTT =%8.4f ms", tvm1.tv_sec * 1000.0 + tvm1.tv_usec / 1000.0);

      /* �W�I�z�X�g�ɓ����������ǂ����̌��� */
      if (icmp->icmp_type == ICMP_UNREACH) {
        switch (icmp->icmp_code) {
          case ICMP_UNREACH_PORT:
            printf("  Reach!\n");
            break;
          case ICMP_UNREACH_HOST:
            printf("  Host Unreachable!\n");
            break;
          case ICMP_UNREACH_NET:
            printf("  Network Unreachable!\n");
            break;
        }
        goto end_program;
      }
    }
    printf("\n");
  }
 end_program:
  close(send_sd);
  close(recv_sd);

  return 0;
}

/*
 * void make_udp_header(struct udphdr *udp);
 * �@�\
 *     UDP�w�b�_�̍쐬
 * ������
 *     struct udphdr *udp;  �쐬����UDP�w�b�_�̐擪�A�h���X
 * �߂�l
 *     �Ȃ�
 */
void make_udp_header(struct udphdr *udp)
{
  udp->uh_sport = htons(33434);
  udp->uh_ulen  = htons((u_int16_t) sizeof (struct udphdr));
  udp->uh_dport = htons(33434);  /* traceroute�̃|�[�g�ԍ� */
  udp->uh_sum   = htons(0);
}

/*
 * void make_ip_header(struct ip *ip, int srcip, int dstip, int iplen);
 * �@�\
 *     IP�w�b�_�̍쐬
 * ������
 *     struct ip *ip;  �쐬����IP�w�b�_�̐擪�A�h���X
 *     int srcip;      ���M��IP�A�h���X
 *     int dstip;      ���M��IP�A�h���X
 *     int iplen;      IP�f�[�^�O�����̑S��
 * �߂�l
 *     �Ȃ�
 */
void make_ip_header(struct ip *ip, int srcip, int dstip, int iplen)
{
  memset(ip, 0, sizeof (struct ip));

  /* IP�w�b�_�̍쐬 */
  ip->ip_v   = IPVERSION;
  ip->ip_hl  = sizeof (struct ip) >> 2;
  ip->ip_id  = htons(0);
  ip->ip_off = 0;

#ifdef __linux
  /* Linux��raw IP�̏ꍇ */
  ip->ip_len = htons(iplen);
  ip->ip_off = htons(0);
#else
  /* BSD��raw IP�̏ꍇ */
  ip->ip_len = iplen;
  ip->ip_off = 0;
#endif
  ip->ip_ttl = 64;
  ip->ip_p   = IPPROTO_UDP;
  ip->ip_src.s_addr = srcip;
  ip->ip_dst.s_addr = dstip;

  /* �`�F�b�N�T���̌v�Z */
  ip->ip_sum = 0;
}

/*
 * void tvsub(struct timeval *out, struct timeval *in);
 * �@�\
 *     strcut timeval�̈����Z�B���ʂ�out�Ɋi�[�����B
 * ������ 
 *     struct timeval *out;  ������鐔
 *     struct timeval *in;   ������
 * �߂�l
 *     �Ȃ�
 */
void tvsub(struct timeval *out, struct timeval *in)
{
  if ((out->tv_usec -= in->tv_usec) < 0) {
    out->tv_sec--;
    out->tv_usec += 1000000;
  }
  out->tv_sec -= in->tv_sec;
}
