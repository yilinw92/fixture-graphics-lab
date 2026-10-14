# 从 CAD 数据到可解释的制造检查

## 这个项目解决什么问题

工程人员拿到图纸后，不能只凭“看起来没问题”进入加工。单位错误会把尺寸放大 25.4 倍；开口或自交轮廓无法定义可靠材料区域；圆孔即使在外接矩形里，也可能切穿一个内凹缺口。项目把输入检查、几何判定、交互查看和可保存报告连成同一条链路。

这是一份独立作品集。讲解时可以说“我设计了一个用于夹具图纸检查的工具”，公司的项目经历则需要另外的事实支撑。

## 先运行最有价值的失败案例

```powershell
.\run.ps1 -Mode Test
.\build\fixture_cli.exe --dxf examples/notch_violation.dxf --out artifacts/notch
.\build\fixture_viewer.exe --dxf examples/notch_violation.dxf
```

这是 140×90 mm、上方带 U 形缺口的零件。H7 中心 `(70,51)`，半径 6。按外接矩形判断，最小边距是 `min(70,51,70,39)-6=33 mm`，完全合格；实际内凹边在 y=55，最近距离 4，剩余材料为 `4-6=-2 mm`，所以已经穿出轮廓。

讲解时不要只说“我做了碰撞检测”。把这个输入、旧方法为何漏判、新方法怎样算、最终报告为何禁止报价讲完整。

## 模块阅读顺序

1. `geometry.hpp`：Scene 同时保留材料外轮廓、圆孔和制造规则；Finding 保存实际值与要求值。
2. `dxf.cpp`：把 group-code/value 转成实体，再处理单位和原点。未知几何不能悄悄忽略。
3. `polygon.cpp`：线段相交、点在多边形内、最近边界点、材料三角化。
4. `geometry.cpp`：广相位过滤、精确间隙、解析面积和报告。
5. `viewer.cpp`：把同一场景送到几何核心，再由 VAO/VBO/shader 画出网格与诊断线。
6. `tests/test_cad.cpp`：先读“矩形判断错误”的回归案例，再读随机对照测试。

## 三个需要真正理解的算法

**有符号边界间隙。** 对每条边 AB，投影参数 `t=clamp(dot(P-A,B-A)/|B-A|²,0,1)`，得到最近点；取所有线段的最小距离。射线奇偶规则判断孔中心在轮廓内外，内为正、外为负，再减孔半径。只判断中心在里面不够，大圆仍可能穿出边界。

**Sweep-and-prune。** 先按圆的左边界排序，剔除右边界已经落在当前圆左侧、且不可能满足 web 阈值的候选，再检查 Y 区间，最后才算 hypot。测试把所有违规孔对的集合与独立暴力算法比较。稀疏孔阵列快，不代表所有输入都快：若所有 X 区间重叠，活跃集合仍可能很大。

**带孔材料三角化。** 汇集外轮廓和孔环的顶点 X；相邻 X 构成一条竖直 slab。选中点扫描所有非竖直边，按交点 Y 排序；第 1–2、3–4 等区间是材料。左右端的交点组成梯形，再拆成三角形。这种方法支持凹多边形和不相交孔，但预算和二次复杂度限制了大图。圆只是绘制时用 96 段近似，面积和切割长度仍用解析圆计算。

## 图形管线与 C++ 能力

工程毫米坐标 → 生成实际材料三角形 → VBO 上传 → VAO 解释 position/colour → vertex shader 的 mat3 视图变换 → 光栅化 → fragment shader → framebuffer。

孔处没有材料三角形，底下的网格能直接露出，不再用背景色圆盘覆盖板面。非法相交孔暂时只画轮廓，避免生成一个貌似合理的材料网格。CPU 网格按几何 revision 缓存，单纯缩放不重新三角化。图形 API 对象由 RAII 管理；窗口必须比 Renderer 更晚析构，才能保证释放 GL 对象时上下文有效。

撤销/重做目前使用有上限的 Scene 快照。小模型实现直接；大模型可以改命令日志和局部缓存失效。不要声称现在已经做了大规模 CAD 编辑器。

## 四次动手练习：每次都要交证据

| 顺序 | 修改任务 | 验收条件 |
| --- | --- | --- |
| 1 | 将 H7 的 y 改为 46、半径仍为 6 | 内凹边距 3 mm，刚好通过；解释为何 46.01 不通过 |
| 2 | 将外轮廓顶点顺序反转 | 面积、违规集合和三角网格面积不变 |
| 3 | 加一个不同半径的孔，使两孔间距刚好 3 mm | 优化算法与暴力算法都通过；减小 0.01 mm 后都失败 |
| 4 | 给 CLI 增加可配置材料厚度 | 正确影响体积，不影响二维间隙、面积和切割长度；负数和 NaN 被拒绝 |

第一题答案：H7 的顶部为 `46+6=52`，到 y=55 的间隙是 3；y=46.01 时只剩 2.99。移动后也要重新检查它与其他孔的间隙，不能只盯着一个约束。

## 面试介绍及追问

> I built a C++ drawing-inspection tool for concave fixtures with circular cutouts. The core imports a deliberately limited DXF subset, normalizes units, validates boundary topology, and computes signed clearances against the actual part outline. I implemented sweep-and-prune candidate filtering and checked it against a brute-force oracle on seeded random scenes. The OpenGL viewer renders a triangulated material domain with holes, shares the same analysis with the CLI, and supports undo, picking and report export. One regression case demonstrates why checking only the bounding rectangle can miss a hole crossing a concave notch.

**为什么不用现成 CAD kernel？** 当前范围只有直边外轮廓和圆孔，自己实现便于展示和测试算法。若产品必须支持曲线、多个嵌套环、可靠布尔运算和极端退化输入，会优先评估成熟几何内核。不要把练习范围扩大成所有 DXF 都支持。

**如何证明三角化正确？** 面积与精确的折线材料区域一致，三角形质心落在材料内，方向反转不改变结果，再配合真实 GL 捕获检查。这些是有用证据，仍不是形式化证明；更复杂轮廓还需要更强的重叠/覆盖检查。

**为什么报价按外接矩形而非净面积？** 当前假设购买矩形毛坯，孔和凹槽不会退回材料成本。真实采购和排样策略需要额外业务模型，演示费率不能当企业报价。

DXF 字段依据：[Autodesk LWPOLYLINE 文档](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-748FC305-F3F2-4F74-825A-61F04D757A50.htm)。
