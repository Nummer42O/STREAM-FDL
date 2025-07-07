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

  const ScopeLock scopedLock(mmVerticesMutex);

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
  LOG_TRACE(LOG_THIS LOG_VAR(member));

  const ScopeLock scopedLock(mmVerticesMutex);
  return this->unprotectedContains(member);
}

bool Graph::contains(const std::string &member) const
{
  LOG_TRACE(LOG_THIS LOG_VAR(member));

  const ScopeLock scopedLock(mmVerticesMutex);
  Members::const_iterator it = std::find_if(
    mVertices.begin(), mVertices.end(),
    [&member](const Members::value_type &existingMember)
    {
      return ::getName(existingMember) == member;
    }
  );
  return it != mVertices.end();
}

size_t Graph::size() const
{
  LOG_TRACE(LOG_THIS);

  const ScopeLock scopedLock(mmVerticesMutex);
  return mVertices.size();
}

#define PSEUDO_CSTR(string) const_cast<char *>((string).c_str())
void Graph::visualise(const std::atomic<bool> &running, cr::milliseconds loopTargetInterval)
{
  LOG_TRACE(LOG_THIS)

  cv::Mat image = cv::Mat::ones(10, 10, CV_8UC4);
  Timestamp start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    if (mUpdateVisualisation)
    {
      LOG_DEBUG("updating visualisation")

      GVC_t *gvc = gvContext();
      Agraph_t *graph = agopen(const_cast<char *>("g"), Agdirected, NULL);
      {
        const ScopeLock scopedLock(mmVerticesMutex);

        for (const MemberPtr &vertex: mVertices)
        {
          Agnode_t *fromNode = agnode(graph, PSEUDO_CSTR(vertex->mPrimaryKey), 1);
          agsafeset(fromNode, const_cast<char *>("label"), PSEUDO_CSTR(::getName(vertex)), const_cast<char *>("<unknown>"));

          MemberProxies outgoing = this->getOutgoing(vertex);
          for (const MemberProxy &to: outgoing)
          {
            Agnode_t *toNode = agnode(graph, PSEUDO_CSTR(to.mPrimaryKey), 1);
            agedge(graph, fromNode, toNode, NULL, 1);
          }
        }
      }
      mUpdateVisualisation = false;

      // std::FILE *graphFile = std::fopen("/tmp/render.png", "wb+");
      // assert(graphFile);
      gvLayout(gvc, graph, "neato");
      // gvRender(gvc, graph, "png", graphFile);
      gvRenderFilename(gvc, graph, "png", "/tmp/render.png");
      gvRenderFilename(gvc, graph, "dot", "/tmp/render.dot");
      gvFreeLayout(gvc, graph);
      // std::fclose(graphFile);

      cv::Mat tmp = cv::imread("/tmp/render.png", cv::IMREAD_UNCHANGED);
      LOG_DEBUG("New image is empty: " << tmp.empty());
      if (!tmp.empty())
        image = std::move(tmp);

      gvFreeContext(gvc);
    }

    cv::imshow("Subgraph", image);

    stop = cr::system_clock::now();
    cr::milliseconds elapsedTime = cr::duration_cast<cr::milliseconds>(stop - start);
    cr::milliseconds remainingTime = loopTargetInterval - elapsedTime;
    if (remainingTime.count() > 0)
      cv::waitKey(remainingTime.count());
    else
      cv::waitKey(1);
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
      if (this->unprotectedContains(targetNode))
        outgoing.push_back(targetNode);
    }
  }
  else
  {
    const Node *node = ::asNode(member);

    for (const auto &[serviceName, clients]: node->mClients)
    {
      for (const MemberProxy &client: clients)
      {
        if (this->unprotectedContains(client))
          outgoing.push_back(client);
      }
    }
    for (const auto &[serviceName, clients]: node->mActionClients)
    {
      for (const MemberProxy &client: clients)
      {
        if (this->unprotectedContains(client))
          outgoing.push_back(client);
      }
    }
    for (const MemberProxy &targetTopic: node->mPublishesTo)
    {
      if (this->unprotectedContains(targetTopic))
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
      incoming.push_back(sourceNode);
  }
  else
  {
    const Node *node = ::asNode(member);

    for (const auto &[serviceName, server]: node->mServers)
      incoming.push_back(server);
    for (const auto &[serviceName, server]: node->mActionServers)
      incoming.push_back(server);
    for (const MemberProxy &targetTopic: node->mSubscribesTo)
      incoming.push_back(targetTopic);
  }

  return incoming;
}

bool Graph::unprotectedContains(const MemberProxy &member) const
{
  LOG_TRACE(LOG_THIS LOG_VAR(member));

  Members::const_iterator it = std::find_if(
    mVertices.begin(), mVertices.end(),
    [&member](const Members::value_type &existingMember)
    {
      return existingMember->mPrimaryKey == member.mPrimaryKey;
    }
  );
  return it != mVertices.end();
}

std::ostream &operator<<(std::ostream &stream, const Graph &graph)
{
  stream << "{\n";
  for (const MemberPtr &member: graph.mVertices)
    stream << member << '\n';
  stream << "}";

  return stream;
}
