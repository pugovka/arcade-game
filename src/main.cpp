#include <Arduino.h>
#include "Tlc5940.h"

const int FIRST_PLAYER = 6;
const int SECOND_PLAYER = 7;
const int START_BUTTON = 2;
const int TLC_PIN_COUNT = 16;
const int TLC_COUNT = 3;
const int BUTTON_TLC_ID = 3;
const int BUTTON_TLC_END_PIN = BUTTON_TLC_ID * TLC_PIN_COUNT;
const int BUTTON_TLC_START_PIN = BUTTON_TLC_END_PIN - TLC_PIN_COUNT;
const int totalRounds = 10;
enum GameState { IDLE, GAME, FINAL };

void setup() {
  pinMode(FIRST_PLAYER, INPUT_PULLUP);
  pinMode(SECOND_PLAYER, INPUT_PULLUP);
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  Tlc.init();
  randomSeed(analogRead(A0));
}

bool wasButtonPressed(int button)
{
  if (digitalRead(button) == 0) {
    delay(10);
    
    return digitalRead(button) == 0;
  }

  return false;
}

/**
 * Turns off all buttons
 */
void turnOffButtonLEDs()
{
  for (int i = BUTTON_TLC_START_PIN; i < BUTTON_TLC_END_PIN; i++) {
    Tlc.set(i, 0);
  }

  Tlc.update();
}

const int sevenSegmentDigits[] = {
  /**
   * Right to left
   */
  0b01111110,
  0b00001100,
  0b10110110,
  0b10011110,
  0b11001100,
  0b11011010,
  0b11111010,
  0b00001110,
  0b11111110,
  0b11011110
};

/**
 * Displays digit on seven segment
 */
void displaySevenSegmentDigit(const int digit, const int segmentNumber)
{
  const int SEGMENT_COUNT = 7;

  /**
   * Add shift to get next seven segment positions
   */
  int shifts[] = {0, 0, 7, 16, 23};
  int pos = shifts[segmentNumber];
  int buffer = sevenSegmentDigits[digit];

  for (int i = pos; i < pos + SEGMENT_COUNT; i++) {
      int nextBit = buffer % 2;
      Tlc.set(i, nextBit == 1 ? 0 : 4095);
      buffer = (buffer - nextBit) / 2;
  }
}

int getSecondDigit(const int score)
{
  return score % 10;
}

int getFirstDigit(const int score)
{
  return (score - getSecondDigit(score)) / 10;;
}

/**
 * Displays players score on seven segments
 */
void displayScore(int score1, int score2)
{
  displaySevenSegmentDigit(getFirstDigit(score2), 1);
  displaySevenSegmentDigit(getSecondDigit(score2), 2);
  displaySevenSegmentDigit(getFirstDigit(score1), 3);
  displaySevenSegmentDigit(getSecondDigit(score1), 4);
  Tlc.update();
}

int currentRound = 1;
bool isRoundInProgress = false;
int firstPlayerScore = 0;
int secondPlayerScore = 0;

void reset()
{
  currentRound = 1;
  isRoundInProgress = false;
  firstPlayerScore = 0;
  secondPlayerScore = 0;
  displayScore(0, 0);
}

void printScores()
{
  displayScore(firstPlayerScore, secondPlayerScore);
}

bool updateScore()
{
  bool isHit = false;

  if (wasButtonPressed(FIRST_PLAYER)) {
    isHit = true;
    firstPlayerScore++;
  } else if (wasButtonPressed(SECOND_PLAYER)) {
    isHit = true;
    secondPlayerScore++;
  }

  if (isHit) {
    delay(10);
    isRoundInProgress = false;
    currentRound++;
    printScores();
  }

  return isHit;
}

/**
 * Turns on button for every player
 */
int selectPlayerButtons(int prevEncodedButtons)
{
  const int PLAYER_BUTTON_COUNT = 8;
  int player1PrevButton = prevEncodedButtons / 100;
  int player2PrevButton = prevEncodedButtons % 100;
  int secondPlayerEndPin = BUTTON_TLC_ID * TLC_PIN_COUNT;
  int firstPlayerStartPin = secondPlayerEndPin - TLC_PIN_COUNT;
  int secondPlayerStartPin = firstPlayerStartPin + PLAYER_BUTTON_COUNT;

  int player1Button = random(firstPlayerStartPin, secondPlayerStartPin);
  int player2Button = random(secondPlayerStartPin, secondPlayerEndPin);

  while (player1PrevButton == player1Button || player2PrevButton == player2Button) {
    player1Button = random(firstPlayerStartPin, secondPlayerStartPin);
    player2Button = random(secondPlayerStartPin, secondPlayerEndPin);
  }

  turnOffButtonLEDs();
  Tlc.set(player1Button, 4095);
  Tlc.set(player2Button, 4095);
  Tlc.update();

  return player1Button * 100 + player2Button;
}

bool isSwitchOn = false;
int lastVariant = 0;
GameState currentState = GameState::IDLE;
unsigned long roundTimer = 0;
unsigned long idleTimer = 0;

void loop()
{
  if (wasButtonPressed(START_BUTTON) && !isSwitchOn) {
    currentState = GameState::GAME;
    isSwitchOn = true;
  } else if (!wasButtonPressed(START_BUTTON) && isSwitchOn) {
    currentState = GameState::IDLE;
    reset();
    turnOffButtonLEDs();
    isSwitchOn = false;
  }

  switch (currentState) {
    case GameState::IDLE:
      if (wasButtonPressed(START_BUTTON)) {
        currentState = GameState::GAME;
        roundTimer = millis();
        reset();
      }

      break;
    case GameState::GAME:
      if (currentRound < totalRounds) {
        if (millis() - roundTimer > 50000) {
          /**
           * Reset game if there was not any action for 50 seconds
           */
          currentState = GameState::IDLE;
          turnOffButtonLEDs();

          break;
        }

        if (isRoundInProgress) {
          if (updateScore()) {
            roundTimer = millis();
          }
        } else {
          roundTimer = millis();
          lastVariant = selectPlayerButtons(lastVariant);
          isRoundInProgress = true;
        }
      } else {
        currentState = GameState::FINAL;
        turnOffButtonLEDs();
        idleTimer = millis();
      }

      break;
    case GameState::FINAL:
      turnOffButtonLEDs();
      
      if (millis() - idleTimer > 10000) {
        currentState = GameState::IDLE;
      }
      
      break;
    default:
      break;
  }
}