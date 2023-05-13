
## 什么是MSP?

MSP是一个开源的，百分百原生的，极简约的文档阅读器，支持Markdown, SVG和PNG的显示，故名MSP(Markdown + SVG + PNG 或 Markdown + SVG + Picture)。MSP不依赖除了操作系统以外的任何第三方的软件。为了保持软件的体积最小，MSP不支持除了Markdown/SVG和PNG以外的其它任何格式的内容。

注：在Windows平台上，利用了Windows Imaging Component (WIC)的解码器，所以默认支持png/jpg/gif等所有WIC可以解码的图像文件，故你可以使用msp当做一个图片阅读器使用，直接把图片文件拖拽到窗口中即可。

MSP阅读器的目标是支持Win64/MacOS/iOS/Linux(X64/ARM/RISC-V)平台。MSP的体系架构如下：

![](svg/x0001.svg)

MSP也可以显示以base64编码嵌入到svg中的png图片，如下所示：

![](svg/x0002.svg)

追求极度简约是MSP的设计哲学。MSP最终的可执行文件在Windows平台上为msp.exe。和该程序同一个目录下只有一个显示配置msp.json和一个运行日志文件msp.log。msp.json是json格式，里面规定的显示的宽度，字体和其它布局信息。如果你设计了比较好的显示主题，可以和其它用户分享，只要把msp.json拷贝给别的用户即可。

![](svg/x0003.svg)

#### 遵循的规范
- Markdown采用[Commonmark](https://spec.commonmark.org)
- SVG目前支持[SVG 1.1](https://www.w3.org/TR/2011/REC-SVG11-20110816/)标准
- 考虑支持[Tiny VG](https://tinyvg.tech/)，其规范的pdf文档已经上传(tinyvg-specification.pdf)

## 在Windows下的编译和运行软件的方法
- 安装Visual Studio 2022 社区版，免费的，为微软的良心点赞。
- 安装最新版的[CMake](https://cmake.org)，就是一个.exe文件，鼠标双击即可。安装好cmake后，缺省它会把cmake的执行文件放在系统路径的环境变量中。打开一个dos窗口，执行cmake，应该可以看到cmake的一些信息。如果发现cmake无法找到，你可以自行查找cmake.exe的目录，加入到系统路径的环境变量中，或者使用绝对路径指定之。
- 以上两步都是安装非常成熟的软件，非常容易，兄弟你大可不必头疼。
- 下载本项目的zip包，或者使用git clone https://github.com/itgotousa/msp.git 拉一个下来。
- 假设本项目的目录在D:\github\msp目录下。你随便找一个地方建立一个build目录，或者任何你喜欢的目录名字。假设你的目录是C:\temp\build
- 打开Visual Studio 2022的X64 Native Tools Command Prompt for VS 2022窗口，切换到C:\temp\build目录，以下操作均在该目录下进行。
- 执行： cmake -G "NMake Makefiles" D:\github\msp\src。如果想编译Release版本，可以使用cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release D:\github\msp\src
- 执行：nmake 完成软件的编译。
- 编译好的可执行文件是：D:\github\msp\build\arch\windows\msp-win64.exe，直接执行该exe即可。

***

如果你在编译过程中出现问题，请联系itgotousa@gmail.com

Enjoy coding, enjoy life!



