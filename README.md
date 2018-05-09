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


HOW TO use CwCheat codes
------------------------
1. Put CwCheat code file as ux0:data/rcsvr/cheat/TITLE_ID.ini (the extension can also be txt or dat), e.g. below is a simple working CwCheat file for Atelier Firis (ASIA version), just save it as ux0:data/rcsvr/cheat/PCSH10026.ini:

        _S PCSH10026
        _G Atelier Firis
        _C0 LOCK DAY LEFT TO 360
        _L 0x000C6FB0 0x0000005B
        _C0 LOCK HOUR TO 0
        _L 0x000C6FB4 0x00000000
        _C0 LOCK HOUR TO 12
        _L 0x000C6FB4 0x0000000C

2. Boot the game with certain TITLE_ID, the CwCheat codes will be loaded and you can see tips with "VITA Remote Cheat" message
3. Press L+R+LEFT+SELECT to call up CwCheat menu and enable/disable sections (enabled sections are prefixed by a CIRCLE)


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
10. Hook system IO functions to allow read/write to ux0:data & ux0:temp, idea from [ioPlus](https://github.com/CelesteBlue-dev/PSVita-RE-tools/tree/master/ioPlus/ioPlus-0.1)
11. [kcp](https://github.com/skywind3000/kcp) for reliable UDP commnication
12. [GLFW](http://www.glfw.org), [gl3w](https://github.com/skaslev/gl3w), [Dear ImGui](https://github.com/ocornut/imgui) for client UI
13. [yaml-cpp](https://github.com/jbeder/yaml-cpp) for client configurations and multi-language support
14. Subproject [libcheat](https://github.com/soarqin/libcheat) for cheat codes support
