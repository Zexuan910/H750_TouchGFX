模拟器修复说明

这版删除了 TouchGFX/gui/src/screen_screen/screenView.cpp 这个旧残留文件。
原文件引用了不存在的 gui/screen_screen/screenView.hpp，会导致 TouchGFX Designer Run Simulator 编译失败。

使用方法：
1. 解压本工程
2. 打开 TouchGFX/H750_TouchGFX.touchgfx
3. 点击 Generate Code
4. 点击 Run Simulator

如果仍然失败，请把 Designer 底部 Console/Log 里的第一条红色 error 截图发出来。
