//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

// public data structure
uint32_t gHistory;

// 1. gShare data structure
uint32_t gshareMask;
uint32_t * gshareTable;

// 2. Tournament data structure
uint32_t tourGlobalMask;
uint32_t tourLocalMask;
uint32_t tourPcMask;

uint32_t * tourGlobalPht;
uint32_t * tourLocalPht;
uint32_t * tourLocalHt;
uint32_t * tourChoicePht;

/*
choosing predictor:
  1: strongly choose local
  2: weakly choose local predictor
  3: weakly choose global predictor
  4: strongly choose global predictor
*/


// 3. custom data structure


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// some helper functions

uint32_t get_mask(int bits) {
  uint32_t mask = 0;
  for(int i=0;i<bits;i++) {
    mask = mask | (1<<i);
  }
  return mask;
}

void init_gshare() {
  gHistory = 0;

  int tableSize = (1<<ghistoryBits);
  gshareTable = (uint32_t *)malloc(sizeof(uint32_t)*tableSize);
  for(int i=0;i<tableSize;i++) {
    gshareTable[i] = WN;
  }

  gshareMask = get_mask(ghistoryBits);
  return;
}

void init_tournament() {
  gHistory = 0;
  tourLocalMask = get_mask(lhistoryBits);
  tourGlobalMask = get_mask(ghistoryBits);
  tourPcMask = get_mask(pcIndexBits);

  // init local predictor
  int localPhtSize = (1<<lhistoryBits);
  tourLocalPht = (uint32_t *)malloc(localPhtSize*sizeof(uint32_t));
  for(int i=0;i<localPhtSize;i++) {
    tourLocalPht[i] = WN;
  }

  // init global predictor
  int globalPhtSize = (1<<ghistoryBits);
  tourGlobalPht = (uint32_t *)malloc(globalPhtSize*sizeof(uint32_t));
  for(int i=0;i<globalPhtSize;i++) {
    tourGlobalPht[i] = WN;
  }

  // init choice predictor
  int choicePhtSize = (1<<ghistoryBits);
  tourChoicePht = (uint32_t *)malloc(choicePhtSize*sizeof(uint32_t));
  for(int i=0;i<choicePhtSize;i++) {
    tourChoicePht[i] = 2;
  }//initially weakly choose global predictor

  //init local history table
  int localHtSize = (1<<pcIndexBits);
  tourLocalHt = (uint32_t *)malloc(localHtSize*sizeof(uint32_t));
  for(int i=0;i<localHtSize;i++) {
    tourLocalHt[i] = 0;
  }

  return;
}

// Initialize the predictor
//
void
init_predictor()
{
  switch (bpType) {
    case STATIC:
      return;
    case GSHARE:
      init_gshare();
      break;
    case TOURNAMENT:
      init_tournament();
      break;
    case CUSTOM:
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//

// some prediction helper functions

uint8_t gshare_predict(uint32_t pc) {
  uint32_t pcLower = (pc&gshareMask);
  uint32_t gHistoryLower = (gHistory&gshareMask);
  uint32_t idx = (pcLower^gHistoryLower);
  uint32_t prediction = gshareTable[idx];
  if(prediction>1) {
    return TAKEN;
  } else {
    return NOTTAKEN;
  }
}

uint8_t tournament_predict(uint32_t pc) {
  uint32_t globalIdx = (gHistory&tourGlobalMask);
  uint32_t choice = tourChoicePht[globalIdx];
  
  // choose the local predictor
  if(choice<2) {
    uint32_t pcLower = (pc&tourPcMask);
    uint32_t localIdx = (tourLocalHt[pcLower]&tourLocalMask);
    uint32_t localPredict = tourLocalPht[localIdx];
    if(localPredict<2) {
      return NOTTAKEN;
    } else {
      return TAKEN;
    }
  }

  // choose the global predictor 
  else {
    uint32_t globalPredict = tourGlobalPht[globalIdx];
    if(globalPredict<2) {
      return NOTTAKEN;
    } else {
      return TAKEN;
    }
  }
}


uint8_t
make_prediction(uint32_t pc)
{
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_predict(pc);
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

// some helper functions
void train_gshare(uint32_t pc, uint8_t outcome) {
  //1. update prediction table
  uint32_t pcLower = (pc&gshareMask);
  uint32_t gHistoryLower = (gHistory&gshareMask);
  uint32_t idx = (pcLower^gHistoryLower);
  // uint32_t prediction = gshare_predict(pc);
  if(outcome == TAKEN) {
    if(gshareTable[idx]<3) {
      gshareTable[idx]++;
    }
  } else {
    if(gshareTable[idx]>0) {
      gshareTable[idx]--;
    }
  }

  //2. update history
  gHistory = ((gHistory<<1) | outcome);
  gHistory = (gHistory&gshareMask);

  return;
}

void train_tourament(uint32_t pc, uint8_t outcome) {
  uint32_t pcLower = (pc&tourPcMask);
  uint32_t localIdx = (tourLocalHt[pcLower]&tourLocalMask);
  uint32_t globalIdx = (gHistory&tourGlobalMask);

  // 1. prediction with local predictor and global predictor
  uint32_t localPredict = tourLocalPht[localIdx];
  if(localPredict<2) {
    localPredict = NOTTAKEN;
  } else {
    localPredict = TAKEN;
  }

  uint32_t globalPredict = tourGlobalPht[globalIdx];
  if(globalPredict<2) {
    globalPredict = NOTTAKEN;
  } else {
    globalPredict = TAKEN;
  }

  uint32_t choice = tourChoicePht[globalIdx];

  // 2. update choice table based on the outcome
  if(outcome==TAKEN) {
    if(localPredict==NOTTAKEN && globalPredict==TAKEN) {
      if(choice<3) {
        tourChoicePht[globalIdx]++;
      }
    } else if(localPredict==TAKEN && globalPredict==NOTTAKEN) {
      if(choice>0) {
        tourChoicePht[globalIdx]--;
      }
    }
  } else {
    if(localPredict==NOTTAKEN && globalPredict==TAKEN) {
      if(choice>0) {
        tourChoicePht[globalIdx]--;
      }
    } else if(localPredict==TAKEN && globalPredict==NOTTAKEN) {
      if(choice<3) {
        tourChoicePht[globalIdx]++;
      }
    }
  }

  // 3. update local predictor and global predictor
  if(outcome==TAKEN) {
    if(tourLocalPht[localIdx]<3) {
      tourLocalPht[localIdx]++;
    }

    if(tourGlobalPht[globalIdx]<3) {
      tourGlobalPht[globalIdx]++;
    }
  } else {
    if(tourLocalPht[localIdx]>0) {
      tourLocalPht[localIdx]--;
    }

    if(tourGlobalPht[globalIdx]>0) {
      tourGlobalPht[globalIdx]--;
    }
  }

  // 4. update gHistory and local history
  gHistory = (gHistory<<1) | outcome;
  gHistory = (gHistory&tourGlobalMask);

  tourLocalHt[pcLower] = (tourLocalHt[pcLower]<<1) | outcome;
  tourLocalHt[pcLower] = (tourLocalHt[pcLower]&tourLocalMask);

  return;
}

void
train_predictor(uint32_t pc, uint8_t outcome)
{
  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      train_gshare(pc, outcome);
      break;
    case TOURNAMENT:
      train_tourament(pc, outcome);
      break;
    case CUSTOM:
    default:
      break;
  }
  return;
}
