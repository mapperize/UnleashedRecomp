#include "tas_windows.h"

#include <nlohmann/json.hpp>
#include <user/paths.h>

using json = nlohmann::json;

std::filesystem::path TASWindow::GetPositionPath()
{
    return GetUserPath() / "positions.json";
}

std::filesystem::path TASWindow::GetPracticeConfigPath()
{
    return GetUserPath() / "practice-config.toml";
}

void to_json(json& j, const be<float>& b) {
    j = (float)b;
}

void from_json(const json& j, be<float>& b) {
    b = j.get<float>();
}

void to_json(json& j, const Quaternion& q){
    j = json{
        {"w", q.w},
        {"x", q.x}, 
        {"y", q.y},
        {"z", q.z}
    };
}
void from_json(const json& j, Quaternion& q){
    j.at("x").get_to(q.x);
    j.at("y").get_to(q.y);
    j.at("z").get_to(q.z);
    j.at("w").get_to(q.w);
}

void to_json(json& j, const Position& p){
    j = json{
        {"position", p.pos}, 
        {"positionName", p.posName},
        {"rotation", p.rot},
        {"is2DMode", p.is2DMode}
    };
}
void from_json(const json& j, Position& p){
    j.at("position").get_to(p.pos);
    j.at("positionName").get_to(p.posName);
    j.at("rotation").get_to(p.rot);
    j.at("is2DMode").get_to(p.is2DMode);
}

void to_json(json& j, const Level& l) {
    j = json{
        {"name", l.name}, 
        {"positions", l.positions}
    };
}

void from_json(const json& j, Level& l) {
    j.at("name").get_to(l.name);
    j.at("positions").get_to(l.positions);
}

void TASWindow::SaveJson(){
    json data = levels;
    std::ofstream out(GetPositionPath());
    out << std::setw(4) << data << std::endl;
}

void TASWindow::ReloadJson(){
    std::ifstream file(GetPositionPath());
    if (file) {
        json data = json::parse(file);
        data.get_to(levels);
        // since this might get freed we need to set currentLevel to nullptr
        currentLevel = NULL;
        forceReload = true;
        printf("\nsuccessfully reloaded json");
    }
    else {
        printf("\nfile probably doesnt exist");
    }
}