VITA RxCheat
============
VITA RxCheat 是一个在PC上通过网络连接到PSVTIA以修改游戏内存和解锁奖杯taiHEN插件

安装说明
------------

1. 将 tai/rcsvr.skprx 复制到你的 taiHEN 插件目录，修改插件的 config.txt 文件，把 rcsvr.skprx 添加到 *KERNEL 加载列表里，举例，如果你把文件复制为 ux0:tai/rcsvr.skprx，那么在 config.txt 里添加两行：

        *KERNEL
        ux0:tai/rcsvr.skprx

2. 将 tai/rcsvr.suprx(完整版本) 或 tai/rcsvr_lt.suprx(轻简版本) 复制到 ux0:tai/

使用客户端连接
---------------------
1. 必须使用完整版插件(rcsvr.suprx)
2. 确保 PSVITA/PSTV 的 IP 地址可以被你的 PC 访问，运行任意游戏，等待屏幕上出现 VITA RxCheat 加载的字样后在 PC 上运行 rcclient，点击自动连接按钮
3. 如果第三步无法连上插件，那么可以通过网络设置菜单或者VitaShell(FTP模式下按SELECT)获取 PSVITA 的 IP 地址，然后在客户端内输入地址并点击连接按钮
4. 现在你可以搜索内存或者解锁奖杯了

使用游戏内菜单
---------------------
1. VITA RxCheat 加载字样出现后，按照提示按下 L+R+LEFT+SELECT 可以呼出菜单
2. 在这里可以开关金手指(如果CwCheat作弊码已加载)或者解锁奖杯

如何使用金手指
---------------------
1. 将CwCheat格式金手指文件放在 ux0:data/rcsvr/cheat/TITLE_ID.ini (扩展名也可以是txt或者dat), 举例: 下面是一个亚版菲莉丝的工作室的金手指文件，将他放在 ux0:data/rcsvr/cheat/PCSH10026.ini:

        _S PCSH10026
        _G Atelier Firis
        _C0 LOCK DAY LEFT TO 360
        _L 0x000C6FB0 0x0000005B
        _C0 LOCK HOUR TO 0
        _L 0x000C6FB4 0x00000000
        _C0 LOCK HOUR TO 12
        _L 0x000C6FB4 0x0000000C

2. 引导对应TITLE_ID的游戏，金手指代码会被自动加载，你可以在 "VITA RxCheat" 字样下面看到额外的加载提示
3. 按下 L+R+LEFT+SELECT 呼出菜单，进入金手指菜单后可以在这里开关金手指项 (前面会有个圆圈表示已经打开的项)

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
11. [kcp](https://github.com/skywind3000/kcp) 实现可靠的UDP网络通信
12. [GLFW](http://www.glfw.org), [gl3w](https://github.com/skaslev/gl3w) 以及 [Dear ImGui](https://github.com/ocornut/imgui) 实现客户端UI
13. [yaml-cpp](https://github.com/jbeder/yaml-cpp) 支持客户端配置和多语言
14. 金手指支持的子项目 [libcheat](https://github.com/soarqin/libcheat)
