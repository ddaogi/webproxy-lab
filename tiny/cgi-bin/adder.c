/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1=0, n2=0;
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);
    n1 = atoi(strchr(arg1, '=') + 1);
    n2 = atoi(strchr(arg2, '=') + 1);
  }
  /*Make the response body */
  
  // sprintf(content, "QUERY_STRING=%s \r\n<p>", buf);
  sprintf(content, "Welcome to add.com:");
  sprintf(content, "%sTHE Internet addition portal. \r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d \r\n<p>",
        content, n1, n2, n1+n2);
  sprintf(content, "%sThanks for visiting!!!\r\n", content);
    

  /* Generate the HTTP response (HEADER+  BODY)*/

  // 이건 telnet으로 접근식 터미널에만 출력됨
  // content를 html로 보내주는건 뭐지?
  // 전체를 보내주지만, 브라우저에 헤더는 출력하지 않음..
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  //헤더와 바디를 구분하는건 \r\n\r\n

  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
