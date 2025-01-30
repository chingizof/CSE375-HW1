// 

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
 


std::map<int, float> bank;
std::vector<std::mutex> mutexes;
std::mutex global_mutex;

const int num_threads = 16;
const int num_accounts = 10000;
const float initial_amount = 10.0f;
const int num_segments = 25;
const int num_dowork = 10000;

void deposit(std::mt19937 &generator) {
  std::uniform_int_distribution<int> account(0, num_accounts-1);
  std::uniform_int_distribution<int> value(1, 10);

  int b1 = account(generator), b2 = account(generator);
  int v =  value(generator); // from 1 to 10

  if (b1 == b2) {
    return;
  }

  // define mutex index in mutexes array
  int idx1 = b1 % num_segments;
  int idx2 = b2 % num_segments;

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
}

float balance() {
  float total = 0.0f;

  std::lock_guard<std::mutex> lock(global_mutex);
  for (int i=0; i < num_accounts; i++) {
    total += bank[i];
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
      balance();
    }
    else {
      deposit(generator);
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  // std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;

  return elapsed.count();
}

int main() {
  std::vector<int> idx_threads = {1, 2, 4, 8, 16, 28};
  std::vector<std::mutex> list(num_segments);
  mutexes.swap(list);

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