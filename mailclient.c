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

#define domain "gmail.com"
#define strip(s) for(int i = 0;; i++){if(s[i] == '\r'){s[i] = '\0';break;}}

int isEmptyOrWhitespace(const char *str) {
    while (*str != '\0') {
        if (*str != ' ' && *str != '\t' && *str != '\n' && *str != '\r') {
            return 0;
        }
        str++;
    }
    return 1;
}

int validateEmailHeaders(char *fromLine, char *toLine, char *subjectLine) {
    if (strncmp(fromLine, "From: ", 6) != 0) {
        //printf("From: %s\n",fromLine);
        return 0;
    }
    else{
        if(fromLine[6] == '@' || isEmptyOrWhitespace(fromLine + 6)){
            return 0;
        }
        char *buf = strstr(fromLine,"@");
        if(isEmptyOrWhitespace(buf + 1)){
            return 0;
        }
    }
    if (strncmp(toLine, "To: ", 4) != 0) {
        return 0;
    }
    else{
        if(toLine[4] == '@' || isEmptyOrWhitespace(toLine + 4)){
            return 0;
        }
        char *buf = strstr(toLine,"@");
        if(isEmptyOrWhitespace(buf + 1)){
            return 0;
        }
    }
    if (strncmp(subjectLine, "Subject: ", 9) != 0 || isEmptyOrWhitespace(subjectLine + 9)) {
        return 0;
    }

    return 1; 
}

int main(int argc,char *argv[])
{
    if(argc < 3){
        fprintf(stderr,"usage: ./client <ip> <port>\n");
        exit(EXIT_FAILURE);
    }
    int y = 0;
    char user[100];
    char pass[100];
    printf("Enter username: ");
    scanf("%[^\n]%*c",user);
    printf("Enter password: ");
    scanf("%[^\n]%*c",pass);
    while(y != 3){
        printf("1. Manage Mail : Shows the stored mails of the logged in user only\n2. Send Mail : Allows the user to send a mail\n3. Quit : Quits the program.\n");
        printf("Enter your choice: ");
        scanf("%d",&y);
        fflush(stdin);
        if(y == 1){
            printf("Manage Mail\n");
            struct addrinfo hints,*servinfo,*temp;
            int sockfd;
            memset(&hints, 0, sizeof (hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            int rv;
            if((rv = getaddrinfo(argv[1],argv[3],&hints,&servinfo)) != 0){
                fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
                exit(EXIT_FAILURE);
            }
            for(temp = servinfo; temp != NULL; temp = temp->ai_next){
                if((sockfd = socket(temp->ai_family,temp->ai_socktype,temp->ai_protocol)) == -1){
                    perror("socket");
                    continue;
                }
                if (connect(sockfd, temp->ai_addr, temp->ai_addrlen) == -1) {
                    close(sockfd);
                    perror("client: connect");
                continue;
                }
                //printf("C: <client connects to SMTP port>\n");
                break;
            }
            if(temp == NULL){
                fprintf(stderr,"failed to bind socket\n");
                exit(EXIT_FAILURE);
            }
            freeaddrinfo(servinfo);
            char buf1[100] = "";
            char ipad[INET_ADDRSTRLEN];

            inet_ntop(AF_INET,(struct sockaddr_in *)temp->ai_addr,ipad,INET_ADDRSTRLEN);
            printf("C: Established connection with server at ip %s port %d\n",ipad,(((struct sockaddr_in *)temp->ai_addr)->sin_port));
            recv(sockfd,buf1,100,0);
            //printf("S: %s",buf1);
            if(strncmp(buf1,"-ERR",4) == 0){
                for(int mi = 0;;mi++){
                    if(buf1[mi] == '\r'){
                        buf1[mi] = '\0';
                        break;
                    }
                }
                strip(buf1);
                printf("Error in managing mail at connection: %s\n",buf1);
                continue;
            }
            sprintf(buf1,"USER %s\r\n",user);
            send(sockfd,buf1,strlen(buf1),0);
            strip(buf1);
            printf("C: %s\n",buf1);
            recv(sockfd,buf1,100,0);
            if(strncmp(buf1,"-ERR",4) == 0){
                strip(buf1);
                printf("Error in managing mail at USER: %s\n",buf1);
                continue;
            }
            sprintf(buf1,"PASS %s\r\n",pass);
            send(sockfd,buf1,strlen(buf1),0);
            strip(buf1);
            printf("C: %s\n",buf1);
            recv(sockfd,buf1,100,0);
            if(strncmp(buf1,"-ERR",4) == 0){
                strip(buf1);
                printf("Error in managing mail at PASS: %s\n",buf1);
                continue;
            }
            while(1){
                char buf1[101] = "";
                memset(buf1,0,101);
                sprintf(buf1,"LIST\r\n");
                send(sockfd,buf1,strlen(buf1),0);
                strip(buf1);
                printf("C: %s\n",buf1);
                char buf2[6000] ="";
                while(1){
                    char buf1[101] = "";
                    int zi = recv(sockfd,buf1,100,0);
                    if(strncmp(buf1,"-ERR",4) == 0){
                        strip(buf1);
                        printf("Error in managing mail at LIST: %s\n",buf1);
                        break;
                    }
                    //strip(buf1);
                    strcat(buf2,buf1);
                    char *z = strstr(buf2,"\r\n.\r\n");
                    if(z != NULL){
                        *z = '\0';
                        break;
                    }
                    memset(buf1,0,101);
                }
                printf("S: %s\n",buf2);
                memset(buf2,0,6000);
                int n;
                printf("Enter the mail number to view or -1 to exit: ");
                scanf("%d",&n);
                if(n == -1){
                    sprintf(buf1,"QUIT\r\n");
                    send(sockfd,buf1,strlen(buf1),0);
                    strip(buf1);
                    printf("C: %s\n",buf1);
                    recv(sockfd,buf1,100,0);
                    printf("S: %s",buf1);
                    break;
                }
                n = htonl(n);
                sprintf(buf1,"RETR %d\r\n",n);
                send(sockfd,buf1,strlen(buf1),0);
                strip(buf1);
                printf("C: RETR %d\n",ntohl(n));
                char temporary[6000]="";
                while(1){
                    char buf1[100] = "";
                    recv(sockfd,buf1,100,0);
                    if(strncmp(buf1,"-ERR",4) == 0){
                        printf("Entered valuue is out of bound\n");
                        printf("Enter the mail number to view or -1 to exit: ");
                        scanf("%d",&n);
                        if(n == -1){
                            sprintf(buf1,"QUIT\r\n");
                            send(sockfd,buf1,strlen(buf1),0);
                            strip(buf1);
                            printf("C: %s\n",buf1);
                            recv(sockfd,buf1,100,0);
                            printf("S: %s",buf1);
                            break;
                        }
                        n = htonl(n);
                        sprintf(buf1,"RETR %d\r\n",n);
                        continue;
                    }
                    strcat(temporary,buf1);
                    char *z = strstr(temporary,"\r\n.\r\n");
                    if(z != NULL){
                        *z = '\0';
                        break;
                    }
                    memset(buf1,0,100);
                }
                if(n == -1){
                    break;
                }
                printf("S: %s\n",temporary);
                memset(temporary,0,6000);
                printf("Enter character d to delete the mail or any other character to continue: ");
                char c;
                fflush(stdin);
                fflush(stdout);
                scanf(" %c",&c);
                if(c == 'd'){
                    sprintf(buf1,"DELE %d\r\n",n);
                    send(sockfd,buf1,strlen(buf1),0);
                    strip(buf1);
                    printf("C: DELE %d\n",ntohl(n));
                    recv(sockfd,buf1,100,0);
                    if(strncmp(buf1,"-ERR",4) == 0){
                        strip(buf1);
                        printf("Error in managing mail at DELE: %s\n",buf1);
                        break;
                    }
                    strip(buf1);
                    printf("S: %s\n",buf1);
                }
                //memset all variables used above to 0
                memset(buf1,0,100);
                memset(temporary,0,6000);
                memset(buf2,0,6000);
                memset(&n,0,sizeof(int));
                memset(&c,0,sizeof(char));
            }
        }
        else if(y == 2){
            printf("Enter Mail:\n");
            fflush(stdin);
            char **mail;
            mail = (char **)malloc(50*sizeof(char *));
            for(int i = 0; i < 50; i++){
                mail[i] = (char *)malloc(80*sizeof(char));
            }
            int i = 0;
            int j = 0;
            char c;
            while(i < 50){
                char line[80];
                j = 0;
                while(j < 80){
                    scanf("%c",&c);
                    if(c == '\n'){
                        line[j] = '\0';
                        break;
                    }
                    line[j] = c;
                    mail[i][j] = c;
                    j++;
                }
                i++;
                if(strcmp(line,".") == 0){
                    break;
                }
                
            }
            /*for(int k = 0; k < i; i++){
                printf("%s\n",mail[i]);
            }*/
            if(!validateEmailHeaders(mail[1],mail[2],mail[3])){
                printf("Invalid email headers\n");
                continue;
            }
            struct addrinfo hints,*servinfo,*temp;
            int sockfd;
            memset(&hints, 0, sizeof (hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            int rv;
            if((rv = getaddrinfo(argv[1],argv[2],&hints,&servinfo)) != 0){
                fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
                exit(EXIT_FAILURE);
            }
            for(temp = servinfo; temp != NULL; temp = temp->ai_next){
                if((sockfd = socket(temp->ai_family,temp->ai_socktype,temp->ai_protocol)) == -1){
                    perror("socket");
                    continue;
                }
                if (connect(sockfd, temp->ai_addr, temp->ai_addrlen) == -1) {
                    close(sockfd);
                    perror("client: connect");
                continue;
                }
                //printf("C: <client connects to SMTP port>\n");
                break;
            }
            if(temp == NULL){
                fprintf(stderr,"failed to bind socket\n");
                exit(EXIT_FAILURE);
            }
            freeaddrinfo(servinfo);
            char buf1[100];
            char ipad[INET_ADDRSTRLEN];

            inet_ntop(AF_INET,(struct sockaddr_in *)temp->ai_addr,ipad,INET_ADDRSTRLEN);
            printf("C: Established connection with server at ip %s port %d\n",ipad,(((struct sockaddr_in *)temp->ai_addr)->sin_port));
            recv(sockfd,buf1,100,0);
            strip(buf1);
            printf("S: %s\n",buf1);
            sscanf(buf1,"%d",&rv);
            if(rv != 220){
                printf("Error in sending mail at connection: %s\n",buf1);
                continue;
            }
            sprintf(buf1,"HELO %s\r\n",domain);
            send(sockfd,buf1,strlen(buf1),0);
            strip(buf1);
            printf("C: %s\n",buf1);
            recv(sockfd,buf1,100,0);
            strip(buf1);
            printf("S: %s\n",buf1);
            sscanf(buf1,"%d",&rv);
            if(rv != 250){
                printf("Error in sending mail at HELO: %s\n",buf1);
                continue;
            }
            char temp1[100];
            char temp3[100];
            strcpy(temp3,mail[1]);
            char *temp2 = strstr(temp3," ");
            sprintf(temp1," <%s>",temp2+1);
            strcpy(temp2,temp1);
            sprintf(buf1,"MAIL %s\r\n",temp3);
            //printf("%s\n",buf1);
            
            send(sockfd,buf1,strlen(buf1),0);
            strip(buf1);
            printf("C: %s\n",buf1);
            recv(sockfd,buf1,100,0);
            strip(buf1);
            printf("S: %s\n",buf1);
            sscanf(buf1,"%d",&rv);
            if(rv != 250){
                printf("Error in sending mail at MAIL : %s\n",buf1);
                continue;
            }
            char temp4[100];
            char temp5[100];
            strcpy(temp5,mail[2]);
            char *temp6 = strstr(temp5," ");
            sprintf(temp4," <%s>",temp6+1);
            strcpy(temp6,temp4);
            sprintf(buf1,"RCPT %s\r\n",temp5);
            //printf("%s\n",buf1);
            
            send(sockfd,buf1,strlen(buf1),0);
            strip(buf1);
            printf("C: %s\n",buf1);
            recv(sockfd,buf1,100,0);
            strip(buf1);
            printf("S: %s\n",buf1);
            sscanf(buf1,"%d",&rv);
            if(rv != 250){
                printf("Error in sending mail at RCPT: %s\n",buf1);
                continue;
            }
            sprintf(buf1,"DATA\r\n");
            
            send(sockfd,buf1,strlen(buf1),0);
            strip(buf1);
            printf("C: %s\n",buf1);
            recv(sockfd,buf1,100,0);
            strip(buf1);
            printf("S: %s\n",buf1);
            sscanf(buf1,"%d",&rv);
            if(rv != 354){
                printf("Error in sending mail at DATA: %s\n",buf1);
                continue;
            }
            for(int i = 1; i < 50; i++){
                sprintf(buf1,"%s\r\n",mail[i]);
                
                int n = send(sockfd,buf1,strlen(buf1),0);
                strip(buf1);
                printf("C: %s\n",buf1);
                //printf("%s %d\n",buf1,n);
                if(strcmp(mail[i],".") == 0){
                    break;
                }
            }
            recv(sockfd,buf1,100,0);
            strip(buf1);
            printf("S: %s\n",buf1);
            sscanf(buf1,"%d",&rv);
            if(rv != 250){
                printf("Error in sending mail at BODY: %s\n",buf1);
                continue;
            }
            sprintf(buf1,"QUIT\r\n");
            
            send(sockfd,buf1,strlen(buf1),0);
            strip(buf1);
            printf("C: %s\n",buf1);
            recv(sockfd,buf1,100,0);
            strip(buf1);
            printf("S: %s\n",buf1);
            sscanf(buf1,"%d",&rv);
            if(rv != 221){
                printf("Error in sending mail at QUIT: %s\n",buf1);
                continue;
            }
            printf("Mail sent successfully\n");
            close(sockfd);
            printf("C: <client hangs up>\n");
        }
        else if(y == 3){
            printf("Quitting\n");
        }
        else{
            printf("Invalid command\n");
        }
    }
    return 0;
}

