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

#define BLACK 0x0000
#define WHITE 0xFFFF

#define RED 0xF00F
#define BLUE 0x00FF
#define GREEN 0x0F0F
#define YELLOW 0x0FFF
#define PINK 0xF3FF
#define PURPLE 0xB01F
#define CYAN 0x6BFF


const uint8_t block_size = 8;

const uint8_t matrix_width = 10;
const uint8_t matrix_height = 20;

const uint8_t min_x = 4;
const uint8_t max_x = 13;

const uint8_t y_offset = 4;
const uint8_t max_y = 23;

const uint8_t button_right = 0;
const uint8_t button_left = 35;

//Function declaration
//------------------------------------------------------------------------------------------
void checkFixation();
void completeRows(uint8_t set_y);

void getInput();
void applyShapeAction();

void summonShape();
uint8_t getNextShape();

bool moveShape(int8_t modif_x, int8_t modif_y, uint8_t modif_rotation);
bool checkCollision();

uint16_t getColor(uint8_t color_ind);

void renderRow(uint8_t set_y);
void renderMatrix(uint8_t set_y=max_y);
void renderShape(bool clean_out, uint8_t set_rotation = 0, uint8_t set_x = 0, uint8_t set_y = 0);

void resetGame();
void gameOver();
//------------------------------------------------------------------------------------------

uint8_t current_x;
uint8_t current_y = 0;
uint8_t current_rotation = 0;
uint8_t current_shape = 0;

bool game_pause = false;

GameAction next_action = none;

uint8_t screen_width = 135;
uint8_t screen_height = 240;

uint32_t total_score = 0;
const uint8_t score_multip_stack = 15;
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

uint8_t game_matrix [20][10];

uint32_t main_timer = 0;
uint32_t tick_time = 300;
// uint8_t tick_counter = 0;
uint8_t falldown_ticks = 3;
uint8_t tick_count_falldown = 0;
bool gm_ticked = false;

uint8_t tick_count_fixation = 0;
uint8_t fixation_ticks = 5;

void setup() {
  // Use serial port

  Serial.begin(115200);

  pinMode(button_left, INPUT_PULLUP);
  pinMode(button_right, INPUT_PULLUP);

  tft.fillScreen(BLACK);

  tft.drawRect(0, 0, 16, 80, RED);
  tft.drawRect(16, 0, 16, 80, BLUE);
  tft.drawRect(32, 0, 16, 80, PINK);
  tft.drawRect(48, 0, 16, 80, YELLOW);
  tft.drawRect(64, 0, 16, 80, GREEN);
  tft.drawRect(80, 0, 16, 80, CYAN);
  tft.drawRect(96, 0, 16, 80, PURPLE);
  tft.drawRect(128, 0, 16, 80, WHITE);

  // Initialise the TFT screen
  tft.init();

  resetGame();
}

//------------------------------------------------------------------------------------------

void loop(void) {

  getInput();

  if ((millis() - main_timer) > tick_time) {
    gm_ticked = true;

    tick_count_falldown++;
  }

  if (!game_pause) {
    if (gm_ticked) {
      applyShapeAction();
      
      if (tick_count_falldown >= falldown_ticks) {
        moveShape(0, 1, 1);
      }
      
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
  current_x = (max_x - min_x) / 2;
  current_y = 0;

  current_shape = getNextShape();
  current_rotation = 0;
}


void getInput() {
  GameAction action = none;

  bool inp_r = digitalRead(button_left);
  bool inp_l = digitalRead(button_right);

  next_action = action;
}


void renderShape(bool clean_out, uint8_t set_rotation, uint8_t set_x, uint8_t set_y) {
  uint8_t mat_x = 0;
  uint8_t mat_y = 0;

  uint16_t color = getColor(current_shape);

  for (uint8_t iy = 0; iy < 4; ++iy) {
    for (uint8_t ix = 0; ix < 4; ++ix) {
      mat_x = set_x + ix;
      mat_y = set_y + iy;
      
      if (shapes[current_shape][set_rotation][iy][ix] > 0) {
        if (clean_out) {
          tft.drawRect(mat_x * block_size, mat_y * block_size, block_size, block_size, BLACK);
        }
        else {
          tft.drawRect(mat_x * block_size, mat_y * block_size, block_size, block_size, color);
        }
      }
    }
  }
}


/*
renders the matrix starting from set_y (default - 23)
from bottom to top (23 - 4)
*/
void renderMatrix(uint8_t set_y) {
  uint8_t mat_y = set_y - y_offset;

  tft.drawRect(0, 0, block_size * matrix_width, block_size * mat_y, BLACK);

  for (uint8_t iy = mat_y; iy >= 0; iy--) {

    renderRow(iy);
  }
}

// ну, тип, красиво же?
void renderRow(uint8_t set_y) {
  for (uint8_t ix = 0; ix < matrix_width; ix++) {

      if (game_matrix[set_y][ix] > 0) {
        tft.drawRect(ix * block_size, set_y * block_size, block_size, block_size, getColor(game_matrix[set_y][ix]));
      }
    }
}

/*
Receives position to shift to (matrix cord)
e.g. set_y = 1 --> shifts matrix down to 1 (from 1)
*/
void shiftMatrix(uint8_t mat_y) {

  for (uint8_t iy = mat_y - 1; iy <= y_offset; ++iy) {
    for (uint8_t ix = 0; ix <= max_x - min_x; ++ix) {
      game_matrix[iy-1][ix] = game_matrix[iy][ix];
    }
  }
}


void completeRows(uint8_t set_y) {

  uint8_t mat_y = set_y - y_offset;

  bool full = true;

  for (uint8_t iy = 0; iy < 4; ++iy) {
    full = true;

    for (uint8_t mat_x = 0; mat_x <= max_x - min_x; ++mat_x) {
      
      if (game_matrix[mat_y + iy][mat_x] == 0) {
        full = false;
        break;
      }
    }

    if (full) {
      shiftMatrix(mat_y + iy);
      renderMatrix(set_y);

      if (add_score = 0) {
        add_score = 100;
      }
      else {
        add_score *= score_multip_stack;
        add_score /= 10;
      }
    }
    else {
      total_score += add_score;
      add_score = 0;
    }
  }
}


uint16_t getColor(uint8_t color_ind) {
  switch (color_ind) {
    case (1): {

      return RED;

      break;
    }
    case (2): {

      return BLUE;
      
      break;
    }
    case (3): {

      return GREEN;
      
      break;
    }
    case (4): {

      return YELLOW;
      
      break;
    }
    case (5): {

      return PINK;
      
      break;
    }
    case (6): {

      return PURPLE;
      
      break;
    }
    case (7): {

      return CYAN;
      
      break;
    }
    default: {

      return BLACK;

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

      if (mat_x < min_x || mat_x > max_x) {
        return false;
      }
      else if (mat_y > max_y) {
        return false;
      }
      else if (game_matrix[max_y][mat_x] > 0 && shapes[current_shape][current_rotation][iy][ix] > 0) {
        return false;
      }
    }
  }

  return true;
}


void checkFixation() {

  tick_count_falldown++;

  if (tick_count_falldown >= falldown_ticks) {
    tick_count_falldown = 0;

    completeRows(current_y);

    if (current_y < y_offset) {
      gameOver();
    }

    summonShape();
  }
}


void resetGame() {

  for (uint8_t iy = 0; iy < matrix_height; ++iy) {
    for (uint8_t ix = 0; ix < matrix_width; ++ix) {
      game_matrix[iy][ix] = 0;
    }
  }

  current_rotation = 0;
  current_shape = getNextShape();

  tick_count_fixation = 0;
  tick_count_falldown = 0;

  total_score = 0;

  main_timer = millis();

  next_action = none;
  current_x = ((max_x - min_x) / 2) + min_x;

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

    if (current_y >= y_offset) {
      renderShape(true, prev_rotation, current_x - modif_x, current_y - modif_y);

      renderShape(false, current_rotation, current_x, current_y);
    }
    
    return true;
  }
}


uint8_t getNextShape() {
  return static_cast<uint8_t>(esp_random() % 7);
}
