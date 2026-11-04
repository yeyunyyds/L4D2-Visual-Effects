# L4D2 Visual Stretch

**求生之路 2 特殊感染者视觉拉伸插件 · 0.4.0**

基于 **L4N v2 / Windows x86**，让特殊感染者随周期竖直伸缩，保留双脚、脚趾的原始位置与朝向，以及原有走路动画。插件在绘制阶段使用骨骼矩阵副本，不写入实体位置、碰撞边界或命中骨骼缓存。

项目展示名为 **L4D2 Visual Stretch**；现有插件标识、游戏内菜单、DLL 和安装包继续使用 `L4D2Visual`，配置路径保持兼容。

[下载正式版](https://github.com/yeyunyyds/L4D2-visual/raw/2dd0a515a8dbbe552698464903ef84e7638b517a/L4D2Visual-0.4.0.zip) · [下载分支](https://github.com/yeyunyyds/L4D2-visual/tree/zip) · [验证记录](docs/validation.md) · [渲染接口研究](docs/research/render-abi.md)

## 下载与分支

| 分支 | 用途 | 内容 |
| --- | --- | --- |
| `main` | 源码开发与文档；默认分支的目标 | 源码、测试、构建与打包脚本、文档、许可证；不提交 DLL、ZIP 或本地配置 |
| `zip` | 安装包下载 | 已发布的 ZIP；不用于源码开发 |

`main` 的初始源码从正式安装包 `L4D2Visual-0.4.0.zip` 中的 `source/` 完整恢复。对应下载分支提交为 `2dd0a515a8dbbe552698464903ef84e7638b517a`；既有 `v0.4.0` 标签保留指向该打包提交。后续源码更改在 `main` 开发，生成的安装包单独发布至 `zip`。

0.4.0 安装包 SHA-256：

```text
f3d14172d534e305598c2805f83d22ec2ff1b2e6e0381cd67462a06fcfe287c2
```

安装包包含插件 DLL 和对应完整源码，不含游戏 DLL、模型或纹理。GitHub 的 **Code → Download ZIP** 下载的是当前分支快照；安装游戏插件请使用上方正式包链接。

## 安装与启用

1. 准备已安装 **L4N v2** 的 Windows x86 游戏环境。游戏构建必须匹配 [render-profile.json](docs/research/render-profile.json) 中记录的 `client.dll`、`engine.dll`、`studiorender.dll`。
2. 完整退出游戏，解压正式包，将 `bin` 合并到游戏根目录。确认插件位于 `bin/neko/plugins/L4D2Visual.dll`，且只加载一份。
3. 启动游戏并进入地图，在 **L4N → L4D2Visual 0.4.0 → 开启拉伸测试** 中启用。
4. 进入 **动画预设与细调** 调整效果；使用 **保存设置** 保存动画参数与菜单外观。
5. 关闭拉伸后检查外观恢复。更新或移除插件前完整退出游戏，不要运行时卸载。

拉伸开关默认关闭，且不跨游戏会话保存。卸载时删除该 DLL 即可停止加载；配置与日志保存在同目录下的 `L4D2Visual/` 文件夹，可按需备份或删除。

## 动画设置

默认使用**平滑往复，周期 0.5 秒，竖直比例 0.5～1.1**。X/Y 不变，所有目标共用时间基准。

| 设置 | 行为 |
| --- | --- |
| 动画预设 | 平滑往复、Q 弹回弹、双跳脉冲 |
| 往复周期 | 每次调整 0.1 秒；范围 0.1～10.0 秒，越短越快 |
| 最小 / 最大比例 | 分别每次调整 0.1；总范围 0.1～3.0，上下限至少相差 0.1 |
| 恢复动画默认 | 恢复平滑往复、0.5 秒、0.5～1.1 |
| 保存设置 | 保存到 `bin/neko/plugins/L4D2Visual/hud.json` |

切换预设保留当前周期与比例；所有曲线限制在设定上下限内。修改参数会重启本轮周期。首次迁移旧配置时使用新的动画默认值并保留菜单外观，之后保存的自定义动画参数会正常恢复。

## 菜单操作

复用 DangoMusic 的原生 HUD 与 ImGui/D3D9 后端，L4N 提供菜单入口。

- `1～9` 选择，`0` 返回，`Esc` 或 `/` 关闭，`-` / `=` 翻页。
- 菜单外观支持背景预设、透明度、字号、位置和可选 F10 开关。
- 外观与动画参数通过主菜单的 **保存设置** 持久化，拉伸开关仍默认关闭。
- **接口与日志诊断** 显示实体关联、引擎 / Studio 回调、已拉伸绘制及日志写入状态。

## 目标与工作方式

目标分类包括 Smoker、Boomer、Hunter、Spitter、Jockey、Charger、Tank、Witch，以及感染者队伍的 `CTerrorPlayer` / `NextBotCombatCharacter`。不额外按模型路径过滤。

绘制关联检查完整实体句柄、当前模型索引、renderable、Studio header、地图代次、休眠状态与快照时效。未知构建、过期目标、无效骨骼或无法确认双脚时保持原始绘制。

从实际渲染骨骼生成独立矩阵副本，沿 Z 轴缩放后恢复双脚及其后代（含脚趾）的完整原始矩阵，再交给 Studio DrawModel。原引擎 DrawModelExecute 保留原始参数，不修改共享骨骼；这不额外提供 IK 求解。骨骼名称、层级与脚部索引按模型缓存，切图或模型头部变化时重建，缓存上限 64 项。

## 兼容性与验证边界

用户已在游戏中确认兼容路径有可见效果，并反馈 0.4.0 帧率有所改善；目前没有量化 FPS 对比数据。离线 ABI 核对、Linux 回归与 Windows x86 交叉编译已在此前发布流程通过，具体历史见 [验证记录](docs/validation.md)。

- **游戏构建限制：** 校验三个游戏 DLL 的磁盘完整 SHA-256；不同构建拒绝启用。已有绘制回调会保留并串接，但其他渲染插件组合仍需实测。
- **渲染开销：** 启用且进图时暂时将 `cl_modelfastpath` 设为 `0`，覆盖可见主模型；关闭或退出地图时恢复。若用户期间主动更改该值，恢复逻辑保留用户的新值。该设置不写入游戏配置，常规绘制路径仍可能增加开销。
- **模型限制：** 无法识别两根 foot 骨骼时跳过。模型替换可能改变脚部权重，脚与腿的蒙皮过渡需实测。
- **画面边界：** 不走 Studio DrawModel 的批量绘制可能保持原样；不扩大渲染裁剪边界。大比例下的裁剪、LOD、阴影和光照一致性仍需验证。
- **碰撞与命中：** 实现不写入相关数据；实际游戏行为仍需与关闭拉伸时做对比，不能把离线检查等同于完整实机验收。

## 排查无效果或异常

1. 确认只加载了一份 0.4.0 DLL，进入地图后启用拉伸，等待至少一秒。
2. 查看 **接口与日志诊断**：关联次数为 0 与关联成功但绘制为 0 属于不同阶段的问题。
3. 查看 `bin/neko/plugins/L4D2Visual/L4D2Visual.log`。日志自动启用，含版本、接口校验、目标分类、模型绘制统计和姿态拒绝原因；启用时每两秒记录状态，避免逐帧写文件。
4. `fastpath=0` 表示兼容路径生效，`-1` 表示该变量未通过验证。构建校验失败时先核对游戏 DLL，不要跳过校验。
5. 复现后关闭拉伸，提交日志、游戏构建、其他渲染插件列表与画面现象。无日志时检查 DLL 加载情况及目录写权限。

## 开发、测试与打包

使用 C++17。Linux 回归需要 `g++` 与 ASan/UBSan；Windows x86 交叉编译需要 `i686-w64-mingw32-g++` 及 Windows/D3D9 开发库。

```sh
git clone --branch main https://github.com/yeyunyyds/L4D2-visual.git
cd L4D2-visual

# 五组本地回归；受限运行器中可单独关闭泄漏检测
ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh

# Windows x86 DLL，可用 CXX_X86 指定编译器
bash scripts/build-mingw.sh

# 生成 build/L4D2Visual-0.4.0.zip（DLL + 对应完整源码）
python3 scripts/package.py

# 可选：使用自己合法持有的游戏 DLL 进行离线构建核对
python3 scripts/verify_render_profile.py \
  --client /path/client.dll \
  --engine /path/engine.dll \
  --studio /path/studiorender.dll
```

`CXX` 可指定本地测试编译器。不要把游戏 DLL 或用户 `hud.json` 提交到仓库。构建结果写入被忽略的 `build/`，发布时将安装包单独提交到 `zip`，不要合并下载分支到 `main`。

```text
src/       插件入口、动画、绘制关联、骨骼处理与诊断
vendor/    DangoMusic、L4N SDK、ImGui 等依赖及声明
tests/     动画、设置、菜单、骨骼姿态与拉伸回归
scripts/   本地测试、交叉编译、打包及 PE / ABI 校验
docs/      验证记录与渲染接口研究
```

## 0.4.0 更新

- 默认周期改为 0.5 秒，竖直比例为 0.5～1.1。
- 增加平滑往复、Q 弹回弹、双跳脉冲预设及 0.1 步进微调。
- 支持保存动画参数，迁移旧配置时保留菜单外观。
- 缓存骨骼元数据，减少重复内存读取；非目标与关闭状态提前返回。
- 保留已确认可见效果的常规模型兼容路径与逐类特感诊断。

## 许可证与致谢

项目采用 [GPL-3.0](LICENSE)。代码来源、版本和第三方声明见 [NOTICE.md](NOTICE.md)。

感谢 [LDBDangoMusic](https://github.com/yeyunyyds/LDBDangoMusic) 的实体采集与原生 HUD，以及 [L4D2 Dxvk 32to64 Bridge](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge) 的 L4N SDK 和相关实现。ImGui、nlohmann/json 等第三方许可保留在 `vendor/`；L4N SDK 头文件的来源与许可边界见 NOTICE。
