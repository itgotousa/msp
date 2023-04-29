
## 什么是MSP?

MSP是一个开源的，百分百原生的，极简约的文档阅读器，支持Markdown, SVG和PNG的显示，故名MSP(Markdown + SVG + PNG)。MSP不依赖除了操作系统以外的任何第三方的软件。为了保持软件的体积最小，MSP不支持除了Markdown/SVG和PNG以外的其它任何格式的内容。MSP阅读器的目标是支持Win64/MacOS/iOS/Linux平台。

MSP的体系架构如下：

![](svg/x0001.svg)

MSP也可以显示以base64编码嵌入到svg中的png图片，如下所示：

![](svg/x0002.svg)

追求极度简约是MSP的设计哲学。MSP最终的可执行文件在Windows平台上为msp.exe。和该程序同一个目录下只有一个显示配置msp.json和一个运行日志文件msp.log。msp.json是json格式，里面规定的显示的宽度，字体和其它布局信息。如果你设计了比较好的显示主题，可以和其它用户分享，只要把msp.json拷贝给别的用户即可。

MSP使用到的第三方部件有：
- zlib
- libpng
- WTL as the GUI interface of Win64 platform.

由于作者对Windows平台的开发最为熟悉，所以本项目优先实现在Windows平台上的开发。以下是几点设计上的考虑
- 把平台无关的代码和操作系统相关的代码分开，方便移植。
- 使用CMake作为跨平台编译的构建系统。
- 最小化调用操作系统的功能。
- 大量借鉴PostgreSQL的源代码，因为作者是PostgreSQL DBA
- Windows下只支持64位编译。
- 在Windows下的渲染使用Direct2D和DirectWrite技术，支持GPU硬件加速

编写代码的几点考量
- *.cpp是C++代码, *.c是纯C代码，*.h是C的头文件，*.hpp是C++的头文件。
- 和平台无关的class的命名使用M开头，如MParser。和平台相关的用N开头，表示Native，如NRender
- 类的成员变量均以m_开头，和非类变量区别开来
- 从PostgreSQL拷贝的源码保持文件名、变量名和函数名不变。


***

如果你有兴趣一同开发，请联系itgotousa@gmail.com


