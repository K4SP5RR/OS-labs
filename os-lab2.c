    #define _POSIX_C_SOURCE 200809L
    #include <stdio.h>
    #include <stdbool.h>      
    #include <stdlib.h>      
    #include <string.h>      
    #include <unistd.h>      
    #include <sys/types.h>   
    #include <sys/socket.h>  
    #include <netinet/in.h>  
    #include <arpa/inet.h>
    #include <signal.h>      
    #include <sys/select.h>  
    #include <errno.h>      

    int port = 8080;
    volatile sig_atomic_t was_sighup = 0;

    void sighup_handler(int signum) {
        (void)signum;
        was_sighup = 1;
    }

    int main(){
        sigset_t blocked_mask;
        sigset_t original_mask;
        sigemptyset(&blocked_mask);
        sigaddset(&blocked_mask, SIGHUP);
        if (sigprocmask(SIG_BLOCK, &blocked_mask, &original_mask) == -1) {
            perror("sigprocmask");
            return EXIT_FAILURE;
        }

        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = sighup_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        if (sigaction(SIGHUP, &action, NULL) == -1) {
            perror("sigaction");
            return EXIT_FAILURE;
        }


        int server_fd;

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1){
            perror("socket");
            return EXIT_FAILURE;
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            perror("setsockopt");
            close(server_fd);
            return EXIT_FAILURE;
        }
        
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
            perror("bind");
            close(server_fd);
            return EXIT_FAILURE;
        }
        if (listen(server_fd, 16) == -1) {
            perror("listen");
            close(server_fd);
            return EXIT_FAILURE;
        }
        printf("Сервер запущен на порту %d\n", port);

        int client_fd = -1;
        while (true) {
            fd_set read_fds;
            FD_ZERO(&read_fds); 
            FD_SET(server_fd, &read_fds);

            if (client_fd != -1)
                FD_SET(client_fd, &read_fds);

            int max_fd = server_fd;

            if (client_fd > server_fd)
                max_fd = client_fd;

            if (was_sighup) {
                printf("Получен SIGHUP!\n");
                was_sighup = 0;
            }
            int res = pselect(max_fd+1, &read_fds, NULL,NULL,NULL,&original_mask);
            if (res == -1){
                if (errno == EINTR){
                    continue;
                }
                perror("pselect");
                break;
            }
            if (FD_ISSET(server_fd, &read_fds)){
                int new_fd = accept(server_fd, NULL, NULL);
                if (new_fd == -1){
                    if (errno == EINTR) {
                        continue;
                    }
                    perror("accept");
                    continue;
                }
                printf("Подключение установлено!\n");
                if (client_fd == -1){
                    client_fd = new_fd;
                }
                else{
                    close(new_fd);
                }

            }
            if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)){
                char buff[256];
                ssize_t res = recv(client_fd, buff, sizeof(buff), 0);
                if (res == -1){
                    if (errno == EINTR) {
                        continue;
                    }
                    perror("recv");
                    close(client_fd);
                    client_fd = -1;
                }
                else{
                    if (res == 0){
                    printf("Клиент корректно закрыл соединение\n");
                    close(client_fd);
                    client_fd = -1;
                    }

                    else{
                        printf("%zd байт получено\n", res);
                    }
                }
            }
        }
        if (client_fd != -1){
            close(client_fd);
        }
        close(server_fd);

        if (sigprocmask(SIG_SETMASK, &original_mask, NULL) == -1) {
            perror("sigprocmask restore");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }