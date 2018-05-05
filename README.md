VITA Remote Cheat
=================
VITA Remote Cheat is a plugin for taihen used to modify memory or unlock trophies from PC throught network

Installation
------------
1. Copy tai/rcsvr.skprx to your taihen plugin folder, modify tai plugin config.txt, add rcsvr.skprx to *KERNEL section, e.g. if you copy it to ux0:tai/rcsvr.skprx, add 2 lines in config.txt as following:

        *KERNEL
        ux0:tai/rcsvr.skprx

2. Copy tai/rcsvr.suprx to ux0:tai/

3. Ensure the IP address of your PSVITA/PSTV can be accessed from your PC, start any game on PSVITA, run rcsvr_client on PC after "VITA Remote Cheat" shown up, click Autoconnect button

4. If step 3 is not connecting, you can get PSVITA IP address in Network Settings or VitaShell (Press SELECT in FTP MODE), input in the client and click Connect button

5. Now your can search memory or unlock trophies as well


Credits
-------
1. [VitaSDK](https://github.com/vitasdk)
2. [henkaku](https://github.com/henkaku/henkaku) [taiHEN](https://github.com/yifanlu/taiHEN)
3. [henkaku 3.65 port](https://github.com/TheOfficialFloW/henkaku) [taiHEN 3.65 port](https://github.com/TheOfficialFloW/taiHEN)
4. Memory modification idea from [rinCheat](https://github.com/Rinnegatamante/rinCheat)
5. Trophy unlocking idea from [TropHAX](https://github.com/SilicaAndPina/TropHAX)
6. Way to prevent games from disabling net [InfiniteNet](https://github.com/Rinnegatamante/InfiniteNet)
7. [kcp](https://github.com/skywind3000/kcp) for reliable UDP commnication
8. [GLFW](http://www.glfw.org) [gl3w](https://github.com/skaslev/gl3w) [Dear ImGui](https://github.com/ocornut/imgui) for client UI
9. [yaml-cpp](https://github.com/jbeder/yaml-cpp) for client configurations and multi-language support
