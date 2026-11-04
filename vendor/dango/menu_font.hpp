#pragma once
#include "imgui/imgui.h"
#include "menu_glyphs.hpp"
namespace dango::hud {
inline void menuFontRanges(ImFontAtlas& atlas,ImVector<ImWchar>& ranges){
 ImFontGlyphRangesBuilder builder;
 builder.AddRanges(atlas.GetGlyphRangesChineseSimplifiedCommon());
 builder.AddText(MenuGlyphs);builder.AddText(u8"动画预设细调平滑往复Q弹回弹双跳脉冲最小最大比例恢复默认秒周期设置");builder.AddText(u8"动画速度切换重启周期未保存兼容渲染");
 builder.AddText(u8"拉伸测试保留双脚脚趾姿态接口日志诊断保存菜单设置已提交失败写入目录权限比例往复绘制开关");
 builder.BuildRanges(&ranges);
}
}
