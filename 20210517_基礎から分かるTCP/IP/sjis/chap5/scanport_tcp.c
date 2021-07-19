/**********************************************************************
 *  TCP�|�[�g�X�L�����v���O���� (scanport_tcp.c)
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
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

enum {CMD_NAME, DST_IP, START_PORT, LAST_PORT};
enum {CONNECT, NOCONNECT};

int tcpportscan(u_int32_t dst_ip, int dst_port);

int main(int argc, char *argv[]) 
{
  u_int32_t dst_ip;  /* �I�_IP�A�h���X          */
  int dst_port;      /* �I�_�|�[�g�ԍ�          */
  int start_port;    /* �X�L�����J�n�|�[�g�ԍ�  */
  int end_port;      /* �X�L�����I���|�[�g�ԍ�  */

  /* �R�}���h���C���������̃`�F�b�N */
  if (argc != 4) {
    fprintf(stderr, "usage: %s dst_ip start_port last_port\n", argv[CMD_NAME]);
    exit(EXIT_FAILURE);
  }

  /* �R�}���h���C������������l���Z�b�g���� */
  dst_ip     = inet_addr(argv[DST_IP]);
  start_port = atoi(argv[START_PORT]);
  end_port   = atoi(argv[LAST_PORT]);

  CHKADDRESS(dst_ip);

  /* ���C�����[�`�� */
  for (dst_port = start_port; dst_port <= end_port; dst_port++) {
    printf("Scan Port %d\r", dst_port);
    fflush(stdout);

    if (tcpportscan(dst_ip, dst_port) == CONNECT) {
      struct servent *se;  /* �T�[�r�X��� */

      se = getservbyport(htons(dst_port), "tcp");
      printf("%5d %-20s\n", dst_port, (se == NULL) ? "unknown" : se->s_name);
    }
  }

  return 0;
}

/*
 * int tcpportscan(u_int32_t dst_ip, int dst_port)
 * �@�\
 *     �w�肵��IP�A�h���X�A�|�[�g�ԍ���TCP�R�l�N�V�������m���ł��邩���ׂ�B
 * ������ 
 *     u_int32_t dst_ip;  ���M��IP�A�h���X (�l�b�g���[�N�o�C�g�I�[�_�[)
 *     int dst_port;      ���M��|�[�g�ԍ�
 * �߂�l
 *     int                CONNECT  ... �R�l�N�V�������m���ł���
 *                        NOCONNECT .. �R�l�N�V�������m���ł��Ȃ�����
 */
int tcpportscan(u_int32_t dst_ip, int dst_port)
{
  struct sockaddr_in dest;  /* �\�P�b�g�A�h���X       */
  int s;                    /* �\�P�b�g�f�B�X�N���v�^ */
  int ret;                  /* ���̊֐��̖߂�l       */

  /* �I�_�A�h���X�̐ݒ� */
  memset(&dest, 0, sizeof dest);
  dest.sin_family      = AF_INET;
  dest.sin_port        = htons(dst_port);
  dest.sin_addr.s_addr = dst_ip;

  /* TCP�\�P�b�g�̃I�[�v�� */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* TCP�R�l�N�V�����m���v�� */
  if (connect(s, (struct sockaddr *) &dest, sizeof dest) < 0)
    ret = NOCONNECT;
  else
    ret = CONNECT;
  close(s);

  return ret;
}
