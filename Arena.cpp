#include "Arena.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <dlfcn.h>
#include <unistd.h>
#include <filesystem>
#include <algorithm>

Arena::Arena() : width(20), height(20), max_rounds(100), watch_live(true), current_round(0) {
    srand(time(nullptr));
}

Arena::~Arena() {
    for (auto robot : robots) {
        delete robot;
    }
    for (auto handle : robot_handles) {
        dlclose(handle);
    }
}

void Arena::initialize(const std::string& config_file) {
    load_config(config_file);
    
    // Initialize grid
    grid.resize(height, std::vector<char>(width, '.'));
    
    // Place obstacles
    int num_mounds = 5, num_pits = 2, num_flamethrowers = 3;
    place_obstacles(num_mounds, num_pits, num_flamethrowers);
    
    // Load robots
    load_robots();
}

void Arena::load_config(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Could not open config file, using defaults\n";
        return;
    }
    
    std::string key, value;
    while (file >> key >> value) {
        if (key == "arena_width") width = std::stoi(value);
        else if (key == "arena_height") height = std::stoi(value);
        else if (key == "max_rounds") max_rounds = std::stoi(value);
        else if (key == "watch_live") watch_live = (value == "yes");
    }
}

void Arena::place_obstacles(int num_mounds, int num_pits, int num_flamethrowers) {
    auto place_random = [&](char type, int count) {
        for (int i = 0; i < count; i++) {
            int row, col;
            do {
                row = rand() % height;
                col = rand() % width;
            } while (grid[row][col] != '.');
            grid[row][col] = type;
        }
    };
    
    place_random('M', num_mounds);
    place_random('P', num_pits);
    place_random('F', num_flamethrowers);
}

void Arena::compile_robot(const std::string& cpp_file) {
    std::string robot_name = cpp_file.substr(6, cpp_file.length() - 10); // Strip "Robot_" and ".cpp"
    std::string so_file = "lib" + robot_name + ".so";
    
    std::string cmd = "g++ -std=c++20 -fPIC -shared " + cpp_file + " RobotBase.o -o " + so_file;
    std::cout << "Compiling " << cpp_file << " to " << so_file << "...\n";
    
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cerr << "Failed to compile " << cpp_file << "\n";
    }
}

RobotBase* Arena::load_robot_library(const std::string& so_file, void*& handle) {
    std::string full_path = "./" + so_file;  // Add ./ prefix
    handle = dlopen(full_path.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load " << so_file << ": " << dlerror() << "\n";
        return nullptr;
    }
    
    typedef RobotBase* (*RobotFactory)();
    RobotFactory create_robot = (RobotFactory)dlsym(handle, "create_robot");
    if (!create_robot) {
        std::cerr << "Failed to find create_robot in " << so_file << "\n";
        dlclose(handle);
        return nullptr;
    }
    
    return create_robot();
}

void Arena::load_robots() {
    std::cout << "\nLoading Robots...\n";
    
    // Find all Robot_*.cpp files
    std::vector<std::string> robot_files;
    for (const auto& entry : std::filesystem::directory_iterator(".")) {
        std::string filename = entry.path().filename().string();
        if (filename.substr(0, 6) == "Robot_" && filename.substr(filename.length() - 4) == ".cpp") {
            robot_files.push_back(filename);
        }
    }
    
    char symbols[] = {'!', '@', '#', '$', '%', '&', '*', '+', '='};
    int symbol_idx = 0;
    
    for (const auto& cpp_file : robot_files) {
        compile_robot(cpp_file);
        
        std::string robot_name = cpp_file.substr(6, cpp_file.length() - 10);
        std::string so_file = "lib" + robot_name + ".so";
        
        void* handle;
        RobotBase* robot = load_robot_library(so_file, handle);
        
        if (robot) {
            robot->set_boundaries(height, width);
            robot->m_name = robot_name;
            robot->m_character = symbols[symbol_idx % 9];
            
            place_robot(robot, robot->m_character);
            
            robots.push_back(robot);
            robot_handles.push_back(handle);
            robot_symbols[robot] = robot->m_character;
            
            int r, c;
            robot->get_current_location(r, c);
            std::cout << "Loaded robot: " << robot_name << " at (" << r << ", " << c << ")\n";
            
            symbol_idx++;
        }
    }
}

void Arena::place_robot(RobotBase* robot, char symbol) {
    int row, col;
    do {
        row = rand() % height;
        col = rand() % width;
    } while (grid[row][col] != '.');
    
    grid[row][col] = 'R';
    robot->move_to(row, col);
}

void Arena::print_arena() {
    std::cout << "\n     ";
    for (int c = 0; c < width; c++) {
        std::cout << c % 10 << "  ";
    }
    std::cout << "\n";
    
    for (int r = 0; r < height; r++) {
        std::cout << (r < 10 ? " " : "") << r << "  ";
        for (int c = 0; c < width; c++) {
            char cell = grid[r][c];
            if (cell == 'R') {
                // Find which robot is here
                for (auto robot : robots) {
                    int rr, cc;
                    robot->get_current_location(rr, cc);
                    if (rr == r && cc == c) {
                        if (robot->get_health() > 0) {
                            std::cout << " R" << robot->m_character;
                        } else {
                            std::cout << " X" << robot->m_character;
                        }
                        break;
                    }
                }
            } else {
                std::cout << "  " << cell;
            }
        }
        std::cout << "\n";
    }
}

void Arena::print_robot_stats(RobotBase* robot, char symbol) {
    int r, c;
    robot->get_current_location(r, c);
    
    if (robot->get_health() <= 0) {
        std::cout << robot->m_name << " " << symbol << " - is out\n";
    } else {
        std::cout << robot->m_name << " " << symbol << " (" << r << "," << c << ") Health: " 
                  << robot->get_health() << " Armor: " << robot->get_armor() << "\n";
    }
}

std::vector<RadarObj> Arena::scan_radar(RobotBase* robot, int direction) {
    std::vector<RadarObj> results;
    int robot_row, robot_col;
    robot->get_current_location(robot_row, robot_col);
    
    if (direction == 0) {
        // Scan 8 adjacent cells
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;  // Skip robot's own position
                int nr = robot_row + dr;
                int nc = robot_col + dc;
                if (in_bounds(nr, nc)) {
                    char cell = grid[nr][nc];
                    if (cell != '.') {
                        // Don't report robot's own position
                        if (!(nr == robot_row && nc == robot_col)) {
                            results.push_back(RadarObj(cell, nr, nc));
                        }
                    }
                }
            }
        }
    } else {
        // Directional scan (3 cells wide)
        auto [dr, dc] = directions[direction];
        
        for (int dist = 1; dist < std::max(height, width); dist++) {
            for (int offset = -1; offset <= 1; offset++) {
                int nr = robot_row + dr * dist;
                int nc = robot_col + dc * dist;
                
                // Apply perpendicular offset for 3-wide beam
                if (dr == 0) nr += offset;
                else if (dc == 0) nc += offset;
                else { 
                    if (offset == -1) { nr -= 1; }
                    else if (offset == 1) { nc += 1; }
                }
                
                if (in_bounds(nr, nc)) {
                    char cell = grid[nr][nc];
                    if (cell != '.') {
                        results.push_back(RadarObj(cell, nr, nc));
                    }
                }
            }
        }
    }
    
    return results;
}

void Arena::handle_shot(RobotBase* shooter, int shot_row, int shot_col) {
    WeaponType weapon = shooter->get_weapon();
    int shooter_row, shooter_col;
    shooter->get_current_location(shooter_row, shooter_col);
    
    std::cout << "  firing " << (weapon == railgun ? "railgun" : weapon == hammer ? "hammer" : 
                                 weapon == grenade ? "grenade" : "flamethrower");
    
    std::vector<std::pair<int,int>> hit_cells;
    
    if (weapon == railgun) {
        // Shoot through everything to edge
        int dr = (shot_row > shooter_row) ? 1 : (shot_row < shooter_row) ? -1 : 0;
        int dc = (shot_col > shooter_col) ? 1 : (shot_col < shooter_col) ? -1 : 0;
        
        int r = shooter_row + dr;
        int c = shooter_col + dc;
        while (in_bounds(r, c)) {
            hit_cells.push_back({r, c});
            r += dr;
            c += dc;
        }
    } else if (weapon == grenade) {
        // 3x3 splash at target
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                int r = shot_row + dr;
                int c = shot_col + dc;
                if (in_bounds(r, c)) {
                    hit_cells.push_back({r, c});
                }
            }
        }
        shooter->decrement_grenades();
    } else if (weapon == hammer) {
        // Just one cell (must be adjacent)
        if (abs(shot_row - shooter_row) <= 1 && abs(shot_col - shooter_col) <= 1) {
            hit_cells.push_back({shot_row, shot_col});
        }
    } else if (weapon == flamethrower) {
        // 4 cells in direction, 3 wide
        int dr = (shot_row > shooter_row) ? 1 : (shot_row < shooter_row) ? -1 : 0;
        int dc = (shot_col > shooter_col) ? 1 : (shot_col < shooter_col) ? -1 : 0;
        
        for (int dist = 1; dist <= 4; dist++) {
            for (int offset = -1; offset <= 1; offset++) {
                int r = shooter_row + dr * dist;
                int c = shooter_col + dc * dist;
                if (dr == 0) r += offset;
                else if (dc == 0) c += offset;
                
                if (in_bounds(r, c)) {
                    hit_cells.push_back({r, c});
                }
            }
        }
    }
    
    // Apply damage to robots in hit cells
    for (auto [r, c] : hit_cells) {
        for (auto target : robots) {
            int tr, tc;
            target->get_current_location(tr, tc);
            if (tr == r && tc == c && target != shooter && target->get_health() > 0) {
                int damage = calculate_damage(weapon);
                int health_before = target->get_health();
                target->take_damage(damage);
                target->reduce_armor(1);
                std::cout << " at (" << r << "," << c << ")";
                std::cout << "\n  " << target->m_name << " takes " << damage << " damage. Health: " 
                          << target->get_health();
                
                if (target->get_health() <= 0) {
                    std::cout << " - DESTROYED!";
                }
            }
        }
    }
    std::cout << "\n";
}

void Arena::handle_movement(RobotBase* robot, int direction, int distance) {
    int curr_row, curr_col;
    robot->get_current_location(curr_row, curr_col);
    
    distance = std::min(distance, robot->get_move_speed());
    
    auto [dr, dc] = directions[direction];
    
    int new_row = curr_row;
    int new_col = curr_col;
    
    for (int step = 0; step < distance; step++) {
        int next_row = new_row + dr;
        int next_col = new_col + dc;
        
        if (!in_bounds(next_row, next_col)) break;
        
        char cell = grid[next_row][next_col];
        
        if (cell == 'M' || cell == 'R') {
            break; // Hit obstacle or robot
        } else if (cell == 'P') {
            new_row = next_row;
            new_col = next_col;
            robot->disable_movement();
            std::cout << "  " << robot->m_name << " fell in a pit!\n";
            break;
        } else if (cell == 'F') {
            new_row = next_row;
            new_col = next_col;
            int damage = calculate_damage(flamethrower);
            robot->take_damage(damage);
            robot->reduce_armor(1);
            std::cout << "  " << robot->m_name << " passed through flames! Takes " << damage << " damage.\n";
        } else {
            new_row = next_row;
            new_col = next_col;
        }
    }
    
    if (new_row != curr_row || new_col != curr_col) {
        grid[curr_row][curr_col] = '.';
        grid[new_row][new_col] = 'R';
        robot->move_to(new_row, new_col);
        std::cout << "  moving to (" << new_row << "," << new_col << ")\n";
    } else {
        std::cout << "  not moving\n";
    }
}

int Arena::calculate_damage(WeaponType weapon) {
    switch (weapon) {
        case railgun: return 10 + rand() % 11;
        case hammer: return 50 + rand() % 11;
        case grenade: return 10 + rand() % 31;
        case flamethrower: return 30 + rand() % 21;
    }
    return 0;
}

bool Arena::check_winner() {
    int alive = 0;
    RobotBase* survivor = nullptr;
    for (auto robot : robots) {
        if (robot->get_health() > 0) {
            alive++;
            survivor = robot;
        }
    }
    
    if (alive == 1) {
        std::cout << "\n\n*** WINNER: " << survivor->m_name << " ***\n\n";
        return true;
    } else if (alive == 0) {
        std::cout << "\n\n*** NO SURVIVORS ***\n\n";
        return true;
    }
    
    return false;
}

bool Arena::in_bounds(int row, int col) const {
    return row >= 0 && row < height && col >= 0 && col < width;
}

char Arena::get_cell(int row, int col) const {
    return in_bounds(row, col) ? grid[row][col] : '#';
}

void Arena::set_cell(int row, int col, char value) {
    if (in_bounds(row, col)) {
        grid[row][col] = value;
    }
}

void Arena::run() {
    std::cout << "\n=========== starting round " << current_round << " ===========\n";
    print_arena();
    
    for (current_round = 0; current_round < max_rounds; current_round++) {
        std::cout << "\n=========== Round " << current_round + 1 << " ===========\n";
        
        for (auto robot : robots) {
            if (robot->get_health() <= 0) continue;
            
            std::cout << "\n" << robot->m_name << " " << robot->m_character << " begins turn.\n";
            print_robot_stats(robot, robot->m_character);
            
            // Radar
            int radar_dir;
            robot->get_radar_direction(radar_dir);
            std::vector<RadarObj> radar_results = scan_radar(robot, radar_dir);
            
            std::cout << "  checking radar ... ";
            if (radar_results.empty()) {
                std::cout << " found nothing.\n";
            } else {
                std::cout << " found '" << radar_results[0].m_type << "' at (" 
                          << radar_results[0].m_row << "," << radar_results[0].m_col << ")\n";
            }
            
            robot->process_radar_results(radar_results);
            
            // Shoot or move
            int shot_row, shot_col;
            if (robot->get_shot_location(shot_row, shot_col)) {
                handle_shot(robot, shot_row, shot_col);
            } else {
                int move_dir, move_dist;
                robot->get_move_direction(move_dir, move_dist);
                if (move_dist > 0) {
                    std::cout << "  moving";
                    handle_movement(robot, move_dir, move_dist);
                } else {
                    std::cout << "  not firing, not moving\n";
                }
            }
        }
        
        if (watch_live) {
            print_arena();
            sleep(1);
        }
        
        if (check_winner()) break;
    }
    
    if (current_round >= max_rounds) {
        std::cout << "\n\nMax rounds reached. Game over.\n";
    }
}
