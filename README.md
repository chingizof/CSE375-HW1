
This is my homework for CSE375 class. 

### Running a program
```
cd cse375
g++ -o final_bank-2 final_bank-2.cc -pthread -O3
./final_bank-2
```

# initial version 1
```======
1 THREADS
total time calculated is: 0.0141139
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.0650543
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.491752
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 2.40351
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 9.18707
total balance at the end is: 100000
```


### without -O3 optimization flag
```
======
1 THREADS
total time calculated is: 0.0665137
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.249698
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 1.09314
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 3.96669
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 15.5787
total balance at the end is: 100000 
```

## Better Random Function + lock_guard, version 2
Let's make random function more thread friendly by using ```random``` (solution taken from https://stackoverflow.com/questions/21237905/how-do-i-generate-thread-safe-uniform-random-numbers). It takes more time, but provides the least amount of bias compared to other approaches (MT19973 is a much higher quality random number generator)

We will generate one generator per thread in ```dowork()``` and pass it to ```deposit()``` as arguments

```
======
1 THREADS
total time calculated is: 0.0134637
total balance at the end is: 99998
======
2 THREADS
total time calculated is: 0.0581936
total balance at the end is: 100015
======
4 THREADS
total time calculated is: 0.478896
total balance at the end is: 99962
======
8 THREADS
total time calculated is: 2.17538
total balance at the end is: 100002
======
16 THREADS
total time calculated is: 8.70445
total balance at the end is: 100080
```

WE can see some collisions occur as balance is not the same anymore. why?

It's because i don't clean the map in between the tasks. 


### thoughts
But, because threads are blocking each other at deposit(), they are actually wasting a lot of time. We should fix that

instead of trying to obtain a lock we use lock_guard, we speedup our code by 2 times. This is because lock_guard (and normal lock) implements a waiting queue where threads will be popped after lock is free, while old approach made each thread keep asking if lock is free every second. 

BEFORE:
```
while (!mutex.try_lock()) {

  }
  bank[b1] -= v;
  bank[b2] += v;
  mutex.unlock();
```

AFTER:
```
std::lock_guard<std::mutex> lock(mutex);
  bank[b1] -= v;
  bank[b2] += v;
```

Result (2x speedup for 4-16 threads, same for 1-2 threads)
```
======
1 THREADS
total time calculated is: 0.013325
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.0557366
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.241161
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 0.996865
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 4.08551
total balance at the end is: 100000
```


# More thread-friendly hashmap, version 3


maybe by making our hashmap more thread friendly we can speed up things more?

I created a ```num_segment``` locks for each segment in hashmap. I check to which segment do b1 and b2 correspond using ```int idx1 = b1 % num_segments;```. However, I noticed that deadlock appear when one thread locks ```lock[idx1]``` while ANOTHER thread locks ```lock[idx2]``` and they wait for each other. here is my implementation:
```
std::lock_guard<std::mutex> lock(mutexes[idx1]);
  if (idx1 != idx2) {
    // std::lock_guard<std::mutex> lock(mutexes[idx2]);
    mutexes[idx2].lock();
  }
  bank[b1] -= v;
  bank[b2] += v;
  if (idx1 != idx2) {
    mutexes[idx2].unlock();
  }
```

This feels like a round table dillema
let's fix it by doing try-lock

### WE CAN DO 100 SEGMENTS and there aqre almost no deadlocks
starting from now we will also test 28 threads because there are 16 CPUs and we can get 2 threads per CPU. Just to be safe and make sure my program is slow because of overflow of threads, I will test it for 28 threads. 
```
======
1 THREADS
total time calculated is: 0.0144017
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.0531901
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.220678
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 0.915
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 3.69764
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 11.4254
total balance at the end is: 100000
```

### when we set num_segments to 50
we get almost the same time, with twice less memory.

```
======
1 THREADS
total time calculated is: 0.0144459
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.0505221
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.214879
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 0.925052
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 3.50734
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 11.614
total balance at the end is: 100000
```

### same time with 20 segments
```
======
1 THREADS
total time calculated is: 0.0149565
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.0522277
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.219291
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 0.919457
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 3.66047
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 11.45
total balance at the end is: 100000
```


## So, adding more segments doesn't improve speed, but it helps to decrease change of deadlock. 
## we will change number of accounts to 10000 so we can better see time difference

### One global lock (bank-3.cc)
```
======
1 THREADS
total time calculated is: 0.180335
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.689999
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 2.31582
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 8.88218
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 31.2315
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 97.7016
total balance at the end is: 100000
```

### 25 local locks (bank-4.cc)
```
======
1 THREADS
total time calculated is: 0.176292
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.681482
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 2.70885
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 7.9603
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 28.2264
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 89.8304
total balance at the end is: 100000
```

### we can see almost no speedup :| doesn't make sense

(? question to Palmiery) do we need to spawn threads and only use future to retrieve results???
but creating a thread for a short task creates too much overhead (by initializing and destroying it), while async uses a thread pool with less overhead 



# let's try to distribute K work among dowork() threads (K/num_threads for each) instead of creating K work for each new thread (bank-5.cc from bank-4.cc)

```
1 THREADS
total time calculated is: 0.163617
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.325767
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.515515
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 0.883933
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 1.63427
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 2.93848
total balance at the end is: 100000
```


interestingly, my program *without locks* doesn't return a speedup and slows down too 

```
======
1 THREADS
total time calculated is: 0.158377
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.175722
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.180461
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 0.179657
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 0.249099
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 0.250507
total balance at the end is: 100000
```


# what if we don't make a global lock on map when we run balance(), but place local locks when we access specific segment (of course it is vulnerable to incorrect values, is one segments's sum that was read gets updated, updating a future sum that is about to be read)

it gets slower
### DISTRIBUTED 10000 work operations (big speedup) compared to global lock
```
======
1 THREADS
total time calculated is: 0.206301
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.227703
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.253227
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 0.355018
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 0.624766
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 1.08966
total balance at the end is: 100000
```
### not-distributed just stops working (deadlock) or takes too much time. 


### and if we measure total work in main() thread instead of summing 


# we will now free locks in balance(), once they get read (from bank-4.cc to bank-6.cc)
# HOW TO USE CONDITIONAL VARIABLES HERE






FRom palmiery notes

allow balance only get one lock, to signal to deposits that they are banned 
- you can try and create array, where you store local balances for a segment. Like segment_balances[10] and balance() only sums segment_balances

- we want to run deposits to run in parallel

- review lecture recording, i didn't get what he meant. 17-25 minutes into a lecture
- use local locks?? NO USE 1 LOCK PER ACCOUNT



### we will move from single mutex to closing each account + using global mutex for balance (bank-7 from bank-2)

we got a bit slower although threads don't need to wait for global lock. interesting. 

GLOBAL MUTEX (bank-2)
```
======
1 THREADS
total time calculated is: 0.177383
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.692306
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 2.58018
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 9.09898
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 34.182
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 103.181
total balance at the end is: 100000
```

EACH ACCOUNT EACH LOCK
```
======
1 THREADS
total time calculated is: 0.182007
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.726818
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 2.77846
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 9.33633
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 34.5904
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 110.874
total balance at the end is: 100000
```

## (ended up with race condition) Create a global array with balances of array segments (bank-8 from bank-7)

Assume there are only 100 segments, each holding a total balance of 100 bank accounts. when I built deposit(), I was aquiring two locks of segments, which has caused a dependency issue and deadlock. However, I prevented that by going in order to aquire locks.

this has resulted in a super strong speedup (x296 speedup for 28 threads, 83x speedup for 1 thread):
```
======
1 THREADS
total time calculated is: 0.00217162
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.0050766
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 0.0128003
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 0.0340872
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 0.11167
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 0.373375
total balance at the end is: 100000
```



actually I realized that I have a race condition




##  (RACE CONDITION) rewrote balance to lock all of the personal locks, sum their values, and unlock them (bank-9.cc)

no good speeedup 
```
======
1 THREADS
total time calculated is: 0.18801
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.662282
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 2.53659
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 10.3668
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 37.3071
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 108.389
total balance at the end is: 100000
```
RACE CONDITION HERE




from lecture notes:
allow balance to take only one lock and signal to threads that balance is executing
if there are several balances at the same time, let them execute. And when the last balance leaves, unlock this mutex
maybe you just need a volatile variable





RACE FREE CODES:
bank 1, 




### I debugged bank-7, with one lock for Balance() and unique locks for accounts. 
when we run balance(), we stop running new deposits and wait for old/running deposits to finish. Similarly, when we run deposit(), it wait for balances to finish first. 
```
======
1 THREADS
total time calculated is: 0.162016
total balance at the end is: 100000
======
2 THREADS
total time calculated is: 0.599277
total balance at the end is: 100000
======
4 THREADS
total time calculated is: 2.29676
total balance at the end is: 100000
======
8 THREADS
total time calculated is: 6.84117
total balance at the end is: 100000
======
16 THREADS
total time calculated is: 22.7772
total balance at the end is: 100000
======
28 THREADS
total time calculated is: 66.6684
total balance at the end is: 100000
```