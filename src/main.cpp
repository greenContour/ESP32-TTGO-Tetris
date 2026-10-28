#include <SPI.h>
#include <TFT_eSPI.h>      // Graphics library

TFT_eSPI tft = TFT_eSPI(); // Invoke library

enum class GameAction{
  none,
  move_left,
  move_right,
  rotate_left,
  rotate_right,
  pause_action,
};

enum class ButtonAction{
  no_action,
  short_press,
  long_press_active,
};


enum class ButtonState{
  no_press,
  active_no_press,
  short_press_state,
  long_press_state,
};


const uint8_t block_size = 8;

const uint8_t matrix_width = 10;
const uint8_t matrix_height = 20;

const uint8_t min_gc_x = 4;
const uint8_t max_gc_x = 13;

const uint8_t render_offset_x = 1;
const uint8_t render_offset_y = 1;

const uint8_t min_gc_y = 4;
const uint8_t max_gc_y = 23;

const uint8_t button_right_pin = 35;
const uint8_t button_left_pin = 0;

const uint32_t thresh_debounce = 20;
const uint32_t thresh_longpress = 180;
// const uint32_t thresh_b_longpress_repeat = 300;

#define GRAY tft.color565(32, 32, 32)
#define DARKGREY tft.color565(4, 4, 4)

// 0b 00000|000000|00000

uint32_t bstate_t_right = 0;
uint32_t bstate_t_left = 0;


// Time for double presses 
const uint32_t thresh_double = 120;

// Second press wait time // partner wait time
uint32_t right_t_wait = 0;
uint32_t left_t_wait = 0;

// uint32_t btimer_longpress_right = 0;
// uint32_t btimer_longpress_left = 0;


// Waits for second press // waits for partner: if (left_waits) => left was pressed, waiting for right
bool right_waits = false;
bool left_waits = false;

ButtonState curr_bstate_right = ButtonState::no_press;
ButtonState curr_bstate_left = ButtonState::no_press;


//Function declaration
//------------------------------------------------------------------------------------------
void checkFixation();
void fixShape();

void renderScore();

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
void renderShape(bool clean_out, uint8_t shape_id, uint8_t set_rotation, int16_t set_x, int16_t set_y);
void renderNextShapes();
void wipeScreen();

void resetGame();
void gameOver();
void pauseGame();
//------------------------------------------------------------------------------------------

// Current position in offset coordinates
// as I'm too lazy to use an int lol
uint8_t current_x;
uint8_t current_y = 0;
// current rotation 0 - 3 |including
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

   {{6, 0, 0, 0},
    {6, 6, 0, 0},
    {0, 6, 0, 0},
    {0, 0, 0, 0},}, 

   {{0, 6, 6, 0},
    {6, 6, 0, 0},
    {0, 0, 0, 0},
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

   {{0, 7, 0, 0},
    {7, 7, 0, 0},
    {7, 0, 0, 0},
    {0, 0, 0, 0},}, 

   {{7, 7, 0, 0},
    {0, 7, 7, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},}, },
};


uint8_t next_shapes[3];
uint8_t max_next_shapes = 3;

uint8_t game_matrix [matrix_height][matrix_width];

uint32_t main_timer = 0;
uint32_t tick_time_current = 300;
uint32_t tick_time_base = 300;
uint8_t tick_decrease = 5;
uint8_t tick_decreases = 0;
uint8_t max_tick_decreases = 30;
// uint8_t tick_counter = 0;
uint8_t falldown_ticks = 3;
uint8_t tick_count_falldown = 0;
bool gm_ticked = false;

uint8_t falldown_speed_modifier = 1;
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

  tft.fillScreen(GRAY);
  tft.drawFastVLine((render_offset_x + matrix_width) * block_size, render_offset_y * block_size, block_size * matrix_height, TFT_BLACK);
  tft.drawFastHLine(render_offset_x * block_size, (render_offset_y + matrix_height) * block_size, block_size * matrix_width, TFT_BLACK);

  resetGame();
  Serial.println("init game");
}

//------------------------------------------------------------------------------------------

void loop(void) {

  getInput();

  if ((millis() - main_timer) > (tick_time_current / falldown_speed_modifier)) {
    gm_ticked = true;

    tick_count_falldown++;
  }

  if (!game_pause) {
    applyShapeAction();

    if (gm_ticked) {
      
      if (tick_count_falldown >= falldown_ticks) {
        moveShape(0, 1, 1);
      }

      gm_ticked = false;

      main_timer = millis();
    }
  }
  else if (next_action == GameAction::pause_action) {
    game_pause = false;

    renderMatrix(max_gc_y);
  }
}


void applyShapeAction() {

  switch (next_action) {
    case (GameAction::move_left): {

      moveShape(-1 ,0, 1);

      break;
    }
    case (GameAction::move_right): {

      moveShape(1, 0, 1);

      break;
    }
    case (GameAction::rotate_left): {

      moveShape(0, 0, 0);

      break;
    }
    case (GameAction::rotate_right): {

      moveShape(0, 0, 2);

      break;
    }
    case (GameAction::pause_action): {

      pauseGame();

      break;
    }
    default: {

      break;
    }
  }
}


//------------------------------------------------------------------------------------------


void summonShape() {
  current_x = (max_gc_x - min_gc_x) / 2 + min_gc_x;
  current_y = 0;

  current_shape = getNextShape();
  current_rotation = 0;
}


ButtonAction getButtonState(ButtonState &current_state, bool phys_input, uint32_t &b_timer) {

  ButtonAction ret = ButtonAction::no_action;

  if (phys_input) {

    switch (current_state) {
      case (ButtonState::no_press): {
        current_state = ButtonState::active_no_press;

        b_timer = millis();

        break;
      }
      case (ButtonState::active_no_press): {
        if ((millis() - b_timer) > thresh_debounce) {
          current_state = ButtonState::short_press_state;
        }

        break;
      }
      case (ButtonState::short_press_state): {
        if ((millis() - b_timer) > thresh_longpress) {
          current_state = ButtonState::long_press_state;
          b_timer = millis();
          ret = ButtonAction::long_press_active;
        }

        break;
      }
      case (ButtonState::long_press_state): {

        if ((millis() - b_timer) > thresh_longpress) {

          current_state = ButtonState::long_press_state;
          ret = ButtonAction::long_press_active;
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
      case (ButtonState::short_press_state): {
        ret = ButtonAction::short_press;
        b_timer = millis();
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
  GameAction game_action = GameAction::none;

  uint32_t now = millis();

  ButtonAction action_left = getButtonState(curr_bstate_left, !digitalRead(button_left_pin), bstate_t_left);
  ButtonAction action_right = getButtonState(curr_bstate_right, !digitalRead(button_right_pin), bstate_t_right);

  bool r_held = curr_bstate_right != ButtonState::no_press;
  bool l_held = curr_bstate_left != ButtonState::no_press;

  falldown_speed_modifier = (curr_bstate_left == ButtonState::long_press_state && curr_bstate_right == ButtonState::long_press_state) ? 3 : 1;

  if (action_left == action_right && action_left == ButtonAction::short_press) { left_waits = right_waits = false; game_action = GameAction::pause_action; }
  else if (action_left == ButtonAction::short_press) {
    if (right_waits) { game_action = GameAction::pause_action; right_waits = false; }
    else if (r_held) { left_waits = true; left_t_wait = now; }
    else { game_action = GameAction::rotate_left; }
  }
  else if (action_right == ButtonAction::short_press) {
    if (left_waits) { game_action = GameAction::pause_action; left_waits = false; }
    else if (l_held) { right_waits = true; right_t_wait = now; }
    else { game_action = GameAction::rotate_right; }
  }

  if (left_waits && (now - left_t_wait) > thresh_double) {
    left_waits = false;
    left_t_wait = now;
    game_action = GameAction::rotate_left;
  }
  if (right_waits && (now - right_t_wait) > thresh_double) {
    right_waits = false;
    right_t_wait = now;
    game_action = GameAction::rotate_right;
  }
  
  if (action_left == ButtonAction::long_press_active && !r_held) { game_action = GameAction::move_left; }
  if (action_right == ButtonAction::long_press_active && !l_held) { game_action = GameAction::move_right; }

  next_action = game_action;
}


// Note: Shape_id is in range of 0-6
void renderShape(bool clean_out, uint8_t shape_id, uint8_t set_rotation, int16_t scrn_x, int16_t scrn_y) {
  uint16_t color = getColor(shape_id + 1);
  int16_t m_scrn_y = 0;
  int16_t m_scrn_x = 0;

  for (uint8_t iy = 0; iy < 4; ++iy) {
    m_scrn_y = scrn_y + iy * block_size;

    if ((m_scrn_y >= render_offset_y * block_size) && (m_scrn_y <= screen_height- block_size)) {

      for (uint8_t ix = 0; ix < 4; ++ix) {
        m_scrn_x = scrn_x + ix * block_size;

        if ((m_scrn_x <= screen_width) && (m_scrn_x >= 0)) {
          
          if (shapes[shape_id][set_rotation][iy][ix] > 0) {
            if (clean_out) {
              tft.fillRect(m_scrn_x, m_scrn_y, block_size, block_size, TFT_BLACK);
              tft.drawFastVLine(m_scrn_x + block_size - 1, m_scrn_y, block_size, DARKGREY);
              tft.drawFastHLine(m_scrn_x, m_scrn_y + block_size - 1, block_size, DARKGREY);
            }
            else {
              tft.fillRect(m_scrn_x, m_scrn_y, block_size, block_size, color);
              tft.drawFastVLine(m_scrn_x + block_size - 1, m_scrn_y, block_size, TFT_BLACK);
              tft.drawFastHLine(m_scrn_x, m_scrn_y + block_size - 1, block_size, TFT_BLACK);
            }
          }
        }
      }
    }
  }
}


/*
Renders the matrix starting from set_y (default - 23)
from bottom to top (23 - 4)
*/
void renderMatrix(uint8_t set_y=23) {
  uint8_t mat_y = set_y - min_gc_y;

  if (mat_y >= matrix_height) {
    mat_y = matrix_height - 1;
    // w! spaghetti code
  }

  tft.fillRect(block_size * render_offset_x, block_size * render_offset_y,
     block_size * matrix_width, block_size * (mat_y + 1), TFT_BLACK);

  for (int8_t iy = mat_y; iy >= 0; iy--) {

    renderRow(iy);
  }
}

// ну, тип, красиво же?
void renderRow(uint8_t row_y) {

  int16_t scrn_y = 0;
  int16_t scrn_x = 0;

  for (uint8_t ix = 0; ix < matrix_width; ix++) {

      scrn_y = (row_y + render_offset_y) * block_size;
      scrn_x = (ix + render_offset_x) * block_size;

      tft.fillRect(scrn_x, scrn_y, block_size, block_size, getColor(game_matrix[row_y][ix]));

      tft.drawFastVLine(scrn_x + block_size - 1, scrn_y, block_size, (game_matrix[row_y][ix] > 0) ? TFT_BLACK : DARKGREY);
      tft.drawFastHLine(scrn_x, scrn_y + block_size - 1, block_size - 1, (game_matrix[row_y][ix] > 0) ? TFT_BLACK : DARKGREY);
    }
}


void wipeScreen() {
  tft.fillRect(block_size * render_offset_x, block_size * render_offset_y,
     block_size * matrix_width, block_size * matrix_height, TFT_BLACK);
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

  uint8_t mat_y = set_y - min_gc_y;
 
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

        renderScore();

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


void renderScore() {
  tft.setTextColor(TFT_WHITE, GRAY);
  tft.drawString("SCORE: " + static_cast<String>(total_score), block_size * render_offset_x, block_size * (matrix_height + 4), 2);
}


// gets matrix shape id 1-7
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
        if ((mat_x < min_gc_x) || (mat_x > max_gc_x)) {
          // Serial.println("XCOLLISION");
          return true;
        }
        else if (mat_y > max_gc_y) {
          // Serial.println("YCOLLISION");
          return true;
        }
        else if (mat_y >= min_gc_y) {
          if (game_matrix[mat_y - min_gc_y][mat_x - min_gc_x] > 0) {
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
        game_matrix[mat_y - min_gc_y][mat_x - min_gc_x] = shapes[current_shape][current_rotation][iy][ix];
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

    if (current_y < min_gc_y) {
      gameOver();
    }

    summonShape();
  }
}


void pauseGame() {
  game_pause = true;
  uint8_t font_size = 2;

  wipeScreen();

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("PAUSE", block_size * (render_offset_x + matrix_width / 2) - tft.textWidth("PAUSE", font_size) / 2, block_size * (render_offset_y + matrix_height / 2) - tft.fontHeight(font_size) / 2, font_size);
}


void resetGame() {
  Serial.println("Reset game!");
  tft.fillScreen(GRAY);

  for (uint8_t iy = 0; iy < matrix_height; ++iy) {
    for (uint8_t ix = 0; ix < matrix_width; ++ix) {
      game_matrix[iy][ix] = 0;
    }
  }

  current_rotation = 0;

  for (int8_t i = 0; i < max_next_shapes; i++) {
    next_shapes[i] = (static_cast<uint8_t>(esp_random() % 7));
  }

  current_shape = getNextShape();

  tick_count_fixation = 0;
  tick_count_falldown = 0;

  tick_time_current = tick_time_base;

  total_score = 0;

  main_timer = millis();

  next_action = GameAction::none;
  current_x = ((max_gc_x - min_gc_x) / 2) + min_gc_x;

  
  renderScore();

  renderMatrix();
}


void gameOver() {

  resetGame();

}


// returns True if succesfully moved the shape 
bool moveShape(int8_t modif_x, int8_t modif_y, uint8_t modif_rotation) {

  current_x += modif_x;
  current_y += modif_y;

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

  if (checkCollision()) {
    current_x -= modif_x;
    current_y -= modif_y;
    current_rotation = prev_rotation;
    
    if (modif_y > 0) {
      checkFixation();
    }

    return false;
  }
  else {

    int16_t scrn_x = (render_offset_x + current_x - min_gc_x) * block_size;
    int16_t scrn_y = (render_offset_y + current_y - min_gc_y) * block_size;

    renderShape(true, current_shape, prev_rotation, scrn_x - static_cast<int16_t>(modif_x * block_size), scrn_y - static_cast<int16_t>(modif_y * block_size));

    renderShape(false, current_shape, current_rotation, scrn_x, scrn_y);
    
    return true;
  }
}


uint8_t getNextShape() {
  uint8_t ret = next_shapes[0];

  for (int8_t i = 0; i < max_next_shapes - 1; i++) {
    next_shapes[i] = next_shapes[i + 1];
  }

  next_shapes[max_next_shapes - 1] = (static_cast<uint8_t>(esp_random() % 7));
  Serial.println(next_shapes[max_next_shapes - 1]);
  renderNextShapes();

  return ret;
}


void renderNextShapes() {
  int16_t render_x = (render_offset_x + matrix_width + 1) * block_size;
  int16_t render_y = render_offset_y * block_size;

  for (int8_t shape_id = 0; shape_id < max_next_shapes; shape_id++) {
    tft.fillRect(render_x, (render_y) + static_cast<int16_t>(shape_id * (4 + 1)) * block_size, 4 * block_size, 4 * block_size, TFT_BLACK);
    renderShape(false, next_shapes[shape_id], 0, render_x, (render_y) + static_cast<int16_t>(shape_id * (4 + 1)) * block_size);
  }
}

