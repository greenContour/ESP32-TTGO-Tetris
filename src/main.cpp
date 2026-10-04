#include <SPI.h>
#include <TFT_eSPI.h>      // Graphics library

TFT_eSPI tft = TFT_eSPI(); // Invoke library

enum GameAction{
  none,
  move_left,
  move_right,
  rotate_left,
  rotate_right,
  pause_action,
};

enum ButtonAction{
  no_action,
  short_press,
  long_press,
};


enum ButtonState{
  no_press,
  active_no_press,
  short_press_state,
  long_press_state,
};


const uint8_t block_size = 8;

const uint8_t matrix_width = 10;
const uint8_t matrix_height = 20;

const uint8_t min_x = 4;
const uint8_t max_x = 13;

const uint8_t render_offset_x = 1;
const uint8_t render_offset_y = 1;

const uint8_t y_offset = 4;
const uint8_t max_y = 23;

const uint8_t button_right_pin = 35;
const uint8_t button_left_pin = 0;

const uint32_t thresh_b_active = 60;
const uint32_t thresh_b_longpress = 180;
// const uint32_t thresh_b_longpress_repeat = 300;

uint32_t btimer_right = 0;
uint32_t btimer_left = 0;

// uint32_t btimer_longpress_right = 0;
// uint32_t btimer_longpress_left = 0;

// ButtonAction prev_bstate_right = ButtonAction::no_action;
// ButtonAction prev_bstate_left = ButtonAction::no_action;

ButtonState curr_bstate_right = ButtonState::no_press;
ButtonState curr_bstate_left = ButtonState::no_press;


//Function declaration
//------------------------------------------------------------------------------------------
void checkFixation();
void fixShape();

void completeRows(uint8_t set_y);

void getInput();
void applyShapeAction();

void summonShape();
uint8_t getNextShape();

bool moveShape(int8_t modif_x, int8_t modif_y, uint8_t modif_rotation);
bool checkCollision();

uint16_t getColor(uint8_t color_ind);

void renderRow(uint8_t set_y);
void renderMatrix(uint8_t set_y);
void renderShape(bool clean_out, uint8_t set_rotation = 0, uint8_t set_x = 0, uint8_t set_y = 0);

void resetGame();
void gameOver();
//------------------------------------------------------------------------------------------

uint8_t current_x;
uint8_t current_y = 0;
uint8_t current_rotation = 0;
uint8_t current_shape = 0;

bool game_pause = false;

GameAction next_action = GameAction::none;

uint8_t screen_width = 135;
uint8_t screen_height = 240;

uint32_t total_score = 0;
const uint8_t score_stack_bonus = 50;
uint8_t bonuses = 0;
const uint16_t add_score_base = 100;
uint16_t add_score = 0;

// shape id, rotation, y, x
const uint8_t shapes[7][4][4][4] = {
  // long line - 1
  {{{0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},}, 

   {{0, 0, 0, 0},
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}, 

   {{0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},},

   {{0, 0, 0, 0},
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}},
  // pyramid - 2
  {{{0, 2, 0, 0},
    {2, 2, 2, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},},

   {{0, 2, 0, 0},
    {0, 2, 2, 0},
    {0, 2, 0, 0},
    {0, 0, 0, 0},},

   {{0, 0, 0, 0},
    {2, 2, 2, 0},
    {0, 2, 0, 0},
    {0, 0, 0, 0},},

   {{0, 2, 0, 0},
    {2, 2, 0, 0},
    {0, 2, 0, 0},
    {0, 0, 0, 0},}},
  // Г - left - 3
  {{{3, 3, 0, 0},
    {0, 3, 0, 0},
    {0, 3, 0, 0},
    {0, 0, 0, 0},},

   {{0, 0, 3, 0},
    {3, 3, 3, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},},

   {{0, 3, 0, 0},
    {0, 3, 0, 0},
    {0, 3, 3, 0},
    {0, 0, 0, 0},}, 

   {{0, 0, 0, 0},
    {3, 3, 3, 0},
    {3, 0, 0, 0},
    {0, 0, 0, 0},}},
  // Г - right - 4
  {{{0, 4, 4, 0},
    {0, 4, 0, 0},
    {0, 4, 0, 0},
    {0, 0, 0, 0},}, 

   {{0, 0, 0, 0},
    {4, 4, 4, 0},
    {0, 0, 4, 0},
    {0, 0, 0, 0},}, 

   {{0, 4, 0, 0},
    {0, 4, 0, 0},
    {4, 4, 0, 0},
    {0, 0, 0, 0},}, 

   {{4, 0, 0, 0},
    {4, 4, 4, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}},
  // square - 5
  {{{5, 5, 0, 0},
    {5, 5, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}, 

   {{5, 5, 0, 0},
    {5, 5, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}, 

   {{5, 5, 0, 0},
    {5, 5, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}, 

   {{5, 5, 0, 0},
    {5, 5, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}},
  // zig - left - 6
  {{{6, 0, 0, 0},
    {6, 6, 0, 0},
    {0, 6, 0, 0},
    {0, 0, 0, 0},}, 

   {{0, 6, 6, 0},
    {6, 6, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}, 

   {{0, 6, 0, 0},
    {0, 6, 6, 0},
    {0, 0, 6, 0},
    {0, 0, 0, 0},}, 

   {{0, 0, 0, 0},
    {0, 6, 6, 0},
    {6, 6, 0, 0},
    {0, 0, 0, 0},}},
  // zig - right - 7
  {{{0, 7, 0, 0},
    {7, 7, 0, 0},
    {7, 0, 0, 0},
    {0, 0, 0, 0},}, 

   {{7, 7, 0, 0},
    {0, 7, 7, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}, 

   {{0, 0, 7, 0},
    {0, 7, 7, 0},
    {0, 7, 0, 0},
    {0, 0, 0, 0},}, 

   {{0, 0, 0, 0},
    {7, 7, 0, 0},
    {0, 7, 7, 0},
    {0, 0, 0, 0},}},
};

uint8_t game_matrix [matrix_height][matrix_width];

uint32_t main_timer = 0;
uint32_t tick_time_current = 300;
uint32_t tick_time_base = 300;
uint8_t tick_decrease = 10;
uint8_t tick_decreases = 0;
uint8_t max_tick_decreases = 15;
// uint8_t tick_counter = 0;
uint8_t falldown_ticks = 3;
uint8_t tick_count_falldown = 0;
bool gm_ticked = false;

uint8_t tick_count_fixation = 0;
uint8_t fixation_ticks = 5;

void setup() {
  // Use serial port

  Serial.begin(115200);
  Serial.println("Started serial");

  // Initialise the TFT screen
  tft.init();
  // Serial.println("Initialised display");

  pinMode(button_left_pin, INPUT_PULLUP);
  pinMode(button_right_pin, INPUT_PULLUP);

  tft.fillScreen(tft.color565(32, 32, 32));
  tft.drawFastVLine((render_offset_x + matrix_width) * block_size, render_offset_y * block_size, block_size * matrix_height, TFT_BLACK);
  tft.drawFastHLine(render_offset_x * block_size, (render_offset_y + matrix_height) * block_size, block_size * matrix_width, TFT_BLACK);
  // tft.fillRect(0, 0, 16, 32, RED);
  // tft.fillRect(16, 0, 16, 80, BLUE);
  // tft.fillRect(32, 0, 16, 80, PINK);
  // tft.fillRect(48, 0, 16, 80, YELLOW);
  // tft.fillRect(64, 0, 16, 80, GREEN);
  // tft.fillRect(80, 0, 16, 80, CYAN);
  // tft.fillRect(96, 0, 16, 80, PURPLE);
  // tft.fillRect(128, 0, 16, 80, WHITE);

  resetGame();
  Serial.println("init game");
}

//------------------------------------------------------------------------------------------

void loop(void) {

  getInput();

  if ((millis() - main_timer) > tick_time_current) {
    gm_ticked = true;

    tick_count_falldown++;
    // Serial.println("Timer ticked!");
  }

  if (!game_pause) {
    // Serial.println("debug: shapeAction");
    applyShapeAction();

    if (gm_ticked) {
      
      if (tick_count_falldown >= falldown_ticks) {
        // Serial.println("debug: moveShape: dropdown");
        moveShape(0, 1, 1);
      }

      // Serial.println("Position x: " + static_cast<String>(current_x) + " Position y: " + static_cast<String>(current_y) + " Rotation: " + static_cast<String>(current_rotation));
      
      gm_ticked = false;

      main_timer = millis();
    }
  }
  else if (next_action == pause_action) {
    game_pause = false;
  }
}


void applyShapeAction() {

  switch (next_action) {
    case (move_left): {

      moveShape(-1 ,0, 1);

      break;
    }
    case (move_right): {

      moveShape(1, 0, 1);

      break;
    }
    case (rotate_left): {

      moveShape(0, 0, 0);

      break;
    }
    case (rotate_right): {

      moveShape(0, 0, 2);

      break;
    }
    default: {

      break;
    }
  }
}


//------------------------------------------------------------------------------------------


void summonShape() {
  current_x = (max_x - min_x) / 2 + min_x;
  current_y = 0;

  current_shape = getNextShape();
  current_rotation = 0;
}


ButtonAction getButtonState(ButtonState &current_state, bool phys_input, uint32_t &b_timer) {

  ButtonAction ret = ButtonAction::no_action;

  if (phys_input) {

    // Serial.println("debug: positive phys_input: current state: " + static_cast<String>(current_state));

    switch (current_state) {
      case (ButtonState::no_press): {
        current_state = ButtonState::active_no_press;

        b_timer = millis();

        break;
      }
      case (ButtonState::active_no_press): {
        if ((millis() - b_timer) > thresh_b_active) {
          current_state = ButtonState::short_press_state;
          b_timer = millis();
        }

        break;
      }
      case (ButtonState::short_press_state): {
        // Serial.println("debug: long press: shortpress to longpress");
        if ((millis() - b_timer) > thresh_b_longpress) {
          current_state = ButtonState::long_press_state;
          b_timer = millis();
          ret = ButtonAction::long_press;
        }
        // Serial.println("getButtonState(): unexpected input state condition");

        break;
      }
      case (ButtonState::long_press_state): {

        // Serial.println("debug: long press: longpress");
        if ((millis() - b_timer) > thresh_b_longpress) {
          // Serial.println("debug: long press: condition met");
          current_state = ButtonState::long_press_state;
          ret = ButtonAction::long_press;
          b_timer = millis();
        }

        break;
      }
      default:
        break;
    }
  }
  else {
    switch (current_state) {
      case (ButtonState::no_press): {
        break;
      }
      case (ButtonState::active_no_press): {
        break;
      }
      case (ButtonState::short_press_state): {
        ret = ButtonAction::short_press;
        b_timer = millis();
        break;
      }
      case (ButtonState::long_press_state): {
        break;
      }
      default:
        break;
    }

    current_state = ButtonState::no_press;
  }

  return ret;
};


void getInput() {
  GameAction action = GameAction::none;
  ButtonAction action_left = ButtonAction::no_action;
  ButtonAction action_right = ButtonAction::no_action;

  bool inp_r = !digitalRead(button_right_pin);

  action_right = getButtonState(curr_bstate_right, inp_r, btimer_right);

  bool inp_l = !digitalRead(button_left_pin);
  action_left = getButtonState(curr_bstate_left, inp_l, btimer_left);

  // Serial.println("debug: getInput: inp_r: " + static_cast<String>(inp_r));
  // Serial.println("debug: getInput: inp_l: " + static_cast<String>(inp_l));

  if (action_right == action_left) {
    if (action_right == ButtonAction::short_press) {
      action = GameAction::pause_action;
    }
    else if (action_right == ButtonAction::long_press) {
      action = GameAction::none;
    }
  }
  else {
    switch (action_left) {
      case (ButtonAction::no_action): {
        break;
      }
      case (ButtonAction::short_press): {
        action = GameAction::rotate_left;
        break;
      }
      case (ButtonAction::long_press): {
        action = GameAction::move_left;
        break;
      }
    }


    switch (action_right) {
      case (ButtonAction::no_action): {
        break;
      }
      case (ButtonAction::short_press): {
        action = GameAction::rotate_right;
        break;
      }
      case (ButtonAction::long_press): {
        action = GameAction::move_right;
        break;
      }
    }
  }

  // if (action_left != ButtonAction::no_action) {
  //   Serial.println("debug: getInput: button_ACTION: " + static_cast<String>(action_left));
  // }

  // if (action != GameAction::none) {
  //   Serial.println("debug: getInput: game_ACTION: " + static_cast<String>(action));
  // }
  

  next_action = action;
}


void renderShape(bool clean_out, uint8_t set_rotation, uint8_t set_x, uint8_t set_y) {
  uint8_t ret_x = 0;
  uint8_t ret_y = 0;

  uint16_t color = getColor(current_shape + 1);

  for (uint8_t iy = 0; iy < 4; ++iy) {
    // Serial.println(set_y + iy);
    if ((set_y + iy) >= y_offset && (set_y + iy) <= max_y) {
      ret_y = (set_y + iy + render_offset_y) - y_offset;

      for (uint8_t ix = 0; ix < 4; ++ix) {

        if ((set_x + ix <= max_x) && (set_x + ix >= min_x)) {
        
          ret_x = (set_x + ix + render_offset_x) - min_x;
          
          
          if (shapes[current_shape][set_rotation][iy][ix] > 0) {
            if (clean_out) {
              tft.fillRect(ret_x * block_size, ret_y * block_size, block_size, block_size, TFT_BLACK);
            }
            else {
              tft.fillRect(ret_x * block_size, ret_y * block_size, block_size, block_size, color);
              tft.drawFastVLine(ret_x * block_size, ret_y * block_size, block_size, TFT_BLACK);
              tft.drawFastHLine(ret_x * block_size, ret_y * block_size, block_size, TFT_BLACK);
            }
          }
        }
      }
    }
  }
}


/*
renders the matrix starting from set_y (default - 23)
from bottom to top (23 - 4)
*/
void renderMatrix(uint8_t set_y=23) {
  uint8_t mat_y = set_y - y_offset;

  if (mat_y >= matrix_height) {
    mat_y = matrix_height - 1;
    // Serial.println("mat y error, exceeds matrix_height");
  }

  tft.fillRect(block_size * render_offset_x, block_size * render_offset_y,
     block_size * matrix_width, block_size * (mat_y + 1), TFT_BLACK);

  for (int8_t iy = mat_y; iy >= 0; iy--) {

    renderRow(iy);

    // Serial.print("rendering row: ");
    // Serial.println(iy);
  }
}

// ну, тип, красиво же?
void renderRow(uint8_t row_y) {
  for (uint8_t ix = 0; ix < matrix_width; ix++) {

      // Serial.print(game_matrix[row_y][ix]);
      // Serial.print(" ");

      tft.fillRect((ix + render_offset_x) * block_size, (row_y + render_offset_y) * block_size, block_size, block_size, getColor(game_matrix[row_y][ix]));

      if (game_matrix[row_y][ix] > 0) {
        // tft.fillRect((ix + render_offset_x) * block_size, (row_y + y_offset) * block_size, block_size, block_size, getColor(game_matrix[row_y][ix]));
        tft.drawFastVLine((ix + render_offset_x) * block_size, (row_y + render_offset_y) * block_size, block_size, TFT_BLACK);
        tft.drawFastHLine((ix + render_offset_x) * block_size, (row_y + render_offset_y) * block_size, block_size, TFT_BLACK);
      }
    }
  // Serial.print("  ");
  // Serial.print(row_y);
  // Serial.println(" END");
}

/*
Receives position to shift to (matrix cord)
e.g. set_y = 1 --> shifts matrix down to 1 (from 1)
*/
void shiftMatrix(uint8_t mat_y) {

  for (int8_t iy = mat_y - 1; iy >= 0; iy--) {
    for (uint8_t ix = 0; ix < matrix_width; ix++) {
      game_matrix[iy + 1][ix] = game_matrix[iy][ix];
    }
  }

  for (uint8_t ix = 0; ix < matrix_width; ix++) {
    game_matrix[0][ix] = 0;
  }
}


void completeRows(uint8_t set_y) {

  uint8_t mat_y = set_y - y_offset;
 
  bool full = true;

  int8_t lower_border = 0;

  for (int8_t iy = 3; iy >= lower_border; iy--) {
    full = true;

    if ((mat_y + iy) < matrix_height) {
      for (uint8_t mat_x = 0; mat_x < matrix_width; mat_x++) {
        
        if (game_matrix[mat_y + iy][mat_x] == 0) {
          full = false;
          break;
        }
      }

      if (full) {
        shiftMatrix(mat_y + iy);
        renderMatrix(set_y + iy);

        iy++;
        lower_border++;

        bonuses++;

        total_score += 100 + score_stack_bonus * bonuses;

        Serial.print("Score: ");
        Serial.println(total_score);

        tick_time_current = tick_time_current - ((total_score / 10000) * tick_decrease);
        if (tick_time_current < tick_time_base - (max_tick_decreases * tick_decrease)) {
          tick_time_current = tick_time_base - (max_tick_decreases * tick_decrease);
        }

        delay(tick_time_current);
      }
      else {
        bonuses = 0;
      }
    } 
  }

  
}


uint16_t getColor(uint8_t color_ind) {
  switch (color_ind) {
    case (1): {

      return TFT_CYAN;

      break;
    }
    case (2): {

      return TFT_RED;
      
      break;
    }
    case (3): {

      return TFT_GREEN;
      
      break;
    }
    case (4): {

      return TFT_YELLOW;
      
      break;
    }
    case (5): {

      return TFT_BLUE;
      
      break;
    }
    case (6): {

      return TFT_PINK;
      
      break;
    }
    case (7): {

      return TFT_ORANGE;
      
      break;
    }
    default: {

      return TFT_BLACK;

      break;
    }
  }
}


// returns True if collides, False if doesn't
bool checkCollision() {
  uint8_t mat_x = 0;
  uint8_t mat_y = 0;

  for (uint8_t iy = 0; iy < 4; ++iy) {
    for (uint8_t ix = 0; ix < 4; ++ix) {
      mat_x = current_x + ix;
      mat_y = current_y + iy;

      if (shapes[current_shape][current_rotation][iy][ix] > 0) {
        if ((mat_x < min_x) || (mat_x > max_x)) {
          // Serial.println("XCOLLISION");
          return true;
        }
        else if (mat_y > max_y) {
          // Serial.println("YCOLLISION");
          return true;
        }
        else if (mat_y >= y_offset) {
          if (game_matrix[mat_y - y_offset][mat_x - min_x] > 0) {
            // Serial.println("MATCOLLISION");
            return true;
          }
        }
      }
      
    }
  }

  return false;
}


void fixShape() {

  uint8_t mat_x = 0;
  uint8_t mat_y = 0;

  for (uint8_t iy = 0; iy < 4; ++iy) {
    for (uint8_t ix = 0; ix < 4; ++ix) {
      mat_x = current_x + ix;
      mat_y = current_y + iy;

      if (shapes[current_shape][current_rotation][iy][ix] > 0) {
        game_matrix[mat_y - y_offset][mat_x - min_x] = shapes[current_shape][current_rotation][iy][ix];
      }
    }
  }
}


void checkFixation() {

  tick_count_fixation++;

  if (tick_count_fixation >= fixation_ticks) {
    tick_count_fixation = 0;

    fixShape();

    completeRows(current_y);

    if (current_y < y_offset) {
      gameOver();
    }

    summonShape();
  }
}


void resetGame() {
  Serial.println("Reset game!");
  // Serial.println("debug: resetGame: matrix reset start");

  for (uint8_t iy = 0; iy < matrix_height; ++iy) {
    for (uint8_t ix = 0; ix < matrix_width; ++ix) {
      // Serial.println("X: " + static_cast<String>(ix) + " Y: " + static_cast<String>(iy));
      game_matrix[iy][ix] = 0;
    }
  }

  // Serial.println("debug: resetGame: matrix reset end");

  current_rotation = 0;
  current_shape = getNextShape();

  tick_count_fixation = 0;
  tick_count_falldown = 0;

  tick_time_current = tick_time_base;

  total_score = 0;

  main_timer = millis();

  next_action = none;
  current_x = ((max_x - min_x) / 2) + min_x;

  // Serial.println("debug: resetGame: matrix render start");
  renderMatrix();
  // Serial.println("debug: resetGame: matrix render end");
}



void gameOver() {

  resetGame();

}


// returns True if succesfully moved the shape 
bool moveShape(int8_t modif_x, int8_t modif_y, uint8_t modif_rotation) {

  current_x += modif_x;
  current_y += modif_y;

  // if (modif_rotation != 1)
    // Serial.println("debug: moveShape: modif_rotation: " + static_cast<char>(modif_rotation));

  uint8_t prev_rotation = current_rotation;

  if (modif_rotation == 0) {
    if (current_rotation == 0) {
      current_rotation = 3;
    }
    else {
      current_rotation--;
    }
  }
  else if (modif_rotation == 2) {
    current_rotation++;

    if (current_rotation >= 4) {
      current_rotation = 0;
    }
  }

  // Serial.println("debug: moveShape: rotated");

  if (checkCollision()) {
    //  Serial.println("debug: moveShape: checkCollision = True");

    current_x -= modif_x;
    current_y -= modif_y;
    current_rotation = prev_rotation;
    
    if (modif_y > 0) {
      checkFixation();
    }

    return false;
  }
  else {

    // Serial.println("debug: moveShape: renderShape");

    renderShape(true, prev_rotation, current_x - modif_x, current_y - modif_y);

    renderShape(false, current_rotation, current_x, current_y);

    // Serial.println("debug: moveShape: renderShape end");
    
    return true;
  }
}


uint8_t getNextShape() {
  return (static_cast<uint8_t>(esp_random() % 7));
}
