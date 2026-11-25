# FPB 原生绘制许可探针 2

基线 Visual v1.1.0。仅诊断分支，不发布正式版，不改变现有视角判断、门控、骨骼计算或 FreeCamera 行为。不安装额外 Hook，不调用未知游戏函数，不写入诊断目标内存。原有 FPB 原生门控补丁照常工作。

## 第二阶段：读取实体候选字段
probe.1 捕获完整 getter 与调用者字节。getter 在参数非空且虚表 +0x23c 的检查通过时原样返回传入指针，否则返回空指针；调用者从 renderable-4 得到参数。probe.2 对 getter、调用者前缀、原生分支和 DrawModel 序言逐字节校验，独立验证本机求生者身份、快照时效、handle/serial、RTTI 与方法地址，再只读 entity+0x25ec。不会调用该 getter 或其虚函数，也不宣称已证明虚函数语义。raw 非零代表该局部 JZ 不跳转，不等价于整个游戏一定绘制该人物。任何验证失败返回 -1。
诊断版本在 FPB 关闭时仍保留实体快照读取，并独立计算 sampleView/sampleXY/sampleEyeZ，以便对照；正式版的逻辑保持原样。probeHandle/probeActor 与 FPB 自己的 local 状态分开记录。仍为 post-render 采样，无法证明调用瞬间的值。
## 测试
退出游戏，备份正式 DLL 和 hud.json，仅替换 DLL，不覆盖 hud.json。建议单机官图测试；不保证 VAC 安全。
1. FPB、动态定位、头/手隐藏和第三人称兼容全部开启；站立、走动、下蹲、低头。
2. 打包至少三次，记录开始、身体消失、恢复和结束的大概时点；如可行录屏。
3. 第三人称手动切换，再回第一人称。
4. 关闭 FPB 后重复打包作为原生基线；无需启用 FreeCamera。
5. 提供 L4D2Visual.log（及存在时 L4D2Visual.previous.log）。
日志自动输出 INFO，无需开启 Debug。关键词 FPB_NATIVE_PROBE。记录已完成渲染后的帧编号、raw、原 JZ 推导的局部分支结果 originalWouldDraw（-1 未知）、补丁字节 jz、视角、肩部 CVar、几何值、门控更新前后状态、骨骼处理计数差。几何值是现有兼容机制记录，关闭兼容后可能无效或旧值。采样发生在 View_Render 返回后，不证明精确 DrawModel 调用瞬间状态。变化及每秒心跳按秒批量输出，每秒最多64个事件；overflow 表示丢弃数。
不使用 raw 控制 FPB，不把非零解释为已验证的第三人称。自动测试/交叉编译不等于实机验证。
