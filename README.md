# L4D2 Visual Effects · 安装包下载

求生之路 2 视觉效果项目；当前提供邦友的酒（模型拉伸）与自定义喷漆。

本分支 `zip` **仅用于保存下载包**。源码、开发说明、完整安装指南和兼容性说明请前往 **[main 分支](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/main)**。

## 当前测试版 0.7.0-test

**[下载 L4D2Visual-0.7.0-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.0-test.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/1e7ad80) · [喷漆使用与联机配置](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/docs/wall-pictures.md)

- 修复图片与近裁剪面相交时整张消失：保留仍在视野内的多边形。
- 拉伸相关控制全部进入 **邦友的酒[开/关]**，图片功能进入 **自定义喷漆**。
- **绑定T键**、**喷后自动下一张** 默认关闭；每人默认最多 **2张**，可调1～8，超出自动删除最早图片。
- 默认大小减半至60，旧配置减半迁移一次；按文件保存尺寸，修改立即作用于同文件已喷图片，支持“仅当前”与“应用所有”。扩大后超出墙面支撑范围则保留原尺寸。
- 新增默认关闭的 **Steam P2P 联机喷漆实验**，参考DangoMusic一起听分支，独立通道45632；图片、位置、大小与删除同步。双方须安装本版L4D2Visual、互填SteamID64、配置相同room并位于同地图。只有L4N的普通用户不能接收本协议。详细配置见源码README。

十二组ASan/UBSan回归及Windows x86交叉编译通过。**双Steam账号互传和新版游戏显示仍需实机验收**。世界采样遮挡仍有局限，动态人物遮挡未覆盖。

包内含DLL、图片目录说明和完整对应源码；不覆盖个人配置。SHA-256：

```text
ca4d32d9349307569d30b859fc28d1c4bbdcf160011d0a1480a59e0918eaa530
```

## 历史测试版 0.6.0-test

**[下载 L4D2Visual-0.6.0-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.6.0-test.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/338f1d7) · [图片挂画指南](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/docs/wall-pictures.md)

新增默认关闭的 **图片挂画（本地）**，只在本机绘制，不上传、不分发，不修改原版 spray。

1. 将 PNG、JPG/JPEG 或 BMP 放入游戏目录 `bin/neko/plugins/L4D2Visual/images/`。
2. 进入 **图片挂画（本地）→ 刷新目录**，开启模块、选择图片，等待后台加载。
3. 对准平整墙面，点击 **对准墙面放置所选图片**。

支持保持比例的尺寸调整、15°旋转、透明度、删除最后一张和清空。每图最多八张；刷新目录、关闭模块或切图清空放置记录。参数可保存，放置记录不跨地图保存。

图片最长边最多 2048 像素，较大图片等比例缩小；源文件上限 32 MiB / 32MP，CPU/GPU 纹理缓存各 64 MiB。解码在后台，纹理与屏幕备份表面复用，支持设备重置后重建。

十一组本地回归与 Windows x86 交叉编译通过。**实际图片解码、显示、透明边缘和窗口切换仍需游戏内验收。** 当前沿用 DangoMusic 世界墙体遮挡采样，动态人物 / 道具遮挡尚未覆盖，并非原生 decal 投射。原有角色 / 特感、拉伸及范围实验保留。

包内含 DLL、图片目录说明和完整对应源码。SHA-256：

```text
13b950661d25e7ad6ffb9d8e2839157807c4a10ba816c6b1f6d45dfca819835a
```

## 历史测试版 0.5.3-test

**[下载 L4D2Visual-0.5.3-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.5.3-test.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/cc32acf) · [完整指南](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/README.md)

- 求生者固定八角色：Nick、Rochelle、Coach、Ellis、Bill、Zoey、Francis、Louis，按角色独立开关并保存。
- 特感固定八实际类型：Smoker、Boomer、Hunter、Spitter、Jockey、Charger、Tank、Witch；NPC 与感染者玩家均按实际类型匹配。
- 共享 Class ID 的角色 / 类型不再联动，保留对应分类全开 / 全关；真人和机器人使用同一角色 / 类型规则。
- 延续逐实体范围实验、手持物排除、普通感染者参与开关、`- / =` 翻页和其他实体控制。
- Tank 类型范围中心同时支持 NPC Tank 和已识别的真人 Tank；范围计算仍是实体到中心的距离。

十组本地回归与 Windows x86 交叉编译通过，仍需实机确认角色 / 类型识别。包内含 DLL 与完整对应源码。

SHA-256：

```text
fea88a6fe858c06638e2cacb4fde2cd075a2ecc4b7fb633b985cb7584aefccc5
```

## 历史测试版 0.5.2-test

**[下载 L4D2Visual-0.5.2-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.5.2-test.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/5e82397) · [完整指南](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/README.md)

- 修正菜单：固定列出全部八种特感，以及求生者、武器 / 物品、手持模型等预定义类型，无需刷出即可预先设置并保存。
- 修正范围实验：逐个判断实体到 Tank / 所选中心的距离；只有范围内实体拉伸，玩家和镜头位置不参与判断。
- 同一类别的多个实例可分别处于范围内外；多个范围中心取并集。实验开启时范围外始终关闭，关闭实验后恢复手动分类规则。
- 保留分类批量开关、一键全部关闭、独立手动识别与开关、`- / =` 翻页、128 / 256 切换。
- 实验默认关闭，默认 Tank / 500 游戏单位，范围 100～5000；继续排除手持物，普通感染者另设默认关闭的参与开关。

八组回归覆盖空场景完整菜单、预先保存类型、镜头无关、同类型内外分离、多个中心、目标失效及排除规则。Windows x86 交叉编译通过，仍需游戏内验收。

普通感染者衣物不随动的问题尚未修复；静态物件和未接入的批量绘制仍不保证覆盖。准星识别仍采用 DangoMusic 的附近候选排序，不检查墙体遮挡。位置判断沿用约 200ms 的网络快照。

安装包包含 DLL 与对应完整源码。SHA-256：

```text
fab9b14ad74ebd27ae14a7e73d1e59e08242f614f0df60252938e5c443f69460
```

## 历史测试版 0.5.1-test（请升级至 0.5.2）

[归档包 L4D2Visual-0.5.1-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.5.1-test.zip)

该版本的菜单仍依赖已观察类别，范围实验误按视角距离触发全局效果。这两项问题已在 0.5.2 修正；此包仅保留用于回退 / 复现。

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

除当前 0.6.0-test 外，其他 `*-test.zip` 为历史测试包，供复现和回退使用；稳定版用户可继续选择 0.4.0。不要将此下载分支合并到 `main`。兼容性限制、日志排查、许可证与开发流程见 [main README](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/README.md)。
