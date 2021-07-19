/**********************************************************************
 *  TCP SYN�v���O���� (tcpsyn.c)
 *        Ver 2.1 2007�N 3�� 6��
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#define  __FAVOR_BSD
#include <netinet/tcp.h>
#include <arpa/inet.h>

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

#define MAXDATA 1024

enum {CMD_NAME, DST_IP, DST_PORT, SRC_IP, SRC_PORT, SEQ};

struct packet_tcp {
  struct ip ip;
  struct tcphdr tcp;
  unsigned char data[MAXDATA];
};

void make_ip_header(struct ip *ip, int src_ip, int dst_ip, int len);
void make_tcp_header(struct packet_tcp *packet, int src_ip, int src_port,
                     int dst_ip, int dst_port, int seq, int ack, int datalen);
u_int16_t checksum(u_int16_t *data, int len);

int main(int argc, char *argv[])
{
  struct packet_tcp send;   /* ���M����TCP�p�P�b�g            */
  struct sockaddr_in dest;  /* ���M��̃A�h���X               */
  u_int32_t src_ip;         /* �n�_IP�A�h���X                 */
  u_int32_t dst_ip;         /* �I�_IP�A�h���X                 */
  u_int16_t src_port;       /* �n�_�|�[�g�ԍ�                 */
  u_int16_t dst_port;       /* �I�_�|�[�g�ԍ�                 */
  int s;                    /* �\�P�b�g�t�@�C���f�B�X�N���v�^ */
  tcp_seq seq;              /* �V�[�P���X�ԍ�                 */
  tcp_seq ack;              /* �m�F�����ԍ�                   */
  int datalen;              /* �f�[�^��                       */
  int iplen;                /* IP�w�b�_��                     */
  int on = 1;               /* ON                             */

  /* �������̃`�F�b�N */
  if (argc != 6) {
    fprintf(stderr, "usage: %s dst_ip dst_port src_ip src_port seq\n", 
            argv[CMD_NAME]);
    exit(EXIT_FAILURE);
  }

  /* raw�\�P�b�g�̃I�[�v�� */
  if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
    perror("socket(SOCK_RAW)");
    exit(EXIT_FAILURE);
  }
  /* IP�w�b�_�����O�ō쐬���� */
  if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof on) < 0) {
    perror("setsockopt(IPPROTO_IP, IP_HDRINCL)");
    exit(EXIT_FAILURE);
  }

  /* �w�b�_�̒l�̐ݒ�*/
  memset(&dest, 0, sizeof dest);
  dest.sin_family = AF_INET;
  dst_ip = dest.sin_addr.s_addr = inet_addr(argv[DST_IP]);
  src_ip = inet_addr(argv[SRC_IP]);
  sscanf(argv[DST_PORT], "%hu", &dst_port);
  sscanf(argv[SRC_PORT], "%hu", &src_port);
  sscanf(argv[SEQ],      "%ul", &seq);
  ack = 0;
  datalen = 0;
  iplen = datalen + (sizeof send.ip) + (sizeof send.tcp);

  /* TCP/IP�w�b�_�̍쐬 */
  memset(&send, 0, sizeof send);
  make_tcp_header(&send, src_ip, src_port, dst_ip, dst_port, seq, ack, datalen);
  make_ip_header(&(send.ip), src_ip, dst_ip, iplen);

  CHKADDRESS(dst_ip);

  /* SYN�p�P�b�g�̑��M */
  printf("SYN send to %s.\n", argv[DST_IP]);
  if (sendto(s, (char *) &send, iplen, 0, (struct sockaddr *) &dest, 
             sizeof dest) < 0) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }
  close(s);

  return 0;
}

/*
 * void make_tcp_header(struct packet_tcp *packet, int src_ip, int src_port, 
 *                      int dst_ip, int dst_port, int seq, int ack, 
 *                      int datalen);
 *
 * �@�\
 *     TCP�w�b�_�̍쐬
 * ������ 
 *     struct packet_tcp *packet;  �쐬����TCP�w�b�_���܂ލ\����
 *     int src_ip;                 ���M��IP�A�h���X
 *     int src_port;               ���M���|�[�g�ԍ�
 *     int dst_ip;                 ���M��IP�A�h���X
 *     int dst_port;               ���M��|�[�g�ԍ�
 *     int seq;                    �V�[�P���X�ԍ�
 *     int ack;                    �m�F�����ԍ�
 *     int datalen;                TCP�f�[�^��
 * �߂�l
 *     �Ȃ�
 */
void make_tcp_header(struct packet_tcp *packet, int src_ip, int src_port, 
                     int dst_ip, int dst_port, int seq, int ack, int datalen)
{
  /* TCP�w�b�_�̍쐬 */
  packet->tcp.th_seq   = htonl(seq);
  packet->tcp.th_ack   = htonl(ack);
  packet->tcp.th_sport = htons(src_port);
  packet->tcp.th_dport = htons(dst_port);
  packet->tcp.th_off   = 5;
  packet->tcp.th_flags = TH_SYN;
  packet->tcp.th_win   = htons(8192);
  packet->tcp.th_urp   = htons(0);

  /* �^���w�b�_�̍쐬 */
  packet->ip.ip_ttl    = 0;
  packet->ip.ip_p      = IPPROTO_TCP;
  packet->ip.ip_src.s_addr = src_ip;
  packet->ip.ip_dst.s_addr = dst_ip;
  packet->ip.ip_sum    = htons((sizeof packet->tcp) + datalen);

  /* �`�F�b�N�T���̌v�Z */
#define PSEUDO_HEADER_LEN 12
  packet->tcp.th_sum = 0;
  packet->tcp.th_sum = checksum((u_int16_t *) &(packet->ip.ip_ttl),
                                PSEUDO_HEADER_LEN + (sizeof packet->tcp) 
                                + datalen);
}

/*
 * void make_ip_header(struct ip *ip, int src_ip, int dst_ip, int iplen);
 * �@�\
 *     IP�w�b�_�̍쐬
 * ������ 
 *     struct ip *ip;  �쐬����IP�w�b�_�̐擪�A�h���X
 *     int src_ip;     ���M��IP�A�h���X
 *     int dst_ip;     ���M��IP�A�h���X
 *     int iplen;      IP�f�[�^�O�����̑S��
 * �߂�l
 *     �Ȃ�
 */
void make_ip_header(struct ip *ip, int src_ip, int dst_ip, int iplen)
{
  /* IP�w�b�_�̍쐬 */
  ip->ip_v   = IPVERSION;
  ip->ip_hl  = sizeof (struct ip) >> 2;
  ip->ip_id  = 0;
#ifdef __linux
  /* Linux��raw IP�̏ꍇ */
  ip->ip_len = htons(iplen);
  ip->ip_off = htons(0);
#else
  ip->ip_len = iplen;
  ip->ip_off = IP_DF;
#endif
  ip->ip_ttl = 2;
  ip->ip_p   = IPPROTO_TCP;
  ip->ip_src.s_addr = src_ip;
  ip->ip_dst.s_addr = dst_ip;

  /* �`�F�b�N�T���̌v�Z */
  ip->ip_sum = 0;
  ip->ip_sum = checksum((u_int16_t *) ip, sizeof ip);
}

/*
 * u_int16_t checksum(u_int16_t *data, int len);
 * �@�\
 *     �`�F�b�N�T���̌v�Z
 * ������ 
 *     u_int16_t *data;  �`�F�b�N�T�������߂�f�[�^
 *     int len;          �f�[�^�̃o�C�g��
 * �߂�l
 *     u_int16_t         �`�F�b�N�T���̒l (�␔�l)
 */
u_int16_t checksum(u_int16_t *data, int len)
{
  u_int32_t sum = 0; /* ���߂�`�F�b�N�T�� */

  /* 2�o�C�g�����Z */
  for (; len > 1; len -= 2) {
    sum += *data++;
    if (sum & 0x80000000) 
      sum = (sum & 0xffff) + (sum >> 16);
  }

  /* �f�[�^������o�C�g�̏ꍇ�̏��� */
  if (len == 1) {
    u_int16_t i = 0;
    *(u_char*) (&i) = *(u_char *) data;
    sum += i;
  }

  /* �����ӂ��܂�Ԃ� */
  while (sum >> 16)
    sum = (sum & 0xffff) + (sum >> 16);

  return ~sum;
}
