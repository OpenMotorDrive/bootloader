import uavcan
import argparse
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('file', nargs=1)
args = parser.parse_args()

node = uavcan.make_node('/dev/ttyACM0', node_id=127)
node_monitor = uavcan.app.node_monitor.NodeMonitor(node)
allocator = uavcan.app.dynamic_node_id.CentralizedServer(node, node_monitor, database_storage=tempfile.mkstemp())

nodes_found = {}

def node_info_response_callback(event):
    nodes_found[event.transfer.source_node_id] = event.message.name

def node_status_callback(event):
    if event.transfer.source_node_id not in nodes_found.keys():
        nodes_found[event.transfer.source_node_id] = None
    else if nodes_found[event.transfer.source_node_id] is None:
        node.request(uavcan.protocol.GetNodeInfo.Request(), 127, response_callback, priority=uavcan.TRANSFER_PRIORITY_LOWEST)

handle = node.add_handler(uavcan.protocol.NodeStatus, node_status_callback)
