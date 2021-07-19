/**********************************************************************
 *  TCP�T���v���T�[�o�v���O���� (tcps.c)
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
#ifdef FORK_SERVER
#include <signal.h>
#include <sys/wait.h>
#endif

#define BUFSIZE      8192
#define BUFSIZE2     32768
#define DEFAULT_PORT 5320

/* ���[�e�B���O�e�[�u����\��������R�}���h */
#ifdef __linux
#define COMMAND_ROUTE "/bin/netstat -rn"
#else
#define COMMAND_ROUTE "netstat -rn"
#endif

/* ARP�e�[�u����\��������R�}���h */
#ifdef __linux
#define COMMAND_ARP "/sbin/arp -an"
#else
#define COMMAND_ARP "arp -an"
#endif

/* TCP�R�l�N�V�����e�[�u����\��������R�}���h */
#ifdef __linux
#define COMMAND_TCP "/bin/netstat -tn"
#else
#define COMMAND_TCP "netstat -n"
#endif

/* NIC�̏���\��������R�}���h */
#ifdef __linux
#define COMMAND_NIC "/sbin/ifconfig -a"
#else
#define COMMAND_NIC "ifconfig -a"
#endif

enum {CMD_NAME, SRC_PORT};

int execute(char *command, char *buf, int bufmax);

#ifdef FORK_SERVER
void reaper()
{
  int status;
  wait(&status);
}
#endif

int main(int argc, char *argv[])
{
  struct sockaddr_in server;  /* �T�[�o�̃A�h���X                     */
  struct sockaddr_in client;  /* �N���C�A���g�̃A�h���X               */
  unsigned int len;           /* sockaddr_in�̒���                    */
  int port;                   /* �T�[�o�̃|�[�g�ԍ�                   */
  int s;                      /* ���b�Z�[�W����M�p�f�B�X�N���v�^     */
  int s0;                     /* �R�l�N�V�����󂯕t���p�f�B�X�N���v�^ */

  /* �������̏���(�|�[�g�ԍ�) */
  if (argc == 2) {
    if ((port = atoi(argv[SRC_PORT])) == 0) {
      struct servent *se;  /* �T�[�r�X��� */

      if ((se = getservbyname(argv[SRC_PORT], "tcp")) != NULL)
        port = (int) ntohs((u_int16_t) se->s_port);
      else {
        fprintf(stderr, "getservbyname error\n");
        exit(EXIT_FAILURE);
      }
    }
  } else
    port = DEFAULT_PORT;

  /* TCP�Ń\�P�b�g���I�[�v������ */
  if ((s0 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* �T�[�o�̃A�h���X��ݒ肷�� */
  memset(&server, 0, sizeof server);
  server.sin_family      = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port        = htons(port);
  if (bind(s0, (struct sockaddr *) &server, sizeof server) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  /* �R�l�N�V�����m���v����t�J�n */
  if (listen(s0, 5) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

#ifdef FORK_SERVER
  signal(SIGCHLD, (void *) reaper);
#endif

  /*
   * �R�l�N�V�����󂯕t�����[�v 
   */
  while (1) {
    /* �R�l�N�V������t���� */
    len = sizeof client;
    if ((s = accept(s0, (struct sockaddr *) &client, &len)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    /* �ڑ������N���C�A���g��IP�A�h���X�̕\�� */
    printf("connected from '%s'\n", inet_ntoa(client.sin_addr));

#ifdef FORK_SERVER
    if (fork() != 0) {
      close(s);
      continue;
    }
    close(s0);
#endif

    /* �T�[�o�������C�����[�`�� */
    while (1) {
      char recv_buf[BUFSIZE];    /* ��M�o�b�t�@               */
      char cmd1[BUFSIZE];        /* 1���[�h�ڂ̃R�}���h        */
      char cmd2[BUFSIZE];        /* 2���[�h�ڂ̃R�}���h        */
      int cn;                    /* ��M�����R�}���h�̃��[�h�� */
      int i;                     /* ��M�����̃J�E���g         */
      char send_head[BUFSIZE];   /* ���M�f�[�^�̃w�b�_         */
      char send_data[BUFSIZE2];  /* ���M�o�b�t�@               */
      int hn;                    /* ���M�w�b�_�̃o�C�g��       */
      int dn;                    /* ���M�f�[�^�̃o�C�g��       */

      /* �R�}���h�̎�M */
      recv_buf[0] = '\0';
      for (i = 0; i < BUFSIZE - 1; i++) {
        if (recv(s, &recv_buf[i], 1, 0) <= 0)
          goto exit_loop;
        if (recv_buf[i] == '\n')  /* ���s�P�ʂŎ�M���������� */
          break;
      }
      recv_buf[i] = '\0';
      printf("receive '%s'\n", recv_buf);

      /* ��M�R�}���h�̏��� */
      if ((cn = sscanf(recv_buf, "%s%s", cmd1, cmd2)) <= 0)
        continue;
      if (cn == 2 && strcmp(cmd1, "show") == 0) {
        if (strcmp(cmd2, "route") == 0)
          dn = execute(COMMAND_ROUTE, send_data, BUFSIZE2);
        else if (strcmp(cmd2, "arp") == 0) 
          dn = execute(COMMAND_ARP, send_data, BUFSIZE2);
        else if (strcmp(cmd2, "tcp") == 0)
          dn = execute(COMMAND_TCP, send_data, BUFSIZE2);
        else if (strcmp(cmd2, "nic") == 0)
          dn = execute(COMMAND_NIC, send_data, BUFSIZE2);
        else
          dn = snprintf(send_data, BUFSIZE2, "parameter error '%s'\n"
                                           "show [route|arp|tcp|nic]\n", cmd2);
      } else {
        if (strcmp(cmd1, "quit") == 0)
          goto exit_loop;

        send_data[0] = '\0';
        if (cn != 1 && strcmp(cmd1, "help") != 0)
          snprintf(send_data, BUFSIZE2 - 1, "command error '%s'\n", cmd1);
        strncat(send_data, "Command:\n"
                           "  show route\n"
                           "  show arp\n"
                           "  show tcp\n"
                           "  show nic\n"
                           "  quit\n"
                           "  help\n", BUFSIZE2 - strlen(send_data) - 1);
        dn = strlen(send_data);
      }

      /* �A�v���P�[�V�����w�b�_�̑��M */
      hn = snprintf(send_head, BUFSIZE - 1, "Content-Length: %d\n\n", dn);
      if (send(s, send_head, hn, 0) < 0)
        break;
      send_head[hn] = '\0';
      printf("%s\n", send_head);

      /* �A�v���P�[�V�����f�[�^�̑��M */
      if (send(s, send_data, dn, 0) < 0)
        break;
      printf("%s\n", send_data);
    }
  exit_loop:
    printf("connection closed.\n");
    close(s);
#ifdef FORK_SERVER
    return 0;
#endif
  }
  close(s0);

  return 0;
}

/*
 * int execute(char *command, char *buf, int bufmax);
 *
 * �@�\
 *     �R�}���h�����s���A�o�͌��ʂ��o�b�t�@�Ɋi�[����
 * ������ 
 *     char *command;  ���s����R�}���h
 *     char *buf;      �o�͌��ʂ��i�[����o�b�t�@
 *     int bufmax;     �o�b�t�@�̑傫��
 * �߂�l
 *     int             �o�b�t�@�Ɋi�[����������
 */
int execute(char *command, char *buf, int bufmax)
{
  FILE *fp;  /* �t�@�C���|�C���^         */
  int i;     /* ���͂����f�[�^�̃o�C�g�� */

  if ((fp = popen(command, "r")) == NULL) {
    perror(command);
    i = snprintf(buf, BUFSIZE, "server error: '%s' cannot execute.\n", command);
  } else {
    i = 0;
    while (i < bufmax - 1 && fscanf(fp, "%c", &buf[i]) == 1) 
      i++;
    buf[i] = '\0';
    pclose(fp);
  }
  return i;
}
