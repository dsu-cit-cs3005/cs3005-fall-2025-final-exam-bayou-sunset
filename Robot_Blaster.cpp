#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

class Robot_Blaster : public RobotBase
{
private:
    int target_row;
    int target_col;
    bool has_target;
    int scan_direction;
    bool in_hiding_spot;
    int turns_waiting;

public:
    Robot_Blaster() : RobotBase(3, 4, grenade), // Balanced: 3 move, 4 armor
                      target_row(-1), target_col(-1), 
                      has_target(false), scan_direction(1),
                      in_hiding_spot(false), turns_waiting(0) {
        srand(time(nullptr) + getpid());
    }

    void get_radar_direction(int& radar_direction) override {
        // Cycle through all directions
        scan_direction++;
        if (scan_direction > 8) scan_direction = 1;
        radar_direction = scan_direction;
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        has_target = false;
        
        int curr_row, curr_col;
        get_current_location(curr_row, curr_col);
        
        // Find closest enemy
        int closest_dist = 9999;
        for (const auto& obj : radar_results) {
            if (obj.m_type == 'R') {
                int dist = abs(obj.m_row - curr_row) + abs(obj.m_col - curr_col);
                if (dist < closest_dist) {
                    closest_dist = dist;
                    target_row = obj.m_row;
                    target_col = obj.m_col;
                    has_target = true;
                }
            }
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        // Only shoot if we have grenades and a clear target
        if (has_target && get_grenades() > 0) {
            shot_row = target_row;
            shot_col = target_col;
            return true;
        }
        return false;
    }

    void get_move_direction(int& move_direction, int& move_distance) override {
        int curr_row, curr_col;
        get_current_location(curr_row, curr_col);
        
        // Check if in corner (hiding spot)
        bool in_corner = (curr_row <= 1 || curr_row >= m_board_row_max - 2) &&
                        (curr_col <= 1 || curr_col >= m_board_col_max - 2);
        
        if (in_corner) {
            in_hiding_spot = true;
            turns_waiting++;
            
            // Stay hidden, only move if enemy is very close
            if (has_target && 
                abs(target_row - curr_row) < 5 && 
                abs(target_col - curr_col) < 5) {
                // Enemy too close, reposition
                move_direction = 1 + (rand() % 8);
                move_distance = 1;
            } else {
                // Stay put
                move_direction = 0;
                move_distance = 0;
            }
        } else {
            // Not in corner yet, move to nearest corner
            if (curr_row < m_board_row_max / 2) {
                // Top half - go to top corner
                if (curr_col < m_board_col_max / 2) {
                    move_direction = 8; // Up-left
                } else {
                    move_direction = 2; // Up-right
                }
            } else {
                // Bottom half - go to bottom corner
                if (curr_col < m_board_col_max / 2) {
                    move_direction = 6; // Down-left
                } else {
                    move_direction = 4; // Down-right
                }
            }
            move_distance = 2; // Move quickly to corner
        }
    }
};

extern "C" RobotBase* create_robot() {
    return new Robot_Blaster();
}
