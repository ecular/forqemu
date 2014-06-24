/*./a.out VMname cmd_line*/
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int cal_num(char *str)
{
    int sum = 0;
    char *end = str+strlen(str);
    char *start = str;
    while(start<=end)
        sum+=*start++;
    return sum%8090+(65535-8090);
}

int main(int argc, char **argv)
{
    int cfd;
    int recbytes;
    int sin_size;
    char buffer[1024]={0};   
    struct sockaddr_in s_add,c_add;
    int portnum = cal_num(argv[1]); 

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == cfd)
    {
        printf("socket fail ! \r\n");
        return -1;
    }

    bzero(&s_add,sizeof(struct sockaddr_in));
    s_add.sin_family=AF_INET;
    s_add.sin_addr.s_addr= inet_addr("127.0.0.1");
    s_add.sin_port=htons(portnum);

    if(-1 == connect(cfd,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
    {
        printf("connect fail !\r\n");
        return -1;
    }

    if(-1 == write(cfd,argv[2],strlen(argv[2])))
    {
        printf("write fail!\r\n");
        return -1;
    }
    close(cfd);
    return 0;
}
