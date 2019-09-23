# **主函数**  
1. 服务器端初始化：  
    > 创建socket -> 设置端口复用 -> 绑定socket与服务器地址 -> 如果未指定端口，动态分配 -> 监听  
    ```
    int on = 1;
    unsigned int port = 4000;
    struct sockaddr_in name;
    int lfd = socket(PF_INET, SOCK_STREAM, 0);  //创建socket
    memset(&name, 0, sizeof(name));  //初始化name
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) //端口复用
    bind(lfd, (struct sockaddr *)&name, sizeof(name))
    if (*port == 0) {   //动态分配端口
        socklen_t namelen = sizeof(name);
        getsockname(lfd, (struct sockaddr *)&name, &namelen);
        *port = ntohs(name.sin_port);
    }
    listen(httpd, 5);
    ```  
2. 服务器**阻塞**等待客户端连接，一旦连接，开辟一个线程并执行对应响应函数：  
    ```
    while(1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_name, &client_name_len);
        printf("received from %s at PORT %d\n", 
                inet_ntop(AF_INET, &client_name.sin_addr, str, sizeof(str)), 
                ntohs(client_name.sin_port));
        pthread_create(&newthread , NULL, (void *)accept_request, (void *)(intptr_t)client_sock);
    }
    ```  
# **响应函数**  
1. 获取请求报文的头：  
    ```
    int i = 0; 
    char buf[1024];
    char method[255];
    /*get_line返回获取到的字节数*/
    int numchars = get_line(client_sock, buf, sizeof(buf)); 
    
    /*不为空，以及未到达上限，将buf中Get或Post拷贝进method*/
    while (!isspace((int)buf[i]) && (i < sizeof(method) - 1)) { 
        method[i] = buf[i];
        i++;
    }
    method[i] = '\0';
    ```  
    <font color=#ff0000>**注意**：如果既不是post也不是get请求，报错退出；</font>
2. 获取请求头的url：
    ```
    int j = i;
    i = 0;
    char url[255];
    while (isspace((int)buf[j]) && (j < numchars)) j++;  //跳过空格

    /*取出报文头的url，该url不包含host*/
    while (!isspace((int)buf[j]) && (i < sizeof(url) - 1) && (j < numchars)) {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';
    ```
3. 设置cgi值：   
    - 如果是**post请求**, cgi=1;  
    - 如果是**带参数的get请求**, cgi=1, 并使用一个指针指向参数;
    - 如果url指向的地址是**可执行文件**, cgi=1;    
    - 其余情况cgi=0;  
  
4. 组合url，让其指向服务器上的一个绝对路径（path），如果path是一个目录（文件夹），默认修改path指向主页地址。  
5. 使用stat函数绑定path：  
    > **失败**，即路径错误，将剩余请求内容读出并丢弃，返回404错误；  
    > **成功**，若**cgi != 1 (get无参请求)**, 直接将path内容读出并send回客户端；若**cgi == 1, 执行cgi脚本，函数参数:client_sock, path, method, query(get请求的请求参数)。