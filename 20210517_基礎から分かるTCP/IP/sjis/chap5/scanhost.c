/**********************************************************************
 *  �z�X�g�X�L�����v���O���� (scanhost.c)
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
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

#define BUFSIZE    4096
#define PACKET_LEN 72

enum {CMD_NAME, START_IP, LAST_IP};

void make_icmp8_packet(struct icmp *icmp, int len, int n);
void tvsub(struct timeval *out, struct timeval *in);
u_int16_t checksum(u_int16_t *data, int len);

int main(int argc, char *argv[])
{
  struct sockaddr_in send_sa;  /* ���M��̃A�h���X                */
  int s;                       /* �\�P�b�g�f�B�X�N���v�^          */
  char send_buff[PACKET_LEN];  /* ���M�o�b�t�@                    */
  char recv_buff[BUFSIZE];     /* ��M�o�b�t�@                    */
  int start_ip;                /* �X�L��������IP�A�h���X�̊J�n�l  */
  int end_ip;                  /* �X�L��������IP�A�h���X�̏I���l  */
  int dst_ip;                  /* �X�L��������IP�A�h���X�̒l      */
  int on = 1;                  /* ON                              */

  if (argc != 3) {
    fprintf(stderr, "usage: %s start_ip last_ip\n", argv[CMD_NAME]);
    exit(EXIT_FAILURE);
  }

  /* �X�L��������IP�A�h���X�͈̔͂�ݒ� */
  start_ip = ntohl(inet_addr(argv[START_IP]));
  end_ip   = ntohl(inet_addr(argv[LAST_IP]));

  memset(&send_sa, 0, sizeof send_sa);
  send_sa.sin_family = AF_INET;

  /* ICMP/IP����M�praw�\�P�b�g�̃I�[�v�� */
  if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    perror("socket(SOCK_RAW, IPPROTO_ICMP)");
    exit(EXIT_FAILURE);
  }
  /* BROADCAST�p�P�b�g�����M�ł���悤�ɂ��� */
  if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof on) < 0) {
    perror("setsockopt(SOL_SOCKET, SO_BROADCAST)");
    exit(EXIT_FAILURE);
  }

  /*
   * �X�L�����z�X�g���C�����[�`��
   */
  for (dst_ip = start_ip; dst_ip <= end_ip; dst_ip++) {
    int i;  /* ���[�v�ϐ� */

    send_sa.sin_addr.s_addr = htonl(dst_ip);
    CHKADDRESS(send_sa.sin_addr);

    for (i = 0; i < 3; i++) {
      struct timeval tv;  /* �����̏�� */

      printf("scan %s (%d)\r", inet_ntoa(send_sa.sin_addr), i + 1);
      fflush(stdout);

      /* ICMP�G�R�[�v���p�P�b�g(���M��������)���쐬���đ��M */
      make_icmp8_packet((struct icmp *) send_buff, PACKET_LEN, i);
      if (sendto(s, (char *) &send_buff, PACKET_LEN, 0,
                 (struct sockaddr *) &send_sa, sizeof send_sa) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
      }

      /* select�̃^�C���A�E�g�̐ݒ� */
      tv.tv_sec  = 0;
      tv.tv_usec = 200 * 1000;

      while (1) {
        fd_set readfd;  /* select����t�@�C���f�B�X�N���v�^ */
        struct ip *ip;  /* ip�w�b�_�\����   */
        int ihlen;      /* �w�b�_��   */

        /* select�Ńp�P�b�g�̓�����҂� */
        FD_ZERO(&readfd);
        FD_SET(s, &readfd);
        if (select(s + 1, &readfd, NULL, NULL, &tv) <= 0)
          break;

        /* �p�P�b�g��M */
        if (recvfrom(s, recv_buff, BUFSIZE, 0, NULL, NULL) < 0) {
          perror("recvfrom");
          exit(EXIT_FAILURE);
        }

        ip = (struct ip *) recv_buff;
        ihlen = ip->ip_hl << 2;
        if (ip->ip_src.s_addr == send_sa.sin_addr.s_addr) {
          struct icmp *icmp;  /* icmp�p�P�b�g�\���� */

          icmp = (struct icmp *) (recv_buff + ihlen);
          if (icmp->icmp_type == ICMP_ECHOREPLY) {
            /* ICMP�G�R�[������Ԃ��Ă���IP�A�h���X��\�� */
            printf("%-15s", inet_ntoa(*(struct in_addr *)
                                      &(ip->ip_src.s_addr)));
            /* ��������(RTT)�̌v�Z�ƕ\�� */
            gettimeofday(&tv, (struct timezone *) 0);
            tvsub(&tv, (struct timeval *) (icmp->icmp_data));
            printf(": RTT = %8.4f ms\n", tv.tv_sec*1000.0 + tv.tv_usec/1000.0);
            goto exit_loop;
          }
        }
      }
    }
 exit_loop: ;
  }
  close(s);

  return 0;
}

/*
 * void make_icmp8_packet(struct icmp *icmp, int len, int n);
 * �@�\
 *     ICMP�p�P�b�g�̍쐬
 * ������ 
 *     struct icmp *icmp;  �쐬����ICMP�w�b�_�̐擪�A�h���X
 *     int len;            ICMP�G�R�[�v���̃p�P�b�g��
 *     int n;              ICMP�G�R�[�v���̃V�[�P���X�ԍ�
 * �߂�l
 *     �Ȃ�
 */
void make_icmp8_packet(struct icmp *icmp, int len, int n)
{
  memset(icmp, 0, len);

  /* ���������f�[�^���ɋL�^�B�l�b�g���[�N�o�C�g�I�[�_�[���C�ɂ��Ȃ� */
  gettimeofday((struct timeval *) (icmp->icmp_data), (struct timezone *) 0);

  /* ICMP�w�b�_�̍쐬 */
  icmp->icmp_type = ICMP_ECHO;
  icmp->icmp_code = 0;
  icmp->icmp_id   = 0;  /* �{��ID�ɂ̓v���Z�XID�Ȃǂ̎��ʎq���i�[���� */
  icmp->icmp_seq  = n;  /* �l�b�g���[�N�o�C�g�I�[�_�[���C�ɂ��Ȃ� */

  /* �`�F�b�N�T���̌v�Z */
  icmp->icmp_cksum = 0;
  icmp->icmp_cksum = checksum((u_int16_t *) icmp, len);
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
