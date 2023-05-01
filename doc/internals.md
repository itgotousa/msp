
## 设计内幕

### utf8编码

![](svg/x5000.svg)

### Markdown的语法树

![](svg/x5001.svg)

Markdown文档被解析器解析后，形成一个语法树。渲染引擎在把各元素显示在屏幕上时，采用深入优先的搜索顺序依次遍历该语法树。图中的编号是遍历的顺序。

语法树上的节点有如下类型：
- MDDocument 为根节点。每一个非空的md文件有且只有一个MDDocument节点。非空表示该文件存在至少一个非空白字符。
- MDHeading节点，有一个level变量表示层级，从1到6。
- MDList节点
- MDCode节点
- MDLink节点

### 控制显示的参数

![](svg/x5002.svg)

在msp.json文件中设置各种显示的参数：
- top_margin : Markdown显示区域距离屏幕顶端的距离，单位是像素。
- bottom_margin : Markdown显示区域距离屏幕底端的距离，单位是像素。
- width : Markdown显示区域的宽度，单位是像素。
- text_font : 正文使用的字体
- text_color : 正文的颜色
- h1_font : 一级标题使用的字体
- h2_font : 二级标题使用的字体
- h3_font : 三级标题使用的字体
- h4_font : 四级标题使用的字体
- h5_font : 五级标题使用的字体
- h6_font : 六级标题使用的字体
- link_color : 超链的颜色
- 后面继续设置。

### 渲染引擎

我打算采用Windows Terminal中最新的Atlas文本渲染引擎来做MSP的文本渲染。 本节内容提供了我对Atlas的研究成果。

