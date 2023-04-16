#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  printf("%s", user_agent_hdr);

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    Close(connfd);
  }
  return 0;
}

void doit(int fd)
{
  int proxyfd, filesize, tmp, n;
  char *host, *port, *p, buf[MAXLINE], bbuf[MAXLINE];
  rio_t rio, rio_client;

  host = "localhost";
  port = "8000";

  proxyfd = Open_clientfd(host, port);
  Rio_readinitb(&rio_client, fd);
  Rio_readinitb(&rio, proxyfd);
  Rio_readlineb(&rio_client, buf, MAXLINE);
  sprintf(bbuf, "%s", buf);
  Rio_readlineb(&rio_client, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(&rio_client, buf, MAXLINE);
    sprintf(bbuf, "%s%s", bbuf, buf);
  }
  Rio_writen(proxyfd, bbuf, strlen(bbuf));
  Rio_readlineb(&rio, buf, MAXLINE);
  sprintf(bbuf, "");
  int cnt = 0;
  while (strcmp(buf, "\r\n"))
  {
    sprintf(bbuf, "%s%s", bbuf, buf);
    if (strstr(buf, "Content-length"))
    {
      p = strchr(buf, ':');
      filesize = atoi(p + 1);
    }
    Rio_readlineb(&rio, buf, MAXLINE);
  }
  sprintf(bbuf, "%s\r\n", bbuf);
  Rio_writen(fd, bbuf, strlen(bbuf));
  sprintf(bbuf, "");
  tmp = filesize;
  while ((n = Rio_readlineb(&rio, buf, MAXLINE)) < tmp)
  {
    sprintf(bbuf, "%s%s", bbuf, buf);
    tmp -= n;
  }
  Rio_writen(fd, bbuf, filesize);
  Close(proxyfd);
}