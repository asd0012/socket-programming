#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>


char webpage[]=
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>asd0012</title>\r\n"
//"<style>body { background-color: #888888 }</style></head>\r\n"
"<img src=\"ccupic.jpeg\"><center></body></html>\r\n"
"<form enctype=\"multipart/form-data\" action=\"upload.php\" method=\"POST\"> <p>Upload your file</p> <input type=\"file\" name=\"uploaded_file\"></input><br /> <input type=\"submit\" value=\"Upload\"></input>"
"<?PHP $file = $_FILES[\"file\"]; move_uploaded_file($file[\"tmp_name\"], \"/Downloads/socket programming/uploads/\" . $file[\"name\"]);?>";




int main(int argc, char *argv[])
{
   struct sockaddr_in server_addr,client_addr;
   int fd_server,fd_client;
   char buf[2048];
   int fd_img;
   int turn_on=1;
   socklen_t sin_len =sizeof(client_addr);
   /* create a socket with TCP and IPV4 protocol*/
   fd_server=socket(AF_INET,SOCK_STREAM,0);
   if(fd_server <0)
   {
	perror("socket");
	exit(1);
   }

   setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR,&turn_on,sizeof(int));

   
   server_addr.sin_family =AF_INET;
   /* load with inet_pton() with any INADDR*/
   server_addr.sin_addr.s_addr =inet_addr("127.0.0.1");
   /* set port number with host8080*/
   server_addr.sin_port =htons(8080);

   if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) ==-1){
	perror("bind");
	close(fd_server);
	exit(1);
   }

   if(listen(fd_server,10) ==-1){
	perror("listen");
	close(fd_server);
	exit(1);
   }
   while(1){
	fd_client =accept(fd_server,(struct sockaddr *) &client_addr,&sin_len);

	if(fd_client==-1){
	    perror("Connection failed...\n");
	    continue;
	}
	
	printf("Got client connection...\n");

	if(!fork()){
	     /* child process */
	     close(fd_server);
	     memset(buf,0,2048);
	     read(fd_client,buf,2047);

	     if(!strncmp(buf, "GET /ccupic.jpeg",16)){
		  fd_img = open("ccupic.jpeg", O_RDONLY);
		  sendfile(fd_client, fd_img,NULL,10000);
		  close(fd_img);
	     }
	     else /* write webpage information to web */
	    	 write(fd_client,webpage,sizeof(webpage) -1);
	     close(fd_client);
	     printf("closeing..\n");
	     exit(0);
	}
	/* parent process*/
	wait(NULL);
  	close(fd_client);	
   }

   return 0;
}
