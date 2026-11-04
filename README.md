# L4D2 Visual Effects · 安装包下载

求生之路 2 视觉效果项目；当前提供模型竖直拉伸效果。

本分支 `zip` **仅用于保存下载包**。源码、开发说明、完整安装指南和兼容性说明请前往 **[main 分支](https://github.com/yeyunyyds/L4D2-visual/tree/main)**。

## 测试版 0.5.0-test

**[下载 L4D2Visual-0.5.0-test.zip](https://github.com/yeyunyyds/L4D2-visual/raw/refs/heads/zip/L4D2Visual-0.5.0-test.zip)** · [完整说明](https://github.com/yeyunyyds/L4D2-visual/blob/main/README.md)

- 独立 Class ID 类别菜单，默认全部关闭，左右方向键翻页，一键全开 / 全关。
- 复用 DangoMusic 的准星附近识别，显示 Class ID 后切换该类别的所有实体。
- 从特感扩展到具有可验证模型身份的网络实体；无双脚模型使用根骨骼锚点。
- 运行时切换 128 / 256 骨骼容量，默认 128；超过所选上限时保持原始绘制。
- 优化目标索引、属性表缓存、诊断计数与骨骼缓冲；支持保存类别规则及渲染模式。

安装后先在 **实体选择** 开启需要的类别，再打开 **拉伸测试**。**性能与兼容** 提供完整兼容和保留快速路径（实验）模式；后者可能漏掉部分主模型。准星识别沿用附近候选排序，不检查墙体遮挡。静态地图道具与未接入的批量绘制不保证覆盖。

七组本地回归与 Windows x86 交叉编译通过，仍需游戏内验证覆盖和性能。安装包含对应完整源码。

SHA-256：

```text
7c046e53c6294c11a0dbcdfd6fead9c8edb2699d531b4a56aaf621bf06ad327b
```

## 正式版 0.4.0

**[下载 L4D2Visual-0.4.0.zip](https://github.com/yeyunyyds/L4D2-visual/raw/2dd0a515a8dbbe552698464903ef84e7638b517a/L4D2Visual-0.4.0.zip)** · [Release 说明](https://github.com/yeyunyyds/L4D2-visual/releases/tag/v0.4.0)

需要 Windows x86、L4N v2 及匹配的游戏构建。完整退出游戏后，将 ZIP 内的 `bin` 合并到游戏根目录，保证只加载一份 `bin/neko/plugins/L4D2Visual.dll`。在 **L4N → L4D2Visual 0.4.0 → 开启拉伸测试** 中启用。

默认周期 **0.5 秒**，竖直比例 **0.5～1.1**；提供平滑往复、Q 弹回弹和双跳脉冲预设。安装包包含插件和对应源码，不含游戏 DLL。

SHA-256：

```text
f3d14172d534e305598c2805f83d22ec2ff1b2e6e0381cd67462a06fcfe287c2
```

除上述 0.5.0-test 外，其他 `*-test.zip` 为历史测试包，供复现和回退使用；稳定版用户可继续选择 0.4.0。不要将此下载分支合并到 `main`。兼容性限制、日志排查、许可证与开发流程见 [main README](https://github.com/yeyunyyds/L4D2-visual/blob/main/README.md)。
