#include <iostream>
#include <map>    // for hashmap
#include <stdlib.h> // for cout, rand
#include <future> // for future
#include <mutex>  // for lock
#include <chrono> // for time
#include <thread> // for threads
#include <vector> // for vectors/arrays
#include <random>
#include <time.h> // for time
#include <condition_variable>
 
const int num_threads = 16;
const int num_accounts = 10000;
const float initial_amount = 10.0f;
const int num_dowork = 10000;

std::map<int, float> bank;
std::mutex global_mutex;
std::array<std::mutex, num_accounts> mtx;
std::condition_variable cv;


int running_balances = 0;
int running_deposits = 0;
std::mutex balance_mutex;

void deposit(std::mt19937 &generator) {
  std::uniform_int_distribution<int> account(0, num_accounts-1);
  std::uniform_int_distribution<int> value(1, 10);

  int b1 = account(generator), b2 = account(generator);
  int v =  value(generator); // from 1 to 10

  if (b1 == b2) {
    return;
  }

  int idx1 = std::min(b1, b2), idx2 = std::max(b1, b2);

  //wait for balances in progress
  {
    std::unique_lock<std::mutex> lock(balance_mutex);
    cv.wait(lock, []{return running_balances == 0;});
    running_deposits += 1;
  }

  std::lock_guard<std::mutex> lock1(mtx[idx1]);
  std::lock_guard<std::mutex> lock2(mtx[idx2]);
  bank[b1] -= v;
  bank[b2] += v;

  {
    std::unique_lock<std::mutex> lock(balance_mutex);
    running_deposits -= 1;
    if (running_deposits == 0) {
      cv.notify_all();
    }
  }
}

float balance() {
  float total = 0.0f;

  // wait for all deposits to leave
  {
    std::unique_lock<std::mutex> lock(balance_mutex);
    cv.wait(lock, []{return running_deposits == 0;});
    running_balances += 1;
  }

  for (int i=0; i < num_accounts; i++) {
    total += bank[i];
  }

  {
    std::unique_lock<std::mutex> lock(balance_mutex);
    running_balances -= 1;
    if (running_balances == 0) {
      cv.notify_all();
    }
  }

  return total;
}

double dowork() {
  static thread_local std::mt19937 generator(std::random_device{}()); // seed the engine to prevent diff threads have same sequences
  std:: uniform_int_distribution<int> chance(0, 99);
  auto start = std::chrono::high_resolution_clock::now();

  for (int i=0; i < num_dowork; i++) {
    int temp = chance(generator);

    if (temp < 5) {
      // balance();
      // if mutex not locked, lock it, increment running balances, run balance
        // when leaves, if there are other running balances then don't unlock
        // if no running balances, unlock and notify all
      // if mutex locked, increment running balancesm run balance()

      if (global_mutex.try_lock()) {
        std::unique_lock<std::mutex> lock(balance_mutex);
        cv.wait(lock, []{return running_deposits == 0;});
      }

      {
        std::lock_guard<std::mutex> lock(balance_mutex);
        running_balances += 1;
      }

      balance();
      // int val = balance();
      // if (val != 100000) { 
      //   std::cout << "DALBAEB" << std::endl;
      // }

      {
        std::lock_guard<std::mutex> lock(balance_mutex);
        running_balances -= 1;
        if (running_balances == 0) {
          global_mutex.unlock();
          cv.notify_all();
        }
      }
    }
    else {
      deposit(generator);

    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  return elapsed.count();
}

int main() {
  std::vector<int> idx_threads = {1, 2, 4, 8, 16, 28};

  for (int num_threads: idx_threads) {
    std::cout << "======" << std::endl;
    std::cout << num_threads << " THREADS" << std::endl;

    bank.clear(); // clear bank before each iteration

    for (int i=0; i < num_accounts; i++) {
      bank.insert({i, initial_amount}); //insert by 10$
    }

    float total = balance();
  
    std::vector<std::future<double>> futures;
    for (int i=0; i < num_threads; i++) {
      futures.emplace_back(std::async(std::launch::async, dowork));
    }

    double total_time = 0.0;
    for(auto &f : futures) {
      total_time += f.get();
    }

    std::cout << "total time calculated is: " << total_time << std::endl;
    std::cout << "total balance at the end is: " << balance() << std::endl;
  }


  return 0;
}



//don't use rand() or functions that are not thread safe. 
// speedup = try to run without locks, break to threads, and see if no lock program is speeding up

//create a makefile to execute
// add -O3 flag, compiler optimization. never compile code ever in your life without -O3

// how does -O3 work? read. 
// use try_lock 


// futures create overhead, maybe not use them and directly write to an array



// когда идешь в баланс, смотри чтобы не было депозитов
// когда идешь в депозит смотри в баланс 