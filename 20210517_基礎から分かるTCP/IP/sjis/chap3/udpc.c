/**********************************************************************
 *  UDP�T���v���N���C�A���g (udpc.c)
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>

#define MSGSIZE      1024
#define BUFSIZE      (MSGSIZE + 1)
#define DEFAULT_PORT 5320

enum {CMD_NAME, DST_IP, DST_PORT};

int main(int argc, char *argv[])
{
  struct sockaddr_in server;  /* �T�[�o�̃A�h���X        */
  unsigned long dst_ip;       /* �T�[�o��IP�A�h���X      */
  int port;                   /* �|�[�g�ԍ�              */
  int s;                      /* �\�P�b�g�f�B�X�N���v�^  */
  unsigned int zero;          /* �[��                    */

  /* �������̃`�F�b�N */
  if (argc != 2 && argc != 3) {
    fprintf(stderr, "Usage: %s  hostname  [port]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* �T�[�o��IP�A�h���X�𒲂ׂ� */
  if ((dst_ip = inet_addr(argv[DST_IP])) == INADDR_NONE) {
    struct hostent *he;  /* �z�X�g��� */

    if ((he = gethostbyname(argv[DST_IP])) == NULL) {
      fprintf(stderr, "gethostbyname error\n");
      exit(EXIT_FAILURE);
    }
    memcpy((char *) &dst_ip, (char *) he->h_addr, he->h_length);
  }

  /* �T�[�o�̃|�[�g�ԍ��𒲂ׂ� */
  if (argc == 3) {
    if ((port = atoi(argv[DST_PORT])) == 0) {
      struct servent *se;  /* �T�[�r�X��� */

      if ((se = getservbyname(argv[DST_PORT], "udp")) != NULL)
        port = (int) ntohs((u_int16_t) se->s_port);
      else {
        fprintf(stderr, "getservbyname error\n");
        exit(EXIT_FAILURE);
      }
    }
  } else
    port = DEFAULT_PORT;

  /* UDP�Ń\�P�b�g���J�� */
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* �T�[�o�̃A�h���X��ݒ肷�� */
  memset(&server, 0, sizeof server);
  server.sin_family      = AF_INET;
  server.sin_addr.s_addr = dst_ip;
  server.sin_port        = htons(port);

  /*
   * �N���C�A���g�������C�����[�`��
   */
  zero = 0;
  printf("UDP>");
  fflush(stdout);

  while (1) {
    char buf[BUFSIZE];  /* ��M�o�b�t�@                   */
    char cmd[BUFSIZE];  /* ���M�o�b�t�@                   */
    int n;              /* ���̓f�[�^�̃o�C�g��           */
    struct timeval tv;  /* select�̃^�C���A�E�g����       */
    fd_set readfd;      /* select�Ō��o����f�B�X�N���v�^ */

    /* select�̃^�C���A�E�g�̐ݒ� */
    tv.tv_sec  = 600;
    tv.tv_usec = 0;

    /* select�ɂ��C�x���g�҂� */
    FD_ZERO(&readfd);
    FD_SET(0, &readfd);  /* �W������ */
    FD_SET(s, &readfd);  /* �T�[�o   */
    if ((select(s + 1, &readfd, NULL, NULL, &tv)) <= 0) {
      fprintf(stderr, "\nTimeout\n");
      break;
    }

    /* �W�����͂���̓��̓R�}���h�̏��� */
    if (FD_ISSET(0, &readfd)) {
      if ((n = read(0, buf, BUFSIZE - 1)) <= 0)
        break;
      buf[n] = '\0';
      sscanf(buf, "%s", cmd);
      if (strcmp(cmd, "quit") == 0)
        break;
      if (sendto(s, buf, n, 0, (struct sockaddr *) &server, sizeof server) < 0)
        break;
    }

    /* �T�[�o����̉������b�Z�[�W�̏��� */
    if (FD_ISSET(s, &readfd)) {
      if ((n = recvfrom(s, buf, MSGSIZE, 0, (struct sockaddr *) 0, &zero)) < 0)
        break;
      buf[n] = '\0';
      printf("%s", buf);
      if (n < MSGSIZE)
        printf("UDP>");
      fflush(stdout);
    }
  }
  close(s);

  return 0;
}
