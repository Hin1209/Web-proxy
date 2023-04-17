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
  char buf[MAXLINE], bbuf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], host[MAXLINE], port[MAXLINE];
  char *p, *srcp, *readp, *portp, *pathp, *hostp;
  rio_t rio, rio_client;

  Rio_readinitb(&rio_client, fd);
  Rio_readlineb(&rio_client, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);
  portp = strchr(uri + 5, ':');
  pathp = strchr(portp, '/');
  int idx = 0;
  for (char *i = portp + 1; i < pathp; i++)
    port[idx++] = *i;
  hostp = strchr(uri, '/');
  hostp = hostp + 2;
  idx = 0;
  for (char *i = hostp; i < portp; i++)
    host[idx++] = *i;

  proxyfd = Open_clientfd(host, port);
  Rio_readinitb(&rio, proxyfd);
  sprintf(bbuf, "%s %s HTTP/1.0\r\n", method, pathp);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(&rio_client, buf, MAXLINE);
    if (strstr(buf, "User-Agent"))
      sprintf(bbuf, "%s%s", bbuf, user_agent_hdr);
    else
      sprintf(bbuf, "%s%s", bbuf, buf);
  }
  Rio_writen(proxyfd, bbuf, strlen(bbuf));
  Rio_readlineb(&rio, buf, MAXLINE);
  sprintf(bbuf, "");
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
  srcp = Malloc(filesize);
  Rio_readnb(&rio, srcp, filesize);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
  Close(proxyfd);
}