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

uint32_t gHistory;

// 1. gShare data structure
uint32_t gshareMask;
uint32_t * gshareTable;

// 2. Tournament data structure

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


uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

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
  // update prediction table
  uint32_t pcLower = (pc&gshareMask);
  uint32_t gHistoryLower = (gHistory&gshareMask);
  uint32_t idx = (pcLower^gHistoryLower);
  uint32_t prediction = gshare_predict(pc);
  if(outcome == TAKEN) {
    if(gshareTable[idx]<3) {
      gshareTable[idx]++;
    }
  } else {
    if(gshareTable[idx]>0) {
      gshareTable[idx]--;
    }
  }

  // update history
  gHistory = ((gHistory<<1) | outcome);
  gHistory = (gHistory&gshareMask);

  return;
}

void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      train_gshare(pc, outcome);
      break;
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }
  return;
}
