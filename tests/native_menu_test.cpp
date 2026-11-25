#include "native_menu_model.hpp"
#include "hud_menu_policy.hpp"
#include <iostream>
#include <stdexcept>
using namespace dango::hud;
void check(bool v){if(!v)throw std::runtime_error("Native menu regression");}
int main(){
 check(opensChild(930)&&opensChild(936)&&!opensChild(931)&&!opensChild(932)&&!opensChild(933)&&!opensChild(934)&&!opensChild(937));
 check(opensChild(9001)&&opensChild(860)&&opensChild(865)&&!opensChild(862)&&opensChild(863)&&opensChild(864));
 check(!opensChild(896)&&!opensChild(9000)&&!opensChild(886)&&opensChild(300));
 for(uintptr_t id=887;id<=895;++id)check(!opensChild(id));
 auto freeRoot=document(R"kv("L4D2Visual" {"FreeCamera" {"user_data" "0x3a2"}})kv");
 check(freeRoot.rows.size()==1&&freeRoot.rows[0].action==930&&opensChild(930));
 MenuStack freeStack;freeStack.enter(freeRoot.rows[0].action);
 check(freeStack.current().page==930);
 auto freePage=document(R"kv("FreeCamera" {"Toggle" {"user_data" "0x3a3"}})kv");
 check(freePage.rows.size()==1&&freePage.rows[0].action==931&&!opensChild(931));
 auto root=document(R"kv("L4D2Visual" {
  "实体缩放与拉伸" {"user_data" "0x320"}
  "第一人称身体" {"user_data" "0x35c"}
  "Tank 行为预测" {"user_data" "0x140"}
  "Tank 石头预测" {"user_data" "0x2bc"}
  "菜单" {"user_data" "0x2329"}
  "保存设置" {"user_data" "0x65"}
 })kv");
 check(root.rows.size()==6&&root.rows[0].action==800&&root.rows[1].action==860);
 check(root.rows[4].action==9001&&opensChild(root.rows[4].action));

 check(opensChild(700)&&opensChild(701));for(uintptr_t id=750;id<=759;++id)check(!opensChild(id));
 check(opensChild(325)&&opensChild(326)&&opensChild(327));for(uintptr_t id=380;id<=388;++id)check(!opensChild(id));
 check(opensChild(321)&&opensChild(322)&&opensChild(323)&&opensChild(324));for(uintptr_t id=370;id<=373;++id)check(!opensChild(id));for(uintptr_t id=360;id<=366;++id)check(!opensChild(id));
 check(opensChild(102)&&opensChild(103)&&opensChild(107));check(!opensChild(120)&&!opensChild(121));for(uintptr_t id=110;id<=118;++id)check(!opensChild(id));
 auto doc=document(R"kv("歌词菜单" { "翻译：关" { "callback" "0x1234" "user_data" "0x2" } "路径 \\ 和 \"引号\"" { "user_data" "0x0" } })kv");
 check(doc.title==u8"歌词菜单"&&doc.rows.size()==2&&doc.rows[0].action==2&&doc.rows[1].action==0);
 check(doc.rows[1].label==u8"路径 \\ 和 \"引号\"");
 check(document("invalid").rows.empty());check(document(R"("x" {"y" {"user_data" "0xNO"}})").rows.empty());
 // Rebuild current state, including parents on Back; no cached labels survive.
 MenuStack menu;menu.enter(20);bool translation=false;
 auto build=[&](){return document(std::string("\"Display\" {\"Translation: ")+(translation?"on":"off")+"\" {\"user_data\" \"0x2\"}}");};
 for(int i=0;i<100;++i){auto current=build();check(menu.select(current,1)==2);translation=!translation;check(build().rows[0].label==(translation?"Translation: on":"Translation: off"));check(menu.frames.size()==2);}
 menu.enter(34);menu.current().sheet=2;menu.back();check(menu.current().page==20&&build().rows[0].label=="Translation: off");
 menu.enter(27);check(menu.current().page==23);menu.back();check(menu.current().page==20);menu.back();check(!menu.back());
 Document many;for(unsigned i=1;i<=23;++i)many.rows.push_back({std::to_string(i),i});
 menu.reset();menu.next(many.rows.size());check(menu.select(many,1)==10&&menu.select(many,9)==18);menu.next(23);menu.next(23);menu.next(23);check(menu.select(many,1)==19&&menu.select(many,6)==0);menu.clamp(2);check(menu.current().sheet==0);menu.previous();check(menu.current().sheet==0);
 // The key that selected the L4N entry must be released; holding a key is one
 // operation, and focus loss also requires a fresh release before accepting keys.
 KeyGate gate;KeyGate::Keys keys{};keys[1]=true;check(!gate.update(keys,true)[1]);keys.fill(false);gate.update(keys,true);keys[1]=true;check(gate.update(keys,true)[1]);check(!gate.update(keys,true)[1]);gate.update(keys,false);check(!gate.update(keys,true)[1]);keys.fill(false);gate.update(keys,true);keys[1]=true;check(gate.update(keys,true)[1]);
 std::cout<<"Passed: live labels, bounded navigation, fresh Back, 1..9 lyric position pagination, disabled rows, key release/repeat/focus gating\n";
}
