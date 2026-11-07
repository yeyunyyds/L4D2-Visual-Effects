# L4D2 Visual Effects 0.7.4

Windows x86 / L4N v2 客户端视觉效果插件。

**联机功能未经测试，不保障 VAC 安全。**

退出游戏后，将安装包的 bin 合并到游戏根目录。安装包根目录只有 bin/ 和 README.md；许可证位于 bin/neko/plugins/L4D2Visual/licenses。不要额外替换游戏的 studiorender.dll，本插件兼容已经验证的两种 Studio DLL。

包内配置是 bin/neko/plugins/L4D2Visual/hud.json。覆盖前备份旧配置；如果保留旧文件，会继续沿用旧开关和样式。诊断 logging / markKey 旧字段已被移除并忽略，旧配置无法重新开启已删除的诊断功能。

Tank 行为预测：总开关默认关闭。画面显示、攻击计时条、朝向箭头、范围及目标连线默认打开，开启总开关即可使用。邦友的酒、实体选择和范围实验、自定义喷漆、T 绑定、自动换图、P2P、自动匹配及 F10 默认关闭。

默认 Tank 样式：冰蓝刻度、竖条、X=-32/Y=-24、整体55%、长度88/厚度8、箭头大小100%、箭头和范围线宽3倍。进入“Tank 行为预测 → 样式与位置预览”调整，精调页用 - / = 修改所选参数；普通菜单仍用 - / = 翻页。完成后保存设置。布局预览是正式样式工具，已保留。

0.7.4 采用统一80项序列表，根据主动画和全部采样动画层识别出拳、投石、移动及其他动作。40/43/45 等用于出拳起手观察，48–51 为投石。投石不触发普通出拳计时。计时依据游戏前摇和普通攻击间隔估算，不代表真实命中、挥空或安全窗口；修改全局动画序号的 Mod 可能误判，表外序号保持未知。无 SourceMod 依赖。

本正式版删除旧的 Mod/MDL 动画映射、共享容器扫描、动画事件 factory/RecvProxy 诊断、JSONL 写入、测试状态页及测试开关。保留“当前 Tank 状态”和常用参数设置，不生成 tank-prediction、tank-animation-events 或 tank-animation-binding 诊断文件；旧文件不会自动删除。

用户已反馈原版 Tank、当前测试 Mod Tank 与上传的原版 studiorender.dll 可用。这不表示所有 Mod、地图、DXVK 情况及多人场景均已充分实测。client.dll / engine.dll 仍限已验证版本，Studio 两种已验证配置继续保留。

邦友的酒和自定义喷漆继续保留。喷漆图片目录为 bin/neko/plugins/L4D2Visual/images；联机及自动匹配分别手动开启。P2P 默认关闭，默认关闭不等于 VAC 安全保证。
