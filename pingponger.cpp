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
using std::list;

class PongProcess : public Process<PongProcess>
{
public:

  Future<int> askPong(int i) { int coin = rand(); if (coin % 2 == 0) { double d = ((double) rand() / (RAND_MAX)) + 1; os::sleep(Seconds(d)); } return i+1; }

};


class PingProcess : public Process<PingProcess>
{
public:

  PingProcess(const list<PID<PongProcess>>& pongers, int numberOfPongers)
  {
    itPongers = pongers.begin();
    this->itPongers = itPongers;
    this->numberOfPongers = numberOfPongers;
    this->pongers = pongers;
  }
 
protected:

  void ping(PID<PongProcess> pid, int i)
  {
    if (i < 10){
      dispatch(pid, &PongProcess::askPong, i).
	then(defer(self(), [this, pid] (int i) mutable -> Nothing {
	      std::cout << "Message: " << i << std::endl;
 	      ping(pid, i);
	      return Nothing();
	    }));
    } else {
      terminate(pid);
      pongsCompleted++;
      if (pongsCompleted == numberOfPongers) {
	terminate(self());
      }
    }
  }

  virtual void initialize()
  {
     for(itPongers = pongers.begin(); itPongers != pongers.end(); itPongers++) {
       ping(*itPongers, 0);
     }

  }

private:
  list<PID<PongProcess>> pongers;
  list<PID<PongProcess>>::const_iterator itPongers;  
  int numberOfPongers;
  int pongsCompleted = 0;
};

int main(int argc, char** argv)
{
  int numberOfPongers = int();
  std::cout << "Number of Pongers:" << std::endl;
  std::cin >> numberOfPongers;

  list<PID<PongProcess>> pongers;


  for(int i = 0; i < numberOfPongers; i++) {

    PongProcess* pong = new PongProcess();
    spawn(pong, true);
    PID<PongProcess> pid = pong->self();
    pongers.push_back(pid);
  }


  PingProcess ping(pongers, numberOfPongers);
  spawn(ping);
  wait(ping);
  
  list<PID<PongProcess>>::const_iterator itPongers;
  for(itPongers = pongers.begin(); itPongers != pongers.end(); itPongers++) {
    wait(*itPongers);
  }

  return 0;
}
