VITA Remote Cheat
=================
VITA Remote Cheat 是一个在PC上通过网络连接到PSVTIA以修改游戏内存和解锁奖杯taiHEN插件

安装说明
============

1. 将 tai/rcsvr.skprx 复制到你的 taiHEN 插件目录，修改插件的 config.txt 文件，把 rcsvr.skprx 添加到 *KERNEL 加载列表里，举例，如果你把文件复制为 ux0:tai/rcsvr.skprx，那么在 config.txt 里添加两行：

        *KERNEL
        ux0:tai/rcsvr.skprx

2. 将 tai/rcsvr.suprx 复制到 ux0:tai/

3. 确保 PSVITA/PSTV 的 IP 地址可以被你的 PC 访问，运行任意游戏，等待屏幕上出现 VITA Remote Cheat 加载的字样后在 PC 上运行 rcclient，点击自动连接按钮

4. 如果第三步无法连上插件，那么可以通过网络设置菜单或者VitaShell(FTP模式下按SELECT)获取 PSVITA 的 IP 地址，然后在客户端内输入地址并点击连接按钮

5. 现在你可以搜索内存或者解锁奖杯了


感谢
------
1. [VitaSDK](https://github.com/vitasdk) 工具链
2. [henkaku](https://github.com/henkaku/henkaku) & [taiHEN](https://github.com/yifanlu/taiHEN) 以支持3.60
3. [henkaku](https://github.com/TheOfficialFloW/henkaku) & [taiHEN](https://github.com/TheOfficialFloW/taiHEN) 的3.65移植以支持3.65
4. 通过UDP调试日志，想法来自 [libdebugnet](https://github.com/psxdev/debugnet)
5. 参考了 [oclockvita](https://github.com/frangarcj/oclockvita) 的显示函数, 以及 [libvita2d](https://github.com/xerpi/libvita2d) 的PGF支持
6. 内存修改，想法来自 [rinCheat](https://github.com/Rinnegatamante/rinCheat)
7. 奖杯解锁，想法来自 [TropHAX](https://github.com/SilicaAndPina/TropHAX)
8. 禁止游戏关闭网络的方法来自 [InfiniteNet](https://github.com/Rinnegatamante/InfiniteNet)
9. 内存池，想法来自 [taipool](https://github.com/Rinnegatamante/taipool), 实现代码修改自 [liballoc 1.1](https://github.com/blanham/liballoc)
10. 系统IO钩子以允许插件读写ux0:data和ux0:temp的文件，想法来自 [ioPlus](https://github.com/CelesteBlue-dev/PSVita-RE-tools/tree/master/ioPlus/ioPlus-0.1)
11. [kcp](https://github.com/skywind3000/kcp) 支持可靠的UDP网络通信
12. [GLFW](http://www.glfw.org), [gl3w](https://github.com/skaslev/gl3w), [Dear ImGui](https://github.com/ocornut/imgui) 实现客户端UI
13. [yaml-cpp](https://github.com/jbeder/yaml-cpp) 支持客户端配置和多语言
