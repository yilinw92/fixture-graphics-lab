# C++ / Graphics 项目讲解

## 60 秒英文介绍

> I built a C++ engineering review tool for a rectangular plate with circular holes. The core checks edge and hole-to-hole clearances and exports reproducible diagnostics and a synthetic cost breakdown. The OpenGL 3.3 viewer uses vertex buffers, a vertex array and GLSL shaders, with a matrix mapping engineering coordinates to clip space. I added picking, pan, cursor-centered zoom and interactive edits. The geometry core is independent of the renderer, so mathematical rules and file exports can be tested without a graphics context. It is a bounded portfolio tool, not a full CAD kernel.

## 如何对应制造工程背景

实际工程经常需要把“能装、能焊、留有绝缘间隙”等要求变成尺寸和验收条件。本项目用合成的板件与孔把这类判断变成可计算、可显示、可导出的规则。可以说明这个题目来自你对制造问题的理解，但不能把新作品说成 Nebula 已经使用的产品。

## 面试官会追问什么

**为什么解析计算，而不是看图像像素判断？**

计算用工程单位中的几何参数，不受屏幕分辨率或缩放影响。渲染的 96 边近似只用于显示；孔间距仍然用圆的解析公式。

**VBO、VAO、shader 分别负责什么？**

VBO 存顶点数据；VAO 记录如何解释顶点数据；vertex shader 做逐顶点处理，fragment shader 处理光栅化产生的片元。不是 VAO 存了全部顶点，也不是 fragment shader 逐个三角形运行。

**为什么 shader 用 mat3，不是 mat4？**

这是二维齐次坐标变换。mat3 足以表达二维平移和缩放。三维透视及相机变换通常使用 mat4；当前工具没有声称实现三维 CAD 场景。

**你如何选择一个孔？**

鼠标像素先经逆视图变换回到毫米坐标，再检查点到孔中心的距离是否小于等于半径。点击判定与视图缩放独立。

**为什么用 RAII？**

把资源生命周期与 C++ 对象绑定，正常退出和异常退出都能释放资源。还要注意 GPU 资源依赖有效上下文，不能先销毁窗口上下文再销毁 Renderer。

**怎样测试图形程序？**

先测试几何、正逆变换和拾取；再测试 CLI 的真实退出码与导出文件；最后建立真实 OpenGL 上下文编译 shader、绘制并读回截图。数学测试通过不等于渲染一定正确，截图好看也不等于几何判断正确。

**这是不是一个真实报价系统？**

不是。费率是演示数据；当前公式由毛坯材料面积、边界切割长度、数量和一次设置费组成。存在几何问题时不生成报价。真正报价还需要工艺、损耗、批次、设备与供应商等信息。

**下一步怎样扩展？**

先明确受支持输入和规则，再考虑 DXF 子集、轮廓拓扑、空间索引或三维显示。不能把这些尚未实现的扩展列为当前能力。

## 建议展示顺序

1. 测试输出与正常场景。
2. 按 `2` 引入边距失败，解释 1 mm 与 3 mm。
3. 移动孔使规则通过，展示即时反馈。
4. 缩放并拾取，解释坐标变换。
5. 导出报告，打开 JSON，解释 synthetic quote。
6. 指出一项自己完成的修改及相应测试。
