#pragma once
#include "world_camera.hpp"
#include "animation_layers.hpp"
#include <optional>
#include <string>
#include <vector>
#include <map>
namespace dango::inventory {
struct Record {
 uint32_t handle=0,index=0,serial=0;std::string className,modelName;int classId=-1;
 std::optional<bool> dormant,isCarryable;std::optional<Vec3> networkOrigin,networkAngles;
 std::optional<int> team,modelIndex,worldModelIndex,effects,character,zombieClass,health;std::optional<uint8_t> lifeState;std::optional<uint32_t> owner,weaponOwner,activeWeapon,viewModel,parent,weaponParent,thrower;
 std::optional<uint8_t> parentAttachment;
 std::optional<Vec3> eyeAngles,velocity,collisionMins,collisionMaxs;
 std::optional<uint32_t> lookAtHandle,enemyHandle;
 std::vector<dango::animation::AnimationLayer> animationLayers;std::string animationLayerReason;
 std::optional<int> sequence;std::optional<float> animationCycle,simulationTime,nextPrimaryAttack,nextAttack;

};
struct Frame {uint32_t epoch=0;uint64_t tick=0;unsigned occupied=0,unknownClass=0,unknownBase=0,unknownTables=0;std::vector<Record> records;std::map<int,std::string> classCatalog;uint32_t requestedFollowHandle=0xffffffffu,followedHandle=0xffffffffu;std::string followRoute;std::vector<std::string> getterHints,bindingChecks,animationEventSchemas;bool modelLookup=false;std::string status="Entity diagnostics off";};
struct FollowRequest {bool enabled=false;uint32_t epoch=0,handle=0;};
}
