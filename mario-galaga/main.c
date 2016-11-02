#include "myLib.h"
#include "font.h"
#include <stdlib.h>
#include <stdio.h>
#include "ship.h"
#include "pawn.h"
#include "trooper.h"
#include "stalkerImage.h"
#include "homescreen.h"
#include "gameover.h"
#include "goodjob.h"

Stalker stalker;
Shooter shooter;
Bullet bullets[MAX];
Alien aliens[24];
int canShoot = 0;
int counter;
int right = 0;
int left = 1;
int goDown = 0;
int stalkercount = 10;
int win = 0;
int alienshit = 0;
int numaliens = 24;
int prevlives = 3;
int prevaliens = 24;
int bowserdead = 1;

enum GBAState {
	START,
	START_NODRAW,
	GAME,
	END_GAME,
	END_GAME_NODRAW,
};


void drawImage3(int r, int c, int width, int height, const u16* image) {
	for(int i = 0; i < height; i++) {
		DMA[DMA_CHANNEL_3].src = &image[OFFSET(i, 0, width)];
		DMA[DMA_CHANNEL_3].dst = &videoBuffer[OFFSET(r++, c, WIDTH)];
		DMA[DMA_CHANNEL_3].cnt = width|DMA_ON;
	}
}

void drawChar(int row, int col, char ch, u16 color)
{
    int r, c;
    for(r=0; r<8; r++)
    {
        for(c=0; c<6; c++)
        {
            if(fontdata_6x8[OFFSET(r, c, 6) + ch*48])
            {
                setPixel(row+r, col+c, color);
            }
        }
    }
}

void drawString(int row, int col, char *str, u16 color)
{
    while(*str)
    {
        drawChar(row, col, *str++, color);
        col+= 6;
    }
}

void drawShooter(int row, int col, int height, int width, volatile u16 color) {
    	int r;
    	for(r=0; r<height; r++) {
       		DMA[3].src = &color;
       		DMA[3].dst = &videoBuffer[OFFSET(row+r, col, 240)];
	       	DMA[3].cnt = width | DMA_DESTINATION_INCREMENT | DMA_SOURCE_FIXED | DMA_ON;     
  	}
}

void moveShooter(int row, int col, int height, int width, int rdel, int cdel) {
	drawShooter(row, col, height, width, BLACK);
	shooter.row = row+rdel;
	shooter.col = col+cdel;
	drawImage3(row+rdel, col+cdel, 20, 30, ship);
}

void drawBullet(Bullet bullet, volatile u16 color) {
	for(int i=0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			setPixel(bullet.row+i, bullet.col+j, color);
		}
	}
}

void createBullet(int row, int col, int height, volatile u16 color) {
	int replace = -1;
	for(int i = 0; i < MAX; i++) {
		if (bullets[i].onScreen == 0) {
			replace = i;
			break;
		}
	}

	if (replace == -1) {
		return;
	}

	Bullet bullet;
	bullet.row = row;
	bullet.col = col;
	bullet.height = height;
	bullet.color = color;
	bullet.onScreen = 1;
	bullets[replace] = bullet;
	drawBullet(bullets[replace], RED);
}

void moveBullet() {
	for(int i = 0; i < MAX; i++) {
		Bullet* currentBullet = bullets + i;
		if (currentBullet->onScreen) {
			drawBullet(*currentBullet, BLACK);
			(*currentBullet).row = (*currentBullet).row - 3;
			drawBullet(*currentBullet, RED); 
			if((*currentBullet).row <= 0) {
				(*currentBullet).onScreen = 0; 
				drawBullet(*currentBullet, BLACK);
			}
		}
	}
}

void bulletCollision() {
	for(int i = 0; i < MAX; i++) {
		Bullet* bullet = bullets + i;
		for(int j = 0; j < 24; j++) {
			Alien* alien = aliens + j;
			int left = (*alien).col;
			int right = left + (*alien).width;
			int bottom = (*alien).row + (*alien).height;
			int top = alien->row;
			if(left <= (*bullet).col && (*bullet).col <= right && (*bullet).row < bottom 						&& bullet->row > top && (*alien).alive && bullet->onScreen) {
				if((*alien).health==1) {
					drawShooter((*alien).row, (*alien).col, (*alien).height, 							(*alien).width, BLACK);
					drawBullet(*bullet, BLACK);
					(*bullet).onScreen = 0;
					(*alien).alive = 0;
					numaliens--;
				} else {
					(*alien).health--;
					drawBullet(*bullet, BLACK);
					(*bullet).onScreen = 0;
				}				
			}
		}
	}
}

void bulletHitStalker() {
	for(int i = 0; i < MAX; i++) {
		Bullet* bullet = bullets + i;
		Stalker* sPoint = &stalker;
		int left = sPoint->col;
		int right = sPoint->col + sPoint->width;
		int bottom = sPoint->row + sPoint->height;
		int top = sPoint->row;
		if(bullet->col >= left && bullet->col <= right && bullet->row >= top && bullet->row 				<= bottom && sPoint->alive && bullet->onScreen) {
			if(sPoint->health <= 1) {
				drawShooter(sPoint->row, sPoint->col, sPoint->height, sPoint->width, 						BLACK);
				drawBullet(*bullet, BLACK);
				bullet->onScreen = 0;
				sPoint->alive = 0;
			} else {
				sPoint->health--;
				drawBullet(*bullet, BLACK);
				bullet->onScreen = 0;
			} 
		}
 
	}
}

void initShip() {
	shooter.row = HEIGHT - 40;
	shooter.col = WIDTH / 2;
	shooter.height = 30;
	shooter.width = 20;
	shooter.rdel = 2;
	shooter.cdel = 1;
	shooter.lives = 3;
	drawImage3(shooter.row, shooter.col, 20, 30, ship);
}

void initStalker() {
	stalker.row = 0;
	stalker.col = WIDTH/2-15;
	stalker.height = 20;
	stalker.width = 20;
	stalker.health = 12;
	stalker.alive = 1;
	drawImage3(stalker.row, stalker.col, stalker.width, stalker.height, stalkerImage);
}

void moveStalker() {
	if(stalker.alive) {
		if(stalker.row + stalker.height >= 240) {
			initStalker();
			shooter.lives--;
		}
		if(stalker.col + stalker.width/2 < shooter.col + shooter.width/2) {
			drawShooter(stalker.row, stalker.col, 25, 20, BLACK);
			stalker.col++;
			stalker.row++;
			drawImage3(stalker.row, stalker.col, 20, 25, stalkerImage);
		} else if(stalker.col + stalker.width/2 > shooter.col + shooter.width/2) {
			drawShooter(stalker.row, stalker.col, 25, 20, BLACK);
			stalker.col--;
			stalker.row++;
			drawImage3(stalker.row, stalker.col, 20, 25, stalkerImage);
		} else {
			drawShooter(stalker.row, stalker.col, 25, 20, BLACK);
			stalker.row++;
			drawImage3(stalker.row, stalker.col, 20, 25, stalkerImage);
		}
	}
}

void initAliens() {
	for(int i = 0; i < 8; i++) {
		aliens[i].row = (i/8)*20+20;
		aliens[i].col = (i%8)*20+20;
		aliens[i].height = 10;
		aliens[i].width = 10;
		aliens[i].health = 8;
		aliens[i].alive = 1;
		aliens[i].pawn = 0;
		drawImage3(aliens[i].row, aliens[i].col, aliens[i].height, aliens[i].width,
			trooper); 
	}
	for(int i = 8; i < 24; i++) {
		aliens[i].row = (i/8)*20+20;
		aliens[i].col = (i%8)*20+20;
		aliens[i].height = 10;
		aliens[i].alive = 20;
		aliens[i].width = 10;
		aliens[i].health = 1;
		aliens[i].pawn = 1;
		drawImage3(aliens[i].row, aliens[i].col, aliens[i].height, aliens[i].width,
			pawn); 
	}
}

void moveAliens() {
	if (aliens[7].col + aliens[7].width > 220) {
		right = 1;
		left = 0;
		goDown = 1;
	}
	if (aliens[0].col + aliens[0].width < 20) {
		left = 1;
		right = 0;
		goDown = 1;
	}	
	for (int i = 0; i < 24; i++) {
		Alien *alien = aliens + i;
		if(aliens->row >= 140) {
			shooter.lives = 0;
		}
		if(left) {
			if(alien->alive) {
				drawShooter(alien->row, alien->col, alien->height, alien->width, BLACK);
			}
			if(goDown) {
				alien->row += 2;
			}
			alien->col = alien->col+2;
			if(alien->alive) {
				if(alien->pawn) {
					drawImage3(alien->row, alien->col, alien->height, alien->width, 
					pawn);
				} else if(!alien->pawn) {
					drawImage3(alien->row, alien->col, alien->height, alien->width, 
					trooper);
				}
			}
		} else if(right) {
			if(alien->alive) {
				drawShooter(alien->row, alien->col, alien->height, alien->width, BLACK);
			}
			if(goDown) {
				alien->row+=2;
			}
			alien->col = alien->col-2;
			if(alien->alive) {
				if(alien->pawn) {
					drawImage3(alien->row, alien->col, alien->height, alien->width, 
					pawn);
				} else if(!alien->pawn) {
					drawImage3(alien->row, alien->col, alien->height, alien->width, 
					trooper);
				}
			}
		}
	}
	goDown = 0;
}

void aliensHit() {
	for(int i = 0; i < 24; i++) {
		Alien* alien = aliens + i;
		if(alien->alive) {
			if((alien->row + alien->height) >= 240) {
				alienshit = 1;
			}
		}
	}		
}

void loseLife() {
	Shooter* pPoint = &shooter;
	Stalker* sPoint = &stalker;
	int stalkleft = sPoint->col;
	int stalkright = sPoint->col + sPoint->width;
	int stalkbottom = sPoint->row + sPoint->height;
	int stalktop = sPoint->row;
	int shootcol = pPoint->col;
	int shootrow = pPoint->row;
	if(shootrow <= stalkbottom && shootrow+pPoint->height >= stalktop && shootcol <= stalkright 		&& shootcol + pPoint->width >= stalkleft) {
		pPoint->lives--;
		drawShooter(stalker.row, stalker.col, 25, 20, BLACK);
		initStalker();
	}	

	for(int i = 0; i < 24; i++) {
		Alien* alien = aliens + i;
		int aleft = alien->col;
		int aright = alien->col + alien->width;
		int abottom = alien->row + alien->height;
		int atop = alien->row;
		if(shootrow <= abottom && shootrow+pPoint->height >= atop && shootcol <= 				aright && shootcol + pPoint->width >= aleft && alien-> alive) {
			pPoint->lives--;
		}			
	}
}

void initWords() {
	char str[15];
	sprintf(str, "%d", shooter.lives);
	drawString(150,10,"Lives: ",RED);
	drawString(150,48,str,RED);
	prevlives = shooter.lives;

	char str2[15];
	sprintf(str2, "%d", numaliens);
	drawString(150,160,"Minions: ", RED);
	drawString(150,210,str2,RED);
	prevaliens = numaliens;
	
	char str1[15];
	sprintf(str1, "%d", stalker.alive);
	drawString(150,80,"Bowser: ", RED);
	drawString(150,125, str1, RED);
}

void livesCounter() {
	if(prevlives != shooter.lives) {
		drawShooter(150,10,10,50,BLACK);
		char str[15];
		sprintf(str, "%d", shooter.lives);
		drawString(150,10,"Lives: ",RED);
		drawString(150,48,str,RED);
		prevlives = shooter.lives;
	}

	if(!stalker.alive && bowserdead) {
		drawShooter(150,80,140,50,BLACK);
		char str1[15];
		sprintf(str1, "%d", stalker.alive);
		drawString(150,80,"Bowser: ", RED);
		drawString(150,125, str1, RED);
		bowserdead--;
	}

	if(prevaliens != numaliens) {
		drawShooter(150,160,10,90,BLACK);
		char str2[15];
		sprintf(str2, "%d", numaliens);
		drawString(150,160,"Minions: ", RED);
		drawString(150,210,str2,RED);
		prevaliens = numaliens;
	}
	
	
}	

void gameWon() {
	int dead = 0;
	if(!(stalker.alive)) {
		for(int i = 0; i < 24; i++) {
			Alien* alien = aliens + i;
			if(!(alien->alive)) {
				dead = 1;
			} else {
				dead = 0;
				break;
			}
		}			
	}
	if(dead) {
		win = 1;
	} else {
		win = 0;
	}

}


void shipMove() {
	if (KEY_DOWN_NOW(BUTTON_LEFT)) {	
		if (shooter.col > 0) {
			moveShooter(shooter.row, shooter.col, shooter.height, 					shooter.width, 0, -shooter.cdel);
		}
	}

	if (KEY_DOWN_NOW(BUTTON_RIGHT)) {
		if (shooter.col < 218) {
			moveShooter(shooter.row, shooter.col, shooter.height, 					shooter.width, 0, shooter.cdel);
		}
	}
}

void shipShoot() {
	if (KEY_DOWN_NOW(BUTTON_A) && counter <= 0) {
		canShoot = 1;
		counter = 5;
	}
	if (canShoot) {
		createBullet(shooter.row-3, shooter.col + shooter.width/2, 2, RED);
	}
	canShoot = 0;
	counter--;
}

void setPixel(int r, int c, unsigned short color) {
	videoBuffer[OFFSET(r, c, 240)] = color;
}

void resetBullets() {
	for(int i = 0; i < MAX; i++) {
		bullets[i].onScreen = 0;
	}
}

void reset() {
	drawShooter(0,0,160,240,BLACK);
	numaliens = 24;
	prevaliens = 24;
	prevlives = 3;
	initStalker();
	initAliens();
	initShip();
	initWords();
	resetBullets();
}

void waitForVblank() {
	while(SCANLINECOUNTER > 160);
	while(SCANLINECOUNTER < 160);
}

int main() {
	REG_DISPCNT = BG2_EN | MODE_3;
	enum GBAState state = START;
	while(1) {
		switch(state) {
			case START:
				drawImage3(0,0,240,160,homescreen);
				drawString(100,85,"MARIO EDITION!",RED);
				drawString(110,65,"Press START to start",RED);
				state = START_NODRAW;
				break;

			case START_NODRAW:
				while(!KEY_DOWN_NOW(BUTTON_START)) {
					state = GAME;
				}

			case GAME:
				waitForVblank();
				reset();
				while(1){
					if(KEY_DOWN_NOW(BUTTON_SELECT)) {
						reset();
						state = START;
						break;
					}
					shipMove();
					shipShoot();	
					moveBullet();
					moveStalker();
					moveAliens();
					bulletCollision();
					bulletHitStalker();
					gameWon();
					loseLife();
					livesCounter();
					aliensHit();
					if(shooter.lives == 0 || alienshit || win) {
						state = END_GAME;
						break;
					}
				}
				break;
		
			case END_GAME:
				if(!win) {
					drawImage3(0,0,240,160,gameover);
					drawString(100,70,"Press START to retry!", RED);
				} else {
					drawImage3(0,0,240,160,goodjob);
					drawString(60,30,"Good job!", RED);
					drawString(80,30,"Press Start to play again!", RED);
				}
				state = END_GAME_NODRAW;
				break;

			case END_GAME_NODRAW:
				while(!KEY_DOWN_NOW(BUTTON_START)) {
					state = START_NODRAW;
				}
		}		
	}
}


