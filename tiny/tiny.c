/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
void echo(int fd);
void doit(int fd); //handles one HTTP transaction

// TINY doesn't use any of the information in the request headers → read_requesthdrs simply reads and ignores them.
void read_requesthdrs(rio_t *rp);


// parses the URI into a filename and an optional CGI argument string
int parse_uri(char *uri, char *filename, char *cgiargs);


void serve_static(int fd, char *filename, int filesize);
void serve_dynamic(int fd, char *filename, char *cgiargs);

void get_filetype(char *filename, char *filetype);

//sends an HTTP response to the client with the appropriate status code and status message in the response line,
// along with an HTML file in the response body that explains the error to the browser’s user.
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);


int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    //connfd: 클라이언트와의 통신을 위한 새로운 FD
    connfd = Accept(listenfd, (SA *)&clientaddr, 
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    // doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close

    printf("----------------------------------------\n");
  }
}
/*
typedef struct {
    int rio_fd;                // Descriptor for this internal buf 
    int rio_cnt;               // Unread bytes in internal buf 
    char *rio_bufptr;          // Next unread byte in internal buf 
    char rio_buf[RIO_BUFSIZE]; // Internal buffer 
} rio_t;
*/
void doit(int fd){
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  printf("-0-0-0--------------------------\n");
  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  
  //GET 이나 HEAD가 아닐경우 에러발생
  if((strcasecmp(method, "GET")) && strcasecmp(method,"HEAD")){
    clienterror(fd, method, "501", "Not implemented",
          "Tiny couldn't find this file");
    return;
  }

  //HEAD일경우 response body를 보내지않고, header만 반환하게 해야됨 (구현안함)


  
  read_requesthdrs(&rio);

  /*Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0){
    clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
    return;
  }

  if (is_static){
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){ // S_ISREG 일반파일인지 확인, 읽기권한이 있는지 확인
      clienterror(fd, filename, "403", "Forbidden",
              "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);

  }
  else{ // serve dynamic content
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }

  printf("****************************************\n");
}
void echo(int fd)
{
	size_t n;
	rio_t rio;
	char io_buf[MAXBUF];
	int fp;
	
	/* Send response headers to client */
	sprintf(io_buf, "HTTP/1.0 200 OK\r\n");
	sprintf(io_buf, "%sServer: Tiny Web Server\r\n", io_buf);
	sprintf(io_buf, "%sConnection: close\r\n\r\n", io_buf);
	Rio_writen(fd, io_buf, strlen(io_buf));
	printf("**  echo server  **\n");
	printf("Response headers:\n");
	printf("%s", io_buf);
	
	fp = open("./logfile", O_WRONLY | O_CREAT,0644);
	
	/* Send response body to client */
	Rio_readinitb(&rio, fd);
	while((n = Rio_readlineb(&rio, io_buf, MAXLINE)) != 0){
		if (strcmp(io_buf, "\r\n") == 0)
			break;
		Rio_writen(fp, io_buf, n);
		Rio_writen(fd, io_buf, n);
	}
	
	close(fp);
}

void clienterror(int fd, char *cause, char*errnum, char* shortmsg, char*longmsg){
  char buf[MAXLINE], body[MAXBUF];

  /* build the HTTP response body*/
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n",body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em> The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html \r\n");
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
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


int parse_uri(char* uri, char *filename, char *cgiargs){
  char *ptr;  

  if(!strstr(uri, "cgi-bin")){ // static content
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if( uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else{ //Dynamic content
    ptr = index(uri, '?');
    if(ptr){
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server \r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

/* get_filetype - Derive file type from filename */
void get_filetype(char* filename, char *filetype){
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mpeg")) //mpeg 확장자 추가
    strcpy(filetype, "video/mpeg");
  else
    strcpy(filetype, "text/plain");
}

//동적 컨텐츠를 클라이언트에 제공하는 함수
void serve_dynamic(int fd, char *filename, char *cgiargs){
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* return first part of http response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0){
    /* Real server would set all CIG vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO); // Redirect stdout to client
    Execve(filename, emptylist, environ); // Run CGI program
  }
  Wait(NULL);
}
