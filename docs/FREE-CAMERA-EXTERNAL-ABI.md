# FreeCamera 外部接口（C ABI v1）

这是 L4D2Visual 实验分支 `feature/external-freecamera-abi` 的可选跨插件摄像机接口。
目的是让 L4D2DangoMusic 等客户端**提出**镜头坐标，而不是让多个 DLL 同时 Hook ClientMode。

## 如何启用

1. L4D2Visual → FreeCamera → **外部接口（ABI）** → 允许（默认关闭）。
2. 可在同一菜单设置 **放开键鼠后交还外部**，范围 1–30 秒，默认 **3 秒**。
3. 外部插件必须主动调用 `SetExternalActive(1)` 并周期调用 `SetPose`，才能取得控制权。
4. L4D2Visual 与外部插件必须在**同一个 Windows x86 L4D2 进程内**。未找到 L4D2Visual 时，调用方应直接关闭该功能，而不是自己安装 ClientMode Hook。

设置保存在 `bin/neko/plugins/L4D2Visual/hud.json`：

```json
{
  "freeCamera": {
    "externalEnabled": true,
    "externalIdleSeconds": 3
  }
}
```

已有配置其他字段无需删除；在菜单保存后也会持久化。

## 控制权状态机

| 条件 | 视角与输入 |
|---|---|
| 外部接口关闭、外部插件未请求 | 原版游戏视角，不接管键鼠 |
| 外部插件首次请求，玩家最近 3 秒有输入 | 暂时保持原版游戏视角 |
| 有效坐标已提交、玩家闲置达到设定秒数 | 外部镜头接管 |
| 已经外部接管，玩家移动鼠标或操作键盘 | **立刻**改为手动 FreeCamera，保留当前镜头位置与角度，用户可继续自由飞行 |
| 用户停止操作 3 秒（或设定值） | 外部镜头重新接管，使用调用方最新提交的坐标 |
| 用户在菜单开启手动 FreeCamera | 手动模式优先，外部不能夺取 |
| 用户关闭外部接口、调用方主动结束、地图失效 | 外部请求失效；没有显式手动模式时恢复原生视角 |
| 调用方超过 15 秒没有更新有效坐标 | 外部镜头租约失效，退出自动控制 |

**重要**：本接口只是视角控制，玩家的网络实体在原位。外部和中断后的手动 FreeCamera 都继续屏蔽发送给服务器的飞行/攻击指令；只有退出 FreeCamera 返回原生视角后，这些指令才会送往游戏。

已被用户打断时 `SetPose` 可以接受新的候选坐标，但不会立即移动用户的镜头。调用方应通过 `GetStatus` 检查当前 `owner`。重新取得控制权时会切换到最新目标坐标；如果希望平滑过渡，调用方自行提供连续的过渡坐标。

## C ABI

版本头文件：[`include/l4d2visual_camera_api.h`](../include/l4d2visual_camera_api.h)。接口不向外暴露 C++ 类、STL 或引擎对象。

| 导出符号 | 功能 |
|---|---|
| `L4D2VisualCameraGetVersion()` | 返回 ABI 版本 1 |
| `L4D2VisualCameraSetExternalActive(int)` | 请求启用/关闭外部控制 |
| `L4D2VisualCameraSetPose(const Pose*)` | 传入世界空间位置、pitch/yaw/roll（角度） |
| `L4D2VisualCameraGetStatus(Status*)` | 返回授权、请求状态、所有者和剩余等待时间 |

所有调用使用 `__cdecl`；请求结构须设置 `size = sizeof(结构)`。输出结构亦须预先设置 `size`；版本或结构大小不匹配时拒绝调用。

推荐由 DangoMusic **按需获取**导出入口，不要保存插件可能卸载后的函数指针：

```cpp
#include <windows.h>
#include "l4d2visual_camera_api.h"

HMODULE visual = GetModuleHandleW(L"L4D2Visual.dll");
if (visual) {
    auto version = reinterpret_cast<L4D2VisualCameraGetVersionFn>(
        GetProcAddress(visual, "L4D2VisualCameraGetVersion"));
    auto request = reinterpret_cast<L4D2VisualCameraSetExternalActiveFn>(
        GetProcAddress(visual, "L4D2VisualCameraSetExternalActive"));
    auto pose = reinterpret_cast<L4D2VisualCameraSetPoseFn>(
        GetProcAddress(visual, "L4D2VisualCameraSetPose"));
    auto status = reinterpret_cast<L4D2VisualCameraGetStatusFn>(
        GetProcAddress(visual, "L4D2VisualCameraGetStatus"));

    if (version && request && pose && status &&
        version() == L4D2VISUAL_CAMERA_ABI_VERSION) {
        L4D2VisualCameraStatusV1 s{};
        s.size = sizeof(s);
        if (status(&s) && s.external_allowed) {
            L4D2VisualCameraPoseV1 p{};
            p.size = sizeof(p);
            p.origin[0] = 100.f; p.origin[1] = 200.f; p.origin[2] = 60.f;
            p.angles[0] = 0.f; p.angles[1] = 90.f; p.angles[2] = 0.f;
            pose(&p);           // submit before requesting active
            request(1);
            // Repeat pose() with fresh camera coordinates during cinematics.
            // Check s.owner == L4D2VISUAL_CAMERA_OWNER_EXTERNAL for actual control.
        }
    }
}
```

上述数字只用于展示 ABI 调用，不是合法或安全的地图镜头坐标。真实镜头轨迹应使用已获取的地图相机/角色位置，并在每帧检验坐标；FreeCamera ABI 不承诺碰撞安全，也不会自动避障。

## 兼容与测试

- 沿用 FreeCamera 原有且已验证的 `OverrideView[19]`、`CreateMove[27]`、`ShouldDrawViewModel[30]` Hook，不新增引擎 Hook。
- 缺少匹配的客户端游戏 DLL/ClientMode 指纹时拒绝绑定；调用方不得假定 `SetPose` 成功就等于画面已接管。读取 `camera_active` 和 `hook_ready`。
- 镜头状态机在 `tests/free_camera_external_policy_test.cpp` 覆盖首轮等待、用户抢占、重复操作延长等待、1–30 秒限制、手动优先、过期和撤销。
- 自动化测试不能代替离线/联机实机验收；尤其需要验证 ExtraMouseSample、真实键盘输入、切图/断连及其他视角插件的行为。
- 目前只提供基础“外部位置接管”协议，DangoMusic 的具体闲置运镜轨迹与碰撞策略需另行实现。
