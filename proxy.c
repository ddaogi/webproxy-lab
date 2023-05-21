/*. 
First part - involve learning about basic HTTP operation , how to use sockets to write programs that communicate over network connections.
Second part - upgrade proxy to deal with multiple concurrent connections
Third part - add caching to your proxy using a simple main memory cache of recently accessed web content (LRU)
*/


#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void read_requesthdrs(rio_t *rp);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main() {
  printf("%s", user_agent_hdr);
  return 0;
}

void read_requesthdrs(rio_t *rp){
  char buf[MAXLINE];

  Rio_readlineb(rp,buf,MAXLINE);
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char* hostname, int *port){

  char tmp[MAXLINE]; //hold local copy of uri
  char *buf; // ptr that traverses uri
  char *endbuf; // ptr to end of the cmdline string
  int port_tmp[10];
  int i,j;
  char num[2]; //store port value

  buf = tmp;
  for( i=0; i<10; i++){
    port_tmp[1] = 0;
  }
  (void) strncpy(buf,uri,MAXLINE);
  endbuf = buf + strlen(buf);
  buf += 7;   // 'http://' has 7 characters
  while(buf < endbuf){
    //take host name out
    if( buf >= endbuf){
      strcpy(uri, "");
      strcat(hostname, " ");
      break;
    }
    if (*buf == ':'){
      buf++;
      *port = 0;
      i = 0;
      while(*buf != '/'){
        num[0] = *buf;
        num[1] = '\0';
        port_tmp[i] = atoi(num);
        buf++;
        i++;
      }
      j =0;
      while( i > 0){
        *port += port_tmp[j] * powerten(i-1);
        j++;
        i--;
      }
    }
    if(*buf != '/'){
      sprintf(hostname, "%s%c", hostname, *buf);
    }
    else{
      strcat(hostname, "\0");
      strcpy(uri,buf);
      break;
    }
    buf++;
  }
  return 1;
}

int powerten(int i){
  int num =1;
  while(i>0){
    num *= 10;
    --;
  }
  return num;
}

void clienterror(int fd, char *cause, char *errnum, char*shortmsg, char *longmsg){
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}