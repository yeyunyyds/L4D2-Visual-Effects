# FreeCamera 外部接口（C ABI v3）

实验分支：`feature/external-freecamera-abi`。只支持同一 L4D2 Windows x86 进程内的 L4N 插件，不新增 ClientMode Hook，也不需要独立测试 EXE。

## 单一相机、单一开关

- FreeCamera 菜单开关、配置的快捷键（默认 F6）以及外部 ABI 使用同一个 FreeCamera 状态。授权默认关闭。
- `SetExternalActive(1)` 只登记 AFK 请求；收到真实游戏输入采样并达到首次等待时间才开启 FreeCamera。用户键鼠优先，默认静止 3 秒后外部参数才生效（可调整 1–30 秒）。
- 外部运镜时用户接管，保留实际镜头位置与角度；期间收到的外部位置不执行，仅刷新会话租约。闲置达到阈值后，在新的有效坐标到达之前保持实际位置。
- F6/配置快捷键或菜单关闭：恢复原生视角，清除外部请求与目标。DangoMusic 不应自动重新开启；用户需再次启动运镜。外部会话期间允许配置的有效快捷键退出，即使普通快捷键绑定关闭；菜单打开时不触发。
- 关闭外部授权会结束当前外部会话并恢复原生视角，但不影响没有外部会话的普通 FreeCamera。DangoMusic 可等待重新授权，再发起新会话。
- 15 秒无有效心跳、地图切换或停用插件时撤销会话。

## ABI v3

头文件：[`include/l4d2visual_camera_api.h`](../include/l4d2visual_camera_api.h)。结构布局沿用 v1，但行为改变，版本号提升至 3。两个插件需同时更新。所有函数使用 `__cdecl`；结构的 `size` 必须预先设为 `sizeof`。版本函数返回 3，其余函数返回 1（成功）或 0（拒绝/不可用）。

| 函数 | 作用 |
|---|---|
| L4D2VisualCameraGetVersion | ABI 版本 |
| L4D2VisualCameraSetInitialIdle | 设置首次 AFK 等待 1–120 秒（默认10） |
| L4D2VisualCameraSetExternalActive | 请求开启或结束现有外部会话 |
| L4D2VisualCameraGetStatus | 授权、请求、当前 owner、闲置时间和实际激活状态 |
| L4D2VisualCameraGetPose | 读取当前实际生效的 FreeCamera 位置及角度；无有效视角时返回 0 |
| L4D2VisualCameraSetPose | 提交绝对世界坐标及角度；用户拥有控制权时仅续租，不改变视角 |

调用方通过 GetModuleHandleW/GetProcAddress 动态解析，不自行 LoadLibrary。首先检查版本和授权，再请求启用，持续发送有效坐标。每次取得 EXTERNAL 控制权时，先 GetPose，再以实际姿态为起点提交连续轨迹。GetPose 返回的不是候选目标。检测到会话被用户关闭时清除调用方的启动状态。

## 验证

状态机测试覆盖统一启停、用户抢占、闲置恢复、关闭后拒绝旧坐标、重新授权、租约失效。实际游戏内的 F6、键鼠接管、再次授权及轨迹连续性仍需实机验证。ABI 不提供引擎碰撞保证；调用方可使用自己的路径检查。
