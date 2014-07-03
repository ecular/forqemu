#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <sys/stat.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SERVER_PORT 8888
#define LISTEN_QUEUE_LENGTH 20
#define BUFFER_SIZE 1024
#define MAX_THREAD_NUM 100

typedef struct{
    char *name;
    int n;
}LIST;

int thread_count = 0;
pthread_mutex_t mutex_list;
pthread_t threads[MAX_THREAD_NUM];
LIST name_list[MAX_THREAD_NUM + 1];

int add_vmname(char *vmname,int conn_socket)
{
    name_list[name_list[0].n + 1].n=(int)conn_socket;
    name_list[name_list[0].n + 1].name = (char *)malloc(strlen(vmname)+3);
    if(name_list[name_list[0].n + 1].name == NULL)
        return 1;
    strcpy(name_list[name_list[0].n + 1].name,vmname);
    name_list[0].n=name_list[0].n+1;
    //printf("reveice notify:%s ,name =%s\n ",vmname,name_list[name_list[0].n].name);
    //fflush(stdout);
    return 0;
}

int del_vmname(char *vmname)
{
    int i = 0,j = 0;
    for(i = 1;i <= name_list[0].n; i++)
    {
        if(strcmp(name_list[i].name,vmname) == 0)
        {
            free(name_list[i].name);
            for(j = i;j < name_list[0].n; j++)
                name_list[j] = name_list[j+1];
            name_list[0].n -= 1;
            return 0;
        }
    }
    return 1;
}

int broadcast_add(char *vmname)
{
    int i, j = 0;
    char send_data[20]={'+'};
    strcat(send_data,vmname);
    strcat(send_data," ");
    for(i = 1;i < name_list[0].n; i++)
    {
        if(write((int)name_list[i].n,send_data,strlen(send_data)) > 0)
        {
            printf("broadcast !%s! to %s success!\n",send_data,name_list[i].name);
        }
        else
        {
            j++;
            printf("broadcast !%s! to %s failed!\n",send_data,name_list[i].name);
        }
    }
    if(!j)
        return 0;
    else
        return 1;
}

int broadcast_del(char *vmname)
{
    int i,j=0;
    char send_data[20]={'-'};
    strcat(send_data,vmname);
    strcat(send_data," ");
    for(i = 1;i <= name_list[0].n; i++)
    {
        if(write((int)name_list[i].n,send_data,strlen(send_data)) > 0)
        {
            printf("broadcast %s to %s success!\n",send_data,name_list[i].name);
        }
        else
        {
            j++;
            printf("broadcast %s to %s failed!\n",send_data,name_list[i].name);
        }
    }
    if(!j)
        return 0;
    else
        return 1;
}

int send_current_status(int conn_socket)//"+vmname1 +vmname2 "
{
    int i;
    char now_status[100]={0};
    for(i = 1;i < name_list[0].n; i++)
    {
        strcat(now_status,"+");
        strcat(now_status,name_list[i].name);
        strcat(now_status," ");
    }
    write(conn_socket,now_status,strlen(now_status));
    printf("send now_status:!%s! to %s\n",now_status,name_list[i].name);
    fflush(stdout);
}

//int mysystem(const char *cmdstring, char *buf, int len)
//{
//    int   fd[2];
//    pid_t pid;
//    int   n, count;
//    memset(buf, 0, len);
//    if(pipe(fd) < 0)
//        return -1;
//    if((pid = fork()) < 0)
//        return -1;
//    else if(pid > 0)      /* parent process */
//    {
//        close(fd[1]);     /* close write end */
//        count = 0;
//        while((n = read(fd[0], buf + count, len)) > 0 && count > len)
//            count += n;
//        close(fd[0]);
//        if(waitpid(pid, NULL, 0) > 0)
//            return -1;
//    }
//    else                  /* child process */
//    {
//        close(fd[0]);     /* close read end */
//        if(fd[1] != STDOUT_FILENO)
//        {
//            if(dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
//            {
//                return -1;
//            }
//            close(fd[1]);
//        }
//        if(execl("/bin/sh", "sh", "-c", cmdstring, (char*)0) == -1)
//            return -1;
//    }
//    return 0;
//}

int find_socket(LIST *list,char *name)
{
    int i;
    for(i=1;i<=list[0].n;i++)
    {
        if(strcmp(list[i].name,name)==0)
            return list[i].n;
    }
    return -1;
}
/**
 * 接收数据的执行函数
 * @param conn_socket
 */
void recv_data(void *conn_socket);

/*
 * 1 RED
 * 2 YELLOW
 * 3 GREEN
 * 0 ERROR
 * */
int get_private(char *vmname)
{
    if(vmname == NULL)
        return 0;
    char *content;
    char result[10];
    int i;
    FILE *fp;
    fp=fopen("/home/ecular/qtproject/build-VmManager-Desktop_Qt_5_2_0_GCC_64bit-Debug/test.xml","r");
    fseek(fp, 0, SEEK_END);
    int readsize = ftell(fp); 
    content = (char *)malloc(readsize+1);
    fseek(fp, 0, SEEK_SET);
    for(i=0;!feof(fp);i++)
        fscanf(fp,"%c",&content[i]);
    fclose(fp);
    char *start=strstr(content,vmname);
    if(start == NULL)
        return 0;
    char *p1 = strstr(start,"<PrivateLevel>");
    char *p2 = strstr(p1,"</PrivateLevel>");
    p1+=14;
    i=0;
    while(p1!=p2)
        result[i++]=*p1++;
    if(strstr(result,"Red")!=NULL)
        return 1;
    if(strstr(result,"Yellow")!=NULL)
        return 2;
    if(strstr(result,"Green")!=NULL)
        return 3;
    free(content);
}
/*
 * 主方法
 */
int main(int argc, char** argv)
{
    name_list[0].n=0;
    pthread_mutex_init(&mutex_list,NULL);
    // 建立Socket
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Socket create failed.\n");
        return EXIT_FAILURE;
    }

    {
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
    }

    // 设置服务器端监听套接字参数
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof (server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    // 绑定端口
    if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof (server_addr)))
    {
        printf("Bind port %d failed.\n", SERVER_PORT);
        return EXIT_FAILURE;
    }

    // 开始监听
    if (listen(server_socket, LISTEN_QUEUE_LENGTH))
    {
        printf("Server Listen Failed!");
        return EXIT_FAILURE;
    }

    // 开始处理客户端连接
    bzero(&threads, sizeof(pthread_t) * MAX_THREAD_NUM);
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);

        // 建立客户端连接
        int client_conn = accept(server_socket, (struct sockaddr *) &client_addr, &length);
        if (client_conn < 0)
        {
            printf("Server Accept Failed!\n");
            return EXIT_FAILURE;
        }

        // 新建线程, 使用新线程与客户端交互
        int pthread_err = pthread_create(threads + (thread_count++), NULL, (void *)recv_data, (void *)client_conn);
        if (pthread_err != 0)
        {
            printf("Create thread Failed!\n");
            return EXIT_FAILURE;
        }

    }
    close(server_socket);
    return (EXIT_SUCCESS);
}

void recv_data(void *conn_socket)
{
    printf("New connected! %x\n",(int)conn_socket);
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    char cmd1[200]={0},cmd2[200]={0};
    char vmname_from[20]={0},vmname_to[20]={0},local_vmname[20]={0};

    int length = 0;
    while((length = recv((int)conn_socket, buffer, BUFFER_SIZE, 0)) >= 0)
    {
        if (length == 0)
        {
            pthread_mutex_lock(&mutex_list);//P
            if(del_vmname(local_vmname) == 0)
            {
                printf("delete success and Thread Exit\n");
                printf("broadcast del result:%d\n",broadcast_del(local_vmname));
                pthread_mutex_unlock(&mutex_list);//V
                fflush(stdout);
                return;
            }
            else
            {
                pthread_mutex_unlock(&mutex_list);//V
                printf("delete failed Thread Exit! and no broadcast..!!!!\n");
                fflush(stdout);
                return;
            }
        }

        if(buffer[0] == '*' && buffer[1] == '#')//*# vmaname
        {
            // name_list[name_list[0].n + 1].n=(int)conn_socket;
            // name_list[name_list[0].n + 1].name = (char *)malloc(strlen(buffer));
            // strcpy(name_list[name_list[0].n + 1].name,buffer+3);
            // name_list[0].n=name_list[0].n+1;
            // printf("reveice notify:%s ,name =%s\n ",buffer,name_list[name_list[0].n].name);
            // fflush(stdout);
            strcpy(local_vmname,buffer+3);
            pthread_mutex_lock(&mutex_list);//P
            if(!add_vmname(local_vmname,(int)conn_socket))
            {
                send_current_status((int)conn_socket);//send "+vmname1 +vmname2 "
                printf("broadcast add result:%d\n",broadcast_add(local_vmname));
                pthread_mutex_unlock(&mutex_list);//V
                printf("%s add success!\n",local_vmname);
                fflush(stdout);
            }
            else
                pthread_mutex_unlock(&mutex_list);//V
        }

        if(strstr(buffer,"applay") != NULL)
        {
            printf("%s #\n",buffer);
            memset(vmname_from,0,20);
            memset(vmname_to,0,20);
            memcpy(vmname_from,buffer,strchr(buffer,' ')-buffer);
            strcat(vmname_to,strstr(buffer,"applay ")+7);
            if(get_private(vmname_from) != 0 && get_private(vmname_to) != 0 && get_private(vmname_from) > get_private(vmname_to))//allow operate(can green --> red)
            {
                printf("judgement result:YES\n");
                write((int)conn_socket,"OK",2);
                strcat(cmd1,"virsh qemu-monitor-command ");
                memcpy(cmd1+27,buffer,strchr(buffer,' ')+1-buffer);
                strcat(cmd1," --hmp 'drive_add 0 id=my_usb_disk1,if=none,file=/home/ecular/tmpstorage/tmp.img'");
                printf("%s\n",cmd1);
                system(cmd1);
                strcat(cmd2,"virsh qemu-monitor-command ");
                memcpy(cmd2+27,buffer,strchr(buffer,' ')+1-buffer);
                strcat(cmd2," --hmp 'device_add usb-storage,id=my_usb_disk1,drive=my_usb_disk1'");
                printf("%s\n",cmd2);
                system(cmd2);
                memset(cmd1,0,200);
                memset(cmd2,0,200);
                memset(buffer,0,BUFFER_SIZE);
            }
            else
            {
                printf("judgement result:NO\n");
                write((int)conn_socket,"NO",2);
                memset(buffer,0,BUFFER_SIZE);
            }
        }

        if(strstr(buffer,"finishcopy") != NULL)
        {
            printf("%s\n",buffer);
            int des_port = find_socket(name_list,strstr(buffer,"finishcopy")+11);
            if(des_port > 0)
            {
                int flag  = write((int)des_port,"TO",2);
                printf("send to %s %d\n",strstr(buffer,"finishcopy")+11,flag);
                fflush(stdout);
            }
            else
            {
                printf("cant find des_port.");
                fflush(stdout);
            }

            fflush(stdout);
            strcat(cmd1,"virsh qemu-monitor-command");
            strcat(cmd1,strstr(buffer,"finishcopy")+10);
            strcat(cmd1," --hmp 'drive_add 0 id=my_usb_disk1,if=none,file=/home/ecular/tmpstorage/tmp.img'");
            printf("%s\n",cmd1);
            system(cmd1);
            strcat(cmd2,"virsh qemu-monitor-command");
            strcat(cmd2,strstr(buffer,"finishcopy")+10);
            strcat(cmd2," --hmp 'device_add usb-storage,id=my_usb_disk1,drive=my_usb_disk1'");
            printf("%s\n",cmd2);
            system(cmd2);
            memset(cmd1,0,200);
            memset(cmd2,0,200);
            // system("virsh qemu-monitor-command xujian --hmp 'drive_add 0 id=my_usb_disk1,if=none,file=/home/ecular/emu/drive0.raw'");
            // system("virsh qemu-monitor-command xujian --hmp 'device_add usb-storage,id=my_usb_disk1,drive=my_usb_disk1'");
        }
        //printf("received data:%s", buffer);
        //fflush(stdout);
        bzero(buffer, BUFFER_SIZE);
    }
    close(*((int *)conn_socket));
}
