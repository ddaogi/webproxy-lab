/*. 
First part - involve learning about basic HTTP operation , how to use sockets to write programs that communicate over network connections.
Second part - upgrade proxy to deal with multiple concurrent connections
Third part - add caching to your proxy using a simple main memory cache of recently accessed web content (LRU)
*/


#include <stdio.h>
#include "csapp.h"
#include "cache.h"
/* Recommended max cache and object sizes */

void read_requesthdrs(rio_t *rp);
void proxy(int connfd);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main() {
  printf("%s", user_agent_hdr);
  return 0;
}

void doit(int clientfd){
  int serverfd, content_length;
  char request_buf[MAXLINE], response_buf[MAXLINE];
  char method[MAXLINE], uri[MAXLINE], path[MAXLINE], hostname[MAXLINE], port[MAXLINE];
  char *response_ptr, filename[MAXLINE], cgiargs[MAXLINE];
  rio_t request_rio, response_rio;



  /*
  1-1 Request Line 읽기 (Client -> Proxy)
  */
  Rio_readinitb(&request_rio, clientfd);
  Rio_readlineb(&request_rio, request_buf, MAXLINE);
  printf("Request headers:\n %s\n", request_buf);

  // 요청 라인 parsing 을 통해 hostname, port, path 저장
  sscanf(request_buf, "%s %s", method, uri); // sscanf는 request_buf를 method랑 uri에 저장
  my_parse_uri(uri, hostname, port, path);

  // Server에 전송하기 위해 요청 라인의 형식 변경: `method uri version` -> `method path HTTP/1.0`
  sprintf(request_buf, "%s %s %s\r\n", method, path, "HTTP/1.0"); // sprintf는 method path http/1.0 을 request_buf에 저장


  /*
  1-2 Request Line 보내기 (Proxy -> Server)
  */
  serverfd = Open_clientfd(hostname,port);
  if (serverfd < 0)
  {
    clienterror(serverfd, method, "502", "Bad Gateway", "📍 Failed to establish connection with the end server");
    return;
  }
  Rio_writen(serverfd,request_buf, strlen(request_buf));
  
  /*
  2 Request Header 읽기, 전송 (client -> proxy -> server)
  */
  read_requesthdrs(&request_rio, request_buf, serverfd, hostname, port);

  /*
  3 Response Header 읽기 ,전송 (server -> proxy -> client)
  */
  Rio_readinitb(&response_rio, serverfd);
  while(strcmp(response_buf, "\r\n")){
    Rio_readlineb(&response_rio, response_buf, MAXLINE);
    if(strstr(response_buf, "Content-length"))
      content_length = atoi(strchr(response_buf, ':') + 1 );
    Rio_writen(clientfd,response_buf,strlen(response_buf));
  }

  /*
  4 Response Body 읽기 & 전송 (server -> proxy -> client)
  */
  response_ptr = malloc(content_length);
  Rio_readnb(&response_rio, response_ptr, content_length);
  Rio_writen(clientfd, response_ptr, content_length);

  //캐싱 가능한 사이즈일 경우
  if(content_length <= MAX_OBJECT_SIZE){

  }
  



  




}

void proxy(int connfd){
  rio_t fromcli_rio;
  HttpRequest request;

  // 클라이언트에서 프록시 서버로 요청
  rio_readinitb(&fromcli_rio, connfd);
  if(parse_http_request(&fromcli_rio, &request) == -1){
    // HTTP request 파싱에 실패했으면 에러메시지를 띄움
    rio_writen(connfd, bad_request_response, strlen(bad_request_response));
    return;
  }
  
  /* ifndef DEBUG - client 의 host와 port를 출력 */
  debug_printf("Host: %s, Port: %d\n", request.host, request.port);

  // proxy ----- (request) ----> server
  //     <-------- (response)
  // client<-----(response) ---- proxy
  // 클라이언트의 요청을 엔드서버로 전달하고, 엔드서버의 응답을 클라이언트로 전달
  forward_http_request(connfd, &request);
}

// uri -> host(IP), port, path 파싱

int parse_uri(const char* uri, int* port, char* hostname, char* pathname){
  // uri : http://54.85.138.98:8000/home.html or /home.html
  // 프로토몰 부분 삭제
  const char *url = strncmp(uri, "http://", 7) == 0 ? uri+7:uri;

  if (strchr(url, ':')){
    sscanf(url, "%[^:]:%i%s", hostname, port, pathname); // 정규 표현식으로 ':'으로 구분된 지점을 나누어 hostname, port, pathname 파싱
  }
  else{
    *port = 80;
    sscanf(url, "%[^/]%s", hostname, pathname);
  }

  //파일 경로가 없으면 root로 지정
  if(pathname == NULL)
    strcpy(pathname, "/");
  return 0;
}

/* client의 request 메시지 파싱 */
int parse_http_request(rio_t *rio, HttpRequest *request)
{
  char line[MAXLINE], version[64], method[64], uri[MAXLINE], path[MAXLINE];
  size_t rc;
  rc = 0;
  request->port = 80; /* HTTP 기본 포트 */
  rio_readlineb(rio, line, MAXLINE);
  /*
   * line에서 뽑아서 채움
   * ↓↓↓ example ↓↓↓
   * line : GET /home.html HTTP/1.1
   * method : GET
   * uri : /home.html
   * version : HTTP/1.1
   */
  sscanf(line, "%s %s %s", method, uri, version);

  /* GET 만 지원 */
  if (strcasecmp(method, "GET") != 0)
  {
    printf("Error: %s is not supported!\n", method);
    return -1;
  }
  debug_printf("Request from client: >---------%s", line); /* ifndef DEBUG */
  /* URI 파싱 : host(client IP), port(client port), path(file path) */
  parse_uri(uri, &(request->port), (char *)&(request->host), path);

  /* request line */
  sprintf(request->content, requestline_hdr_format, path); /* 여기서 path는 /index.html 같은 거 /{filename} */

  /* 다음 헤더내용 한 줄씩 read */
  while ((rc = rio_readlineb(rio, line, MAXLINE)) > 0)
  {
    /* http request header 마지막 줄은 \r\n */
    if (strcmp(line, endof_hdr) == 0)
    {
      strcat(request->content, line);
      break;
    }
    else if (strstr(line, "Host:"))
    {
      strcat(request->content, line);
      // Host: 192.168.1.1:8000
      parse_http_host(line, (char *)&(request->host), &(request->port));
    }
    else if (strstr(line, "Connection:"))
    {
      strcat(request->content, connection_hdr);
    }
    else if (strstr(line, "User-Agent:"))
    {
      strcat(request->content, user_agent_hdr);
    }
    else if (strstr(line, "Proxy-Connection:"))
    {
      strcat(request->content, proxy_connection_hdr);
    }
    else
    {
      /* others */
      strcat(request->content, line);
    }
  }
  if (rc < 0) /* 읽자마자 EOF */
  {
    printf("Error when reading request!\n");
    return -1;
  }
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

int my_parse_uri(const char* uri, int* port, char* hostname, char* pathname){
  const char* parsed_uri;
  
  //  'http://' 부분을 잘라줌
  if(strncmp(uri, "http://", 7) == 0) 
    parsed_uri = uri+7;
  else
    parsed_uri = uri;
  
  // port번호가 있을시에 
  if(strchr(parsed_uri,':')){
    sscanf(parsed_uri, "%[^:]:%i%s", hostname, port, pathname);
  }
  else{ // port번호가 없을 경우
    sscanf(parsed_uri, "%[^/]%s", hostname, pathname);
    *port = 80;
  }

  if(pathname == NULL){
    strcpy(pathname,"/");
  }

  return 0;  
}