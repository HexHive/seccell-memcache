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
  ADDRINT ptr;
  UINT32 size;
  BOOL load;
} access_t;
#define N_ACCESSES 50000000
access_t accesses[N_ACCESSES];
unsigned count_accesses = 0;

void flush_accesses() {
  // cout << "Flushing accesses...." << endl;
  for(unsigned i = 0; i < count_accesses; i++) {
    char buf[100];
    int n = snprintf(&PinLogBuf[PinLogBufSize], sizeof(buf), "%012lx (%s): %012lx (%02d, %c)\n", 
                    accesses[i].ip, 
                    accesses[i].rtn, 
                    accesses[i].ptr, 
                    accesses[i].size, 
                    (accesses[i].load)? 'R': 'W');

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
  
  accesses[count_accesses++] = {rtn, ip, ptr, size, load};

  if(count_accesses == N_ACCESSES)
    flush_accesses();
}


/******************** Instrumentation Functions *************/
VOID InstrumentAccess(INS ins, VOID *v) {
  BOOL isRead = INS_IsMemoryRead(ins);
  BOOL isWrit = INS_IsMemoryWrite(ins);

  if(!(isRead || isWrit))
    return;

  RTN rtn = INS_Rtn(ins);
  string name_cpp = (RTN_Valid(rtn))? RTN_Name(rtn): "Anon";
  char *rtn_name = (char *) malloc(name_cpp.length() + 1);
  strcpy(rtn_name, name_cpp.c_str());

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
  // cout << "Loading image " << IMG_Name(img) << endl;

  /* The accounting will start at cache_get, and end at the final instruction 
    * (if it is a return), or at the exit function */
  RTN cacheGetFn = RTN_FindByName(img, "cache_get");
  if(RTN_Valid(cacheGetFn)) {
    RTN_Open(cacheGetFn);

    INS entryIns = RTN_InsHead(cacheGetFn);
    // cout << "cache_get at 0x" << hex << INS_Address(entryIns) << endl;
    INS_InsertCall(entryIns, IPOINT_BEFORE, (AFUNPTR)SetTrigger, IARG_BOOL, TRUE, IARG_END);

    INS retIns = RTN_InsTail(cacheGetFn);
    if(INS_IsRet(retIns)) {
      // cout << "exit at 0x" << hex << INS_Address(retIns) << endl;
      INS_InsertCall(retIns, IPOINT_BEFORE, (AFUNPTR)SetTrigger, IARG_BOOL, FALSE, IARG_END);
    }
    RTN_Close(cacheGetFn);

  }

  // RTN exitRtn = RTN_FindByName(img, "exit");
  // if(RTN_Valid(exitRtn)) {
  //   if(EndAddr == 0)
  //       EndAddr = RTN_Address(exitRtn);
  //   cout << "exit at 0x" << hex << EndAddr << endl;
  // }
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