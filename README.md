# L4D2 Visual Effects · 安装包下载

求生之路 2 视觉效果项目；当前提供邦友的酒（模型拉伸）与自定义喷漆。

本分支 `zip` **仅用于保存下载包**。源码、开发说明、完整安装指南和兼容性说明请前往 **[main 分支](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/main)**。

## 当前正式版 0.7.2

**免责声明：联机功能未经测试，不保障 VAC 安全。**

**[下载 L4D2Visual-0.7.2.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.2.zip)** · [正式Release](https://github.com/yeyunyyds/L4D2-Visual-Effects/releases/tag/v0.7.2) · [对应完整源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/v0.7.2)

安装包顶层只有 **bin/** 和 **README.md**，许可文件保留在bin内。源码独立下载，不塞进安装包。退出游戏后合并bin目录即可，需L4N v2 / Windows x86，不覆盖个人配置。

用户已确认0.7.2基础喷漆和T键正常。正式版保持同一路径，仅调整版本号、打包与文档；T绑定及联机总开关仍默认关闭。手部/武器深度遮挡尚未实现；Esc后在插件菜单手动恢复显示。0.7.1的DXVK渲染回归已撤回，请停用0.7.1。

联机功能发布时未经实机验证，不承诺VAC安全。安装和开关说明均在包内README。

SHA-256：

```text
6bdaf40a1eeff8dd57773dc819377ff562c7f34ac13e3ca507e063bae821a861
```

历史测试包保留原结构和哈希；其README中的旧验收状态以本节为准。

## Tank 攻击叠加层自动计时 · 0.7.3-tank-layer-timer-preview7

**[下载 preview7 测试包](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.3-tank-layer-timer-preview7.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/66c0611) · [使用说明](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/66c0611/docs/TANK-PREVIEW.md)

已将实际识别到的 Attack_Moving / Hulk_RunAttack1_gesture / Hulk_RunAttack2_gesture 等攻击叠加动画接入自动计时。按名称、权重、播放进度识别起手，处理重复采样、同名动作重启、多层混合与权重短暂失活。无需 F7 或人工标记，也无需打开诊断记录；开启 Tank 预测与绘制即可观察加粗的头顶黄色计时条和菜单剩余秒数。

使用实时攻击间隔参数估算，**不是服务器真实冷却，归零不保证可攻击或安全**。投石独立处理。事件字段校验和共享模型映射尚未修复，新的动画层计时不依赖事件 Hook。仅打开诊断时沿用事件观察，关闭诊断后恢复。

21 组原生测试与 Windows x86 构建通过；用已提供的 782 条日志回放，29 次起手对应 29 次计时启动，重复输入没有再次触发。此为离线回放，新 DLL 尚需游戏内验证。反馈时可打开诊断并提供 schema 7 的 tank-prediction.jsonl 与 previous 文件。

根目录仅 bin/ 和 README.md；正式版保持 0.7.2。**联机功能未经测试，不保障 VAC 安全。**

SHA-256：`291e4407d74d607c2f12adef65ad1290e844c9c14221479816d08518b4696eb5`

## 历史 Tank 实际动画事件诊断 · 0.7.3-tank-events-preview6

**[下载 preview6 诊断包](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.3-tank-events-preview6.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/dc3f73d) · [测试步骤](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/dc3f73d/docs/TANK-PREVIEW.md)

在 preview5 已确认的 CTEPlayerAnimEvent 上观察事件工厂和三个字段接收代理；原回调照常执行，不改变事件参数。仅在 Tank 预测和诊断记录同时开启时安装，关闭后恢复；默认关闭。不是套用 SDK 的事件虚表槽位，也不依赖 SourceMod。

新增独立 tank-animation-events.jsonl，记录完整/不完整字段组、实际事件编号和完整玩家句柄；tank-animation-binding.json 保存绑定检查与入口代码诊断。tank-prediction.jsonl 升为 schema 6，并附回调计数和最近事件。请在本地测试普通挥拳、挥空和投石后挥拳约 20～30 秒，提供上述三份文件及存在的 previous 日志；无需 F7 或人工标记。

**尚未实机验证实际接收覆盖率和 AI Tank 事件编号，未据此接入冷却。** 同时增加模型 include 名称记录，序列 80 的全局映射仍未实现。20 组原生测试、Windows x86 构建及安装包检查通过。根目录仅 bin/ 和 README.md，正式版保持 0.7.2。

**联机功能未经测试，不保障 VAC 安全。** SHA-256：`d8e2b8cf20084129e0d31182d3f00cfb625473dcb2485a74d7778e239d2e12e1`

## 历史 Tank 动画来源诊断 · 0.7.3-tank-animation-preview5

**[下载动画来源诊断包](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.3-tank-animation-preview5.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/6c87ffc) · [调查依据](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/6c87ffc/docs/tank-animation-sources.md)

追踪主体之外的叠加动画层：只读解析实际 SDK UtlVector 元数据，记录所有 Tank 层的序列、进度、权重、顺序及可解析的名称。补充动画事件类字段信息和模型本地序列数量。没有新增事件 Hook，不调用接收代理，不依赖 SourceMod。

本包用于定位真正的普通拳起手来源，**尚未验证普通拳自动识别或冷却恢复**。开启 Tank 预测与诊断，测试普通挥拳、挥空、投石后挥拳约 20～30 秒，再提供 schema 5 的 tank-prediction 日志。无需 F7 或人工标记。共享模型映射尚未实现，字段不可读时记录原因。

19 组原生测试和 Windows x86 构建通过，未进行游戏实测。安装包根目录仅 bin/ 与 README.md；正式版仍为 0.7.2。**联机功能未经测试，不保障 VAC 安全。**

SHA-256：`c8967ffe78122a4dbb423cb603e3cd11f542c385fb1c8f14989b510a07d42c78`

## 历史 Tank观测计时实验 · 0.7.3-tank-stopwatch-preview4

后续日志中自动出拳计时未触发，用户反馈 F7 无效。以下为历史说明，不作为当前可用性保证；请改用 preview5 调查动画来源。

**[下载观测计时实验包](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.3-tank-stopwatch-preview4.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/5dae6c8) · [使用说明](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/experiment/tank-cooldown/docs/TANK-PREVIEW.md)

改为“观测到出拳后启动估算秒表”，不再把陈旧AI计时显示成0秒真实冷却。读取动态正常/挥空间隔并优先用较短值，动画起手/循环去重，不依赖扣血。投石显示动作状态与结束后经过时间，不套用拳击间隔。

自动路径仅支持明确解析到的拳击活动，普通拳击可能仍位于未解析共享动画/动画层，不能声称全自动覆盖。人工校准：在Tank预测菜单开启“F7手动标记出拳”（默认关），关闭插件菜单，看见起手时按F7标记准星方向Tank；或使用菜单逐Tank标记。小条黄色=动画估算，紫色=人工估算，灰色=未知。计时结束仅表示可能恢复，不保证可攻击或安全。

新日志schema=4保留事件编号/起点/间隔/来源，方便比对；需开启诊断记录。十八组回归与Windows x86构建通过，未进行Windows游戏实测，仍需人工标记与实际行为校准。

仅L4N，无SourceMod，正式版仍为0.7.2。安装包顶层只有bin/与README.md。**联机功能未经测试，不保障VAC安全；Tank实验同样不提供VAC保证。**

SHA-256：`7186e817807ada1e8aa0401631808b11f36ec483bd410ea15b43a0298f3e91e0`

## Tank冷却实验 · 0.7.3-tank-cooldown-preview3

**[下载冷却实验包](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.3-tank-cooldown-preview3.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/57a40b0) · [实验说明](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/experiment/tank-cooldown/docs/TANK-PREVIEW.md)

补齐局部网络表的角色/武器攻击计时、已解码Source模拟时间和动画cycle读取。冷却使用实际推进的Source时钟计算，暂停/切图/缺失时标未知，不用固定1.5秒代替。菜单显示数值，Tank上方显示小条：灰色未知、黄色部分计时下限、青色两项客户端计时均读到；零秒不代表AI一定可出拳。

仅依赖L4N，不需要SourceMod。功能默认关闭，在Tank行为预测实验页启用并开启诊断记录。局部表可能仅发给所有者，AI Tank字段可能未同步；本环境未运行Windows游戏，需实机记录验证。十七组回归与x86编译通过。

包顶层仅bin/与README.md；源码独立提供。**不保障VAC安全。** 正式版仍为0.7.2。

SHA-256：`2df8c45e38ba9e617760932863833289ffb1d6c440f5a8a5fe22a2783463b598`

## Tank行为预测实验 · 0.7.3-tank-preview2

**[下载preview2](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.3-tank-preview2.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/d59dfc1) · [使用说明](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/experiment/tank-threat/docs/TANK-PREVIEW.md)

- 修正近竖直瞄准时的二维yaw锁向误报，暂停此时不可靠的地面范围，方向线保留真实pitch。
- 既有Studio钩子在关闭拉伸/快速绘制时也采集Tank动画CPU元数据，补足可能绕过Engine模型回调的路径。不增加Source GPU操作，不改变骨骼，不调用新游戏虚函数。
- 日志schema=2新增动画路径/失败原因、字段表元数据、pitch与采样时间；仅采集角色、Tank及武器，排除团队等非角色对象。

仍只依赖L4N，默认关闭，不需要SourceMod。十六组回归和x86构建通过；动作标签可读性及预测准确率仍需实机验证，unknown不能视为没有攻击。开启诊断记录后生成新的tank-prediction.jsonl。

安装包顶层仅bin/与README.md。**联机功能未经测试，不保障VAC安全；Tank实验同样不提供VAC保证。** 正式版仍为0.7.2。

SHA-256：`cb1211a77ac117203f68d6438b246b8844ccfbfb74c1482aae6291650a06c9bf`

## Tank行为预测实验 · 0.7.3-tank-preview

**[下载独立实验包](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.3-tank-preview.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/cf53276) · [说明与单人验收](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/experiment/tank-threat/docs/TANK-PREVIEW.md)

仅依赖L4N，不需要SourceMod、Left4DHooks或服务端插件。功能默认关闭，在“Tank行为预测（实验）”开启。正式版继续使用上方0.7.2，本包不设为正式Release。

- 只读尝试获取实时ConVar距离/角度参数、瞄准/身体朝向、基础动画、可读攻击计时，绘制参数估计区和短时间移动外推；参数未知时不代入固定手长。
- 尝试读取m_lookatPlayer，明确标为注视目标；可用的敌对网络字段、注视目标和朝向推测分别标注，不把“看着谁”等同于AI真实锁敌。模糊目标、死者与失效句柄不当作有效目标。
- 记录可能的朝向停滞，不凭空设定“投石后手长缩短”。每50ms定向更新当前战斗实体，在已验证模型钩子只读CPU元数据，不新增Source回调GPU调用或深度借用。
- 开启诊断记录后生成有界tank-prediction.jsonl，可用于本地单人投石前后、贴身经过与挥空对照。详细步骤见包内README。

十六组ASan/UBSan回归和Windows x86构建通过。**当前是字段验证/预测原型，未运行Windows游戏，不能宣称已经高准确率或读到所有字段。arc语义、实际碰撞/命中、动画层与AI状态仍需本地单人校准。** 线条沿用Present，有遮挡局限。安装前备份0.7.2 DLL，只加载一份。

安装包顶层仅bin/与README.md，许可在bin内，源码独立提供。
**免责声明：联机功能未经测试，不保障VAC安全；Tank实验同样不提供VAC保证。**

SHA-256：`c008111caa10b7693e87f944afe83846ba4a3d62bbe6ed6224a29c1a88c3f33c`

## 历史热修复测试版 0.7.2-test

**[下载 L4D2Visual-0.7.2-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.2-test.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/ed9c177) · [使用与已知限制](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/docs/wall-pictures.md)

- 撤回0.7.1的Source回调GPU绘制和深度借用，恢复此前基础可用的Present方案；设备丢失跳过绘制，Present失败不继续恢复拷贝。
- 修正T键的DXVK鼠标状态 / 窗口判断及控制台状态恢复。T仍默认关闭，请在按键设置中开启，页面新增输入状态和最近请求结果。
- Esc自动暂停喷漆显示，返回游戏后在喷漆菜单手动“恢复喷漆显示”；图片不清空。**手部正确深度遮挡仍未实现。**
- 日志每份约4MiB轮转，保留当前与previous两份，避免无限增长。保留同局自动匹配与逐图大小等其他功能。

十五组本地回归及Windows x86编译通过。**本环境未运行DXVK游戏，不能宣称闪烁 / 崩溃已实测消除。** 0.7.1已标记停用；没有崩溃转储，资源管理器卡死原因尚未确认。

包内含DLL和完整源码。SHA-256：

```text
0a51cc8b141a52b4e7b8f84b4df6ef3f04257f9d571e4b3ee6a9e90ffa7359e8
```

## 0.7.1-test（停用：DXVK 闪烁 / 崩溃回归）

**请勿使用此版。用户报告默认本地喷图即出现闪烁 / 崩溃；0.7.2已撤回其场景GPU绘制路径。**

**[下载 L4D2Visual-0.7.1-test.zip](https://github.com/yeyunyyds/L4D2-Visual-Effects/raw/refs/heads/zip/L4D2Visual-0.7.1-test.zip)** · [对应源码](https://github.com/yeyunyyds/L4D2-Visual-Effects/tree/6741b05) · [使用说明](https://github.com/yeyunyyds/L4D2-Visual-Effects/blob/main/docs/wall-pictures.md)

- 修复“邦友的酒”中“邦”显示问号：补全菜单字库并加入覆盖回归。
- 跟进DangoMusic最新一起听分支6d327fc，新增默认开启的同局自动匹配。双方安装本版、进入同局并开启联机总开关，即可探测最多三名其他真人，不必手填ID/room；保留手动名单模式。联机总开关仍默认关闭。
- 喷漆不再在Present顶层覆盖绘制，移到Source场景回调，使用现有深度测试且不写深度；界面显示鼠标、失焦或深度表面不可用时暂停图片，避免覆盖手部和Esc界面。HUD仍独立绘制。
- 保留T绑定、自动下一张、逐文件即时尺寸、FIFO数量限制及P2P分片同步。

十四组ASan/UBSan回归和Windows x86编译通过。**真实自动互传、D3D9/DXVK深度可用性、手部与Esc层级仍需实机验收。** 若图片不显示，请查看喷漆菜单中的渲染状态；无深度时不会回退成顶层覆盖。

包内包含DLL和完整对应源码，不覆盖个人配置。SHA-256：

```text
4c77f52b3cd102138ff04d7b3f4e0d909f601dfd6a7c7445c42d71e3aa335132
```

## 历史测试版 0.7.0-test

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
