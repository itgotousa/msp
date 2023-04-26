
## 什么是MSP?

MSP是一个开源的，极简的文档阅读器，支持Markdown, SVG和PNG的显示，故名MSP(Markdown + SVG + PNG)。为了保持软件的体积最小，不支持其它任何格式的内容。MSP阅读器的目标是支持Win64/MacOS/iOS/Linux平台。

MSP的体系架构如下：

![](svg/x0001.svg)

MSP也可以显示以base64编码嵌入到svg中的png图片，如下所示：

![](svg/x0002.svg)

MSP使用到的库有：
- zlib
- WTL for Win64
- libpng


由于作者对Windows平台的开发最为熟悉，所以本项目优先实现在Windows平台上的开发。以下是几点设计上的考虑
- 把平台无关的代码和操作系统相关的代码分开，方便移植。
- 最小化调用操作系统的功能。
- 大量借鉴PostgreSQL的源代码，因为作者是PostgreSQL DBA
- Windows下只支持64位编译。
- 在Windows下的渲染使用Direct2D和DirectWrite技术，支持GPU硬件加速



===

如果你有兴趣一同开发，请联系itgotousa@gmail.com


