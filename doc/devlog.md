
#### 开发日志：
***
- 20230427 - 正在把PG的内存池和动态哈希表的源代码移植过来。下一步先实现flex/bison对md/svg文件的语法解析。
- 20230428 - PostgreSQL的内存池已经前移完毕。下一步迁移动态哈希表模块.
- 20230507 - 动态GIF可以跑通了，目前考虑支持APNG.
- 20230510 - 引入了plutosvg的库，已经可以顺利渲染svg了。

![](svg/msplogo.svg)


		POINT pt;
		SIZE sz;
		RECT rc;
		GetClientRect(&rc);
		GetScrollSize(sz);
		GetScrollOffset(pt);
