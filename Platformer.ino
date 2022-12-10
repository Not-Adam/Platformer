#include <Arduino.h>
typedef uint8_t PROGMEM prog_uint8_t;

void(* resetFunc) (void) = 0; //declare reset function @ address 0


// Screen constants
const int WIDTH = 32;
const int HEIGHT = 32;

// Map Generation Constants
const int MAP_SIZE = 50;

const int MAX_PLATFORM_SIZE = 7;
const int MIN_PLATFORM_SIZE = 5;

const int MAX_JUMP = 3;
const int MIN_JUMP = 2;

const int MAX_FALL = 3;
const int MIN_FALL = -3;

const int ENEMY_SPAWN_CHANCE = 5;

int platforms[MAP_SIZE / MIN_PLATFORM_SIZE][4];

uint8_t state = 0; // 0: Menu, 1: Playing, 2: Lost, 3: P1 Won, 4: P2 Won

// Delay constants
const int GRAVITY_DELAY = 200;      // 200 ms
const int JUMP_DELAY = 1500;        // 1500 ms
const int ENEMY_TICK_DELAY = 500;   // 500 ms



// Display output buffer
uint8_t buf[32][32];

// Color information for each element type (Platform, Players, Enemies, etc)
const prog_uint8_t elements[7] = {
	B000, // Empty
	B001, // Platform
	B100, // P1
	B010, // P2
	B101, // Enemy
	B111, // Text
	B110, // Flag
};

void selectLine(byte line) {
	PORTC = line; // Setting 4 left-most bits to appropriate line number
}

/*
When pushing data for each row:
- Latch off
- For each pixel: Set upper and lower RGB pins and pulse clock pin
- Latch down
*/
void display() {
	int rowLower;
	for (int row = 0; row < HEIGHT / 2; row++) {
        rowLower = row + HEIGHT / 2;

		PORTB &= ~(1 << 2); // digitalWrite(LAT, LOW);

		for (int col = 0; col < WIDTH; col++) {
			int data = 0;
			data |= pgm_read_byte(&elements[buf[rowLower][col]]);
			data <<= 3;
			data |= pgm_read_byte(&elements[buf[row][col]]);
			data <<= 2;
			PORTD = data;

			PORTB |= (1 << 0); // digitalWrite(CLK, HIGH);
			PORTB &= ~(1 << 0); // digitalWrite(CLK, LOW);
		}

		PORTB |= (1 << 2); // digitalWrite(LAT, HIGH);
        selectLine(row);
	}
}




// Player positions
int player1X ;
float player1Y;
float player1VY;

int player2X;
float player2Y;
float player2VY;

int cameraX;




void setup() {
    DDRB |= B000111; // Setting CLK, OE, and LAT pins to output.
	DDRC |= B001111; // Setting 4 Address pins (A0 - A3) to output.
	DDRD |= B11111100; // Setting our 6 RGB pins (2 - 7) to output.

    // Setting up random seed
    randomSeed(analogRead(A7));

    // Map generation
    int startX = 0;
    int endX = (int) random(MIN_PLATFORM_SIZE, MAX_PLATFORM_SIZE);
    int platformY = HEIGHT / 2;

    int i = 0;
    while (endX <= MAP_SIZE) {
        platforms[i][0] = startX;
        platforms[i][1] = endX;
        platforms[i][2] = platformY;

        startX = endX + (int) random(MIN_JUMP, MAX_JUMP);
        endX = startX + (int) random(MIN_PLATFORM_SIZE, MAX_PLATFORM_SIZE);
        platformY += (int) random(MIN_FALL, MAX_FALL);
        while (platformY > HEIGHT - 5 || platformY < 2) {
            platformY += (int) random(MIN_FALL, MAX_FALL);
        }
        i++;
        if (random(ENEMY_SPAWN_CHANCE - 1) == 0) {
            platforms[i][3] = 1;
        }
    }
    platforms[i - 1][3] = 2;

    player1X = 0;
    player1Y = HEIGHT / 2 - 1;
    player1VY = 0;

    player2X = 1;
    player2Y = HEIGHT / 2 - 1;
    player2VY = 0;

    cameraX = (player1X + player2X) / 2 - WIDTH / 2;
}

long enemyCooldown;
long gravityCooldown;
long jumpCooldownP1;
long jumpCooldownP2;
long dotCooldown;
long inputDelay;

long dotTick;
long enemyTick;

void loop() {
    memset(buf, 0, sizeof(buf)); // Clearing the display
    switch (state) {
        case 0: // Menu
            if (millis() > dotCooldown + 1000) {
                dotTick++;
                dotCooldown = millis();
            }

            for (int i = 0; i < dotTick % 3 + 1; i++) {
                buf[4][4 + i * 2] = 5;
            }

            if (map(analogRead(A4), 256, 752, 0, 1) || map(analogRead(A5), 256, 752, 0, 1)) {
                state = 1; // Playing
            }

            break;

        case 1: // Playing
            buf[(int) player1Y][player1X - cameraX] = 2; // Player 1
            buf[(int) player2Y][player2X - cameraX] = 3; // Player 2

            int startX, endX, boundaryEnd, boundaryStart, size, pos, index, y, x;
            for (auto platform : platforms) {
                startX = platform[0] - cameraX;
                boundaryStart = startX < 0 ? 1 : startX;
                
                endX = platform[1] - cameraX;
                boundaryEnd = endX < WIDTH ? endX : WIDTH;

                size = platform[1] - platform[0];
        
                if (endX < 0) { // If platform end is before viewport start.
                    continue;
                }
                if (startX > WIDTH) { // If platform start point is ahead viewport
                    continue;
                }

                for (int i = boundaryStart; i < boundaryEnd; i++) {
                    buf[platform[2]][i] = 1;
                }

                if (platform[3] == 1) { // Has enemy
                    pos = boundaryStart + enemyTick % size;

                    if (pos > 0 && pos < WIDTH) {
                        if (buf[platform[2] - 1][pos] == 2 || buf[platform[2] - 1][pos] == 3) { // Checking if new enemy position will be over a player, if so, kill
                            state += 1; // Death
                            break;
                        }
                        buf[platform[2] - 1][pos] = 4;
                    }
                } else if (platform[3] == 2) { // Has Flag
                    pos = boundaryStart + size / 2;
                    if (pos > 0 && pos < WIDTH) {
                        y = platform[2] - 1;
                        x = boundaryStart + size / 2;
                        // Draw flag
                        buf[y][x] = 6;
                        buf[y - 1][x] = 6;
                        buf[y - 2][x] = 6;
                        buf[y - 3][x] = 6;
                        buf[y - 4][x] = 6;
                        buf[y - 2][x - 1] = 6;
                        buf[y - 2][x - 2] = 6;
                        buf[y - 3][x - 1] = 6;
                    }
                }
            }

            // Player
            // --------------------------------
            if (player1Y <= 0 || player1Y >= HEIGHT // Y-Position check
            || player1X - cameraX < 0 || player1X - cameraX > WIDTH // Checking if player is within screen
            || player2Y <= 0 || player2Y >= HEIGHT // Y-Position check
            || player2X - cameraX < 0 || player2X - cameraX > WIDTH) { // Checking if player is within screen
                state = 2; // Death
            }

            if (millis() - enemyCooldown > ENEMY_TICK_DELAY) { // Every 500 ms, advance all enemies
                enemyTick++;
                enemyCooldown = millis();
            }

            // Checking inputs and adding gravity
            if (millis() - gravityCooldown > GRAVITY_DELAY) {
                player1X += map(analogRead(A4), 256, 752, -1, 1);
                player2X += map(analogRead(A5), 0, 600, -1, 1);
                gravityCooldown = millis();
                
                player1VY += 0.01;
                player2VY += 0.01;

                // Updating camera
                cameraX = (player1X + player2X) / 2 - WIDTH / 2;
            }

            // Checking if player is on platform
            if (buf[(int) player1Y + 1][player1X - cameraX] == 1 && millis() > jumpCooldownP1 + 100) {
                player1VY = 0;
                player1Y = (int) (player1Y);
            }
            if (buf[(int) player2Y + 1][player2X - cameraX] == 1 && millis() > jumpCooldownP2 + 100) {
                player2VY = 0;
                player2Y = (int) (player2Y);
            }


            // Checking if player is on flag
            if (buf[(int) player1Y][player1X - cameraX] == 6) {
                state = 3; // P1 Win
            }
            if (buf[(int) player2Y][player2X - cameraX] == 6) {
                state = 4; // P2 Win
            }

            player1Y += player1VY;
            player2Y += player2VY;
            
            // Cooldown till next jump is available
            if (!digitalRead(13) && millis() > jumpCooldownP1 + JUMP_DELAY) {
                player1VY = -0.025;
                jumpCooldownP1 = millis();
            }
            if (!digitalRead(12) && millis() > jumpCooldownP2 + JUMP_DELAY) {
                player2VY = -0.025;
                jumpCooldownP2 = millis();
            }
            // --------------------------------

            // Enemy
            // --------------------------------
            // Checking if player 1 is above enemy, if so, kill
            if (buf[(int) player1Y + 1][player1X - cameraX] == 4) {
                for (auto platform : platforms) {
                    startX = platform[0] - cameraX;
                    endX = platform[1] - cameraX;
                    if (player1X - cameraX >= startX && player1X - cameraX <= endX) {
                        platform[3] = 0;
                        break;
                    }
                }
            }
            // Checking if player 2 is above enemy, if so, kill
            if (buf[(int) player2Y + 1][player2X - cameraX] == 4) {
                for (auto platform : platforms) {
                    startX = platform[0] - cameraX;
                    endX = platform[1] - cameraX;
                    if (player2X - cameraX >= startX && player2X - cameraX <= endX) {
                        platform[3] = 0;
                        break;
                    }
                }
            }
            // --------------------------------
            break;

        case 2: // Lost
            // Y
            buf[5][4] = 1;
            buf[6][5] = 1;
            buf[7][6] = 1;
            buf[8][7] = 1;
            buf[7][8] = 1;
            buf[6][9] = 1;
            buf[5][10] = 1;
            buf[9][7] = 1;
            buf[10][7] = 1;
            buf[11][7] = 1;
            buf[12][7] = 1;

            // O
            buf[5][14] = 1;
            buf[5][15] = 1;
            buf[5][16] = 1;
            buf[5][17] = 1;

            buf[6][13] = 1;
            buf[6][18] = 1;
            buf[7][13] = 1;
            buf[7][18] = 1;
            buf[8][13] = 1;
            buf[8][18] = 1;
            buf[9][13] = 1;
            buf[9][18] = 1;
            buf[10][13] = 1;
            buf[10][18] = 1;
            buf[11][13] = 1;
            buf[11][18] = 1;

            buf[12][14] = 1;
            buf[12][15] = 1;
            buf[12][16] = 1;
            buf[12][17] = 1;

            // U
            buf[5][21] = 1;
            buf[5][27] = 1;
            buf[6][21] = 1;
            buf[6][27] = 1;
            buf[7][21] = 1;
            buf[7][27] = 1;
            buf[8][21] = 1;
            buf[8][27] = 1;
            buf[9][21] = 1;
            buf[9][27] = 1;
            buf[10][21] = 1;
            buf[10][27] = 1;
            buf[11][21] = 1;
            buf[11][27] = 1;

            buf[12][22] = 1;
            buf[12][23] = 1;
            buf[12][24] = 1;
            buf[12][25] = 1;
            buf[12][26] = 1;

            // L
            buf[26][3] = 1;
            buf[25][3] = 1;
            buf[24][3] = 1;
            buf[23][3] = 1;
            buf[22][3] = 1;
            buf[21][3] = 1;
            buf[20][3] = 1;
            buf[19][3] = 1;

            buf[26][3] = 1;
            buf[26][4] = 1;
            buf[26][5] = 1;
            buf[26][6] = 1;
            buf[26][7] = 1;

            // O
            buf[26][11] = 1;
            buf[26][12] = 1;
            buf[26][13] = 1;
            buf[26][14] = 1;

            buf[25][10] = 1;
            buf[25][15] = 1;
            buf[24][10] = 1;
            buf[24][15] = 1;
            buf[23][10] = 1;
            buf[23][15] = 1;
            buf[22][10] = 1;
            buf[22][15] = 1;
            buf[21][10] = 1;
            buf[21][15] = 1;
            buf[20][10] = 1;
            buf[20][15] = 1;

            buf[19][11] = 1;
            buf[19][12] = 1;
            buf[19][13] = 1;
            buf[19][14] = 1;

            // S
            buf[19][18] = 1;
            buf[19][19] = 1;
            buf[19][20] = 1;
            buf[19][21] = 1;
            
            buf[22][17] = 1;
            buf[21][17] = 1;
            buf[20][17] = 1;

            buf[23][18] = 1;
            buf[23][19] = 1;
            buf[23][20] = 1;
            buf[23][21] = 1;

            buf[24][22] = 1;
            buf[25][22] = 1;

            buf[26][18] = 1;
            buf[26][19] = 1;
            buf[26][20] = 1;
            buf[26][21] = 1;

            // T
            buf[19][24] = 1;
            buf[19][25] = 1;
            buf[19][26] = 1;
            buf[19][27] = 1;
            buf[19][28] = 1;

            buf[20][26] = 1;
            buf[21][26] = 1;
            buf[22][26] = 1;
            buf[23][26] = 1;
            buf[24][26] = 1;
            buf[25][26] = 1;
            buf[26][26] = 1;
            
            if (map(analogRead(A4), 256, 752, 0, 1) || map(analogRead(A5), 256, 752, 0, 1)) {
                resetFunc(); // Back to Menu
            }
            break;
        
        case 3: // Player 1 won
            buf[15][15] = 2;
            buf[16][16] = 2;
            buf[15][16] = 2;
            buf[16][15] = 2;

            if (map(analogRead(A4), 256, 752, 0, 1) || map(analogRead(A5), 256, 752, 0, 1)) {
                resetFunc(); // Back to Menu
            }
            break;

        case 4: // Player 2 won
            buf[15][15] = 3;
            buf[16][16] = 3;
            buf[15][16] = 3;
            buf[16][15] = 3;

            if (map(analogRead(A4), 256, 752, 0, 1) || map(analogRead(A5), 256, 752, 0, 1)) {
                resetFunc(); // Back to Menu
            }
            break;
    }




    display();
}