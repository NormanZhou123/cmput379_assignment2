 #include <stdio.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <sys/param.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <fcntl.h>
 #include <signal.h>
 #include <string.h>
 #include <pthread.h>

 int port;
 struct sockaddr_in Socket_Addr;
 int Server_fd,Server_res;
 int entries;

 struct whiteboard
 {
     int number;
     char** messages;
     int* type;
     int* length;
 }wb;

 void savefile(char* filename)
 {
     FILE* fw;
     int i;
     fw=fopen(filename,"w");
     fprintf(fw,"%d\n",wb.number);
     for(i=0;i<wb.number;i++)
     {
         fprintf(fw,"%d\n",wb.type[i]);
         fprintf(fw,"%d\n",wb.length[i]);
         if(wb.length[i]!=0)
         {
             fprintf(fw,"%s\n",wb.messages[i]);
         }
     }
     fclose(fw);
 }

 int itoa(int num,char* buf)
 {
     int i=0,ans=0;
     if(num<=9)
     {
         buf[0]='0'+num;
         buf[1]='\0';
         return(1);
     }
     else
     {
         ans=itoa(num/10,buf);
         buf[ans]='0'+num%10;
         buf[ans+1]='\0';
         return(ans+1);
     }
 }

 int Read_Msg(int client_fd,char* msg,int len)
 {
     int i,l;
     i=0;
     do
     {
         l = read(client_fd,msg+i,1);
         i++;
     }while(l!=0 && ((len==-1 && msg[i-1]!='\n')||(i<len)));
     if(l==0)
     {
         return(0);
     }
     else
     {
         msg[i]='\0';
         return(i);
     }
 }

 void Send_Msg(char msgType,int num,int len,char* msg,int client_fd)
 {
     char buf1[512]={};
     char *buf2;
     int l;
     buf2=calloc(len+1,sizeof(char));
     if(msg!=NULL)
     {
         memcpy(buf2,msg,len*sizeof(char));
     }
     buf2[len]='\n';
     buf1[0]='!';
     itoa(num,&buf1[1]);
     l=strlen(buf1);
     buf1[l]=msgType;
     itoa(len,&buf1[l+1]);
     l=strlen(buf1);
     buf1[l]='\n';
     write(client_fd,buf1,l+1);
     write(client_fd,buf2,len+1);
 }


 void welcome(int client_fd)
 {
     int l1,l2;
     char buf1[512]="CMPUT379 Whiteboard Server v0\n";
     char buf2[512];
     l1=strlen(buf1);
     itoa(wb.number,buf2);
     l2=strlen(buf2);
     buf2[l2]='\n';
     write(client_fd,buf1,l1);
     write(client_fd,buf2,l2+1);
 }

 void sigterm_handler(int signal_num)
 {
     savefile("whiteboard.all");
     exit(0);
 }

 void loadfile(char* filename)
 {
     FILE* fr;
     int i;
     fr=fopen(filename,"r");
     if(fr==NULL)
     {
         wb.number=entries;
         wb.messages=calloc(wb.number,sizeof(char*));
         wb.type=calloc(wb.number,sizeof(int));                 //0 none 1 p -1 c
         wb.length=calloc(wb.number,sizeof(int));
         for(i=0;i<wb.number;i++)
         {
             wb.messages[i]=NULL;
             wb.type[i]=0;
             wb.length[i]=0;
         }
     }
     else
     {
         fscanf(fr,"%d",&wb.number);
         wb.messages=calloc(wb.number,sizeof(char*));
         wb.type=calloc(wb.number,sizeof(int));                 //0 none 1 p -1 c
         wb.length=calloc(wb.number,sizeof(int));
         for(i=0;i<wb.number;i++)
         {
             fscanf(fr,"%d",&wb.type[i]);
             fscanf(fr,"%d",&wb.length[i]);
             if(wb.length[i]!=0)
             {
                 wb.messages[i]=calloc(wb.length[i]+1,sizeof(char));
                 fgets(wb.messages[i],wb.length[i]+1,fr);
                 fgets(wb.messages[i],wb.length[i]+1,fr);
                 //fscanf(fr,"%[^\n]",wb.messages[i]);
             }
             else
             {
                 wb.messages[i]=NULL;
             }
         }
         fclose(fr);
     }
 }

 void ask(char* buf,int len,int client_fd)
 {
     int i;
     char num[250];
     i=1;
     while(i<len && buf[i]>='0'&& buf[i]<='9')
     {
         num[i-1]=buf[i];
         i++;
     }
     num[i-1]='\0';
     if(i==len-1 && buf[i]=='\n')
     {
          i=atoi(num);
          if(i>0 && i<=wb.number)
          {
              if(wb.type[i-1]>=0)
              {
          //        printf("%d\n%s",wb.length[i-1],wb.messages[i-1]);
                  Send_Msg('p',i,wb.length[i-1],wb.messages[i-1],client_fd);
              }
              else
              {
                  Send_Msg('c',i,wb.length[i-1],wb.messages[i-1],client_fd);
              }
          }
          else
          {
              Send_Msg('e',i,14,"No such entry!",client_fd);
          }
     }
 }

 int updata(char* buf,int len,int client_fd)
 {
     int i,j,id,length,type,l;
     char num1[250];
     char num2[250];
     char* msg;
     i=1;
     j=0;
     while(i<len && buf[i]>='0'&& buf[i]<='9')
     {
         num1[j]=buf[i];
         i++;
         j++;
     }
     num1[i-1]='\0';
     id=atoi(num1);
     type=0;
     if(buf[i]=='p')
     {
         type=1;
     }
     else
     {
         if(buf[i]=='c')
         {
             type=-1;
         }
     }
     j=0;
     i++;
     while(i<len && buf[i]>='0'&& buf[i]<='9')
     {
         num2[j]=buf[i];
         i++;
         j++;
     }
     num2[j]='\0';
     length=atoi(num2);
     if(i==len-1 && buf[i]=='\n')
     {
          if(id>0 && id<=wb.number)
          {
              id--;
              msg=calloc(length+2,sizeof(char));
              l = Read_Msg(client_fd,msg,length+1);
              if(l==0)
              {
                  return(-1);
              }
              else
              {
                  if(wb.messages[id]!=NULL)
                  {
                      free(wb.messages[id]);
                      wb.messages[id]=NULL;
                  }
                  wb.type[id]=type;
                  wb.length[id]=length;
                  if(length>0)
                  {
                      wb.messages[id]=calloc(length,sizeof(char));
                      memcpy(wb.messages[id],msg,length*sizeof(char));
                      wb.messages[id][length]='\0';
                  }
              }
              Send_Msg('e',id,0,"",client_fd);
          }
          else
          {
              Send_Msg('e',id,14,"No such entry!",client_fd);
          }
     }
     return(0);
 }

 void *run(void *arg)
 {
     int client_fd = ((int*)arg)[0];
     int len=0;
     char* str;
     char buf[512];
     welcome(client_fd);
     while(1)
     {
         len = Read_Msg(client_fd,buf,-1);//read data from client
         if(len==0)
         {
             close(client_fd);
             break;
         }
         else
         {
             //printf("Get Msg : [%s]\n",buf);
             if(buf[0]=='?')
             {
                 ask(buf,len,client_fd);
             }
             else
             {
                 if(buf[0]=='@')
                 {
                     if(updata(buf,len,client_fd)==-1)
                     {
                         close(client_fd);
                         break;
                     }
                 }
             }
         }
     }
     return NULL;
 }

 void Listening()
 {
     int client_fd=0;
     int err;
     unsigned client_port=0;
     char client_ip[20];
     pthread_t tid;
     struct sockaddr_in client_addr;
     err=listen(Server_fd,5);
     while(1)
     {
         socklen_t socklen = sizeof(client_addr);
         client_fd = accept(Server_fd,(struct sockaddr *)&client_addr,&socklen);
         if(client_fd>=0)
         {
             inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,client_ip,sizeof(client_ip));
             client_port = ntohs(client_addr.sin_port);
             pthread_create(&tid,NULL,run,(void *)&client_fd);
         }
     }
 }

 void Bind_Socket()
 {
    bzero(&Socket_Addr,sizeof(Socket_Addr));
    Socket_Addr.sin_family=AF_INET;
    Socket_Addr.sin_port=htons(port);           //port
    Socket_Addr.sin_addr.s_addr= htonl(INADDR_ANY);    //ip
    Server_fd=socket(AF_INET,SOCK_STREAM,0);
    Server_res = bind(Server_fd,(struct sockaddr *)&Socket_Addr,sizeof(Socket_Addr));
 }

 void p_daemon()
 {
     int pid, fd;
     int i;
     if ((pid = fork()) == -1) exit(1);
     if (pid != 0) exit(0);
     if (setsid() == -1) exit(1);
     if ((pid = fork()) == -1) exit(1);
     if (pid != 0) exit(0);
     for (i = 0; i < NOFILE; i++)
     {
         close(i);
     }
     //if (chdir("/") == -1) exit(1);
     if (umask(0) == -1) exit(1);
     if ((fd = open("/dev/null", O_RDWR)) == -1) exit(1);
     dup2(fd, STDIN_FILENO);
     dup2(fd, STDOUT_FILENO);
     dup2(fd, STDERR_FILENO);
     close(fd);
     if (signal(SIGTERM, sigterm_handler) == SIG_ERR) exit(1);
 }

 int main(int argc, char *argv[])
 {
     wb.number=0;
     p_daemon();
     port=atoi(argv[1]);
     if(strcmp(argv[2],"-f")==0)
     {
         loadfile(argv[3]);
     }
     else
     {
         if(strcmp(argv[2],"-n")==0)
         {
             entries=atoi(argv[3]);
         }
         loadfile("");
     }
     Bind_Socket();
     Listening();
     return(0);
 }
