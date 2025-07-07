#include "dynamic-subgraph/member-base.hpp"

#include <cassert>


Member::AttributeMapping Member::getAttributes() const
{
  LOG_TRACE(this);

  AttributeMapping output;
  for (Attribute &attribute: mmAttributes)
  {
    sharedMem::Response shmResponse = MAKE_RESPONSE;
    if (attribute.sharedMemory.receive(shmResponse, false))
    {
      //! NOTE: as the shared memory does not overwrite unread memory we need to make sure its "empty"
      while (attribute.sharedMemory.receive(shmResponse, false)) {}

      assert(shmResponse.header.type == sharedMem::NUMERICAL);
      LOG_TRACE(this << " Attribute " << attribute.name << " got new value " << shmResponse.numerical.value);
      attribute.lastValue = shmResponse.numerical.value;
    }
    output.emplace(attribute.name, attribute.lastValue);
  }
  return output;
}

void Member::addAttributeSource(const AttributeDescriptor &attributeName, const SingleAttributesResponse &response)
{
  LOG_TRACE(this << LOG_VAR(attributeName) "requestId: " << response.requestID << "memAddress: " << response.memAddress);

  SharedMemory shm(util::parseString(response.memAddress));
  sharedMem::Response shmResponse = MAKE_RESPONSE;
  shm.receive(shmResponse);
  assert(shmResponse.header.type == sharedMem::NUMERICAL);
  LOG_TRACE(this << "Initial attribute " << attributeName << " value: " << shmResponse.numerical.value);

  mmAttributes.emplace_back(attributeName, std::move(shm), response.requestID, shmResponse.numerical.value);
}


MemberPtr::MemberPtr(Member *member, AtomicCounter *useCounter):
  mpMember(member),
  mpUseCounter(useCounter)
{
  //! NOTE: expected to be done by data store to avoid races with DataStore::run
  // mpUseCounter->increase();
  LOG_TRACE(LOG_THIS << *this);
}

MemberPtr::~MemberPtr()
{
  LOG_TRACE(LOG_THIS);
  if (!mpMember)
    return;

  mpUseCounter->decrease();
  LOG_TRACE(*this);
}

MemberPtr::MemberPtr(const MemberPtr &other)
{
  LOG_TRACE(LOG_THIS LOG_VAR(other));

  mpMember = other.mpMember;
  if (!mpMember)
    return;

  mpUseCounter = other.mpUseCounter;
  mpUseCounter->increase();
  LOG_TRACE(*this);
}

MemberPtr &MemberPtr::operator=(const MemberPtr &other)
{
  LOG_TRACE(LOG_THIS LOG_VAR(other));

  mpMember = other.mpMember;
  if (!mpMember)
    return *this;

  mpUseCounter = other.mpUseCounter;
  mpUseCounter->increase();
  LOG_TRACE(*this);

  return *this;
}

MemberPtr::MemberPtr(MemberPtr &&other)
{
  LOG_TRACE(LOG_THIS LOG_VAR(other));

  mpMember = other.mpMember;
  if (!mpMember)
    return;

  //! NOTE: invalidate others member pointer so the destructor doesn't
  //!       decrease the counter
  other.mpMember = nullptr;
  mpUseCounter = other.mpUseCounter;
}

MemberPtr &MemberPtr::operator=(MemberPtr &&other)
{
  LOG_TRACE(LOG_THIS LOG_VAR(other));

  mpMember = other.mpMember;
  if (!mpMember)
    return *this;

  //! NOTE: invalidate others member pointer so the destructor doesn't
  //!       decrease the counter
  other.mpMember = nullptr;
  mpUseCounter = other.mpUseCounter;

  return *this;
}

std::ostream &operator<<(std::ostream &stream, const Member *member)
{
  stream << (member->mIsTopic ? "Topic(" : "Node(") << member->mPrimaryKey << '@' << static_cast<const void *>(member) << ')';

  return stream;
}

std::ostream &operator<<(std::ostream &stream, const MemberPtr &member)
{
  if (!member.mpMember)
    stream << "Ptr()";
  else
    stream << "Ptr(" << member.mpMember << ", counter=" << *member.mpUseCounter << ", name='" << ::getName(member) << "')";

  return stream;
}

std::ostream &operator<<(std::ostream &stream, const MemberProxy &proxy)
{
  stream << (proxy.mIsTopic ? "Proxy(Topic(" : "Proxy(Node(") << proxy.mPrimaryKey << "))";

  return stream;
}
