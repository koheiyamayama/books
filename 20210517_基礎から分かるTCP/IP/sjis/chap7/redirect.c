/**********************************************************************
 *  ICMP���_�C���N�g�v���O���� (redirect.c)
 *        Ver 2.2 2013�N 7�� 10��
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
#include <netinet/ip_icmp.h>
#define __FAVOR_BSD
#include <netinet/udp.h>
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

enum {CMD_NAME, TARGET_IP, OLD_ROUTER, NEW_ROUTER, DST_IP};

void make_udp_header(struct udphdr *udp);
void make_ip_header(struct ip *ip, int target_ip, int dst_ip, int proto,
                    int iplen);
void make_icmp5_header(struct icmp *icmp, u_int32_t gw_ip);
u_int16_t checksum(u_int16_t *data, int len);

int  main(int argc, char *argv[])
{
  struct sockaddr_in dest;   /* �I�_�z�X�g�̃A�h���X            */
  unsigned char buff[1500];  /* ���M�o�b�t�@                    */
  struct ip *ip_new;         /* IP�w�b�_�ւ̃|�C���^            */
  struct ip *ip_old;         /* IP�w�b�_�ւ̃|�C���^            */
  struct icmp *icmp;         /* ICMP�w�b�_�ւ̃|�C���^          */
  struct udphdr *udp;        /* UDP�w�b�_�ւ̃|�C���^           */
  int s;                     /* �\�P�b�g�t�@�C���f�B�X�N���v�^  */
  int size;                  /* �e��o�C�g��                    */
  int on = 1;                /* ON                              */

  /* �R�}���h���C���������̃`�F�b�N */
  if (argc != 5) {
    fprintf(stderr, "usage %s targetd_host old_router new_router dst_ip\n",
            argv[CMD_NAME]);
    exit(EXIT_FAILURE);
  }

  /* raw�\�P�b�g�̃I�[�v�� */
  if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    perror("socket(SOCK_RAW)");
    exit(EXIT_FAILURE);
  }
  /* IP�w�b�_�����O�ō쐬���� */
  if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof on) < 0) {
    perror("setsockopt(IP_HDRINCL)");
    exit(EXIT_FAILURE);
  }

  /* �e�w�b�_�̃|�C���^�̐ݒ� */
  /* IP(20) + ICMP(8) + IP(20) + UDP(8) */
  ip_new = (struct ip     *) (buff);
  icmp   = (struct icmp   *) (buff + 20);
  ip_old = (struct ip     *) (buff + 20 + 8);
  udp    = (struct udphdr *) (buff + 20 + 8 + 20);
  size   = 20 + 8 + 20 + 8;

  /* �p�P�b�g�̍쐬 */
  make_udp_header(udp);
  make_ip_header(ip_old, inet_addr(argv[TARGET_IP]), inet_addr(argv[DST_IP]),
                 IPPROTO_UDP, 100);
  make_icmp5_header(icmp, inet_addr(argv[NEW_ROUTER]));
  make_ip_header(ip_new, inet_addr(argv[OLD_ROUTER]),
                 inet_addr(argv[TARGET_IP]), IPPROTO_ICMP, size);

  /* ���M�A�h���X�̐ݒ� */
  memset(&dest, 0, sizeof dest);
  dest.sin_family      = AF_INET;
  dest.sin_addr.s_addr = inet_addr(argv[TARGET_IP]);
  CHKADDRESS(dest.sin_addr.s_addr);

  /* �p�P�b�g�̑��M */
  if (sendto(s, buff, size, 0, (struct sockaddr *) &dest, sizeof dest) < 0) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }

  close(s);

  return 0;
}

/*
 * void make_ip_header(struct ip *ip, int target_ip, int dst_ip, int proto,
 *                     int iplen)
 * �@�\
 *     IP�w�b�_�̍쐬
 * ������
 *     struct ip *ip;  �쐬����IP�w�b�_�̐擪�A�h���X
 *     int target_ip;  ���M��IP�A�h���X
 *     int dst_ip;     ���M��IP�A�h���X
 *     int proto;      �v���g�R��
 *     int iplen;      IP�f�[�^�O�����̑S��
 * �߂�l
 *     �Ȃ�
 */
void make_ip_header(struct ip *ip, int target_ip, int dst_ip, int proto,
                    int iplen)
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
  ip->ip_off = htons(IP_DF);
#else
  /* BSD��raw IP�̏ꍇ */
  ip->ip_len = iplen;
  ip->ip_off = IP_DF;
#endif
  ip->ip_ttl = 2;
  ip->ip_p   = proto;
  ip->ip_src.s_addr = target_ip;
  ip->ip_dst.s_addr = dst_ip;

  /* �`�F�b�N�T���̌v�Z (raw ip�̓w�b�_�`�F�b�N�T�����v�Z���Ă���邪�A
     ICMP�f�[�^����IP�w�b�_�܂ł͌v�Z���Ă���Ȃ�) */
  ip->ip_sum = 0;
  ip->ip_sum = checksum((u_int16_t *) ip, sizeof ip);
}

/*
 * void make_icmp5_header(struct icmp *icmp, u_int32_t gw_ip);
 * �@�\
 *     ICMP �^�C�v5(���_�C���N�g)�w�b�_�̍쐬
 * ������
 *     struct icmp *icmp;  �쐬����ICMP�w�b�_�̐擪�A�h���X
 *     int n;              ICMP�G�R�[�v���̃V�[�P���X�ԍ�
 * �߂�l
 *     �Ȃ�
 */
void make_icmp5_header(struct icmp *icmp, u_int32_t gw_ip)
{
  icmp->icmp_type  = ICMP_REDIRECT;
  icmp->icmp_code  = ICMP_REDIRECT_HOST;
  icmp->icmp_gwaddr.s_addr = gw_ip;
  icmp->icmp_cksum = 0;
  icmp->icmp_cksum = checksum((u_int16_t *) icmp, 8 + 20 + 8);
  /* ICMP (8) + IP (20) + UDP (8) */
}

/*
 * void make_udp_header(struct udphdr *udp);
 * �@�\
 *     UDP�w�b�_�̍쐬�B�e�t�B�[���h�̒l�̓_�~�[�B
 * ������
 *     struct udphdr *udp;  �쐬����UDP�w�b�_�̐擪�A�h���X
 * �߂�l
 *     �Ȃ�
 */
void make_udp_header(struct udphdr *udp)
{
  udp->uh_sport = htons(2000);
  udp->uh_ulen  = htons(72);
  udp->uh_dport = htons(33434);
  udp->uh_sum   = htons(9999);
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
