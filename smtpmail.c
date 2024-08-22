#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/sendfile.h>
#include<sys/wait.h>
#include<signal.h>
#include<time.h>
#include<sys/stat.h>

#define domain "gmail.com"

int verify_user(char *user)
{
    FILE *fp;
    char buf[100];
    fp = fopen("user.txt","r");
    while(fgets(buf,100,fp)!=NULL)
    {
        sscanf(buf,"%s",buf);
        //printf("%s\n",buf);
        buf[strlen(buf)] = '\0';
        if(strcmp(buf,user)==0)
        {
            //printf("user verified\n");
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}
int main()
{
    int sockfd,newsocfd;
    int clilen;
    struct sockaddr_in cli_addr,serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(20000);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("Unable to bind local address\n");
        exit(0);
    }
    printf("server listening at port %d\n",serv_addr.sin_port);
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    char buf[101];
    char buf1[101];
    char filn[96];
    char filn1[100];
    char ipad[INET_ADDRSTRLEN];
    int n;
    while(1)
    {
        clilen = sizeof(cli_addr);
        newsocfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        if(fork()==0)
        {
            inet_ntop(AF_INET,&cli_addr.sin_addr,ipad,INET_ADDRSTRLEN);
            printf("Established connection with client at ip %s port %d\n",ipad,cli_addr.sin_port);
            close(sockfd);
            sprintf(buf,"220 %s SMTP ready\r\n",domain);
            send(newsocfd,buf,strlen(buf),0);
            //printf("%s\n",buf);
            char from[100];
            char to[100];
            char *dom;
            char maildata[4000];
            while(1)
            {
                n = recv(newsocfd,buf,100,0);
                buf[n] = '\0';
                strcpy(buf1,buf);
               
                for(int i=0;i<strlen(buf1);i++){
                    if(buf1[i]>='a' && buf1[i]<='z')
                        buf1[i] = buf1[i] - 32;
                } //printf("recieved %s changed to %s\n",buf,buf1);
                if(strncmp(buf1,"QUIT",4)==0)
                {
                    sprintf(buf,"221 %s closing connection\r\n",domain);
                    send(newsocfd,buf,strlen(buf),0);
                    //printf("sent at QUIT%s\n",buf);
                    close(newsocfd);
                    exit(0);
                }
                if(strncmp(buf1,"HELO",4)==0)
                {
                    sprintf(buf,"250 %s\r\n",domain);
                    send(newsocfd,buf,strlen(buf),0);
                    //printf("sent at HELO%s\n",buf);
                    continue;
                }
                if(strncmp(buf1,"MAIL FROM:",10)==0)
                {
                    //printf("in mail from %s\n",buf);
                    sscanf(buf+11,"<%[^>]",from);
                    sprintf(buf,"250 user OK\r\n");
                    send(newsocfd,buf,strlen(buf),0);
                    //printf("sent at MAIL FROM%s\n",buf);
                    continue;
                }
                if(strncmp(buf1,"RCPT TO:",8)==0)
                {
                    sscanf(buf+9,"<%[^>]",to);
                    dom = strstr(to,"@");
                    //printf("%s\n",dom);
                    if(strcmp(dom+1,domain)!=0)
                    {
                        sprintf(buf,"550 No such user here\r\n");
                        send(newsocfd,buf,strlen(buf),0);
                        //printf("%s\n",buf);
                        continue;
                    }
                    sscanf(to,"%[^@]",to);
                    //printf("%s\n",to);
                    if(verify_user(to))
                    {
                        sprintf(buf,"250 OK\r\n");
                        send(newsocfd,buf,strlen(buf),0);
                        //printf("%s\n",buf);
                    }
                    else
                    {
                        sprintf(buf,"550 No such user here\r\n");
                        send(newsocfd,buf,strlen(buf),0);
                        //printf("%s\n",buf);
                    }
                    continue;
                }
                if(strncmp(buf1,"DATA",4)==0)
                {
                    sprintf(buf,"354 Start mail input; end with <CRLF>.<CRLF>\r\n");
                    send(newsocfd,buf,strlen(buf),0);
                    sprintf(filn1,"./%s/",to);
                    mkdir(filn1,0777);
                    sprintf(filn,"./%s/mymailbox.txt",to);
                    int fd = open(filn,O_WRONLY|O_CREAT|O_APPEND,0666);
                    while(1)
                    {
                        char buf[101];
                        n = recv(newsocfd,buf,100,0);
                        buf[n] = '\0';
                        //printf("%s %d\n",buf,n);
                        strcat(maildata,buf);
                        char *z = strstr(maildata,"\r\n.\r\n");
                        if(z!=NULL)
                        {
                            //*z = '\0';
                            break;
                        }
                    }
                    //add time recieved in maildata between subject and mail body
                    time_t t = time(NULL);
                    struct tm tm = *localtime(&t);
                    char timed[4000];
                    sprintf(timed,"Time recieved: %d-%d-%d %d:%d:%d\r\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
                    char *sub = strstr(maildata,"Subject:");
                    char *body = strstr(sub,"\r\n");
                    strcat(timed,body+2);
                    strcpy(body+2,timed);
                    //write maildata to file
                    write(fd,maildata,strlen(maildata));
                    close(fd);
                    sprintf(buf,"250 OK\r\n");
                    send(newsocfd,buf,strlen(buf),0);
                    //printf("%s\n",buf);
                    printf("Mail recieved from %s to %s@%s\n",from,to,domain);
                    continue;
                }
            }
            close(newsocfd);
            exit(0);
        }
        else
        {
            close(newsocfd);
            continue;
        }
    }
}
