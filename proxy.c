#include <stdio.h>
#include <pthread.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void *doit(void *vargp);
void parse_uri(char *uri, char *host, char *port, char **path);

int main(int argc, char **argv)
{
  pthread_t tid;
  int listenfd, *connfd;
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
    connfd = Malloc(sizeof(int));
    *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    Pthread_create(&tid, NULL, doit, connfd);
  }
  return 0;
}

void *doit(void *vargp)
{
  int proxyfd, fd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], host[MAXLINE], port[MAXLINE];
  char *srcp, *path;
  rio_t rio, rio_client;
  fd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);

  Rio_readinitb(&rio_client, fd);
  Rio_readlineb(&rio_client, buf, MAXLINE);

  sscanf(buf, "%s %s %s", method, uri, version);
  parse_uri(uri, host, port, &path);

  proxyfd = Open_clientfd(host, port);
  Rio_readinitb(&rio, proxyfd);
  transfer_request(&rio_client, proxyfd, method, path, buf);
  transfer_response(&rio, fd, buf);

  free(srcp);
  Close(proxyfd);
  Close(fd);
}

void parse_uri(char *uri, char *host, char *port, char **path)
{
  char *portp, *hostp;
  int idx = 0;

  portp = strchr(uri + 5, ':');
  *path = strchr(portp, '/');

  for (char *i = portp + 1; i < *path; i++)
    port[idx++] = *i;

  hostp = strchr(uri, '/');
  hostp = hostp + 2;
  idx = 0;

  for (char *i = hostp; i < portp; i++)
    host[idx++] = *i;
}

void transfer_request(rio_t *rio_client, int proxyfd, char *method, char *path, char *buf)
{
  char buf_cat[MAXLINE];
  sprintf(buf_cat, "%s %s HTTP/1.0\r\n", method, path);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rio_client, buf, MAXLINE);
    if (strstr(buf, "User-Agent"))
      sprintf(buf_cat, "%s%s", buf_cat, user_agent_hdr);
    else
      sprintf(buf_cat, "%s%s", buf_cat, buf);
  }
  Rio_writen(proxyfd, buf_cat, strlen(buf_cat));
}

void transfer_response(rio_t *rio, int fd, char *buf)
{
  int filesize;
  char buf_cat[MAXLINE], *p, *srcp;

  Rio_readlineb(rio, buf, MAXLINE);
  sprintf(buf_cat, "");
  while (strcmp(buf, "\r\n"))
  {
    sprintf(buf_cat, "%s%s", buf_cat, buf);
    if (strstr(buf, "Content-length"))
    {
      p = strchr(buf, ':');
      filesize = atoi(p + 1);
    }
    Rio_readlineb(rio, buf, MAXLINE);
  }
  sprintf(buf_cat, "%s\r\n", buf_cat);
  Rio_writen(fd, buf_cat, strlen(buf_cat));
  sprintf(buf_cat, "");
  srcp = Malloc(filesize);
  Rio_readnb(rio, srcp, filesize);
  Rio_writen(fd, srcp, filesize);
}