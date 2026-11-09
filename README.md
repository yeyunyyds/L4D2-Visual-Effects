# L4D2 Visual Effects

求生之路 2 的客户端视觉效果插件，基于 **Windows x86 / L4N v2**。项目及 DLL 标识为 `L4D2Visual`。

当前为 **0.8.0-main-test1 本地主线整合测试**，合并了实体形变、识别修复、Tank 范围提醒和 `feature/tank-rock-prediction`。尚未推送远端 main，也未发布正式版本；现有正式下载仍是 [0.7.4](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v0.7.4)。

## 功能与入口

| 主菜单入口 | 内容 | 总开关默认 |
| --- | --- | --- |
| 邦友的酒 | 周期拉伸、实体分类与批量开关、范围实验、128/256 骨骼切换 | 关 |
| 实体缩放与拉伸 | 整体大小、XYZ 比例，8种特感/8名求生者、道具和动态类别 | 关 |
| 自定义喷漆 | 本地图片、墙面显示、单图大小、数量限制、可选 T 键/自动下一张 | 关 |
| Tank 行为预测 | 朝向、估算攻击扇区、动作计时、样式与位置、范围内提醒 | 关 |
| Tank 石头预测 | 绿色轨迹、静态世界落点标记及显示外观 | 关 |

手动识别在「邦友的酒 → 实体分类」和「实体缩放与拉伸」中使用同一套目标身份：默认优先角色本体，可切换候选或「本机手持」，再独立调整武器/附件。使用已校验的客户端射线和身体范围；切图、失败、角色资料不足时不沿用旧识别。类别开关作用于同类型/角色的全部实体，不是临时实体实例。

Tank 范围提醒位于「Tank 行为预测 → 显示内容 → 攻击范围内提醒」。新配置中红线默认开，红色计时条/填充/描边默认关，Tank 总开关仍默认关；原有条、方向及范围样式保留用户提供的默认值。旧配置的显式设置继续保留。

石头预测独立于 Tank 行为总开关：本功能关闭时不采样石头、不调用落点查询。开启后子项默认显示轨迹和落点，预测窗口500 ms。采用固定重力320和半尺寸16的假定碰撞箱，落点圆表示估算接触位置，不表示伤害范围；动态实体碰撞和网络延迟不保证准确。未包含刀石、自动攻击或 JSONL 采集实验。

P2P 联机喷漆及自动搜寻默认关闭。**联机功能未经测试，不保障 VAC 安全。**

## 安装与测试

参见 [安装说明](docs/INSTALL.md) 与 [本次整合及验收项目](docs/MAIN-INTEGRATION.md)。测试包根目录仅 `bin/` 和 `README.md`，许可证位于 bin 内；配套源码单独提供，不放入安装包。更新前完全退出游戏，备份已有 DLL 和 `hud.json`，保留图片目录。

只需替换插件，不能替换游戏自带 `studiorender.dll`。原版与此前验证版本的 Studio 接入继续共用。首次启用原生形变、描边或场景射线后 DLL 可能固定到游戏进程结束，更新需要彻底退出游戏。

- [实体缩放与拉伸](docs/ENTITY-TRANSFORM.md)
- [共用准星识别修复](docs/UNRELEASED-AIM-FIX.md)
- [Tank 范围提醒](docs/TANK-WARNINGS.md)
- [Tank 石头预测设计及整合说明](docs/design/tank-rock-prediction.md)

客户端显示和预测均不代表服务器真实命中/安全窗口。BSP 地图刷子、光束、粒子或不在网络实体目录的静态道具，不保证支持骨骼形变。

## 构建

```sh
ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh
CXX_X86=i686-w64-mingw32-g++ bash scripts/build-mingw.sh
python3 scripts/package.py
```

测试使用原生 C++17 编译器和 ASan/UBSan；DLL 使用 Windows x86 MinGW，安装包需要已构建的 DLL。main 保留源码与文档，安装 ZIP 仅在构建目录生成；实测通过后再推送和安排正式版本。

## 许可

GPL-3.0，见 [LICENSE](LICENSE)。DangoMusic、Skeeto 及第三方来源见 [NOTICE.md](NOTICE.md) 和 vendor 内许可证。不捆绑游戏 DLL、原版模型或第三方插件二进制。
