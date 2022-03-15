#include <cstdint>
#include <iostream>
#include <fstream>
#include <set>
#include "pin.H"

using std::cout;
using std::cerr;
using std::endl;
using std::hex;
using std::ofstream;
using std::string;

/******************** Logging infrastructure **********************/
KNOB<string> KnobLogFile(KNOB_MODE_WRITEONCE, "pintool",
    "d", "/tmp", "specify log output directory");
ofstream PinLog;
#define LOGBUFSIZE 8192
char PinLogBuf[LOGBUFSIZE];
int PinLogBufSize = 0;
typedef struct {
  char *rtn;
  ADDRINT ip;
  /* Markers */
  BOOL marker;
  const char *where; /* "start"/"end" */
  /* Non-markers: loads/stores */
  BOOL load;
  ADDRINT ptr;
  UINT32 size;
} access_t;
#define N_ACCESSES 30000000
access_t accesses[N_ACCESSES];
unsigned count_accesses = 0;
uint64_t icount = 0, dcount = 0;

void flush_accesses() {
  // cout << "Flushing accesses...." << endl;
  for(unsigned i = 0; i < count_accesses; i++) {
    char buf[100];
    int n;
    if(accesses[i].marker) {
      n = snprintf(&PinLogBuf[PinLogBufSize], sizeof(buf), "%012lx (%s): %s\n",
                  accesses[i].ip, 
                  accesses[i].rtn, 
                  accesses[i].where);
    } else {
      n = snprintf(&PinLogBuf[PinLogBufSize], sizeof(buf), "%012lx (%s): %012lx (%02d, %c)\n", 
                  accesses[i].ip, 
                  accesses[i].rtn, 
                  accesses[i].ptr, 
                  accesses[i].size, 
                  (accesses[i].load)? 'R': 'W');
    }
    PinLogBufSize += n;
    
    if(PinLogBufSize > 5 * LOGBUFSIZE / 6) {
      PinLog << PinLogBuf;
      PinLogBuf[0] = '\0';
      PinLogBufSize = 0;
    }
  }
  count_accesses = 0;
  // cout << "Done" << endl;
}

/******************** Tracking triggers ***********************/
int AccessTrackingOn = 0;
VOID SetTrigger(BOOL activate) {
  AccessTrackingOn = activate;
}

/******************** Runtime instrumentation *****************/
VOID DoAccessAccounting(char *rtn, ADDRINT ip, ADDRINT ptr, UINT32 size, BOOL load) {
  if(!AccessTrackingOn)
    return; 
  
  dcount++;
  accesses[count_accesses++] = {rtn, ip, false, NULL, load, ptr, size};

  if(count_accesses == N_ACCESSES)
    flush_accesses();
}

VOID PrintMarker(char *rtn, ADDRINT ip, const char *where) {
  accesses[count_accesses++] = {rtn, ip, true, where, false, 0, 0};

  if(count_accesses == N_ACCESSES)
    flush_accesses();
}

VOID CountInst(void) {
  if(!AccessTrackingOn)
    return; 

  icount++;
}

/******************** Instrumentation Functions *************/
VOID InstrumentAccess(INS ins, VOID *v) {
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountInst, IARG_END);

  RTN rtn = INS_Rtn(ins);
  string name_cpp = (RTN_Valid(rtn))? RTN_Name(rtn): "Anon";
  char *rtn_name = (char *) malloc(name_cpp.length() + 1);
  strcpy(rtn_name, name_cpp.c_str());

  BOOL isRead = INS_IsMemoryRead(ins);
  BOOL isWrit = INS_IsMemoryWrite(ins);

  if(INS_Opcode(ins) == XED_ICLASS_VERR) {
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)PrintMarker, IARG_PTR, rtn_name,
    IARG_INST_PTR, IARG_PTR, "repstart", IARG_END);
    return;
  } else if(INS_Opcode(ins) == XED_ICLASS_VERW) {
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)PrintMarker, IARG_PTR, rtn_name,
    IARG_INST_PTR, IARG_PTR, "repend", IARG_END);
    return;
  } else if(!(isRead || isWrit))
    return;

  if(isRead){
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)DoAccessAccounting, IARG_PTR, rtn_name, IARG_INST_PTR, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_BOOL, TRUE, IARG_END);
    if(INS_HasMemoryRead2(ins)) {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)DoAccessAccounting, IARG_PTR, rtn_name, IARG_INST_PTR, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_BOOL, TRUE, IARG_END);
    }
  }
  if(isWrit)
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)DoAccessAccounting, IARG_PTR, rtn_name, IARG_INST_PTR, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_BOOL, FALSE, IARG_END);
}

VOID ImageLoad(IMG img, VOID *v) {
  /* We will enable statistics accounting during the call to cache_get_wrapper */
  RTN cacheGetWrapperFn = RTN_FindByName(img, "cache_get_wrapper");
  if(RTN_Valid(cacheGetWrapperFn)) {
    RTN_Open(cacheGetWrapperFn);
    RTN_InsertCall(cacheGetWrapperFn, IPOINT_BEFORE, (AFUNPTR)SetTrigger,
                    IARG_BOOL, TRUE, IARG_END);
    RTN_InsertCall(cacheGetWrapperFn, IPOINT_AFTER, (AFUNPTR)SetTrigger,
                    IARG_BOOL, FALSE, IARG_END);
    RTN_Close(cacheGetWrapperFn);
  }

  /* We will simulate a compartment switch around cache_get */
  const char *rtn_name = "cache_get";
  RTN cacheGetFn = RTN_FindByName(img, rtn_name);
  if(RTN_Valid(cacheGetFn)) {
    RTN_Open(cacheGetFn);
    RTN_InsertCall(cacheGetFn, IPOINT_BEFORE, (AFUNPTR)PrintMarker, 
                                IARG_PTR, rtn_name,
                                IARG_INST_PTR, 
                                IARG_PTR, "switch_cache", 
                                IARG_END);
    RTN_InsertCall(cacheGetFn, IPOINT_AFTER, (AFUNPTR)PrintMarker, 
                                IARG_PTR, rtn_name,
                                IARG_INST_PTR, 
                                IARG_PTR, "switch_wrapper", 
                                IARG_END);
    RTN_Close(cacheGetFn);
  }
}

/******************** INIT/FINI *************************/
VOID Init() {
  std::string logfile = KnobLogFile.Value();
  // cout << "Logging to " << logfile << endl;
  PinLog.open(logfile.c_str());
  PinLogBufSize = 0;
}
INT32 Usage() {
  cerr << "This tool does stuff" << endl;
  return -1;
}

VOID Fini(int code, void *v) {
  flush_accesses();
  
  PinLog << PinLogBuf;
  PinLogBuf[0] = '\0';
  PinLogBufSize = 0;

  cerr << icount << " instructions " << endl;
  cerr << dcount << " accesses " << endl;
}
 
/******************** MAIN ****************************/
int main(int argc, char **argv) {
  PIN_InitSymbols();
  if(PIN_Init(argc, argv)) {
      return Usage();
  }
  Init();

  /* For finding and instrumenting triggers */
  IMG_AddInstrumentFunction(ImageLoad, NULL);
  /* For triggering on main() */
  // INS_AddInstrumentFunction(Triggers, NULL);

  /* Instrument loads for accounting */
  INS_AddInstrumentFunction(InstrumentAccess, NULL);

  PIN_AddFiniFunction(Fini, NULL);
  PIN_StartProgram();
}