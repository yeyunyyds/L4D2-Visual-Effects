#pragma once
#include "imgui/imgui.h"
#include "menu_glyphs.hpp"
namespace dango::hud {
inline void menuFontRanges(ImFontAtlas& atlas,ImVector<ImWchar>& ranges){
 ImFontGlyphRangesBuilder builder;
 builder.AddRanges(atlas.GetGlyphRangesChineseSimplifiedCommon());
 builder.AddText(MenuGlyphs);builder.AddText(u8"图片挂画仅本机目录等待刷新无法创建读取完整后台加载所选关闭开启选择本地已选择暂无请放入后清空现有当前地图对准墙面放置所选大小旋转透明度长边角度缩小放大逆顺时针恢复参数默认应用于之后新删除最后一张已达先平整调整位置足够或纹理解码尺寸过缓存满旧显卡上限显存写入处理异常重试接口校验查看日志不可用请求过期请重新操作提交可保存");builder.AddText(u8"特感八种类型设置未保存");builder.AddText(u8"求生者八名角色设置未保存");builder.AddText(u8"实体类型范围外关闭逐个计算距离有效触发目标范围内实体生效");builder.AddText(u8"实体分类特感求生者场景可拾取武器与物品手上道具其他开启本分类全部关闭含实验进入手动识别上次当前该清除结果用作范围触发类别尚未对准目标后选择实验开关默认游戏单位最近距离附近发现有效状态内临时全开待机排除离开恢复总开关衣物可能不随动感染者玩家");builder.AddText(u8"←→");builder.AddText(u8"性能与兼容渲染模式完整保留快速路径实验可能漏掉部分主模型");builder.AddText(u8"类别准星附近已识别此所有请选择开启或关闭未到目标请对准实体并稍等进入地图后左右翻页");builder.AddText(u8"骨骼上限切换未保存（128/256）实体选择全部开启关闭类别当前地图单个准星识别暂不可用角色道具侏儒垃圾桶左右方向键翻页返回仅完整句柄有效时生效失效请重新没有可识别模型进入地图按一已");builder.AddText(u8"动画预设细调平滑往复Q弹回弹双跳脉冲最小最大比例恢复默认秒周期设置");builder.AddText(u8"动画速度切换重启周期未保存兼容渲染");
 builder.AddText(u8"拉伸测试保留双脚脚趾姿态接口日志诊断保存菜单设置已提交失败写入目录权限比例往复绘制开关");
 builder.BuildRanges(&ranges);
}
}
