 #include<stdio.h>
 #include<stdlib.h>
 #include<string.h>
 #include<sys/socket.h>
 #include<sys/types.h>
 #include<unistd.h>
 #include<netinet/in.h>
 #include<arpa/inet.h>
 #include <openssl/aes.h>
 #define AES_BITS 256
 #define MSG_LEN 256

 int port=8888;
 char* ip_address="127.0.0.1";
 unsigned char key[1000][33];
 int key_count;
 char base64_encode[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

 int Base64ToByte(unsigned char* base64,int len,unsigned char* byte)
 {
     int i=0;
     int j=0;
     int l;
     l=len/4*3;
     int num[4];
     while(i<len)
     {
         if(base64[i+2]=='=')
         {
             l-=2;
             base64[i+3]='A';
             base64[i+2]='A';
         }
         else
         {
            if(base64[i+3]=='=')
            {
                l-=1;
                base64[i+3]='A';
            }
         }
         num[0]=strchr(base64_encode,base64[i])-base64_encode;
         num[1]=strchr(base64_encode,base64[i+1])-base64_encode;
         num[2]=strchr(base64_encode,base64[i+2])-base64_encode;
         num[3]=strchr(base64_encode,base64[i+3])-base64_encode;
         byte[j]=num[0]*4+num[1]/16;
         byte[j+1]=num[1]%16*16+num[2]/4;
         byte[j+2]=num[2]%4*64+num[3];
         j+=3;
         i+=4;
     }
     byte[l]='\0';
     return(l);
 }

 int ByteToBase64(unsigned char* byte,int len,unsigned char* base64)
 {
     int i=0;
     int j=-1;
     while(i<len)
     {
         if(i+2<len)
         {
             j++;
             base64[j]=base64_encode[byte[i]/4];
             j++;
             base64[j]=base64_encode[byte[i+1]/16+byte[i]%4*16];
             j++;
             base64[j]=base64_encode[byte[i+1]%16*4+byte[i+2]/64];
             j++;
             base64[j]=base64_encode[byte[i+2]%64];
         }
         else
         {
             if(i+2==len)
             {
                 j++;
                 base64[j]=base64_encode[byte[i]/4];
                 j++;
                 base64[j]=base64_encode[byte[i+1]/16+byte[i]%4*16];
                 j++;
                 base64[j]=base64_encode[byte[i+1]%16*4];
                 j++;
                 base64[j]='=';
             }
             else
             {
                 j++;
                 base64[j]=base64_encode[byte[i]/4];
                 j++;
                 base64[j]=base64_encode[byte[i]%4*16];
                 j++;
                 base64[j]='=';
                 j++;
                 base64[j]='=';
             }
         }
         i+=3;
     }
     base64[j+1]='\0';
     return(j+1);
 }

 void GetKeys(char *filename)
 {
     int i=0;
     long tail;
     FILE* fr;
     char base64[50];
     fr=fopen(filename,"r");
     if(fr!=NULL)
     {
         fseek(fr,0,SEEK_END);
         tail=ftell(fr);
         fseek(fr,0,SEEK_SET);
         while(ftell(fr)<tail-1)
         {
             fscanf(fr,"%s",base64);
             Base64ToByte(base64,44,key[i]);
             i++;
         }
         key_count=i;
         fclose(fr);
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
         close(client_fd);
         printf("Disconnected from the server!\n");
         exit(0);
         return(0);
     }
     else
     {
         msg[i]='\0';
         return(i);
     }
 }

 int aes_encrypt(unsigned char* in,unsigned char* key,unsigned char* out)
 {
     int i;
     if(!in || !key || !out) return 0;
     unsigned char iv[AES_BLOCK_SIZE*2];
     for( i=0; i<AES_BLOCK_SIZE*2; ++i)
     {
     	iv[i]=0;
     }
     AES_KEY aes;
     if(AES_set_encrypt_key((unsigned char*)key, 256, &aes) < 0)
     {
         return 0;
     }
     int len=strlen(in);
     AES_cbc_encrypt((unsigned char*)in, (unsigned char*)out, len, &aes, iv, AES_ENCRYPT);
     return 1;
 }

 int aes_decrypt(unsigned char* in,unsigned char* key,unsigned char* out)
 {
     int i;
     if(!in || !key || !out) return 0;
     unsigned char iv[AES_BLOCK_SIZE*2];
     for( i=0; i<AES_BLOCK_SIZE*2; ++i)
     {
     	iv[i]=0;
     }
     AES_KEY aes;
     if(AES_set_decrypt_key((unsigned char*)key, 256, &aes) < 0)
     {
         return 0;
     }
     int len=strlen(in);
     AES_cbc_encrypt((unsigned char*)in, (unsigned char*)out, len, &aes, iv, AES_DECRYPT);
     return 1;
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
     buf1[0]='@';
     itoa(num,&buf1[1]);
     l=strlen(buf1);
     buf1[l]=msgType;
     itoa(len,&buf1[l+1]);
     l=strlen(buf1);
     buf1[l]='\n';
     write(client_fd,buf1,l+1);
     write(client_fd,buf2,len+1);
 }


 void Get_Msg(int client_fd)
 {
     int i,j,id,length,type,l,len;
     char num1[250];
     char num2[250];
     char buf[500];
     char* msg;
     i=1;
     j=0;
     len=Read_Msg(client_fd,buf,-1);
    // printf("{%s}",buf);
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
     msg=calloc(length+2,sizeof(char));
     Read_Msg(client_fd,msg,length+1);
     msg[length]='\0';
     printf("Get Message: %s\n",msg);
     if(type==-1)
     {
         try_aes_decrypt(msg);
     }
 }

 void Ask_Msg(int num,int client_fd)
 {
     char buf1[512]={};
     int l;
     buf1[0]='?';
     itoa(num,&buf1[1]);
     l=strlen(buf1);
     buf1[l]='\n';
     write(client_fd,buf1,l+1);
     Get_Msg(client_fd);
 }

 int try_aes_decrypt(char* in)
 {
     int i=0,j=0;
     int ans=-1;
     char text[1000]={};
     char byte[1000]={};
     char const_str[1000]="CMPUT379 Whiteboard Encrypted v0\n";//\n";
     for(i=0;i<key_count;i++)
     {
         Base64ToByte(in,strlen(in),byte);
         aes_decrypt(byte,key[i],text);
         if(strlen(text)>=strlen(const_str))
         {
             ans=1;
            // printf("%s\n",text);
             for(j=0;j<strlen(const_str);j++)
             {
                 if(text[j]!=const_str[j])
                 {
                     ans=-1;
                     break;
                 }
             }
             if(ans==1)
             {
                 printf("It's encrypted message,the true message is:\n");
                 printf("%s\n",&text[strlen(const_str)]);
                 return(ans);
             }
         }
     }
     printf("It's encrypted message,We don't have the true key\n");
     return(ans);
 }

 void Update_Msg(int num,int type,int client_fd)
 {
     char text[1000]={};
     unsigned char buf[500]={};
     unsigned char encode[1000]={},enc[1000]={};
     unsigned char tmp[1000]={};
     int len,i;
     printf("Please input clear text(len<100):\n");
     fgets(text, 1000, stdin);
     //fgets(text, 1000, stdin);
     scanf("%[^\n]",text);
     if(type==1)
     {
         Send_Msg('p',num,strlen(text),text,client_fd);
     }
     else
     {
         strcpy(encode,"CMPUT379 Whiteboard Encrypted v0\n");//\n");
         strcat(encode,text);
         while(strlen(encode)%32!=0)
         {
             strcat(encode," ");
         }
         i=0;
        // printf("%d{%s}\n",len,encode);
         do{
             aes_encrypt(encode,key[i],enc);
             len=ByteToBase64(enc,strlen(enc),text);
           //  printf("%d{%s}\n",len,text);
             Base64ToByte(text,strlen(text),enc);
           //  printf("%d{%s}\n",strlen(enc),enc);
             aes_decrypt(enc,key[i],tmp);
            // printf("%d{%s}\n",i,tmp);
             i++;
         }while(strcmp(tmp,encode)!=0 && i<key_count);
         Send_Msg('c',num,len,text,client_fd);
        // printf("%d{%s}\n",len,text);
     }
     Read_Msg(client_fd,buf,-1);
     //printf("{%s}",buf);
     Read_Msg(client_fd,buf,-1);
    // printf("{%s}",buf);
 }

 int bind_client()
 {
     int client_fd;
     int err,n;
     struct sockaddr_in addr_ser;
     client_fd=socket(AF_INET,SOCK_STREAM,0);
     bzero(&addr_ser,sizeof(addr_ser));
     addr_ser.sin_family=AF_INET;
     addr_ser.sin_addr.s_addr=inet_addr(ip_address);
     addr_ser.sin_port=htons(port);
     err=connect(client_fd,(struct sockaddr *)&addr_ser,sizeof(addr_ser));
     if(err==-1)
     {
         printf("Can not connnect to serve!\n");
         return(-1);
     }
     else
     {
         return(client_fd);
     }
 }

 void run()
 {
     int client_fd,max,type;
     char order[256];
     char ret[500];
     char* msg;
     int len,number;
     client_fd=bind_client();
     if(client_fd!=-1)
     {
         len=Read_Msg(client_fd,ret,-1);
         len=Read_Msg(client_fd,ret,-1);
         max=atoi(ret);
         printf("The number of entries in the whiteboard : %d\n",max);
         while(1)
         {
             printf("Please input your message(Get/Update/Quit)\n");
             scanf("%s",order);
             if(strcmp(order,"Quit")==0)
             {
                 break;
             }
             if(strcmp(order,"Get")==0)
             {
                 printf("Please input number(1~%d)\n",max);
                 scanf("%s",order);
                 number=atoi(order);
                 if(number>max || number<=0)
                 {
                     printf("ERROR NUMBER!\n");
                 }
                 else
                 {
                     Ask_Msg(number,client_fd);
                 }
             }
             if(strcmp(order,"Update")==0)
             {
                 printf("Please input number(1~%d)\n",max);
                 scanf("%s",order);
                 number=atoi(order);
                 if(number>max || number<=0)
                 {
                     printf("ERROR NUMBER!\n");
                     continue;
                 }
                 if(key_count>0)
                 {
                     printf("Whether it is encrypted(1=No 2=Yes)\n");
                     scanf("%s",order);
                     type=atoi(order);
                     if(type>2 || type<=0)
                     {
                         printf("ERROR TYPE!\n");
                         continue;
                     }
                 }
                 else
                 {
                     type=0;
                 }
                 Update_Msg(number,type,client_fd);
             }
         }
         close(client_fd);
     }
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

 int main(int argc, char *argv[])
 {
     port=atoi(argv[2]);
     ip_address=argv[1];
     if(argc==4)
     {
         GetKeys(argv[3]);
     }
     run();
     return 0;
 }
