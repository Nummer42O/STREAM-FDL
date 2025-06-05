#include "dynamic-subgraph/graph.hpp"

#include "common.hpp"

#include <cassert>


const Graph::Vertex *Graph::add(Member::Ptr member)
{
  LOG_TRACE(LOG_THIS LOG_VAR(member));

  Vertices::iterator it = mVertices.find(member->mPrimaryKey);
  if (it != mVertices.end())
    return nullptr;

  LOG_DEBUG(LOG_THIS LOG_VAR(member->cmIsTopic))
  Vertex vertex;
  if (member->cmIsTopic)
  {
    auto topic = static_cast<const Topic *>(member);

    vertex.type = Vertex::TYPE_NODE;
    std::transform(
      topic->mPublishers.begin(), topic->mPublishers.end(),
      std::back_inserter(vertex.incoming),
      [](const Topic::Edges::value_type &edge) -> MemberIds::value_type
      {
        return edge.node;
      }
    );
    std::transform(
      topic->mSubscribers.begin(), topic->mSubscribers.end(),
      std::back_inserter(vertex.outgoing),
      [](const Topic::Edges::value_type &edge) -> MemberIds::value_type
      {
        return edge.node;
      }
    );
  }
  else
  {
    auto node = static_cast<const Node *>(member);
    vertex.type = Vertex::TYPE_NODE;

    std::transform(
      node->mServers.begin(), node->mServers.end(),
      std::back_inserter(vertex.incoming),
      [](const Node::ServiceMapping::value_type &entry) -> MemberIds::value_type
      {
        return entry.second;
      }
    );
    std::transform(
      node->mActionServers.begin(), node->mActionServers.end(),
      std::back_inserter(vertex.incoming),
      [](const Node::ServiceMapping::value_type &entry) -> MemberIds::value_type
      {
        return entry.second;
      }
    );
    std::copy(node->mSubscribesTo.begin(), node->mSubscribesTo.end(), std::back_inserter(vertex.incoming));

    for (const Node::ClientMapping::value_type &service: node->mClients)
      std::copy(service.second.begin(), service.second.end(), std::back_inserter(vertex.outgoing));
    for (const Node::ClientMapping::value_type &service: node->mActionClients)
      std::copy(service.second.begin(), service.second.end(), std::back_inserter(vertex.outgoing));
    std::copy(node->mPublishesTo.begin(), node->mPublishesTo.end(), std::back_inserter(vertex.outgoing));
  }

  auto [emplacedIt, isEmplaced] = mVertices.emplace(member->mPrimaryKey, std::move(vertex));
  assert(isEmplaced);
  return &(emplacedIt->second);
}

const Graph::Vertex *Graph::get(const PrimaryKey &primary) const
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary));

  Vertices::const_iterator it = mVertices.find(primary);
  if (it != mVertices.end())
    return &(it->second);

  return nullptr;
}

const Graph::Vertex *Graph::add(const PrimaryKey &primary, Vertex vertex)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary) LOG_VAR(&vertex));

  Vertices::iterator it = mVertices.find(primary);
  if (it != mVertices.end())
    return nullptr;

  mVertices.emplace(primary, std::move(vertex));
  return &(it->second);
}

static void getBlindspotsInternal(
  const Graph &graph,
  MemberIds currentPath,
  MemberIds &ioAllVertices,
  MemberIds &oBlindSpots
)
{
  LOG_TRACE(LOG_VAR(&graph) LOG_VAR(currentPath) LOG_VAR(ioAllVertices) LOG_VAR(oBlindSpots));

  // get outgoing edges (and their count) for currently observed vertex
  const PrimaryKey &currentVertex = currentPath.back();
  const MemberIds &outgoingEdges = graph.get(currentVertex)->outgoing;
  size_t nrOutgoingEdges = outgoingEdges.size();

  // if there are no more outgoing edges or the only outgoing edge is already in the path,
  // i.e. the path loops, the current node should be monitored as a potential blind spot
  if (nrOutgoingEdges == 0ul ||
      (nrOutgoingEdges == 1ul && std::find(currentPath.begin(), currentPath.end(), outgoingEdges.front()) != currentPath.end()))
  {
    oBlindSpots.push_back(currentVertex);
    return;
  }

  // if there are outgoing edges, investigate all vertices they lead to
  for (const PrimaryKey &outgoingEdgeTo: outgoingEdges)
  {
    // if the vertex has been visited already, ignore it
    MemberIds::const_iterator it = std::find(ioAllVertices.begin(), ioAllVertices.end(), outgoingEdgeTo);
    if (it == ioAllVertices.end())
      continue;

    // if the vertex has not been visited, mark it as such and investigate it
    ioAllVertices.erase(it);
    currentPath.push_back(outgoingEdgeTo);
    getBlindspotsInternal(graph, currentPath, ioAllVertices, oBlindSpots);
  }
}

MemberIds getBlindspots(const Graph &graph)
{
  LOG_TRACE(LOG_VAR(&graph));

  // get a vector of all vertices in the graph
  MemberIds allVertices;
  std::transform(
    graph.mVertices.begin(), graph.mVertices.end(),
    allVertices.begin(),
    [](const Graph::Vertices::value_type &vertex) -> MemberIds::value_type
    {
      return vertex.first;
    }
  );

  MemberIds blindspots;
  // visited vertices get removed, so initiate searches until there are no more unchecked vertices
  while (!allVertices.empty())
  {
    PrimaryKey vertex = allVertices.back();
    allVertices.pop_back();
    getBlindspotsInternal(graph, {vertex}, allVertices, blindspots);
  }

  return blindspots;
}
