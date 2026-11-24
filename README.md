# L4D2 Visual Effects

面向《Left 4 Dead 2》的 **Windows x86 / L4N v2** 客户端视觉效果插件。插件及 DLL 名称均为 `L4D2Visual`。

**当前正式版：v1.0.0。** [下载正式安装包](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v1.0.0) · [安装说明](docs/INSTALL.md) · [更新日志](docs/releases/v1.0.0.md) · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/v1.0.0) · [下载分支](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/zip)。历史正式版 [v0.8.0](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v0.8.0) 保留供回退。

## 功能

| 菜单入口 | 功能 | 默认 |
| --- | --- | --- |
| 实体缩放与拉伸 | 实体分类、准星识别、整体/XYZ 形变、「邦友的酒」周期拉伸与范围触发 | 关闭 |
| 第一人称身体（FPB） | 原生角色身体显示、头/手臂骨骼隐藏、动态锚点、逐模型校准和可选第三人称兼容 | 关闭 |
| FreeCamera | WASD/鼠标自由飞行、Space 上升、Shift 下降、速度设置、第一人称武器隐藏 | 关闭，F6 绑定默认关闭 |
| Tank 行为预测 | 朝向、攻击范围、动作计时、红色 HUD 范围提醒、建筑遮挡修复 | 关闭 |
| Tank 石头预测 | 轨迹与静态世界落点、绘制样式配置 | 关闭 |
| 菜单与诊断 | F10 菜单、外观、Info/Debug 日志与各模块状态 | Info 日志 |

**FPB** 使用经过验证的原生 `C_TerrorPlayer::DrawModel` 路径，启用时只修改本地人物绘制门控中的一字节跳转位移，关闭时检查原始字节恢复；异常时报告 `Unknown`，不会盲目覆盖第三方修改。已移除纯诊断型 `ShouldDraw` Hook。FPB 关闭时不会再额外读取 Studio 模型数据。

> **实验分支：FreeCamera ABI v2**。手动和外部共用启停开关，键鼠优先，闲置 1–30 秒（默认 3 秒）后交还外部；支持 GetPose，F6/配置快捷键或菜单关闭即取消外部会话。见 [外部 Camera API 说明](docs/FREE-CAMERA-EXTERNAL-ABI.md)。

**FreeCamera** 可通过菜单直接切换；F6 绑定需要在菜单中单独开启，默认不占用 F6。三个 ClientMode Hook 在第一次实际请求 FreeCamera 时才安装，以后关闭时透传原生调用。第一人称武器隐藏与人物世界位置保持相互独立。菜单中的「运行数据与诊断」用于查看 Hook、运行阶段与计数。服务器环境下的鼠标转向卡顿曾有反馈，本版包含额外鼠标采样修复，但尚未取得修复后的服务器实机复验结论。

`main` 开发版新增 `freeCamera.hotkey` 字符串配置，可在 `hud.json` 中手动改为 `f7`、`p` 等；现有 `f6Binding` 继续控制是否启用快捷键，默认 F6、默认关闭。**该改动尚未包含在已发布的 v1.0.0 安装包中。** 详见 [FreeCamera 配置说明](docs/FREE-CAMERA.md)。

**Tank 遮挡修复** 使用只读的世界几何 `TraceRay` 判断。建筑遮住 Tank 时，预测 HUD 的计时条、方向箭头、范围线和红色填充同步隐藏；目标连线沿用原行为。三点射线是近似判断，不是逐像素深度测试，动态物体及半遮挡结果可能不同。**本版本不修改任何 Tank 或特感的原生 Glow 属性，也不包含特感独立描边功能。** 旧配置 `tankPrediction.warnOutline` 被忽略，不再保存。

石头落点使用静态世界碰撞估算：固定重力 320，假定碰撞箱半尺寸 16；不代表实际命中区域，不保证动态实体碰撞或网络时序。实体分类、模型形变、HUD 等原有功能仍复用共享 Source / Studio / D3D9 接口。

## 安装与更新

参见 [安装说明](docs/INSTALL.md)。从正式 Release 下载 `L4D2Visual-1.0.0.zip`，解压后得到 `bin/` 和 `README.md`，将 `bin/` 合并到游戏目录。**更新前完全退出游戏，备份 `L4D2Visual.dll` 和 `bin/neko/plugins/L4D2Visual/hud.json`**；不要替换游戏自带的 `studiorender.dll`。可保留旧配置；旧版 Glow 和喷漆键不再生效。

只支持明确校验过的 Windows x86 游戏模块；指纹不匹配时相关原生功能会拒绝执行。对于第三方插件、特殊模型、不同 DXVK/D3D9 环境，不承诺全面兼容。客户端 HUD 的射线/计时预测也不代表服务器的真实判定。**不对联机或 VAC 安全作保证**，请先在离线环境验收。

## 构建及发布状态

`v1.0.0` 由 GitHub Actions 从正式版本源码构建、验证并发布 Windows x86 安装包、配套源码 ZIP 和 SHA-256 校验文件；`main` 仍保留日常候选 CI。版本标签与安装包源码对应，历史 Release 不会覆盖。

```sh
ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh
CXX_X86=i686-w64-mingw32-g++-posix bash scripts/build-mingw.sh
python3 scripts/package.py
```

构建产物为 `build/L4D2Visual-1.0.0.zip`。源码、依赖许可与署名见 [LICENSE](LICENSE)、[NOTICE.md](NOTICE.md) 和 [v1.0.0 更新日志](docs/releases/v1.0.0.md)。安装包内的 README 附有完整许可内容。

## 设计与说明

- [FreeCamera 与第一人称武器隐藏](docs/FREE-CAMERA.md)
- [第一人称身体和骨骼处理](docs/FIRST-PERSON-BODY-ANATOMY.md)
- [Tank 范围提醒与建筑遮挡](docs/TANK-WARNINGS.md)
- [模块组成和运行时生命周期](docs/MODULE-ARCHITECTURE.md)
- [石头预测](docs/design/tank-rock-prediction.md)
- [日志说明](docs/LOGGING.md)

本项目基于 GPL-3.0 发布，不捆绑游戏 DLL、游戏模型或其他插件二进制。
