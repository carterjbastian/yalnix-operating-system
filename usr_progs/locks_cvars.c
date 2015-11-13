
int global; 

int main(int argc, char*argv[]) { 
  
  global = 5;
  TracePrintf(1, "Start: locks_cvars.c\n");

  int rc; 
  
  rc = CvarSignal(4);
  TracePrintf(1, "Signal cvar that doesn't exist. rc = %d\n", rc);

  rc = CvarBroadcast(4);
  TracePrintf(1, "Broadcast cvar that doesn't exist. rc = %d\n", rc);

  rc = CvarWait(4);
  TracePrintf(1, "Wait on cvar that doesn't exist. rc = %d\n", rc);
  
  rc = Acquire(4);
  TracePrintf(1, "Acquire lock that doesn't exist. rc = %d\n", rc);
    
  rc = Release(4);
  TracePrintf(1, "Release lock that doesn't exist. rc = %d\n", rc);

  int lock_id;
  rc = LockInit(&lock_id);
  TracePrintf(1, "Init lock. rc = %d, lock_id = %d\n", rc, lock_id);
  
  int cvar_id;
  rc = CvarInit(&cvar_id);
  TracePrintf(1, "Init cvar. rc = %d, cvar_id = %d\n", rc, cvar_id);

  // test locks 
  int pid;
  pid = GetPid();
  rc = Fork();
  if (rc == 0) { 
    
    rc = Fork();
    if (rc == 0) { 
      
      TracePrintf(1, "P3 trying to acquire lock\n");
      Acquire(lock_id);
      TracePrintf(1, "P3 has lock - sleeping now\n");
      Pause();
      Pause();
      TracePrintf(1, "P3 done work - releasing lock\n");
      Release(lock_id);
      Exit(100);

    } else { 

      TracePrintf(1, "P2 trying to acquire lock\n");
      Acquire(lock_id);
      TracePrintf(1, "P2 has lock - sleeping now\n");
      Pause();
      Pause();
      TracePrintf(1, "P2 done work - releasing lock\n");
      Release(lock_id);
      int status;
      Wait(&status);
      Exit(200);

    }  
    
  } else { 
    
    TracePrintf(1, "P1 trying to acquire lock\n");
    Acquire(lock_id);
    TracePrintf(1, "P1 has lock - sleeping now\n");
    Pause();
    TracePrintf(1, "P1 done work - releasing lock\n");
    Release(lock_id);
    int status;
    Wait(&status);
  } 
  
  TracePrintf(1, "Finished Testing Locks\n", rc);
  // test cvars
  return 0;
}




