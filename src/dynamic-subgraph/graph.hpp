#pragma once

#include "dynamic-subgraph/members.hpp"

#include <vector>
#include <thread>
#include <map>
#include <atomic>


class Graph
{
friend class DataStore;

public:
  struct Vertex
  {
    enum Type {
      TYPE_NODE, TYPE_TOPIC
    } type;
    MemberIds incoming, outgoing;
  };

private:
  using Vertices = std::map<PrimaryKey, Vertex>;

public:
  Graph() {}
  Graph(Graph &&other);
  Graph(const Graph &other);

  const Vertex *addNode(
    const PrimaryKey &primary,
    MemberIds incomingEdges = {},
    MemberIds outgoingEdges = {}
  ) { return add(primary, Vertex{.type = Vertex::TYPE_NODE, .incoming = std::move(incomingEdges), .outgoing = std::move(outgoingEdges)}); }
  const Vertex *addTopic(
    const PrimaryKey &primary,
    MemberIds incomingEdges = {},
    MemberIds outgoingEdges = {}
  ) { return add(primary, Vertex{.type = Vertex::TYPE_TOPIC, .incoming = std::move(incomingEdges), .outgoing = std::move(outgoingEdges)}); }

  const Vertex *add(
    Member::Ptr member
  );

  const Vertex *get(
    const PrimaryKey &primary
  ) const;

  void reset() { mVertices.clear(); }

  friend MemberIds getBlindspots(
    const Graph &graph
  );

  void visualise(
    const std::atomic<bool> &running
  );

  void updateVisualisation();

private:
  const Vertex *add(
    const PrimaryKey &primary,
    Vertex vertex
  );

private:
  Vertices mVertices;
  bool mUpdateVisualisation;
  std::mutex mVisualisationMutex;
};

MemberIds getBlindspots(
  const Graph &graph
);
