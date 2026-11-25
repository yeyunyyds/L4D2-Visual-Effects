# FPB 原生绘制许可探针 1

基线 Visual v1.1.0。仅诊断分支，不发布正式版，不改变现有视角判断、门控、骨骼计算或 FreeCamera 行为。不安装额外 Hook，不调用未知游戏函数，不写入诊断目标内存。原有 FPB 原生门控补丁照常工作。

## 假设与限制
已验证 DrawModel 分支：client+0x2234d1 push edi; call client+0xf670; add esp,4; cmp byte ptr [eax+0x25ec],0; jz +0x13。原生分支为零时跳过绘制，非零时继续；字段语义、getter 参数和返回对象尚未证实。
探针仅在当前 PE/SHA 和完整分支指纹匹配时读取原始字段；仅支持已完整识别的无参数依赖的 MOV EAX,imm32; RET / MOV EAX,[abs32]; RET。其他 getter 标为 raw=-1、reason=getter-unresolved，不调用、不猜测 EDI 或对象地址。自动记录调用点前96字节和 getter 前160字节（含模块基址），以便后续反汇编。第一阶段可能只能收集机器码，无法直接得到原生许可。

## 测试
退出游戏，备份正式 DLL 和 hud.json，仅替换 DLL，不覆盖 hud.json。建议单机官图测试；不保证 VAC 安全。
1. FPB、动态定位、头/手隐藏和第三人称兼容全部开启；站立、走动、下蹲、低头。
2. 打包至少三次，记录开始、身体消失、恢复和结束的大概时点；如可行录屏。
3. 第三人称手动切换，再回第一人称。
4. 关闭 FPB 后重复打包作为原生基线；无需启用 FreeCamera。
5. 提供 L4D2Visual.log（及存在时 L4D2Visual.previous.log）。
日志自动输出 INFO，无需开启 Debug。关键词 FPB_NATIVE_PROBE。记录已完成渲染后的帧编号、raw、原 JZ 推导的局部分支结果 originalWouldDraw（-1 未知）、补丁字节 jz、视角、肩部 CVar、几何值、门控更新前后状态、骨骼处理计数差。几何值是现有兼容机制记录，关闭兼容后可能无效或旧值。采样发生在 View_Render 返回后，不证明精确 DrawModel 调用瞬间状态。变化及每秒心跳按秒批量输出，每秒最多64个事件；overflow 表示丢弃数。
不使用 raw 控制 FPB，不把非零解释为已验证的第三人称。自动测试/交叉编译不等于实机验证。
