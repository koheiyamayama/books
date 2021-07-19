/**********************************************************************
 *  TCP�T���v���N���C�A���g�v���O���� (tcpv6c.c)
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
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSIZE      8192
#define DEFAULT_PORT "5320"

enum {CMD_NAME, DST_IP, DST_PORT};

int main(int argc, char *argv[])
{
  struct addrinfo info;    /* �ڑ���̏��                 */
  struct addrinfo *res0;   /* �A�h���X���X�g�̐擪�|�C���^ */
  struct addrinfo *res;    /* �A�h���X���X�g               */
  char ip[NI_MAXHOST];     /* IP�A�h���X�𕶎���Ƃ��Ċi�[ */
  int s;                   /* �\�P�b�g�f�B�X�N���v�^       */
  int n;                   /* ���̓f�[�^�̃o�C�g��         */
  int len;                 /* �A�v���P�[�V�����f�[�^�̒��� */
  char send_buf[BUFSIZE];  /* ���M�o�b�t�@                 */
  int error;               /* getaddrinfo�̃G���[�R�[�h    */

  /* �������̃`�F�b�N */
  if (argc != 2 && argc != 3) {
    fprintf(stderr, "Usage: %s  hostname  [port]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* IP�A�h���X�A�|�[�g�ԍ��̐ݒ�(DNS�ւ̖₢���킹) */
  memset(&info, 0, sizeof info);
  info.ai_family   = AF_UNSPEC;
  info.ai_socktype = SOCK_STREAM;
  error = getaddrinfo(argv[DST_IP], (argc == 3) ? argv[DST_PORT] : DEFAULT_PORT,
                      &info, &res0);
  if (error) {
    fprintf(stderr, "'%s' %s\n", argv[DST_IP], gai_strerror(error));
    exit(EXIT_FAILURE);
  }

  /* �R�l�N�V�����̊m�����[�v */
  for (res = res0; res; res = res->ai_next) {
    /* �\�P�b�g���J�� */
    s = -1;
    if ((s = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) >= 0) {
      /* �ڑ�����T�[�o�̃A�h���X�ƃv���g�R���̎�ނ̕\�� */
      if (getnameinfo((struct sockaddr *) res->ai_addr, res->ai_addrlen, ip,
                      sizeof ip, NULL, 0, NI_NUMERICHOST)) {
        fprintf(stderr, "getnameinfo error\n");
        exit(EXIT_FAILURE);
      }
      printf("connecting '%s' ", ip);
      if (res->ai_addr->sa_family == AF_INET6)
        printf("by IPv6. ");
      else if (res->ai_addr->sa_family == AF_INET)
        printf("by IPv4. ");
      else
        printf("by unkown protocol. ");
      fflush(stdout);

      /* �R�l�N�V�����m���v���J�n */
      if (connect(s, res->ai_addr, res->ai_addrlen) >= 0) {
        printf("connected.\n");
        break;
      } else {
        printf("error.\n");
        close(s);
      }
    }
  }
  if (res == NULL) {
    printf("\n");
    fprintf(stderr, "socket or connect error\n");
    exit(EXIT_FAILURE);
  }

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

      /* �X�g���[���^�̃f�[�^�̎�M���� */
      for (i = 0; i < BUFSIZE - 1; i++) {
        if (recv(s, &recv_buf[i], 1, 0) <= 0)
          goto exit_loop;
        if (recv_buf[i] == '\n')  /* ���s�P�ʂŎ�M���������� */
          break;
      }
      if (i >= 1 && recv_buf[i - 1] == '\r')  /* ���s��CR�ALF�̎��̏��� */
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
  freeaddrinfo(res0);
  printf("connection closed.\n");

  return 0;
}
