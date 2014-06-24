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

#define SERVER_PORT 8888
#define LISTEN_QUEUE_LENGTH 20
#define BUFFER_SIZE 1024
#define MAX_THREAD_NUM 100

int thread_count = 0;
pthread_t threads[MAX_THREAD_NUM];

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
    char vmname_from[20]={0},vmname_to[20]={0};

    int length = 0;
    while((length = recv((int)conn_socket, buffer, BUFFER_SIZE, 0)) >= 0)
    {
        printf("loop!\n");
        if (length == 0)
        {
            printf("Thread Exit\n");
            return;
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
                write((int)conn_socket,"NO",2);
                memset(buffer,0,BUFFER_SIZE);
            }
        }

        if(strstr(buffer,"finishcopy") != NULL)
        {
            printf("%s\n",buffer);
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
