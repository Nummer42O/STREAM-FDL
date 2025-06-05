# STREAM-FDL
Fault Detection and Localization Module for STREAM Pipeline.

> [!NOTE]
> As the fault trajectory extraction is not really a core contribution, it is not implemented (yet).

> [!NOTE]
> As ROS2 message forwarding is not yet implemented in IPC, the plugin-system is ignored for now too.

# TODO

## high priority
- possible optimisation:
  - make FD and SGB run in sync, not independent
  - eliminates need for FD thread
  - watchlist doesn't need to be mt-safe anymore -> allows easier, more STL like access
- Alerts:
  - severities?
  - DB
- reimplement ODDAD, with 3-sigma-rule fallback

## low priority
- rethink some of the move semantics
- determine actual config file schema
  - transfer most, if not all, of the "magical value macros" to config
  - see thesis

## ideas
- "new (adjacent) nodes" list
- actual attribute names:
  - name mapping in IPC for default attributes
  - attribute name mapping in config for attributes which are manually piped into IPC

# Custom Full Graph View Query
The blind spot check uses a custom query to retrieve the whole graph, which looks as follows:

```sql
OPTIONAL MATCH (a:Active) WHERE (a)-[:publishing]->() OR ()-[:subscribing]->(a) OR (a)-[:sending]->() OR ()-[:sending]->(a) OR (a)-[:timer]->() WITH a
OPTIONAL MATCH (p:Passive) WHERE ()-[:publishing]->(p) OR (p)-[:subscribing]->() WITH a,p
OPTIONAL MATCH (a)-[pub:publishing]->(p) WITH a,p,pub
OPTIONAL MATCH (p)-[sub:subscribing]->(a) WITH a,p,pub,sub
OPTIONAL MATCH (a)-[send:sending]->(target)
RETURN {
  active: collect(DISTINCT a),
  passive: collect(DISTINCT p),
  pub: collect(DISTINCT {
    from: startNode(pub),
    to: endNode(pub),
    rel: pub}),
  sub: collect(DISTINCT {
    from: startNode(sub),
    to: endNode(sub),
    rel: sub}),
  send: collect(DISTINCT {
    from: startNode(send),
    to: endNode(send),
    rel: send})
} AS result
```

Alternatively, it can be written as inline version:

```sql
OPTIONAL MATCH (a:Active) WHERE (a)-[:publishing]->() OR ()-[:subscribing]->(a) OR (a)-[:sending]->() OR ()-[:sending]->(a) OR (a)-[:timer]->() WITH a OPTIONAL MATCH (p:Passive) WHERE ()-[:publishing]->(p) OR (p)-[:subscribing]->() WITH a,p OPTIONAL MATCH (a)-[pub:publishing]->(p) WITH a,p,pub OPTIONAL MATCH (p)-[sub:subscribing]->(a) WITH a,p,pub,sub OPTIONAL MATCH (a)-[send:sending]->(target) RETURN { active: collect(DISTINCT a), passive: collect(DISTINCT p), pub: collect(DISTINCT { from: startNode(pub), to: endNode(pub), rel: pub}), sub: collect(DISTINCT { from: startNode(sub), to: endNode(sub), rel: sub}), send: collect(DISTINCT { from: startNode(send), to: endNode(send), rel: send}) } AS result
```

The query was converted to the form seen in [data-store.cpp](src/dynamic-subgraph/data-store.cpp) using the following Python3 script, where the variable `query` is a string of the inlined version above.

```python
MAX_STRING_SIZE: int = 64
MAX_ARRAY_SIZE: int  = 16

query: str = """…"""

print('{')
line: int = 0;
while (line * MAX_STRING_SIZE < len(query)):
    substr: str = query[line * MAX_STRING_SIZE: (line + 1) * MAX_STRING_SIZE]
    print('  {', end='')
    print(", ".join(map("'{}'".format, substr)), end='')
    if (len(substr) < MAX_STRING_SIZE):
        print(", '\\0'", end='')
    print("},")
    line += 1
if (line < MAX_ARRAY_SIZE):
    print("  {'\\0'}")
print('}')
```