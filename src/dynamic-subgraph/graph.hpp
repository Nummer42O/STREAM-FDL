#pragma once

#include "dynamic-subgraph/members.hpp"

#include <vector>
#include <thread>
#include <map>
#include <atomic>
#include <chrono>
namespace cr = std::chrono;
#include <iostream>


class Graph
{
friend class DataStore;

public:
  Graph() {}
  Graph(Graph &&other);
  Graph(const Graph &other);

  bool add(
    const MemberPtr &member
  );
  bool contains(
    const MemberProxy &member
  ) const;
  void reset() { mVertices.clear(); }

  void visualise(
    const std::atomic<bool> &running,
    cr::milliseconds loopTargetInterval
  );
  void updateVisualisation();

  MemberProxies getOutgoing(
    const MemberPtr &member
  );
  MemberProxies getIncoming(
    const MemberPtr &member
  );

  friend std::ostream &operator<<(
    std::ostream &stream,
    const Graph &graph
  );

private:
  Members mVertices;
  std::mutex mVerticesMutex;

  std::atomic<bool> mUpdateVisualisation;
};
std::ostream &operator<<(std::ostream &stream, const Graph &graph);