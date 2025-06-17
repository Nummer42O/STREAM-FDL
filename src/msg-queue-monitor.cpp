#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <iostream>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <chrono>
namespace cr = std::chrono;

#include "ipc/common.hpp"
#include "ipc/util.hpp"

struct TestMsg{
  msgType_t key;
  requestId_t requestId;
  pid_t senderId;

  char buffer[8192 - sizeof(pid_t) - sizeof(requestId_t)];
};

bool receiveMsg(int msgQueueId, TestMsg &payload, long idx)
{
  ssize_t nrBytes = ::msgrcv(
    msgQueueId,
    &payload, sizeof(TestMsg) - sizeof(msgType_t),
    idx,
    IPC_NOWAIT | MSG_COPY
  );
  if (nrBytes == -1)
  {
    assert(errno == ENOMSG);
    return false;
  }

  return true;
}

void printMsg(const TestMsg &msg)
{
  pid_t receiverPid = (msg.key & 0x00000000FFFFFFFF);
  std::cout <<
    (receiverPid == SERVER_PSEUDO_PID ? "Request" : "Response") << ":\n"
    "\tmsg type:     " << ((msg.key & 0xFFFFFFFF00000000) >> 32) << "\n"
    "\treceiver pid: " << receiverPid << "\n";
  if (receiverPid == SERVER_PSEUDO_PID)
  {
    std::cout <<
      "\trequest id:   " << msg.requestId << "\n"
      "\tsender pid:   " << msg.senderId << "\n";
  }
  std::cout << '\n';
}

int main()
{
  std::time_t timestamp = cr::system_clock::to_time_t(cr::system_clock::now());
  std::cout << "\n[" << std::put_time(std::localtime(&timestamp), "%x %X") << "]\n";

  int msgQueueId = util::getMsgQueueId(1);
  TestMsg msg;
  long idx = 0l;
  while (receiveMsg(msgQueueId, msg, idx++))
  {
    printMsg(msg);
  }
}
