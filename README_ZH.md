安装说明
============

1. 将 tai/rcsvr.skprx 复制到你的 taihen 插件目录，修改插件的 config.txt 文件，把 rcsvr.skprx 添加到 *KERNEL 加载列表里，举例，如果你把文件复制为 ux0:tai/rcsvr.skprx，那么在 config.txt 里添加两行：

        *KERNEL
        ux0:tai/rcsvr.skprx

2. 将 tai/rcsvr.suprx 复制到 ux0:tai/

3. 确保 PSVITA/PSTV 的 IP 地址可以被你的 PC 访问，然后通过网络设置菜单或者VitaShell(FTP模式下按SELECT)获取 PSVITA 的 IP 地址

4. 首先运行任意游戏，等待屏幕上出现 VITA Remote Cheat 加载的字样后在 PC 上运行 rcsvr_client，输入 PSVITA 的 IP 地址，点击连接按钮

5. 现在你可以搜索内存或者解锁奖杯了
