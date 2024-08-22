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

int verify_pass(char *user,char *pass)
{
    FILE *fp;
    char buf[100];
    fp = fopen("user.txt","r");
    char user1[100];
    char pass1[100];
    while(fgets(buf,100,fp)!=NULL)
    {
        sscanf(buf,"%s %s",user1,pass1);
        //printf("%s\n",buf);
        user1[strlen(user1)] = '\0';
        pass1[strlen(pass1)] = '\0';
        if(strcmp(user1,user)==0 && strcmp(pass1,pass)==0)
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
    serv_addr.sin_port = htons(40000);
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
            char user[100];
            inet_ntop(AF_INET,&cli_addr.sin_addr,ipad,INET_ADDRSTRLEN);
            printf("Established connection with client at ip %s port %d\n",ipad,(cli_addr.sin_port));
            close(sockfd);
            sprintf(buf,"+OK POP3 server ready <%s>\r\n",domain);
            send(newsocfd,buf,strlen(buf),0);
            int state = 0;
            int ndel = 0;
            int del[1000];
            for(int i=0;i<1000;i++){
                del[i] = 0;
            }
            while(1){
                n = recv(newsocfd,buf,100,0);
                buf[n] = '\0';
                //printf("client: %s",buf);
                strcpy(buf1,buf);
                for(int i=0;i<n;i++){
                    if(buf[i]>='a' && buf[i]<='z'){
                        buf[i] = buf[i] - 'a' + 'A';
                    }
                }
                if(state==0){
                    if(strncmp(buf,"USER",4)==0){
                        sscanf(buf1,"%s %s",buf,user);
                        if(verify_user(user)){
                            sprintf(buf,"+OK user %s exists\r\n",user);
                            send(newsocfd,buf,strlen(buf),0);
                            state = 1;
                        }
                        else{
                            sprintf(buf,"-ERR user %s does not exist\r\n",user);
                            send(newsocfd,buf,strlen(buf),0);
                        }
                    }
                    else{
                        sprintf(buf,"-ERR invalid command\r\n");
                        send(newsocfd,buf,strlen(buf),0);
                    }
                }
                else if(state==1){
                    if(strncmp(buf,"PASS",4)==0){
                        char pass[100];
                        sscanf(buf1,"%s %s",buf,pass);
                        if(verify_pass(user,pass)){
                            sprintf(buf,"+OK user %s authenticated\r\n",user);
                            send(newsocfd,buf,strlen(buf),0);
                            state = 2;

                        }
                        else{
                            sprintf(buf,"-ERR invalid password\r\n");
                            send(newsocfd,buf,strlen(buf),0);
                        }
                    }
                    else if(strncmp(buf,"QUIT",4)==0){
                        sprintf(buf,"+OK POP3 server signing off\r\n");
                        send(newsocfd,buf,strlen(buf),0);
                        printf("Connection closed with client at ip %s port %d\n",ipad,cli_addr.sin_port);
                        close(newsocfd);
                        exit(0);
                    }
                    else{
                        sprintf(buf,"-ERR invalid command\r\n");
                        send(newsocfd,buf,strlen(buf),0);
                    }
                }
                else if(state==2){
                    char maildata[50][50][80];
                    char path[100];
                    sprintf(path,"%s/mymailbox.txt",user);
                    FILE *fp;
                    fp = fopen(path,"r");
                    int i = 0;
                    int j = 0;
                    int maillength[50];
                    for(int k=0;k<50;k++){
                        maillength[k] = 0;
                    }
                    // open the file and get each line until u encounter . in a single line that's end of mail, stores all mails in mail data array
                    while(fgets(maildata[j][i],80,fp)!=NULL){
                        maillength[j] = maillength[j] + strlen(maildata[j][i]) - 2;
                        if(maildata[j][i][0]=='.'&&maildata[j][i][1]=='\r'&&maildata[j][i][2]=='\n'){
                            i = 0;
                            j++;
                        }
                        else{
                            i++;
                        }
                    }
                    if(strncmp(buf,"LIST",4)==0){
                        int count = 0;
                        for(int k=0;k<j;k++){
                            count += del[k];
                        }
                        sprintf(buf,"+OK %d messages\r\n",j-count);
                        send(newsocfd,buf,strlen(buf),0);
                        char bufff[1000] = "";
                        for(int k=0;k<j;k++){
                            if(del[k]==0){
                                char sender[100];
                                char subject[100];
                                char date[100];
                                char time[100];
                                sscanf(maildata[k][0],"From: %s",sender);
                                sscanf(maildata[k][3],"Time recieved: %s %s",date,time);
                                int i = 0;
                                for(int j=0;maildata[k][2][j]!='\r';j++){
                                    subject[i++] = maildata[k][2][j];
                                }
                                subject[i] = '\0';
                                sprintf(bufff,"%d %s %s %s %s\r\n",k+1,sender,date,time,subject);
                                send(newsocfd,bufff,strlen(bufff),0);
                                //printf("%s",bufff);
                            }
                        }
                        sprintf(buf,".\r\n");
                        //printf("%s",buf);
                        send(newsocfd,buf,3,0);
                    }
                    else if(strncmp(buf,"RETR",4)==0){
                        int num;
                        sscanf(buf1,"%s %d",buf,&num);
                        num = ntohl(num);
                        if(num<=j && num>0 && del[num-1]==0){
                            sprintf(buf,"+OK %d octets\r\n",maillength[num-1]);
                            //printf("%s %d",buf,strlen(buf));
                            int zi = send(newsocfd,buf,strlen(buf),0);
                            //printf("%d\n",zi);
                            for(int i=0;i<50;i++){
                                bzero(buf,101);
                                sprintf(buf,"%s",maildata[num-1][i]);
                                int mi = send(newsocfd,buf,strlen(buf),0);
                                //printf("%s %d\n",buf,mi);
                                if(maildata[num-1][i][0]=='.'&&maildata[num-1][i][1]=='\r'&&maildata[num-1][i][2]=='\n'){
                                    break;
                                }
                            }
                        }
                        else{
                            sprintf(buf,"-ERR no such message\r\n");
                            send(newsocfd,buf,strlen(buf),0);
                        }
                    }
                    else if(strncmp(buf,"DELE",4)==0){
                        int num;
                        sscanf(buf1,"%s %d",buf,&num);
                        num = ntohl(num);
                        if(num<=j && num>0 && del[num-1]==0){
                            del[num-1] = 1;
                            sprintf(buf,"+OK message %d deleted\r\n",num);
                            send(newsocfd,buf,strlen(buf),0);
                        }
                        else{
                            sprintf(buf,"-ERR no such message\r\n");
                            send(newsocfd,buf,strlen(buf),0);
                        }
                    }
                    else if(strncmp(buf,"RSET",4)==0){
                        for(int k=0;k<1000;k++){
                            del[k] = 0;
                        }
                        sprintf(buf,"+OK maildrop has %d messages\r\n",j);
                        send(newsocfd,buf,strlen(buf),0);
                    }
                    else if(strncmp(buf,"QUIT",4)==0){
                        int count = 0;
                        for(int k=0;k<j;k++){
                            count += del[k];
                        }
                        int fp1;
                        fp1 = open(path,O_WRONLY|O_TRUNC,0666);
                        for(int k=0;k<j;k++){
                            if(del[k]==0){
                                printf("sending mail %d\n",k+1);
                                for(int i=0;i<50;i++){
                                    write(fp1,maildata[k][i],strlen(maildata[k][i]));
                                    if(maildata[k][i][0]=='.'&&maildata[k][i][1]=='\r'&&maildata[k][i][2]=='\n'){
                                        break;
                                    }
                                }
                            }
                        }
                        sprintf(buf,"+OK POP3 server signing off (%d messages left)\r\n",j-count);
                        send(newsocfd,buf,strlen(buf),0);
                        printf("Connection closed with client at ip %s port %d\n",ipad,cli_addr.sin_port);
                        close(fp1);
                        close(newsocfd);
                        exit(0);
                    }
                    else{
                        sprintf(buf,"-ERR invalid command\r\n");
                        send(newsocfd,buf,strlen(buf),0);
                    }
                }
            }
            close(newsocfd);
            exit(0);
        }
        else
        {
            close(newsocfd);
        }
    }
}
