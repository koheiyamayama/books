/**********************************************************************
 *  UDP�T���v���T�[�o (udps.c)
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

#define BUFSIZE      32768
#define MSGSIZE      1024
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

enum {CMD_NAME, DST_PORT};

int execute(char *command, char *buf, int bufmax);

int main(int argc, char *argv[])
{
  struct sockaddr_in server;  /* �T�[�o�̃A�h���X           */
  struct sockaddr_in client;  /* �N���C�A���g�̃A�h���X     */
  unsigned int len;           /* sockaddr_in�̒���          */
  int port;                   /* �T�[�o�̃|�[�g�ԍ�         */
  int s;                      /* �\�P�b�g�f�B�X�N���v�^     */
  int cn;                     /* ��M�����R�}���h�̃��[�h�� */
  int sn;                     /* ���M���b�Z�[�W�̃o�C�g��   */
  int rn;                     /* ��M���b�Z�[�W�̃o�C�g��   */
  int i;                      /* ���[�v�ϐ�                 */
  char cmd1[BUFSIZE];         /* 1���[�h�ڂ̃R�}���h        */
  char cmd2[BUFSIZE];         /* 2���[�h�ڂ̃R�}���h        */
  char recv_buf[BUFSIZE];     /* ��M�o�b�t�@               */
  char send_buf[BUFSIZE];     /* ���M�o�b�t�@               */

  /* �������̏���(�|�[�g�ԍ�) */
  if (argc == 2) {
    if ((port = atoi(argv[DST_PORT])) == 0) {
      struct servent *se;     /* �T�[�r�X��� */

      if ((se = getservbyname(argv[DST_PORT], "udp")) != NULL)
        port = (int) ntohs((u_int16_t) se->s_port);
      else {
        fprintf(stderr, "getservbyname error\n");
        exit(EXIT_FAILURE);
      }
    }
  } else
    port = DEFAULT_PORT;

  /* UDP�Ń\�P�b�g���I�[�v������ */
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* �T�[�o�̃A�h���X��ݒ肷�� */
  memset(&server, 0, sizeof server);
  server.sin_family        = AF_INET;
  server.sin_addr.s_addr   = htonl(INADDR_ANY);
  server.sin_port          = htons(port);
  if (bind(s, (struct sockaddr *) &server, sizeof server) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  /* �T�[�o�������C�����[�`�� */
  while ((rn = recvfrom(s, recv_buf, BUFSIZE - 1, 0,
                        (struct sockaddr *) &client,
                        (len = sizeof client, &len))) >= 0) {
    recv_buf[rn] = '\0';
    printf("receive from '%s'\n", inet_ntoa(client.sin_addr));
    printf("receive data '%s'\n", recv_buf);

    /* ��M�R�}���h�̏��� */
    if ((cn = sscanf(recv_buf, "%s%s", cmd1, cmd2)) <= 0)
      sn = 0; /* continue; */
    else if (cn == 2 && strcmp(cmd1, "show") == 0) {
      if (strcmp(cmd2, "route") == 0)
        sn = execute(COMMAND_ROUTE, send_buf, BUFSIZE);
      else if (strcmp(cmd2, "arp") == 0)
        sn = execute(COMMAND_ARP, send_buf, BUFSIZE);
      else if (strcmp(cmd2, "tcp") == 0)
        sn = execute(COMMAND_TCP, send_buf, BUFSIZE);
      else if (strcmp(cmd2, "nic") == 0)
        sn = execute(COMMAND_NIC, send_buf, BUFSIZE);
      else
        sn = snprintf(send_buf, BUFSIZE, "parameter error '%s'\n"
                                         "show [route|arp|tcp|nic]\n", cmd2);
    } else {
      if (strcmp(cmd1, "quit") == 0)
        break;

      send_buf[0] = '\0';
      if (cn != 1 && strcmp(cmd1, "help") != 0)
        snprintf(send_buf, BUFSIZE, "command error '%s'\n", cmd1);
      strncat(send_buf, "Command:\n"
                        "  show route\n"
                        "  show arp\n"
                        "  show tcp\n"
                        "  show nic\n"
                        "  quit\n"
                        "  help\n", BUFSIZE - strlen(send_buf) - 1);
      sn = strlen(send_buf);
    }

    /* ���b�Z�[�W���M���[�v (�Ō�ɕK��MSGSIZE�����̃p�P�b�g�𑗐M����) */
    for (i = 0; i <= sn; i += MSGSIZE) {
      int size;  /* ���M���郁�b�Z�[�W�̃T�C�Y */

      if (sn - i > MSGSIZE)
        size = MSGSIZE;
      else if (sn - i <= 0)
        size = 0;
      else
        size = sn - i;
      if (sendto(s,&send_buf[i],size,0,(struct sockaddr *)&client,len) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
      }
    }
    printf("%s", send_buf);
  }
  close(s);

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
