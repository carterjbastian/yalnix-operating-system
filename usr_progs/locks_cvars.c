
int main(int argc, char*argv[]) { 
  
  int global = 0;
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
      TracePrintf(1, "P2 got exit code of P3: %d\n", status);
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
    TracePrintf(1, "P1 got exit code of P2: %d\n", status);
  } 
  
  TracePrintf(1, "Finished Testing Locks\n", rc);


  // test cvar
  TracePrintf(1, "Starting Cvar Testing\n", rc);
  rc = Fork();

  if (rc == 0) { 
    
    rc = Fork();
    if (rc == 0) { 

      rc = Fork();
      if (rc == 0) { 
        
        // P4
        TracePrintf(1, "P4 awake, trying to acquire lock\n");
        Acquire(lock_id);
        TracePrintf(1, "P4 acquired lock, now waiting for global to be > 0\n");
        while(global < 2) { 
          CvarWait(cvar_id, lock_id);
        } 
        TracePrintf(1, "P4 finished waiting. Working....\n");
        Pause();
        global++;
        TracePrintf(1, "P4 set global = %d\n", global);
        TracePrintf(1, "P4 signaled, now releasing lock\n");
        Release(lock_id);
              
      } else { 
      
        // P3 
        TracePrintf(1, "P3 awake, trying to acquire lock\n");
        Acquire(lock_id);
        TracePrintf(1, "P3 acquired lock, now waiting for global to be > 0\n");
        while(global < 1) { 
          CvarWait(cvar_id, lock_id);
        } 
        TracePrintf(1, "P3 finished waiting. Working....\n");
        Pause();
        global++;
        TracePrintf(1, "P3 set global = %d\n", global);
        CvarSignal(cvar_id);
        TracePrintf(1, "P3 signaled, now releasing lock\n");
        Release(lock_id);
        int status;
        Wait(&status);
      }
      
    } else { 
      
      // P2
      TracePrintf(1, "P2 awake, trying to acquire lock\n");
      Acquire(lock_id);
      TracePrintf(1, "P2 acquired lock, now waiting for global to be > 1\n");
      while(global < 2) { 
        CvarWait(cvar_id, lock_id);
      } 
      TracePrintf(1, "P2 finished waiting. Working....\n");
      Pause();
      global++;
      TracePrintf(1, "P2 set global = %d\n", global);
      CvarSignal(cvar_id);
      TracePrintf(1, "P2 signaled, now releasing lock\n");
      Release(lock_id);
      int status;
      Wait(&status);
    } 
    
  } else { 
    
    // P1
    Pause();
    Pause();
    TracePrintf(1, "P1 awake, trying to acquire lock\n");
    Acquire(lock_id);
    TracePrintf(1, "P1 acquired lock, doing work...\n");
    Pause();
    global++;
    TracePrintf(1, "P1 set global = %d\n", global);
    CvarBroadcast(cvar_id);
    TracePrintf(1, "P1 broadcasted and now releasing lock\n");
    Release(lock_id);
    int status;
    Wait(&status);
    TracePrintf(1, "P1 got exit code of p@: %d\n", status);
  }

  TracePrintf(1, "Finished with cvar waiting... global = %d\n", global);
  TracePrintf(1, "Finished testing Cvars.\n");

  TracePrintf(1, "End: locks_cvars.c\n");

  return 0;
}
