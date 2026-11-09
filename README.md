> 未发布修改：已修复共用准星识别与菜单开关一致性，并调整范围提醒入口/默认值。说明见 [未发布修复](docs/UNRELEASED-AIM-FIX.md)。已有 preview2/3 下载包未更新。

> 当前分支：0.8.0-transform-preview3。新增 Tank 范围内提醒（红色计时条、范围红线/填充、实验性原生描边，分别默认关闭），见[提醒说明](docs/TANK-WARNINGS.md)；保留准星手动识别、类别开关与参数调整；基于Skeeto路径的实体缩放与XYZ拉伸，扩展角色/道具/手持模型及动态Class ID，默认关闭。见[使用和验证说明](docs/ENTITY-TRANSFORM.md)。正式版仍为0.7.4，尚需游戏内验证新路径。

# L4D2 Visual Effects

求生之路 2 客户端视觉效果插件，基于 **Windows x86 / L4N v2**。项目标识及 DLL 名称为 `L4D2Visual`。

[下载正式版 0.7.4](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v0.7.4) · [安装说明](docs/INSTALL.md) · [更新说明](docs/releases/v0.7.4.md) · [下载分支](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/zip)

**联机功能未经测试，不保障 VAC 安全。**

提供三项主要功能：

- **邦友的酒**：模型拉伸，按特感、八名求生者、场景武器、手持道具等类别选择；支持准星识别与128/256骨骼容量切换。
- **自定义喷漆**：本地图片、尺寸调整、数量限制、可选 T 键绑定及默认关闭的 P2P 联机实验。
- **Tank 行为预测**：自动出拳计时、朝向、范围及目标线；支持三套样式、实体跟随布局和独立尺寸调整。

0.7.4 将 Tank 识别精简为统一80项序列表，移除旧 Mod 动画映射、共享容器扫描、动画事件诊断和测试菜单。用户已反馈原版 Tank、当前 Mod Tank 与上传的原版 Studio DLL 可用；重排动画编号的 Mod 仍可能误判。计时和范围均为估算，不代表实际命中或安全窗口，无 SourceMod 依赖。

**默认设置**：Tank 总开关关闭，相关画面显示、计时条、箭头、范围和目标连线打开；开启总开关即可使用。其他主动功能默认关闭。默认样式沿用用户确认的冰蓝竖条、55%大小、位置 -32/-24、箭头及范围线宽3倍。

退出游戏，将安装包中的 bin 合并到游戏根目录。包根目录只有 bin/ 与 README.md，许可文件位于 bin 内。覆盖旧 hud.json 前请备份；保留旧配置会继续沿用旧开关。源码位于默认 main 分支，安装 ZIP 仅放下载分支和 Releases，不打包游戏 DLL、模型或用户日志。

本项目保留现有 Present 绘制路径。曾报告 DXVK 闪烁/崩溃的 0.7.1-test 已弃用。联机实验、所有模型组合及所有 DXVK 环境未经全面验证。

构建与验证：

```bash
CXX_X86=i686-w64-mingw32-g++-posix bash scripts/build-mingw.sh
ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh
python3 scripts/package.py
```

历史研究位于 docs/research，内容可能描述已移除的测试实现；以当前源码及安装说明为准。许可证和第三方声明见 LICENSE、NOTICE.md 及 vendor 内许可文件。
