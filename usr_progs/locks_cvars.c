int global = 0;

int main(int argc, char*argv[]) { 
  
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
  /*
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

*/
  // test cvar
  TracePrintf(1, "Starting Cvar Broadcast Testing\n", rc);
  rc = Fork();

  if (rc == 0) { 
    
    rc = Fork();
    if (rc == 0) { 

      rc = Fork();
      if (rc == 0) { 
        // P4
        work(4, cvar_id, lock_id);
        Exit(400);        
      } else { 
        // P3
        work(3, cvar_id, lock_id);
        int status;
        Wait(&status);
        Exit(300);
      }
      
    } else { 
      
      // P2
      work(2, cvar_id, lock_id);
      int status;
      Wait(&status);
      Exit(200);
    } 
    
  } else { 
    
    // P1
    Pause();
    Pause();
    Pause();
    Pause();
    Pause();
    Pause();
    TracePrintf(1, "P1 awake. Acquire, signal, release\n");
    Acquire(lock_id);
    CvarBroadcast(cvar_id);
    Release(lock_id);
    int status;
    Wait(&status);
  }

  TracePrintf(1, "Finished Broadcast testing.\n");


  // -------------------------
  
  
  TracePrintf(1, "Starting Cvar Signal Testing\n", rc);
  rc = Fork();

  if (rc == 0) { 
    
    rc = Fork();
    if (rc == 0) { 

      rc = Fork();
      if (rc == 0) { 
        // P4
        work_and_signal(4, cvar_id, lock_id);
        Exit(400);        
      } else { 
        // P3
        work_and_signal(3, cvar_id, lock_id);
        int status;
        Wait(&status);
        Exit(300);
      }
      
    } else { 
      
      // P2
      work_and_signal(2, cvar_id, lock_id);
      int status;
      Wait(&status);
      Exit(200);
    } 
    
  } else { 
    
    // P1
    Pause();
    Pause();
    Pause();
    Pause();
    Pause();
    TracePrintf(1, "P1 awake. Acquire, signal, release\n");
    Acquire(lock_id);
    CvarSignal(cvar_id);
    Release(lock_id);
    int status;
    Wait(&status);
  }

  TracePrintf(1, "Finished Signal Testing\n");
  TracePrintf(1, "End: locks_cvars.c\n");

  return 0;
}


void work(int fake_pid, int cvar_id, int lock_id) { 
  Pause();
  TracePrintf(1, "P%d awake, trying to acquire lock\n", fake_pid);
  Acquire(lock_id);
  TracePrintf(1, "P%d has lock.\n", fake_pid);
  TracePrintf(1, "P%d waiting.\n", fake_pid);
  CvarWait(cvar_id, lock_id);
  TracePrintf(1, "P%d done waiting. Doing work\n", fake_pid);
  Pause();
  TracePrintf(1, "P%d releasing lock!\n", fake_pid);
  Release(lock_id);
};


void work_and_signal(int fake_pid, int cvar_id, int lock_id) { 
  TracePrintf(1, "P%d awake, trying to acquire lock\n", fake_pid);
  Acquire(lock_id);
  TracePrintf(1, "P%d has lock.\n", fake_pid);
  TracePrintf(1, "P%d waiting.\n", fake_pid);
  CvarWait(cvar_id, lock_id);
  TracePrintf(1, "P%d done waiting. Doing work\n", fake_pid);
  Pause();
  Pause();
  TracePrintf(1, "P%d signaling and releasing lock\n", fake_pid);
  CvarSignal(cvar_id);
  Release(lock_id);
};
