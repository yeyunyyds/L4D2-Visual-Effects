#include "menu_font.hpp"
#include <stdexcept>
#include <iostream>
int main(){ImFontAtlas atlas;ImVector<ImWchar> ranges;dango::hud::menuFontRanges(atlas,ranges);
 const wchar_t* required=L"石头预测轨迹落点外观不透明度线宽圆大小邦友的酒自定义喷漆同局自动匹配握手绑定数量限制预测锁敌瞄准毫秒停滞诊断样式预览琥珀战术冰蓝刻度横竖厚度模拟高级实体透视缩放前摇粗细精调倍率选中参数减少增加百分比参数设置窗口";
 for(auto c=required;*c;++c){bool found=false;for(int i=0;i+1<ranges.Size&&ranges[i];i+=2)if(*c>=ranges[i]&&*c<=ranges[i+1])found=true;if(!found)throw std::runtime_error("Required menu glyph omitted");}
 std::cout<<"PASS: menu atlas includes effect title and automatic spray discovery glyphs\n";
}
