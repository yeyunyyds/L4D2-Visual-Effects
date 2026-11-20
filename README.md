# L4D2 Visual Effects

求生之路 2 的客户端视觉效果插件，基于 **Windows x86 / L4N v2**。项目及 DLL 标识为 `L4D2Visual`。

当前正式版为 **0.8.0**，基于已通过用户验收的整合测试版发布，包含实体形变、共用准星识别修复、Tank 范围提醒和石头预测。

[下载正式版](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v0.8.0) · [安装说明](docs/INSTALL.md) · [更新说明](docs/releases/v0.8.0.md) · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/v0.8.0) · [下载分支](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/zip)

## 功能与入口

| 主菜单入口 | 内容 | 总开关默认 |
| --- | --- | --- |
| 实体缩放与拉伸 | 整体大小、XYZ 比例、实体分类与准星识别；子菜单包含「邦友的酒」周期拉伸 | 形变关 |
| 第一人称身体 | 参考 Evo 风格的独立第一人称身体渲染、骨骼隐藏、动态定位及 MOD 校准（实验） | 开 |
| Tank 行为预测 | 朝向、估算攻击扇区、动作计时、样式与位置、范围内提醒 | 关 |
| Tank 石头预测 | 绿色轨迹、静态世界落点标记及显示外观 | 关 |
| 菜单 | 外观设置、Info / Debug 日志级别 | Info |

手动识别在「实体缩放与拉伸 → 邦友的酒 → 实体分类」和「实体缩放与拉伸」中使用同一套目标身份：默认优先角色本体，可切换候选或「本机手持」，再独立调整武器/附件。使用已校验的客户端射线和身体范围；切图、失败、角色资料不足时不沿用旧识别。类别开关作用于同类型/角色的全部实体，不是临时实体实例。

Tank 范围提醒位于「Tank 行为预测 → 显示内容 → 攻击范围内提醒」。新配置中红线默认开，红色计时条/填充/描边默认关，Tank 总开关仍默认关；原有条、方向及范围样式保留用户提供的默认值。旧配置的显式设置继续保留。

石头预测独立于 Tank 行为总开关：本功能关闭时不采样石头、不调用落点查询。开启后子项默认显示轨迹和落点，预测窗口500 ms。采用固定重力320和半尺寸16的假定碰撞箱，落点圆表示估算接触位置，不表示伤害范围；动态实体碰撞和网络延迟不保证准确。未包含刀石、自动攻击或 JSONL 采集实验。


## 实验分支：Evo 风格第一人称身体与日志分级

`experimental/module-host-first-person-body` 从无喷漆的模块化架构分支集成第一人称身体。该功能以 Evo 的第一人称视觉风格为设计参考，由本项目**独立实现**；未复制、截取、移植或复用 Evo 的代码、二进制程序及专有资源，也不代表两项目存在官方关联。

第一人称身体默认采用当前实机调出的视角基线：动态锚点与视角朝向跟随开启，前后 / 左右 / 高度偏移为 `-13 / -1 / -1`，头部与第三人称手臂隐藏开启，GPU 裁剪关闭。跳跃、蹲伏及第三方 MOD 仍可能有穿模，不能视作完全验证的稳定版本。

默认 Info 只保存事件及错误信息。若需要周期性实体接口、绘制统计和模块状态快照，进入「菜单 → 日志级别」开启 Debug，再保存设置。详细信息见[日志说明](docs/LOGGING.md)及[第一人称身体技术说明](docs/FIRST-PERSON-BODY-DYNAMIC-ALIGNMENT.md)。

实验版的功能注册在 `feature_composition.hpp`，Studio 包装通过中立回调接入，不使公共引擎层直接依赖第一人称功能。**此实验分支不等于 v0.8.0 正式版。**

## TLTPV 第三人称兼容（默认关闭，实验）

「第一人称身体 → TLTPV 第三人称兼容」默认**关**。本功能只适配
TLTPV 的原生 `firstperson` / `thirdpersonshoulder` 切换，不实现其他模式。
开启后，通过实时本机位置、相机眼位高度和防抖状态机确认视角；第三人称时恢复原生绘制，
停止骨骼处理并在游戏帧恢复原生 DrawModel 门控；切回第一人称时重置
动态定位追踪。关闭时维持旧版行为。

检测只读经 SHA-256 验证的 L4D2 `client.dll` 内部
`c_thirdpersonshoulder`，并结合本机网络实体与摄像机之间的水平距离。
这是针对已验证客户端版本的实验检测，不是 TLTPV 官方 API。
若镜头碰撞、快速位移或版本不匹配导致判断不明确，会优先使用原生绘制；
本方案仍需游戏内验证。「邦友的酒」现位于「实体缩放与拉伸」第一页第一项。

## 安装与测试

参见 [安装说明](docs/INSTALL.md) 与 [本次整合及验收项目](docs/MAIN-INTEGRATION.md)。安装包根目录仅 `bin/` 和 `README.md`，许可证位于 bin 内；配套源码单独提供，也可从 v0.8.0 标签获取。更新前完全退出游戏，备份已有 DLL 和 `hud.json`，保留图片目录。

只需替换插件，不能替换游戏自带 `studiorender.dll`。原版与此前验证版本的 Studio 接入继续共用。首次启用原生形变、描边或场景射线后 DLL 可能固定到游戏进程结束，更新需要彻底退出游戏。

- [实体缩放与拉伸](docs/ENTITY-TRANSFORM.md)
- [共用准星识别修复](docs/AIM-IDENTIFICATION.md)
- [Tank 范围提醒](docs/TANK-WARNINGS.md)
- [Tank 石头预测设计及整合说明](docs/design/tank-rock-prediction.md)

客户端显示和预测均不代表服务器真实命中/安全窗口。BSP 地图刷子、光束、粒子或不在网络实体目录的静态道具，不保证支持骨骼形变。

## 构建

正式版由 GitHub Actions 在 v0.8.0 标签触发后运行测试、构建 x86 DLL、打包、更新 zip 分支并发布 Release。下面命令用于开发环境；正式发布不依赖本地二进制。

```sh
ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh
CXX_X86=i686-w64-mingw32-g++ bash scripts/build-mingw.sh
python3 scripts/package.py
```

测试使用原生 C++17 编译器和 ASan/UBSan；DLL 使用 Windows x86 MinGW，安装包需要已构建的 DLL。main 保留源码与文档；安装包和校验文件通过 zip 分支与 Release 提供，不放入 main。

## 许可

GPL-3.0，见 [LICENSE](LICENSE)。DangoMusic、Skeeto 及第三方来源见 [NOTICE.md](NOTICE.md) 和 vendor 内许可证。不捆绑游戏 DLL、原版模型或第三方插件二进制。

自定义喷漆实验已移除：不再提供图片加载、T 键绑定、P2P 同步或自动发现。旧配置中的 `wallPictures` 与 `sprayP2P` 被忽略，下次保存设置时不再写出；已有图片与缓存文件不会自动删除。

## 开发结构

功能通过模块协议注册，菜单、配置、采集需求、生命周期和覆盖层调度与插件入口分离。
注册点为 `src/feature_composition.hpp`，依赖边界与线程/恢复顺序见 [模块结构](docs/MODULE-ARCHITECTURE.md)。

> TLTPV 兼容修复：移动时不再以 200ms 旧实体位置直接驱动 DrawModel 门控，
> 俯视角还会验证相机眼位高度；正常渲染期间不再重写视角状态。
