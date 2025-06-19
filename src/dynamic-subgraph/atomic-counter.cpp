#include "dynamic-subgraph/atomic-counter.hpp"

#include "common.hpp"
#include "atomic-counter.hpp"


void AtomicCounter::increase()
{
  const ScopeLock scopedLock(mValueMutex);
  ++mValue;
}

void AtomicCounter::decrease()
{
  const ScopeLock scopedLock(mValueMutex);
  if (mValue > 0ul)
    --mValue;
}

size_t AtomicCounter::get()
{
  const ScopeLock scopedLock(mValueMutex);
  return mValue;
}

bool AtomicCounter::nonZero()
{
  const ScopeLock scopedLock(mValueMutex);
  return mValue > 0ul;
}

std::ostream &operator<<(std::ostream &stream, const AtomicCounter &counter)
{
  stream << counter.mValue;

  return stream;
}
