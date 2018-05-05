VITA Remote Cheat
=================
VITA Remote Cheat is a plugin for 3.60/3.65 taiHEN used to modify memory or unlock trophies from PC throught network

Installation
------------
1. Copy tai/rcsvr.skprx to your taiHEN plugin folder, modify tai plugin config.txt, add rcsvr.skprx to *KERNEL section, e.g. if you copy it to ux0:tai/rcsvr.skprx, add 2 lines in config.txt as following:

        *KERNEL
        ux0:tai/rcsvr.skprx

2. Copy tai/rcsvr.suprx to ux0:tai/

3. Ensure the IP address of your PSVITA/PSTV can be accessed from your PC, start any game on PSVITA, run rcclient on PC after "VITA Remote Cheat" shown up, click Autoconnect button

4. If step 3 is not connecting, you can get PSVITA IP address in Network Settings or VitaShell (Press SELECT in FTP MODE), input in the client and click Connect button

5. Now your can search memory or unlock trophies as well


Credits
-------
1. [VitaSDK](https://github.com/vitasdk) for toolchain
2. [henkaku](https://github.com/henkaku/henkaku) & [taiHEN](https://github.com/yifanlu/taiHEN) for 3.60 support
3. 3.65 port of [henkaku](https://github.com/TheOfficialFloW/henkaku) & [taiHEN](https://github.com/TheOfficialFloW/taiHEN) for 3.65 support
4. Debug log through UDP idea from [libdebugnet](https://github.com/psxdev/debugnet)
5. [oclockvita](https://github.com/frangarcj/oclockvita) for basic blitting functions, [libvita2d](https://github.com/xerpi/libvita2d) for PGF support
6. Memory modification idea from [rinCheat](https://github.com/Rinnegatamante/rinCheat)
7. Trophy unlocking idea from [TropHAX](https://github.com/SilicaAndPina/TropHAX)
8. Way to prevent games from disabling net from [InfiniteNet](https://github.com/Rinnegatamante/InfiniteNet)
9. Memory pool idea from [taipool](https://github.com/Rinnegatamante/taipool), codes modified from [liballoc 1.1](https://github.com/blanham/liballoc)
10. [kcp](https://github.com/skywind3000/kcp) for reliable UDP commnication
11. [GLFW](http://www.glfw.org), [gl3w](https://github.com/skaslev/gl3w), [Dear ImGui](https://github.com/ocornut/imgui) for client UI
12. [yaml-cpp](https://github.com/jbeder/yaml-cpp) for client configurations and multi-language support
