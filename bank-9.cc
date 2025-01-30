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
 
const int num_threads = 16;
const int num_accounts = 10000;
const float initial_amount = 10.0f;
const int num_dowork = 10000;
const int num_segments = 100;

std::map<int, float> bank;
std::mutex global_mutex;
std::array<std::mutex, num_accounts> mtx;
std::array<std::mutex, num_segments> blnc_mtx;
std::array<int, num_segments> global_balances;

void deposit(std::mt19937 &generator) {
  std::uniform_int_distribution<int> account(0, num_accounts-1);
  std::uniform_int_distribution<int> value(1, 10);

  int b1 = account(generator), b2 = account(generator);
  int v =  value(generator); // from 1 to 10

  if (b1 == b2) {
    return;
  }

  int first_acc = std::min(b1, b2);
  int second_acc = std::max(b1, b2);

  std::lock_guard<std::mutex> lock1(mtx[first_acc]);
  std::lock_guard<std::mutex> lock2(mtx[second_acc]);
  bank[b1] -= v;
  bank[b2] += v;
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
    //   std::cout << "local balance " << balance() << std::endl;
    balance();
    // int val = balance();
    // if (val != 100000) { 
    //   std::cout << "DALBAEB" << std::endl;
    // }
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
  for (int i = 0; i < num_segments; i++) {
    global_balances[i] = (int) (num_accounts*initial_amount / num_segments);
  }

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