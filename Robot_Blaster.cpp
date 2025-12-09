#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <cstdlib>

class Robot_Blaster : public RobotBase
{
private:
    int target_row;
    int target_col;
    bool has_target;
    int scan_direction;
    int stuck_counter;
    int last_row;
    int last_col;

public:
    Robot_Blaster() : RobotBase(2, 5, grenade), 
                      target_row(-1), target_col(-1), 
                      has_target(false), scan_direction(0),
                      stuck_counter(0), last_row(-1), last_col(-1) {}

    void get_radar_direction(int& radar_direction) override {
        // Cycle through all directions to find enemies
        scan_direction++;
        if (scan_direction > 8) scan_direction = 0;
        radar_direction = scan_direction;
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        has_target = false;
        
        // Find closest robot
        int curr_row, curr_col;
        get_current_location(curr_row, curr_col);
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
        
        // Check if stuck
        if (curr_row == last_row && curr_col == last_col) {
            stuck_counter++;
        } else {
            stuck_counter = 0;
        }
        last_row = curr_row;
        last_col = curr_col;
        
        // If stuck, try random direction
        if (stuck_counter > 3) {
            move_direction = 1 + (rand() % 8);
            move_distance = 1;
            stuck_counter = 0;
            return;
        }
        
        // Move toward center or hunt
        int center_row = m_board_row_max / 2;
        int center_col = m_board_col_max / 2;
        
        int dr = center_row - curr_row;
        int dc = center_col - curr_col;
        
        // Pick direction based on largest difference
        if (abs(dr) > abs(dc)) {
            move_direction = (dr > 0) ? 5 : 1; // Down or Up
        } else if (abs(dc) > 0) {
            move_direction = (dc > 0) ? 3 : 7; // Right or Left
        } else {
            // At center, patrol randomly
            move_direction = 1 + (rand() % 8);
        }
        
        move_distance = 2;
    }
};

extern "C" RobotBase* create_robot() {
    return new Robot_Blaster();
}
