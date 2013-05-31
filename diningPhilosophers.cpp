#include <iostream>

#include <process/defer.hpp>
#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/pid.hpp>
#include <process/process.hpp>

#include <stout/nothing.hpp>
#include <stout/os.hpp>

using namespace process;

using std::string;
using std::vector;


class TableProcess : public Process<TableProcess>
{
public:
  TableProcess()
  {
    int numberOfPhilosophers = 5;
    this->philosophersLeft = numberOfPhilosophers;
    this->numberOfPhilosophers = numberOfPhilosophers;
  }

  Future<bool> askForFork(int i)
  {
    if (i == numberOfPhilosophers) {
      i = 0;
    }
    if (forks[i]) {
      forks[i] = false;
      return true;
    } else {
      return false;
    }
  }

  Nothing replaceFork(int i)
  {
    if (i == numberOfPhilosophers) {
      i = 0;
    }
    forks[i] = true;
    return Nothing();
  }

  void checkIfDone()
  {
    std::cout << philosophersLeft << std::endl;
    philosophersLeft--;
    if (philosophersLeft ==  0) {
      std::cout << "All Done" << std::endl;
      done.set(Nothing());
      terminate(self());
    }
  }
  
  Future<Nothing> empty() { return done.future(); }
  
  virtual void initialize()
  {
    for (int i = 0; i < numberOfPhilosophers; i++){
      forks.push_back(true);
    }
    srand(time(0));
  }

private:
  Promise<Nothing> done;
  vector<bool> forks;
  int philosophersLeft;
  int numberOfPhilosophers;
};

class Table {
public:
  Table () { spawn(process); }
  ~Table () { terminate(process); wait(process); }

  Future<Nothing> empty() { return dispatch(process, &TableProcess::empty); }
  Future<bool> askForFork(int i) { return dispatch(process, &TableProcess::askForFork, i); }
  Future<Nothing>  replaceFork(int i) { return dispatch(process, &TableProcess::replaceFork, i); }
  void checkIfDone() { dispatch(process, &TableProcess::checkIfDone); }

private:
  TableProcess process;
};

class PhilosopherProcess : public Process<PhilosopherProcess>
{
public:

  PhilosopherProcess(Table *table, int philosopherNumber, int servingsLeft)
  {
    this->table = table;
    this->philosopherNumber = philosopherNumber;
    this->servingsLeft = servingsLeft;
  }

protected:
  void think() {
    int i = rand();
    if (i % 2 == 0) {
      double d = ((double) rand() / (RAND_MAX)) + 1;
      os::sleep(Seconds(d));
    }
  }

  Future<Nothing> replaceBoth()
  {
    return table->replaceFork(philosopherNumber).then([this] (Nothing) { return table->replaceFork(philosopherNumber + 1); });
  }

  virtual void initialize()
  {
    perform();
  }

  void eat() 
  {
    std::cout << philosopherNumber << " Eating" << std::endl;
    os::sleep(Seconds(((double) rand() / (RAND_MAX)) + 1));
    std::cout << philosopherNumber << " Done Eating" << std::endl;

    servingsLeft--;
  }

  Nothing perform()
  {
    if (servingsLeft == 0) {
      table->checkIfDone();
      terminate(self());
      return Nothing();
    }

    table->askForFork(philosopherNumber)
      .then(defer(self(), [this] (bool available) {
        if (available) {
	  std::cout << philosopherNumber << "picked up left fork" << std::endl;
	  table->askForFork(philosopherNumber + 1)
	    .then(defer(self(), [this] (bool available) -> Future<Nothing> {
	      if (available) {
		std::cout << philosopherNumber << "picked up right fork" << std::endl;
	        eat();
		return replaceBoth().then([this] (Nothing) { return perform(); });
	      } else {                
		std::cout << philosopherNumber << " Right fork unavailable" << std::endl;
		return table->replaceFork(philosopherNumber).then([this] (Nothing) { think(); return perform(); });
 	      }
	    }));
	} else {
	  std::cout << philosopherNumber << " Left fork unavailable" << std::endl;
          think();
          return perform();
        }
      }));
  }


private:
  Table *table;
  int philosopherNumber;
  int servingsLeft;
};

int main(int argc, char** argv)
{
  int numberOfPhilosophers = 5;
//   std::cout << "How many philosophers?" << std::endl;
//   std::cin >> numberOfPhilosophers;

  int servings;
  std::cout << "How many times should each philosopher eat?" << std::endl;
  std::cin >> servings;

  Table table;

  for (int i = 0; i < numberOfPhilosophers; i++) {
    PhilosopherProcess* philosopher = new PhilosopherProcess(&table, i, servings);
    spawn(philosopher, true);
  }

  table.empty().await();

  return 0;
}
