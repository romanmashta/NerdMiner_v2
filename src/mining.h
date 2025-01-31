#pragma once

// Mining
#define MAX_NONCE_STEP  5000000U
#define MAX_NONCE       25000000U
#define TARGET_NONCE    471136297U
#define DEFAULT_DIFFICULTY  1e-4
#define KEEPALIVE_TIME_ms       30000
#define POOLINACTIVITY_TIME_ms  60000

#define TARGET_BUFFER_SIZE 64

#define FRAMES_PER_SECOND 14
#define DELAY 1000/FRAMES_PER_SECOND
#define REDRAW_EVERY FRAMES_PER_SECOND

#define SAVESTAT_TIME (5 * 60) //Save stats every 5 min seconds

void runMonitor(void *name);
void runStratumWorker(void *name);
void runMiner(void *name);
String printLocalTime(void);

typedef struct{
  uint8_t bytearray_target[32];
  uint8_t bytearray_pooltarget[32];
  uint8_t merkle_result[32];
  uint8_t bytearray_blockheader[80];
  uint8_t bytearray_blockheader2[80];
  double poolDifficulty;
  bool inRun;
  bool newJob;
  bool newJob2;
}miner_data;
