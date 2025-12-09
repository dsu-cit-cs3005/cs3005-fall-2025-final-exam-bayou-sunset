#ifndef ARENA_H
#define ARENA_H

#include "RobotBase.h"
#include "RadarObj.h"
#include <vector>
#include <string>
#include <map>

class Arena {
private:
    int width;
    int height;
    int max_rounds;
    bool watch_live;
    int current_round;
    
    std::vector<std::vector<char>> grid;  // The arena board
    std::vector<RobotBase*> robots;       // All robots
    std::vector<void*> robot_handles;     // For dlopen/dlclose
    std::map<RobotBase*, char> robot_symbols;  // Robot display characters
    
    // Helper functions
    void load_config(const std::string& config_file);
    void place_obstacles(int num_mounds, int num_pits, int num_flamethrowers);
    void load_robots();
    void compile_robot(const std::string& cpp_file);
    RobotBase* load_robot_library(const std::string& so_file, void*& handle);
    void place_robot(RobotBase* robot, char symbol);
    
    // Game loop helpers
    void print_arena();
    void print_robot_stats(RobotBase* robot, char symbol);
    std::vector<RadarObj> scan_radar(RobotBase* robot, int direction);
    void handle_shot(RobotBase* shooter, int shot_row, int shot_col);
    void handle_movement(RobotBase* robot, int direction, int distance);
    int calculate_damage(WeaponType weapon);
    bool check_winner();
    
    // Utility
    bool in_bounds(int row, int col) const;
    char get_cell(int row, int col) const;
    void set_cell(int row, int col, char value);
    
public:
    Arena();
    ~Arena();
    
    void initialize(const std::string& config_file);
    void run();
};

#endif // ARENA_H
