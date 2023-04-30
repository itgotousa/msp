
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
