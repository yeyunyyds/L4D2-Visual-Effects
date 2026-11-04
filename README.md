# L4D2 Visual Effects · 安装包下载

求生之路 2 视觉效果项目；当前提供模型竖直拉伸效果。

本分支 `zip` **仅用于保存下载包**。源码、开发说明、完整安装指南和兼容性说明请前往 **[main 分支](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/main)**。

## 当前测试版 0.5.1-test

**[下载 L4D2Visual-0.5.1-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.5.1-test.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/ffb73e1) · [完整指南](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/README.md)

- 实体开关整理为特感、求生者、场景可拾取武器与物品、手上道具、其他，各分类支持全开 / 全关。
- 保留“一键全部关闭（含实验）”，翻页恢复为 `- / =`。
- 独立手动识别页面，识别后明确选择开启或关闭同一 Class ID 的所有实体。
- 新增默认关闭的范围实验：默认 Tank / 500 游戏单位，可选择触发类别，范围 100～5000，支持 ±100 / ±10。
- 进入范围后临时拉伸全部可处理实体，退出恢复手动规则；实验排除手持武器与可关联附件，普通感染者默认不参与并可单独开启。
- 保留 128 / 256 骨骼容量切换、完整兼容 / 实验快速模式和旧版配置兼容。

距离以当前视角位置计算。普通感染者衣物不随身体变形的问题尚未修复；实验范围覆盖仍受绘制路径限制。识别沿用 DangoMusic 的附近候选排序，不检测墙体遮挡。

八组本地回归与 Windows x86 交叉编译通过，游戏内体验和性能仍需实测。包内含 DLL 和对应完整源码。

SHA-256：

```text
9cb08e89bf0dd6cdd323039936f79a7dfc99cbd55edf038b4701e83e45fc4a2b
```

## 历史测试版 0.5.0-test

**[下载 L4D2Visual-0.5.0-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.5.0-test.zip)** · [完整说明](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/README.md)

- 独立 Class ID 类别菜单，默认全部关闭，左右方向键翻页，一键全开 / 全关。
- 复用 DangoMusic 的准星附近识别，显示 Class ID 后切换该类别的所有实体。
- 从特感扩展到具有可验证模型身份的网络实体；无双脚模型使用根骨骼锚点。
- 运行时切换 128 / 256 骨骼容量，默认 128；超过所选上限时保持原始绘制。
- 优化目标索引、属性表缓存、诊断计数与骨骼缓冲；支持保存类别规则及渲染模式。

安装后先在 **实体选择** 开启需要的类别，再打开 **拉伸测试**。**性能与兼容** 提供完整兼容和保留快速路径（实验）模式；后者可能漏掉部分主模型。准星识别沿用附近候选排序，不检查墙体遮挡。静态地图道具与未接入的批量绘制不保证覆盖。

七组本地回归与 Windows x86 交叉编译通过，仍需游戏内验证覆盖和性能。安装包含对应完整源码。

SHA-256：

```text
905d308c9d77b81541411bd4cbabe1f8e2d3c298cf87154a4c1ba4bcf527fdd1
```

## 正式版 0.4.0

**[下载 L4D2Visual-0.4.0.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/2dd0a515a8dbbe552698464903ef84e7638b517a/L4D2Visual-0.4.0.zip)** · [Release 说明](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v0.4.0)

需要 Windows x86、L4N v2 及匹配的游戏构建。完整退出游戏后，将 ZIP 内的 `bin` 合并到游戏根目录，保证只加载一份 `bin/neko/plugins/L4D2Visual.dll`。在 **L4N → L4D2Visual 0.4.0 → 开启拉伸测试** 中启用。

默认周期 **0.5 秒**，竖直比例 **0.5～1.1**；提供平滑往复、Q 弹回弹和双跳脉冲预设。安装包包含插件和对应源码，不含游戏 DLL。

SHA-256：

```text
f3d14172d534e305598c2805f83d22ec2ff1b2e6e0381cd67462a06fcfe287c2
```

除当前 0.5.1-test 外，其他 `*-test.zip` 为历史测试包，供复现和回退使用；稳定版用户可继续选择 0.4.0。不要将此下载分支合并到 `main`。兼容性限制、日志排查、许可证与开发流程见 [main README](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/README.md)。
