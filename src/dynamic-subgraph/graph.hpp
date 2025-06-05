#pragma once

#include "dynamic-subgraph/members.hpp"

#include <vector>
#include <map>


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
  const Vertex *addNode(
    PrimaryKey primary,
    MemberIds incomingEdges = {},
    MemberIds outgoingEdges = {}
  ) { return add(primary, Vertex{.type = Vertex::TYPE_NODE, .incoming = std::move(incomingEdges), .outgoing = std::move(outgoingEdges)}); }
  const Vertex *addTopic(
    PrimaryKey primary,
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


private:
  const Vertex *add(
    PrimaryKey primary,
    Vertex vertex
  );

private:
  Vertices mVertices;
};

MemberIds getBlindspots(
  const Graph &graph
);
