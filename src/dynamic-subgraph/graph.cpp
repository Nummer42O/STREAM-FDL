#include "dynamic-subgraph/graph.hpp"

#include "common.hpp"

#include <graphviz/gvc.h>
#include <graphviz/cgraph.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <cassert>
#include <mutex>
#include <set>
#include <cstdio>


Graph::Graph(Graph &&other):
  mVertices(std::move(other.mVertices))
{
  LOG_TRACE(LOG_THIS LOG_VAR(&other));
}

Graph::Graph(const Graph &other):
  mVertices(other.mVertices)
{
  LOG_TRACE(LOG_THIS LOG_VAR(&other));
}

bool Graph::add(const MemberPtr &member)
{
  LOG_TRACE(LOG_THIS LOG_VAR(member));

  const ScopeLock scopedLock(mVerticesMutex);

  Members::iterator it = std::find_if(
    mVertices.begin(), mVertices.end(),
    [&member](const Members::value_type &existingMember)
    {
      return existingMember == member;
    }
  );
  if (it != mVertices.end())
    return false;

  mVertices.push_back(member);
  return true;
}

bool Graph::contains(const MemberProxy &member) const
{
  Members::const_iterator it = std::find_if(
    mVertices.begin(), mVertices.end(),
    [&member](const Members::value_type &existingMember)
    {
      return existingMember->mPrimaryKey == member.mPrimaryKey;
    }
  );
  return it != mVertices.end();
}

void Graph::visualise(const std::atomic<bool> &running)
{
  LOG_TRACE(LOG_THIS)

  cv::Mat image = cv::Mat::ones(10, 10, CV_8UC4);
  while (running.load())
  {
    if (mUpdateVisualisation)
    {
      LOG_DEBUG("updating visualisation")

      GVC_t *gvc = gvContext();
      Agraph_t *graph = agopen(const_cast<char *>("g"), Agdirected, NULL);
      {
        const ScopeLock scopedLock(mVerticesMutex);

        char cPrimaryKey[37];
        for (const MemberPtr &vertex: mVertices)
        {
          std::strcpy(cPrimaryKey, vertex->mPrimaryKey.c_str());
          Agnode_t *fromNode = agnode(graph, cPrimaryKey, 1);
          for (const MemberProxy &to: this->getOutgoing(vertex))
          {
            std::strcpy(cPrimaryKey, to.mPrimaryKey.c_str());
            Agnode_t *toNode = agnode(graph, cPrimaryKey, 1);
            agedge(graph, fromNode, toNode, NULL, 1);
          }
        }
      }
      mUpdateVisualisation = false;

      std::FILE *graphFile = std::fopen("/tmp/render.png", "wb+");
      assert(graphFile);
      gvLayout(gvc, graph, "neato");
      gvRender(gvc, graph, "png", graphFile);
      gvFreeLayout(gvc, graph);
      std::fclose(graphFile);

      cv::Mat tmp = cv::imread("/tmp/render.png", cv::IMREAD_UNCHANGED);
      LOG_DEBUG("New image is empty: " << tmp.empty());
      if (!tmp.empty())
        image = std::move(tmp);
    }

    cv::imshow("Subgraph", image);
    cv::waitKey(100);
  }
}

void Graph::updateVisualisation()
{
  // std::lock_guard<std::mutex> scopedLock(mVisualisationMutex);
  mUpdateVisualisation = true;
}

MemberProxies Graph::getOutgoing(const MemberPtr &member)
{
  MemberProxies outgoing;
  if (member->mIsTopic)
  {
    const Topic *topic = ::asTopic(member);

    for (const auto &[edge, targetNode]: topic->mSubscribers)
    {
      if (!this->contains(targetNode))
        continue;

      outgoing.push_back(targetNode);
    }
  }
  else
  {
    const Node *node = ::asNode(member);

    for (const auto &[serviceName, clients]: node->mClients)
      for (const MemberProxy &client: clients)
      {
        if (!this->contains(client))
          continue;

        outgoing.push_back(client);
      }
    for (const auto &[serviceName, clients]: node->mActionClients)
      for (const MemberProxy &client: clients)
      {
        if (!this->contains(client))
          continue;

        outgoing.push_back(client);
      }
    for (const MemberProxy &targetTopic: node->mPublishesTo)
    {
      if (!this->contains(targetTopic))
        continue;

      outgoing.push_back(targetTopic);
    }
  }

  return outgoing;
}

MemberProxies Graph::getIncoming(const MemberPtr &member)
{
  MemberProxies incoming;
  if (member->mIsTopic)
  {
    const Topic *topic = ::asTopic(member);

    for (const auto &[edge, sourceNode]: topic->mPublishers)
    {
      if (!this->contains(sourceNode))
        continue;

      incoming.push_back(sourceNode);
    }
  }
  else
  {
    const Node *node = ::asNode(member);

    for (const auto &[serviceName, server]: node->mServers)
    {
      if (!this->contains(server))
        continue;

      incoming.push_back(server);
    }
    for (const auto &[serviceName, server]: node->mActionServers)
    {
      if (!this->contains(server))
        continue;

      incoming.push_back(server);
    }
    for (const MemberProxy &targetTopic: node->mPublishesTo)
    {
      if (!this->contains(targetTopic))
        continue;

      incoming.push_back(targetTopic);
    }
  }

  return incoming;
}
