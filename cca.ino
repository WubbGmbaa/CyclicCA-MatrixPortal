/* ----------------------------------------------------------------------
Implementation of 2D cyclic cellular automata algorithm for 32x32 HUB75
matrix on Adafruit Matrix Portal M4 using Protomatter.

http://www.bryangratz.net

See https://softologyblog.wordpress.com/2013/08/29/cyclic-cellular-automata/
and http://www.mirekw.com/ca/rullex_cycl.html

Further reading - "Cyclic Cellular Automata in Two Dimensions" by R. Fisch,
J. Gravner, D. Griffeath

Some code based on Adafruit "Protomatter Simple" example sketch

TODO:
- Replace magic number matrix size with global var
- Implement Moore neighborhoods
------------------------------------------------------------------------- */

#include <Adafruit_Protomatter.h>
#include <cmath> //for modulo

//HUB75 RGB matrix pins for MATRIX PORTAL M4
uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
uint8_t addrPins[] = {17, 18, 19, 20, 21};
uint8_t clockPin   = 14;
uint8_t latchPin   = 15;
uint8_t oePin      = 16;

//CCA Rule Vars
/*
  examples:
  r2/t2/c6/nn - Squarish Spirals, Jason Rampe (softology, visions of chaos)
  r2/t5/c3/nn - Cubism, Jason Rampe
  r2/t3/c5/nn - Maps, Mirek Wojtowicz (CELLebration)
*/
int r = 2;
int t = 5;
int c = 3;

//global cell array decs
int grid[32][32];
int nextGrid[32][32];

int h;

int brightness = 255; //max 255

// Matrix setup
Adafruit_Protomatter matrix(
  32,          // Width of matrix (or matrix chain) in pixels
  6,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
  4, addrPins, // # of address pins (height is inferred), array of pins
  clockPin, latchPin, oePin, // Other matrix control pins
  false);      // No double-buffering here (see "doublebuffer" example)

// fill grid with random values 
void randomizeGrid(){
  for(int x=0; x<32; x++) {
    for(int y=0; y<32; y++){
      grid[x][y] = random(c);
    }
  }
}

//modulo fucntion, but negatives mirrored. do not care about values less than negative range, so this suffices
int progMod(int a, int b){
  return (a + b) % b; 
}

//update screen with current grid values mapped to hue
void displayGrid(){
  //write grid cells to matrix pixels
  for(int x=0; x<matrix.width(); x++) {
    for(int y=0; y<matrix.height(); y++){
      matrix.drawPixel(x, y, matrix.colorHSV((((65535*(grid[x][y])/c)+h)%65535),255,brightness)); //colorHSV first arg splits hue space evenly into c possible colors plus offset
    }
  }
  matrix.show(); // Copy data to matrix buffers
}

//main CA loop function
void cycleIterate(){
  for(int x=0; x<matrix.width(); x++) {
    for(int y=0; y<matrix.height(); y++){
      countPixels(x,y);
    }
  }
  //copy temp grid to main grid
  for(int x=0; x<matrix.width(); x++) {
    for(int y=0; y<matrix.height(); y++){
      grid[x][y] = nextGrid[x][y];
    }
  }
}

//check ca rules for NN neighborhood
void countPixels(int xLoc, int yLoc){
  int pixCount = 0;
  
  //determine "eat" state for current test cell
  int nextVal;
  if(grid[xLoc][yLoc] == c - 1){
    nextVal = 0;
  }else{
    nextVal = grid[xLoc][yLoc]+1;
  }
  
  //test all cells in NN neighborhood for given range, iteratively count "eat" cells
  for(int n=1; n<=r; n++){
    for(int i=0; i<n; i++){
      if (grid[progMod((xLoc - (n-i)),32)][progMod((yLoc + i),32)] == nextVal){
        pixCount++;
      }
      if (grid[progMod((xLoc + i),32)][progMod((yLoc + (n-i)),32)] == nextVal){
        pixCount++;
      }
      if (grid[progMod((xLoc + (n-i)),32)][progMod((yLoc - i),32)] == nextVal){
        pixCount++;
      }
      if (grid[progMod((xLoc - i),32)][progMod((yLoc - (n-i)),32)] == nextVal){
        pixCount++;
      }
    }
  }

  //compare with threshold, write to temp grid
  if(pixCount >= t){
    nextGrid[xLoc][yLoc] = nextVal;
  }else{
    nextGrid[xLoc][yLoc] = grid[xLoc][yLoc];
  }
}

//seed fresh grid, pick new colors, hold disp for 1s
void initGrid(){
  randomizeGrid();
  h = random(65535);
  displayGrid();
  delay(1000);
}

// SETUP - RUNS ONCE AT PROGRAM START --------------------------------------
void setup(void) {
  Serial.begin(9600);
  pinMode(3, INPUT_PULLUP);
  
  //read floating pin for random seed
  randomSeed(analogRead(A4));

  // Initialize matrix...
  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status != PROTOMATTER_OK) {
    // DO NOT CONTINUE if matrix setup encountered an error.
    for(;;);
  }
  
  initGrid();
}

// LOOP - RUNS REPEATEDLY AFTER SETUP --------------------------------------
void loop(void) {
  if(digitalRead(3) == LOW){ //Matrix portal "DOWN" button re-seeds grid at any time
    initGrid();
  }

  cycleIterate(); //Do CA
  displayGrid(); //Refresh screen
  delay(50); //200fps cap
}