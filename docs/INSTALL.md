# FPB Native Gate Probe — diagnostic only

请勿覆盖自己的 hud.json。此为实验诊断 DLL，不是正式版。[诊断测试说明](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/experimental/fpb-native-gate-probe/docs/FPB-NATIVE-GATE-PROBE.md)。

# L4D2Visual 安装说明

## 正式版本：v1.1.0

下载：[L4D2Visual v1.1.0 正式 Release](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v1.1.0)。正式安装包为 `L4D2Visual-1.1.0.zip`；配套提供相同源码提交的源码包与两个 SHA-256 校验文件。历史 [v0.8.0](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v0.8.0) 保留。

支持：**Left 4 Dead 2，Windows x86，L4N v2，以及符合插件 PE/SHA-256 校验的游戏模块版本**。其他游戏 DLL 版本将因安全检查拒绝对应 Hook 或补丁，不能通过强制关闭校验来解决。

## 安装

1. **完全退出游戏**，备份现有 `bin/neko/plugins/L4D2Visual.dll` 和 `bin/neko/plugins/L4D2Visual/hud.json`。
2. 从上述正式 Release 下载 `L4D2Visual-1.1.0.zip`；解压后包含 `bin/` 和 `README.md`，没有第二层安装 ZIP。
3. 把 `bin/` 合并到游戏目录。安装包内的默认配置位于 `bin/neko/plugins/L4D2Visual/hud.json`。
4. 老用户可以保留自己原有的 `hud.json`，新功能缺失的设置使用默认值。不要用新默认配置覆盖旧版自定义样式，除非你希望全部重置。
5. 在 L4N 插件菜单中打开 L4D2Visual。正式 DLL 版本字符串为 `1.1.0`；以日志和菜单显示的实际 DLL 版本为准。

**不要替换游戏自带的 `studiorender.dll`。** 若使用过 Hook/原生补丁，更新 DLL 前必须完全退出游戏，不能仅返回主菜单。

## 功能及默认状态

- **实体缩放与拉伸**：分类识别、整体/XYZ 比例、「邦友的酒」动画与范围触发；总开关关闭。
- **第一人称身体（FPB）**：原生玩家身体、头/手臂隐藏、动态模型定位、逐模型校准；默认关闭，第三人称兼容默认关闭。异常恢复显示 `Clean / Patched / Unknown`，`Unknown` 时不再启用原生补丁。
- **FreeCamera**：菜单开关、自由飞行、第一人称手臂武器隐藏；功能默认关闭，**F6 快捷键绑定默认关闭**，首次请求才安装三个 ClientMode Hook。
- **Tank 行为预测**：条、箭头、攻击范围、红色警告，以及**只读的建筑遮挡修复**；总开关关闭。建筑遮挡选项默认开，但只有 Tank 预测启用后才会工作。
- **Tank 石头预测**：独立菜单、轨迹和静态世界落点；总开关关闭，预测窗口 500 ms，使用固定重力 320 和假定半尺寸 16 的碰撞箱。
- **日志**：默认 Info；Debug 可查看周期性诊断信息。

Tank 预测的红色范围线偏好默认开，红色进度条和填充默认关。旧版原生 Glow 描边功能已彻底移除，`tankPrediction.warnOutline` 被忽略；不再修改 Tank 或特感的 Glow 字段。已删除的自定义喷漆 / P2P 配置不会恢复。

## 外部相机控制（DangoMusic 等）

FreeCamera 菜单中主动开启外部接口授权，默认关闭。支持 ABI v4、首次闲置等待、键鼠优先接管、再次闲置交还。DangoMusic v1.0.0 可使用该接口。支持在 hud.json 中自定义快捷键及移动键。当前游戏 DLL 采用 2026 年 11 月的已验证版本标识，未知版本仍拒绝对应 Hook。

## 验收与注意事项

建议分别检查 FPB 开关/切图/死亡后的还原、FreeCamera 菜单首次开启和 F6 可选绑定、Tank 在墙体后方时条/箭头/范围同步遮挡，以及高压环境下的模型绘制性能。异常恢复 `Unknown` 时不要尝试通过重新开启功能覆盖该状态，查看 FPB 诊断并完全退出游戏。

Tank 与石头位置/攻击范围都是客户端估算，不表示真实伤害或服务器判定。世界射线不等于逐像素遮挡，部分遮挡、动态道具、特殊 MOD 可能有差异。**联机/VAC 安全不作保证**；请优先离线测试。

## 包内结构与许可证

安装包根目录仅有 `bin/` 与 `README.md`，不含独立的许可证安装文件。完整 GPL-3.0 及第三方许可原文会追加到包内 README。源代码仓库保留 `LICENSE`、`NOTICE.md` 与第三方许可证文件。

Source commit 和版本可在包内 README 末尾、配套源码 ZIP 及 GitHub Release 构建记录中核对。服务器中 FreeCamera 鼠标转向问题尚未取得最新补丁的完整实机复验，仍应留意兼容性。
