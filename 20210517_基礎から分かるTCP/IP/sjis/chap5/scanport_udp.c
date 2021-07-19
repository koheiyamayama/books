/**********************************************************************
 *  UDP�|�[�g�X�L�����v���O���� (scanport_udp.c)
 *        Ver 2.0 2004�N 7�� 10��
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
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#define  __FAVOR_BSD
#include <netinet/udp.h>

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

#define MAXBUFF 8192

enum {CMD_NAME, DST_IP, START_PORT, LAST_PORT};
enum {CONNECT, NOCONNECT};

int udpportscan(u_int32_t dst_ip, int dst_port, int send_sd, int recv_sd);

int main(int argc, char *argv[])
{
  int recv_sd;       /* ��M�p�f�B�X�N���v�^                      */
  int send_sd;       /* ���M�p�f�B�X�N���v�^                      */
  u_int32_t dst_ip;  /* �I�_IP�A�h���X (�l�b�g���[�N�o�C�g�I�[�_) */
  int end_port;      /* �X�L�����J�n�|�[�g�ԍ�                    */
  int start_port;    /* �X�L�����I���|�[�g�ԍ�                    */
  int dst_port;      /* �X�L��������|�[�g�ԍ�                    */

  /* �R�}���h���C���������̐��̃`�F�b�N */
  if (argc != 4) {
    fprintf(stderr, "usage: %s dst_ip start_port last_port\n", argv[CMD_NAME]);
    exit(EXIT_FAILURE);
  }

  /* �R�}���h���C������������l���Z�b�g���� */
  dst_ip     = inet_addr(argv[DST_IP]);
  start_port = atoi(argv[START_PORT]);
  end_port   = atoi(argv[LAST_PORT]);

  CHKADDRESS(dst_ip);

  /* ���M�pUDP�\�P�b�g�̃I�[�v�� */
  if ((send_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket(SOCK_DGRAM)");
    exit(EXIT_FAILURE);
  }

  /* ��M�praw�\�P�b�g�̃I�[�v�� */
  if ((recv_sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    perror("socket(SOCK_RAW)");
    exit(EXIT_FAILURE);
  }

  /* �X�L�����|�[�g���C�����[�v */
  for (dst_port = start_port; dst_port <= end_port; dst_port++) {
    printf("Scan Port %d\r", dst_port);
    fflush(stdout);

    if (udpportscan(dst_ip, dst_port, send_sd, recv_sd) == CONNECT) {
      /* select���^�C���A�E�g�����Ƃ� */
      struct servent *se;  /* �T�[�r�X��� */

      se = getservbyport(htons(dst_port), "udp");
      printf("%5d %-20s\n", dst_port, (se==NULL) ? "unknown" : se->s_name);
    }
    usleep(10 * 1000);
  }
  close(send_sd);
  close(recv_sd);

  return 0;
}

/*
 * int udpportscan(u_int32_t dst_ip, int dst_port)
 * �@�\
 *     �w�肵��IP�A�h���X�A�|�[�g�ԍ���UDP�ŒʐM�ł��邩���ׂ�B
 * ������ 
 *     u_int32_t dst_ip;  ���M��IP�A�h���X (�l�b�g���[�N�o�C�g�I�[�_)
 *     int dst_port;      ���M��|�[�g�ԍ�
 * �߂�l
 *     int                CONNECT  ... �|�[�g���J���Ă���
 *                        NOCONNECT .. �|�[�g���܂��Ă���
 */
int udpportscan(u_int32_t dst_ip, int dst_port, int send_sd, int recv_sd)
{
  struct sockaddr_in send_sa;  /* ���M��̃A�h���X���      */
  char buff[MAXBUFF];          /* �p�P�b�g��M�p�o�b�t�@    */
  struct timeval tv;           /* select�̃^�C���A�E�g����  */
  fd_set select_fd;            /* select�p�r�b�g�}�b�v      */

  /* ���M��A�h���X�̐ݒ� */
  memset(&send_sa, 0, sizeof send_sa);
  send_sa.sin_family      = AF_INET;
  send_sa.sin_addr.s_addr = dst_ip;
  send_sa.sin_port        = htons(dst_port);

  /* UDP�p�P�b�g�̑��M (�w�b�_�̂ݑ��M�A�f�[�^��0�o�C�g) */
  sendto(send_sd, NULL, 0, 0, (struct sockaddr *) &send_sa, sizeof send_sa);

  /* select�̃^�C���A�E�g�̐ݒ� */
  tv.tv_sec  = 1;
  tv.tv_usec = 0;
  while (1) {
    /* select�Ō�������f�B�X�N���v�^�̐ݒ� */
    FD_ZERO(&select_fd);
    FD_SET(recv_sd, &select_fd);

    if (select(recv_sd + 1, &select_fd, NULL, NULL, &tv) > 0) {
      struct icmp *icmp;   /* icmp�w�b�_                    */
      struct ip *ip;       /* IP�w�b�_�\����                */
      int ihlen;           /* IP�w�b�_�̒���                */
      struct ip *ip2;      /* ICMP�f�[�^����IP�w�b�_�\����  */
      int ihlen2;          /* ICMP�f�[�^����IP�w�b�_�̒���  */
      struct udphdr *udp;  /* �|�[�g�ԍ�                    */

      /* ICMP�p�P�b�g�̎�M (56 = 20(IP) + 8(ICMP) + 20(IP) + 8(UDP)) */
      if (recvfrom(recv_sd, buff, MAXBUFF, 0, NULL, NULL) < 56)
        continue;

      ip     = (struct ip *) buff;
      ihlen  = ip->ip_hl << 2;
      icmp   = (struct icmp *) ((char *) ip + ihlen);
      ip2    = (struct ip *) icmp->icmp_data;
      ihlen2 = ip2->ip_hl << 2;
      udp    = (struct udphdr *) ((char *) ip2 + ihlen2);

      /* ��M�����p�P�b�g���|�[�g���B�s�\�Ȃ�while���[�v�𔲂��� */
      if ((ip->ip_src.s_addr == send_sa.sin_addr.s_addr)
          && (icmp->icmp_type == ICMP_UNREACH)
          && (icmp->icmp_code == ICMP_UNREACH_PORT)
          && (ip2->ip_p == IPPROTO_UDP)
          && (udp->uh_dport == send_sa.sin_port))
        break;
    } else
      return CONNECT;
  }
  return NOCONNECT;
}
