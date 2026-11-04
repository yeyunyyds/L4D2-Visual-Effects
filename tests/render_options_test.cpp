#include "render_settings.hpp"
#include "entity_selection.hpp"
#include "class_catalog.hpp"
#include "target_index.hpp"
#include "render_contract.hpp"
#include "studio_pose.hpp"
#include "entity_inventory.hpp"
#include "json.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cstring>
void check(bool condition){if(!condition)throw std::runtime_error("render options regression");}
template<std::size_t Capacity> void capacityCase(){
 using namespace visual;
 std::array<Matrix3x4,257> source{};
 for(std::size_t i=0;i<source.size();++i){source[i].m[0][0]=1;source[i].m[1][1]=1;source[i].m[2][2]=1;source[i].m[2][3]=float(i);}
 auto original=source;
 BasicPoseCopy<Capacity> output{};
 check(stretchCopy(source.data(),Capacity,0,1.1f,output));check(output.count==Capacity);
 auto before=output;
 check(!stretchCopy(source.data(),Capacity+1,0,1.1f,output));check(std::memcmp(&output,&before,sizeof(output))==0);
 // Rejection at the LAST bone must not partially overwrite the destination.
 source[Capacity-1].m[0][0]=std::numeric_limits<float>::quiet_NaN();
 check(!stretchCopy(source.data(),Capacity,0,1.1f,output));check(std::memcmp(&output,&before,sizeof(output))==0);
 source=original;source[Capacity-1].m[2][3]=std::numeric_limits<float>::max();
 check(!stretchCopy(source.data(),Capacity,0,3,output));check(std::memcmp(&output,&before,sizeof(output))==0);
 source=original;check(stretchCopy(source.data(),1,0,1.1f,output));
 check(std::memcmp(output.bones.data()+1,before.bones.data()+1,(Capacity-1)*sizeof(Matrix3x4))==0);
 check(std::memcmp(source.data(),original.data(),sizeof(source))==0);
 StudioPose studio;studio.count=unsigned(Capacity);studio.feet={1,2};studio.fixed={1,2,3};
 check(stretchCopy(source.data(),Capacity,0,1.1f,output));preserveFeet(studio,source.data(),output);
 for(auto i:studio.fixed)check(std::memcmp(&source[i],&output.bones[i],sizeof(Matrix3x4))==0);
}
int main(){using namespace visual;using nlohmann::json;
 check(normalizedBoneLimit(0)==128&&normalizedBoneLimit(129)==128&&normalizedBoneLimit(256)==256);
 check(nextBoneLimit(128)==256&&nextBoneLimit(256)==128);
 for(const auto& j:{json::object(),json{{"boneLimit",129}},json{{"boneLimit",-1}},json{{"boneLimit","256"}},json{{"boneLimit",256.0}},json{{"boneLimit",nullptr}}})check(readBoneLimit(j)==128);
 check(readRenderCompatibility(json::object())&&readRenderCompatibility(json{{"renderCompatibility","off"}}));
 check(!readRenderCompatibility(json{{"renderCompatibility",false}}));
 json saved={{"font",27}};writeRenderCompatibility(saved,false);check(!readRenderCompatibility(json::parse(saved.dump())));writeBoneLimit(saved,256);check(readBoneLimit(json::parse(saved.dump()))==256&&saved["font"]==27);
 capacityCase<128>();capacityCase<256>();
 struct PoseReader {unsigned reads=0;std::size_t bytes=0;bool read(std::uint32_t,void* out,std::size_t size){++reads;bytes=size;std::memset(out,0,size);return true;}} reader;
 BasicPoseCopy<128> small{};BasicPoseCopy<256> large{};StudioPose metadata;
 metadata.count=129;check(!readDrawPose(reader,0x1000,metadata,small)&&reader.reads==0);
 check(readDrawPose(reader,0x1000,metadata,large)&&large.count==129&&reader.bytes==129*sizeof(Matrix3x4));
 metadata.count=256;check(readDrawPose(reader,0x1000,metadata,large));
 unsigned reads=reader.reads;metadata.count=257;check(!readDrawPose(reader,0x1000,metadata,large)&&reader.reads==reads);
 metadata.count=128;check(readDrawPose(reader,0x1000,metadata,small)&&small.count==128);
 metadata.count=1;check(readDrawPose(reader,0x1000,metadata,small)&&reader.bytes==sizeof(Matrix3x4));
 float rootAnchor{};check(poseAnchor(metadata,small.bones.data(),rootAnchor)&&rootAnchor==0);
 auto selection=readEntitySelection(json{{"animationPeriodMs",500}});
 check(!selection.selected(260));check(!selection.anyEnabled());
 selection.classes[260]=true;check(selection.selected(260));check(!selection.selected(270));
 // All instances carrying the same class ID share this rule, independent of handles.
 selection.names[260]="Tank";writeEntitySelection(saved,selection);
 auto restored=readEntitySelection(json::parse(saved.dump()));
 check(restored.selected(260)&&!restored.selected(270)&&restored.names.at(260)=="Tank");
 restored.setAll(true);check(restored.selected(100)&&restored.selected(260));check(restored.classes.empty());
 restored.classes[260]=false;check(!restored.selected(260)&&restored.selected(270));
 restored.setAll(false);check(!restored.anyEnabled()&&!restored.selected(260));
 check(!restored.selected(-1)&&!restored.selected(4097));
 auto invalid=readEntitySelection(json{{"entitySelection",{{"classes",{{"bad",true},{"-1",true},{"4097",true},{"260","on"},{"270",true}}}}}});
 check(invalid.classes.size()==1&&invalid.selected(270));
 // Snapshot index excludes invalid model identities and keeps stable record pointers.
 dango::inventory::Frame f;dango::inventory::Record r;r.index=4;r.serial=3;r.handle=0x3004;r.className="CPhysicsProp";r.modelIndex=20;
 f.records.push_back(r);r.index=5;r.modelIndex.reset();f.records.push_back(r);
 TargetIndex<dango::inventory::Frame,8> index;
 struct Entry{std::uint32_t entity,serial;};
 index.rebuild(f,[](unsigned){return Entry{0x1000,3};});
 check(index.records[4]==&f.records[0]&&index.records[5]==nullptr&&index.bases.at(0x1000)==4);
 index.rebuild(f,[](unsigned){return Entry{0x1000,4};});check(index.bases.empty());
 f.records.clear();index.rebuild(f,[](unsigned){return Entry{0,0};});check(index.records[4]==nullptr&&index.bases.empty());
 // Bounded descriptor traversal rejects foreign addresses, cycles and corrupt IDs.
 struct CatalogReader {
  unsigned reads=0;std::array<unsigned char,1024> bytes{};
  bool read(std::uint32_t address,void* out,std::size_t count){++reads;if(address<0x1000||std::uint64_t(address-0x1000)+count>bytes.size())return false;std::memcpy(out,bytes.data()+address-0x1000,count);return true;}
  void node(unsigned offset,int id,unsigned next,const char* name){dango::entitywire::ClientClass c{};c.id=id;c.next=next;c.name=0x1000+offset+32;std::memcpy(bytes.data()+offset,&c,sizeof(c));std::strcpy(reinterpret_cast<char*>(bytes.data()+offset+32),name);}
 } catalogReader;
 catalogReader.node(0,260,0x1080,"Tank");catalogReader.node(128,270,0x1000,"Witch");
 ClassCatalog catalog;auto owned=[](std::uint32_t address){return address>=0x1000&&address<0x1400;};
 catalog.observe(catalogReader,0x1000,owned);check(catalog.names.size()==2&&catalog.descriptors.size()==2&&catalog.names.at(260)=="Tank");
 unsigned catalogReads=catalogReader.reads;catalog.observe(catalogReader,0x1000,owned);check(catalogReader.reads==catalogReads);
 catalog.clear();catalogReader.node(0,260,0x9000,"Tank");catalog.observe(catalogReader,0x1000,owned);check(catalog.names.size()==1);
 catalog.clear();catalogReader.node(0,4097,0,"Bad");catalog.observe(catalogReader,0x1000,owned);check(catalog.names.empty());
 catalogReader.node(0,260,0,"bad name");catalog.observe(catalogReader,0x1000,owned);check(catalog.names.empty());
 std::cout<<"PASS: 128/256 boundaries, late rejection preserves output, live-bone copies, foot preservation, settings migration, class-ID grouping, all toggles, snapshot indexing\n";
}
