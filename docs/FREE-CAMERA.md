# FreeCamera（v1.0.0）

FreeCamera 是 L4D2Visual 的可选客户端自由视角功能。**默认关闭**，可以在 L4N 插件菜单中切换。**F6 快捷键绑定独立且默认关闭**，用户打开该绑定后才会用 F6 切换；即使不绑定 F6，菜单开关仍可使用。

## 操作

- WASD 水平飞行，鼠标调整朝向。
- Space 上升，Shift 下降。
- 菜单可调整最高飞行速度，默认 1000 游戏单位/秒，调节步长 50；运动具有平滑加减速。
- Esc 或切出游戏窗口会暂停移动；重新进入游戏后恢复。
- 可选择隐藏第一人称手臂和武器（该子选项默认开启，但只有 FreeCamera 真正运行时才生效）。
- 「运行数据与诊断」子菜单显示 Hook、运行状态、渲染步进与输入统计；含紧急关闭入口。

## 原生行为与 Hook

在玩家**首次实际请求** FreeCamera 之前，插件不安装 FreeCamera 的 `ClientMode` Hook。首次启用后安装 `OverrideView[19]`、`CreateMove[27]` 和 `ShouldDrawViewModel[30]`（仅适用于指纹验证通过的客户端 DLL）。之后即使关闭 FreeCamera，也不反复热卸载，而是透传游戏原始调用。

当摄像机尚处于绑定/启动阶段时，`CreateMove` 保留游戏原生输入，ViewModel 显示也保持原生决策；只有真正运行时才接管视角和隐藏武器。运行阶段在诊断页标为 `off / binding / starting / active / paused`。

FreeCamera 不切换游戏的第三人称模式。FPB 若同时开启，会保留原生人物在世界中的位置与骨骼矩阵；移动摄像机不会拉动人物本体，也不会触碰服务器实体或游戏碰撞数据。

## 兼容性与限制

- 仅支持已经验证 PE/SHA-256、函数指纹、ClientMode 虚表及布局的 Windows x86 游戏 DLL。验证失败时拒绝对应 Hook，不进行猜测性写入。
- 在异常布局、地图重载或插件失效时会停止自由视角并尝试恢复原生行为。
- 与第三方 ClientMode Hook、帧生成器、特殊第三人称 MOD 的组合无法全面保证兼容。
- 使用时建议先在离线环境测试。**不保证联机/VAC 安全。**

该功能随 v1.0.0 正式版发布。基础使用路径已有实机测试；联机服务器中的鼠标转向卡顿仍有未确认解决的反馈，详见下方额外鼠标采样修复说明。

## 联机鼠标转向修复（待实机复验）

发生长渲染 Gap 时不再丢弃此前积累的鼠标角度，Gap 只停止移动速度。CreateMove 的鼠标增量放入有符号整数队列，积累不截断，每次真实渲染最多应用每轴 4096 计数，余量保留。暂停、切出游戏窗口或手动关闭时仍丢弃待处理输入，以避免恢复后突然转头。

运行诊断新增 X/Y received/applied/dropped、pending、鼠标采样总数和零增量样本数。这些计数从 CreateMove 开始，不能证明 Windows 设备端是否出现输入丢失。无新增 Hook 或鼠标插值，不改飞行速度积分；修复仍须在服务器复验。

## Source 额外鼠标采样（待联机实测）

在游戏渲染帧率高于网络模拟 tick 时，Source 可调用 `CInput::ExtraMouseSample`，该函数生成 `command_number == 0` 的临时用户命令并走 `ClientMode::CreateMove`。旧版 FreeCamera 将零编号命令整体跳过，漏采渲染帧间的鼠标位移，并且未锁定额外采样后的世界人物视角。

目前在同一现有 CreateMove Hook 中识别并处理这些临时命令，将鼠标 X/Y 增量纳入队列，同时照常锁定人物方向；正常网络命令也继续工作。不增加 Hook 或额外鼠标插值。诊断页列出 `Server cmds / extra mouse samples / extra with input`，用于检查当前 L4D2 版本是否走该采样路径。包含 120 个采样帧、30 个网络命令和 90 个额外采样的回归测试。此修复仍需在原服务器实机确认。

## 开发版：通过 hud.json 自由绑定快捷键（尚未发布）

不需要游戏内录制按键。编辑 `bin/neko/plugins/L4D2Visual/hud.json` 中的 `freeCamera`：

```json
"freeCamera": {
  "hideViewmodel": true,
  "f6Binding": true,
  "hotkey": "f6"
}
```

将 `hotkey` 改成 `f7`、`f8`、`p`、`mouse4` 等即可；支持 F1–F24（F10 除外）、字母、数字、数字键盘、方向键、Tab、Enter、Insert、Delete、Home、End、PageUp/PageDown、空格、常见修饰键和鼠标中键/侧键。键名忽略大小写与前后空格。F10 已被 HUD 占用，Esc 用于菜单关闭，故不允许绑定。绑定到 WASD、Space、Shift 等游戏操作键会同时触发原有操作，不推荐。

**`f6Binding` 仍是快捷键总开关**：可在 FreeCamera 设置菜单中开启，也可直接设为 `true`。现有旧配置未写 `hotkey` 时继续默认 F6。错误键名或非字符串类型会禁用快捷键触发，并在诊断中显示无效（不会悄悄切回 F6 造成误触）。为避免热键和数字菜单互相冲突，HUD 菜单打开时不触发快捷键。编辑后重启游戏加载配置；仅在 HUD 菜单内“保存设置”不会重读磁盘文件。

## 实验分支：外部 Camera API（尚未进入正式 v1.0.0）

FreeCamera 可以通过菜单显式授权同进程 L4N 插件（例如 L4D2DangoMusic）调用版本化的 C ABI，按世界坐标控制外部镜头。用户的鼠标/键盘输入会立即让自动控制退回手动 FreeCamera，并且**不重置摄像机坐标**；没有任何键鼠操作持续 1–30 秒（默认 3 秒）后才允许外部重新接管。接口授权默认关闭。

完整结构、可调用符号、返回状态、控制权仲裁及调用示例： [FREE-CAMERA-EXTERNAL-ABI.md](FREE-CAMERA-EXTERNAL-ABI.md)。当前没有自动实现 PEAK 式镜头轨迹。
