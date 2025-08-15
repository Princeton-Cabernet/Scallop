from bfrt_agent import BFRuntimeAgent
from bfrt_agent import ExactKey
from bfrt_agent import IntData, IntArrayData, BoolData, BoolArrayData
from bfrt_agent import Match, Action

# Observations:
# Egress port maxxes out at 71, rounds up to 128 at 72

bfa = BFRuntimeAgent(verbose=False)

bfa.load_table("$pre.node")
bfa.flush_table("$pre.node")

# Add as many entries as possible until the retrieved entry mismatches with the inserted entry
# or there is an error
node_id = 0
replication_id = 0
egress_port = 0

while True:
    node_id += 1
    replication_id = (replication_id + 1) % 65536
    egress_port = (egress_port + 1) % 72

    if (node_id + 1) % 25000 == 0:
        print(f"Node ID: {node_id + 1}")

    # Build the keys
    match_node = Match([
        ExactKey("$MULTICAST_NODE_ID", node_id)
    ])
    # Build the action
    action_node = Action([
        IntData("$MULTICAST_RID", replication_id),
        IntArrayData("$DEV_PORT", [egress_port])
    ])

    try:
        # Add the entry
        bfa.add_entry("$pre.node", match_node, action_node)
        # Get the entry
        key, data = bfa.get_entry("$pre.node", match_node)

        if key["$MULTICAST_NODE_ID"]["value"] != node_id or data["$MULTICAST_RID"] != replication_id or data["$DEV_PORT"] != [egress_port]:
            print(f"Mismatch: {node_id} {replication_id} [{egress_port}]")
            print(key["$MULTICAST_NODE_ID"]["value"], data["$MULTICAST_RID"], data["$DEV_PORT"])
            break

    except Exception as e:
        print(f"Error: {e}")
        print(f"node_id: {node_id}")
        print(f"replication_id: {replication_id}")
        print(f"egress_port: {egress_port}")
        break