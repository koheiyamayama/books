/*****************************************************************
 *  TCP�T���v���N���C�A���g�v���O���� (tcpc.c)
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
 ****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE      8192
#define DEFAULT_PORT 5320

enum {CMD_NAME, DST_IP, DST_PORT};

int main(int argc, char *argv[])
{
  struct sockaddr_in server;  /* �T�[�o�̃A�h���X             */
  unsigned long dst_ip;       /* �T�[�o��IP�A�h���X           */
  int port;                   /* �|�[�g�ԍ�                   */
  int s;                      /* �\�P�b�g�f�B�X�N���v�^       */
  int n;                      /* ���̓f�[�^�̃o�C�g��         */
  int len;                    /* �A�v���P�[�V�����f�[�^�̒��� */
  char send_buf[BUFSIZE];     /* ���M�o�b�t�@                 */

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

      if ((se = getservbyname(argv[DST_PORT], "tcp")) != NULL)
        port = (int) ntohs((u_int16_t) se->s_port);
      else {
        fprintf(stderr, "getservbyname error\n");
        exit(EXIT_FAILURE);
      }
    }
  } else
    port = DEFAULT_PORT;

  /* TCP�Ń\�P�b�g���J�� */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* �T�[�o�̃A�h���X��ݒ肵�R�l�N�V�������m������ */
  memset(&server, 0, sizeof server);
  server.sin_family      = AF_INET;
  server.sin_addr.s_addr = dst_ip;
  server.sin_port        = htons(port);
  if (connect(s, (struct sockaddr *) &server, sizeof server) < 0) {
    perror("connect");
    exit(EXIT_FAILURE);
  }
  printf("connected to '%s'\n", inet_ntoa(server.sin_addr));

  /* 
   * �N���C�A���g�������C�����[�`��
   */
  while (1) {
    char cmd[BUFSIZE];       /* ���̓R�}���h�̌����p */
    char recv_buf[BUFSIZE];  /* ��M�o�b�t�@         */

    /*
     * �R�}���h���͏����E���M����
     */
    printf("TCP>");
    fflush(stdout);

    /* �R�}���h�̓��� */
    if (fgets(send_buf, BUFSIZE - 2, stdin) == NULL)
      break;
    cmd[0] = '\0';
    sscanf(send_buf, "%s", cmd);
    if (strcmp(cmd, "quit") == 0)
      break;
    if (strcmp(cmd, "") == 0)
      strcpy(send_buf, "help\n");

#ifdef HTTP
    strncat(send_buf, "Connection: keep-alive\n\n", BUFSIZE
                                       - strlen(send_buf) - 1);
#endif

    /* �R�}���h�̑��M */
    if (send(s, send_buf, strlen(send_buf), 0) <= 0) {
      perror("send");
      break;
    }

    /*
     * �A�v���P�[�V�����w�b�_�̎�M�E���
     */
    len = -1;
    while (1) {
      char *cmd1;  /* �R�}���h��1���[�h�� */
      char *cmd2;  /* �R�}���h��2���[�h�� */
      int i;       /* ��M�����̃J�E���g  */

      /* �X�g���[���^���b�Z�[�W�̎�M���� */
      for (i = 0; i < BUFSIZE - 1; i++) {
        if (recv(s, &recv_buf[i], 1, 0) <= 0)
          goto exit_loop;
        if (recv_buf[i] == '\n')  /* ���s�P�ʂŎ�M���������� */
          break;
      }
      if (i >= 1 && recv_buf[i - 1] == '\r') /* ���s��CR�ALF�̎��̏��� */
        i--;
      if (i == 0)  /* ��s�̏ꍇ�́A�w�b�_���I���A�f�[�^�����n�܂� */
        break;

      /* �w�b�_�̉�͏��� */
      recv_buf[i] = '\0';
      cmd1 = strtok(recv_buf, ": ");
      cmd2 = strtok(NULL, " \0");
#ifdef DEBUG
      printf("[%s, %s]\n", cmd1, cmd2);
#endif
      if (strcmp("Content-Length", cmd1) == 0)
        len = atoi(cmd2);
    }

#ifdef HTTP
    if (len == -1) {
      while ((n = recv(s, recv_buf, BUFSIZE - 1, 0)) > 0) {
        recv_buf[n] = '\0';
        printf("%s", recv_buf);
        fflush(stdout);
      }
      close(s);
      return 0;
    }
#endif

    /*
     * �A�v���P�[�V�����f�[�^�̎�M�A��ʂւ̏o��
     */
    while (len > 0) {
      if ((n = recv(s, recv_buf, BUFSIZE - 1, 0)) <= 0)
        goto exit_loop;
      recv_buf[n] = '\0';
      len -= n;
      printf("%s", recv_buf);
      fflush(stdout);
    }
  }
 exit_loop:
  n = snprintf(send_buf, BUFSIZE, "quit\n");
  send(s, send_buf, n, 0);
  close(s);

  return 0;
}
