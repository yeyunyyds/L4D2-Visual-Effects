# Tank 攻击动画与事件来源调查

本次只追踪自动动作信号；不以手动标记、掉血、距离或固定 sequence 编号判定出拳。正式版 0.7.2 不变；实验分支仍仅依赖 L4N。

## 已确认的采样缺口

现有日志只给出了主体 `m_nSequence` 与主体 cycle。移动主体序列仍可能同时叠加另一层攻击手势，因此 `Run_4` 本身不能证明没有出拳。

L4D2 分支 SDK 的 [c_baseanimatingoverlay.cpp](https://github.com/alliedmodders/hl2sdk/blob/l4d2/game/client/c_baseanimatingoverlay.cpp) 定义了独立的 `DT_Animationlayer`：`m_nSequence`、`m_flCycle`、`m_flPrevCycle`、`m_flWeight`、`m_nOrder`。其动画事件分发也按叠加层的序列和 cycle 处理。

`m_AnimOverlay` 是通过 `RecvPropUtlVector` 接收的容器，不是内联层数组。[dt_utlvector_recv.cpp](https://github.com/alliedmodders/hl2sdk/blob/l4d2/public/dt_utlvector_recv.cpp) 的 `CRecvPropExtra_UtlVector` 记录元素大小、容器偏移和最大层数；元素属性的 stride 实际存的是元素编号。不能把 RecvProp offset 简单相加当成层地址。

preview5 按这一元数据形状验证后读取 Tank 的所有层，包括权重为 0 的层。层字段偏移来自实际 RecvTable，元素大小来自实际 extra；容器数量、容量、指针、地址溢出和浮点值均检查。读取前后比较容器头，变化时舍弃该次采样。绝不调用接收代理、resize、SetupBones 或新的实体虚函数。校验失败只记录原因。

这仍是每 50ms 的状态采样，日志每约 100ms 写一次，并非逐事件回调。短动作可能漏采，尚未据此接入自动出拳计时。

旧日志还显示 Tank / CTerrorPlayer 的主体 `m_nSequence` 与基础动画实体使用不同的接收代理，同一个玩家的继承表中也同时出现了同偏移的不同代理。地址差异只证明接收入口不同，不能证明它触发了挥拳；需要当前 client.dll 的代码或经验证的代理入口才能继续追踪其具体作用。preview5 保留这些代理元数据，不修改它们。

## 动画事件线索

公开 L4D2 逆向资料的 [PlayerAnimEvent_t](https://github.com/SilvDev/Left4DHooks/blob/master/sourcemod/scripting/include/left4dhooks.inc) 将 `PLAYERANIMEVENT_HAND_ATTACK`（33）和 `PLAYERANIMEVENT_HAND_LOW_ATTACK`（34）关联到 `CTankClaw::OnSwingStart`。这给出了查找入口，但不等于已经验证当前客户端二进制的事件编号、调用约定或 AI 出拳覆盖率。这里仅参考资料，不安装、不调用、不依赖该项目。

同一 SDK 分支的 [c_sdk_player.cpp](https://github.com/alliedmodders/hl2sdk/blob/l4d2/game/client/sdk/c_sdk_player.cpp) 展示了客户端事件接收链：`C_TEPlayerAnimEvent::PostDataUpdate` 使用 `m_hPlayer`、`m_iEvent`、`m_nData` 调用玩家的 `DoAnimationEvent`。这是 SDK 示例实现，不能直接当成 L4D2 的实际实现。

preview5 从已有的模块内 ClientClass 描述符目录查找名称包含 `AnimEvent` 的类，只记录其事件工厂指针和 RecvTable 元数据。不会调用工厂、安装事件 Hook 或修改接收代理。目录可能不完整；找不到类不证明游戏没有该事件。

下一步需要运行时先确认实际事件类、字段和接收位置，再按当前 client.dll 验证回调；不能直接套用服务端地址或其他 Source 游戏的虚表槽位。

## 序列 80 与共享模型

旧日志中序列 80 只有一个合并的拒绝原因，不能据此证明它一定超出了本地表，更不能宣布它就是拳击。

[studio.cpp](https://github.com/alliedmodders/hl2sdk/blob/l4d2/public/studio.cpp) 的 `CStudioHdr::pSeqdesc` 在存在虚拟模型时，通过 `m_seq[i].group/index` 映射到另一个模型的本地序列；`studiohdr_t::numlocalseq` 只代表本地表数量。

preview5 额外记录 `localSequenceCount`，区分本地表越界与描述符校验失败。尚未实现虚拟模型缓存读取，不会猜测私有布局或硬编码 sequence 80。

## 需要下一份日志回答的问题

1. 普通挥拳时，主体仍为移动动作，而某一叠加层是否出现攻击标签、权重上升和 cycle 重启？
2. 该层是本地序列还是需要共享模型映射？
3. 当前客户端是否暴露动画事件类，实际事件字段是什么？

在这些问题验证前，不能宣称已获得可靠的普通拳起手事件或真实攻击冷却。


## preview6：从元数据推进到实际接收观察

用户上传的 schema 5 片段共 69 条、8.44 秒，已确认实际 `CTEPlayerAnimEvent` 的 `m_hPlayer / m_iEvent / m_nData` 偏移分别为 16 / 20 / 24；主体全为 Idle_Rage，7 个叠加层全无权重。这段片段没有足够的攻击变化，不能排除叠加层，也不能证明不存在事件。

本构建的网络接口没有 SDK 的 GetIClientUnknown 槽位，故不直接套用 PostDataUpdate 虚表索引。preview6 选择公开 SDK 的 CreateEventFn() 与 RecvVarProxyFn(data, struct, out) ABI，在实际注册指针处进行观察，避免未知成员函数调用约定。

接入前先校验已知 client.dll 文件哈希；工厂必须可以只读解码为静态单例 getter；单例 ClassSlot=1 与 DataBaseSlot=10 的 getter 只解码、不调用。返回描述符、数据基址、接收表和三个字段必须吻合。通过后使用比较交换临时替换工厂与三个字段代理指针；原回调保留并以原参数执行，原工厂返回值不变。检查不通过则仅保存绑定诊断。

新路径只在 Tank 预测和诊断记录同时开启时安装；关闭后恢复。回调内不写文件、不发网络消息、不做 GPU 工作；有界队列在工作线程写独立事件日志。每组记录是否具备工厂边界和三个字段，不将缺字段、跨地图、跨线程或超时组误认为完整事件。

独立日志及状态中的最近事件减少待机记录轮换造成的证据丢失。新绑定文件保留有限的模块相对入口和代码字节，方便后续验证实际 PostDataUpdate 路径；不会主动调用这些虚表方法。当前事件观察仍可能受引擎缓存代理或客户端预测路径影响，需用工厂/代理计数判断覆盖。

模型的 include 名称仅从经边界验证的 v48 数据中读取；不把 studiohdr_t 的 private cache 指针猜成 virtualmodel_t，也不直接以导入顺序推算全局序列。


## preview7：已确认攻击层接入自动计时

后续两份日志的 schema 6 部分共有 782 条、约 97.95 秒。实际攻击层出现了 Attack_Moving / ACT_TERROR_ATTACK_MOVING、Hulk_RunAttack1_gesture、Hulk_RunAttack2_gesture；第 0 层的权重及 cycle 均有变化。以此确认之前主体动画路径漏掉了实际攻击手势，但不将动画本身当作命中证明。

新增每 Tank/完整句柄的 LayerAttackDetector，识别有权重的已解析攻击标签、序列变化和 cycle 重启，合并同次多层激活，避免权重短暂失活、未解析帧或重复时间导致重复计时。输入缺失保持未知；长间隔和初次观察的中途动作不建立完整新窗口。

检测到起手后直接向现有 SwingStopwatch 提交独立的 layerOnset 信号，source 为 layer-observed-punch。主体仍可保持 Run_4，投石状态保持独立。此路径在诊断关闭时照常工作，不依赖尚未成功读取的事件编号。

生产检测器和秒表的离线回放得到 29 次起手/29 次计时，与人工检查的 17 段 Hulk_RunAttack1_gesture、7 段 Attack_Moving、5 段 Hulk_RunAttack2_gesture 对应；重复样本没有再次启动。回放不是实机准确率保证，也不能证明动画起手与实际冷却恢复或挥拳命中完全同步。
