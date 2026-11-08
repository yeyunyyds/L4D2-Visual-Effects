# L4D2 Visual Effects 下载

Tank 石头实验（基于 0.7.2）：[demo3 安装包](L4D2Visual-0.7.2-tank-rock-demo3.zip) · [匹配源码](L4D2Visual-0.7.2-tank-rock-demo3-source.zip)。默认候选重力 320（需授权），新增默认 50% 不透明度红色填充圆、可关闭静态地图射线落点、到达时间重力拟合与 schema3 代理诊断。地图检测需单独开启；没有有效交点时圆标为未来终点。**demo3 的游戏效果仍待验证，模拟时间代理尚未作为可信时钟使用；无完整石头 hull 碰撞或绝对延迟补偿。** 实验子菜单标题为“Tank 石头实验 demo3”。不包含后续 0.7.4 / 0.8.0 的变更。[demo2 历史包](L4D2Visual-0.7.2-tank-rock-demo2.zip)。

当前正式版：[0.7.4](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v0.7.4)。本分支仅提供安装包与校验文件；源码在默认 main 分支。

功能预览：[0.8.0-transform-preview3](L4D2Visual-0.8.0-transform-preview3.zip)，新增 Tank 范围内提醒：计时条红底、范围红线/填充、实验性原生描边，均默认关闭；保留实体形变和准星识别。源码见 [v0.8.0-transform-preview3](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/v0.8.0-transform-preview3)。

0.7.4 使用精简的统一 Tank 动作序列表，移除旧模型映射和动画事件诊断。Tank 总开关默认关，相关绘制项默认开；其他功能默认关。覆盖 hud.json 前请备份。

**联机功能未经测试，不保障 VAC 安全。**

安装包根目录只有 bin/ 与 README.md。历史包保留用于回退；请优先使用正式版。
