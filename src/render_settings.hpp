#pragma once
namespace visual {
inline unsigned normalizedBoneLimit(unsigned value) { return value == 256 ? 256u : 128u; }
inline unsigned nextBoneLimit(unsigned value) { return normalizedBoneLimit(value) == 128 ? 256u : 128u; }
template<class Json> unsigned readBoneLimit(const Json& j) {
 auto it=j.find("boneLimit");
 if(it==j.end() || !it->is_number_integer())return 128;
 return *it==256 ? 256u : 128u;
}
template<class Json> void writeBoneLimit(Json& j,unsigned limit){j["boneLimit"]=normalizedBoneLimit(limit);}
template<class Json> bool readRenderCompatibility(const Json& j){
 auto it=j.find("renderCompatibility");return it==j.end() || !it->is_boolean()?true:it->template get<bool>();
}
template<class Json> void writeRenderCompatibility(Json& j,bool enabled){j["renderCompatibility"]=enabled;}
} // namespace visual
