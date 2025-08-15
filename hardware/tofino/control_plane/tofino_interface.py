from bfrt_agent import BFRuntimeAgent
from bfrt_agent import ExactKey
from bfrt_agent import IntData, IntArrayData, BoolData, BoolArrayData
from bfrt_agent import Match, Action

from datetime import datetime, timedelta
import argparse
import asyncio
import websockets
import json

from ipaddress import IPv4Address

########################################

TOFINO_MODEL_CPU_PORT = 64
TOFINO_HARDWARE_CPU_PORT = 192

NO_RTX_SSRC = 0 # Reserved SSRC to indicate no retransmission stream. Relevant for non-video streams.

########################################

DEST_MACS = {
    "10.0.211.1": "0x506b4bc40191",
    "10.0.211.2": "0x1c34da7587f3"
}

EGRESS_PORTS = {
    "10.0.211.1": 4,
    "10.0.211.2": 44
}

AV1_TEMPLATE_ID_MODS = {
    "L1T2": {
        "divisor": 3,
        "template_id_mods": {
            "key":  [0],
            "base": [1],
            "high": [1, 2]
        }
    },
    "L1T3": {
        "divisor": 5,
        "template_id_mods": {
            "key":  [0],
            "base": [1],
            "mid":  [1, 2],
            "high": [1, 2, 3, 4]
        }
    }
}

########################################

class PRE:
    """
    Semantic mappings:
    #     * Multicast group ID (MGID) represents a source stream: (sip, sport, ssrc) [16-bit]
    #     * Node ID (NID) represents a "receiver" of a particular source stream: (sip, sport, ssrc, dip, dport) [32-bit]
    #     * L1 Exclusion ID (XID) represents the exclusion ID for a particular node ID [16-bit]
    #     * Replication ID (RID) represents a "destination": (dip, dport) [16-bit]
    # Tofino PRE table mappings:
    #     * Multicast group ID (MGID) to list of node IDs (NIDs)
    #     * Replication ID (RID) to egress port (eport)
    #     * Node ID (NID) to list of replication IDs (RIDs)
    # Tofino sfu.p4 table mappings:
    #     * [Duplicate] Source stream (sip, sport, ssrc) to MGID
    #     * Source stream and AV1 layer to exclusion ID (XID)
    #     * [Duplicate] Replication ID (RID) to destination (dip, dport)
    
    Maximum number of entities:
        * Meetings (mapped to multicast groups in pre.mgid): 64,000 (theoretically 2x2^16, but limited by the Tofino)
        * All participants in all meetings (mapped to node IDs in pre.node): 4,294,967,295 (2^32 - 1)
        * Participants in one meeting (mapped to replication IDs in pre.node and L1 exclusion IDs in pre.mgid): 65,535 (2^16 - 1)
        * Egress ports in one meeting (mapped to L2 exclusion IDs in pre.prune): 287 (theoretically 2^16, but limited by the Tofino)
    """

    ########################################

    def __init__(self, hardware_mode=False, verbose=False):

        self.verbose = verbose

        # Local state
        self.sender_meeting_map = {} # Source stream to meeting ID
        self.meeting_participants = {} # Meeting ID to list of participants
        self.meetingparticipant_eport_map = {} # Meeting ID and participant to egress port

        # Data plane mappings
        self.meeting_mgid_map = {} # Meeting ID to multicast group ID
        self.participant_nodeid_map = {} # Participant ID to node ID
        self.meetingparticipant_replicationid_map = {} # Meeting ID and participant ID to replication ID
        self.meetingparticipant_l1exclusionid_map = {} # Meeting ID and participant ID to L1 exclusion ID
        self.eport_l2exclusionid_map = {} # Egress port to L2 exclusion ID

        # Installed Tofino rules
        self.tofino_senders_replication = set()
        self.tofino_participants_nodes = set()
        self.tofino_mgids_participants = []
        self.tofino_eports_l2exclusionids = set()
        self.tofino_ipv4_routes = set()

        # Initialize the BF runtime agent
        self.bfa = BFRuntimeAgent(verbose=self.verbose)

        # Load the replication tables
        self.bfa.load_table("SwitchIngress.av1_template_id_mod_lookup")
        self.bfa.flush_table("SwitchIngress.av1_template_id_mod_lookup")
        
        self.bfa.load_table("SwitchIngress.packet_replication")
        self.bfa.flush_table("SwitchIngress.packet_replication")
        
        self.bfa.load_table("SwitchIngress.recv_report_forwarding")
        self.bfa.flush_table("SwitchIngress.recv_report_forwarding")
        
        self.bfa.load_table("SwitchIngress.nack_pli_forwarding")
        self.bfa.flush_table("SwitchIngress.nack_pli_forwarding")
        
        self.bfa.load_table("SwitchIngress.video_layer_suppression")
        self.bfa.flush_table("SwitchIngress.video_layer_suppression")
        
        self.bfa.load_table("$pre.node")
        self.bfa.flush_table("$pre.node")
        
        self.bfa.load_table("$pre.mgid")
        self.bfa.flush_table("$pre.mgid")
        
        self.bfa.load_table("$pre.prune")
        self.bfa.flush_table("$pre.prune")
        
        self.bfa.load_table("SwitchEgress.ipv4_route")
        self.bfa.flush_table("SwitchEgress.ipv4_route")

        # Setup the copy_to_cpu capability
        self.bfa.load_table("$pre.port")
        self.bfa.flush_table("$pre.port")
        dev_port = TOFINO_HARDWARE_CPU_PORT if hardware_mode else TOFINO_MODEL_CPU_PORT
        match_node = Match([ExactKey("$DEV_PORT", dev_port)])
        action_node = Action([BoolData("$COPY_TO_CPU_PORT_ENABLE", True)])
        self.bfa.add_entry("$pre.port", match_node, action_node)
        print(f"Tofino DEV_PORT {dev_port} configured as CPU port.")

        # Setup the AV1 frame type lookup table assuming L1T3 as the default SVC structure
        self.svc_structure = "L1T2"
        self.update_av1_svc_structure(self.svc_structure)
    
    ########################################

    def printc(self, msg):
        if self.verbose:
            print(msg)

    ########################################

    def update_av1_svc_structure(self, structure):
        self.printc(f"Tofino interface: update_av1_svc_structure() called with SVC spatial/temporal structure {structure}")
        if structure not in AV1_TEMPLATE_ID_MODS:
            self.printc(f"SVC structure {structure} unknown to the Tofino interface program.")
            return
        self.bfa.flush_table("SwitchIngress.av1_template_id_mod_lookup")
        for template_id in range(2**6):
            mod_template_id = template_id % AV1_TEMPLATE_ID_MODS[structure]["divisor"]
            # Build the keys
            match_template_id = Match([
                ExactKey("hdr.av1.dep_template_id", template_id),
            ])
            # Build the action
            action_template_id = Action([
                IntData("mod", mod_template_id)
            ], "set_av1_template_id_mod")
            # Add the entry
            self.bfa.add_entry("SwitchIngress.av1_template_id_mod_lookup",
                               match_template_id, action_template_id)

    ########################################

    def add_stream(self, mid, sip, sport, ssrc, ssrc_rtx, dip, dport):

        def add_sender_to_replication_setup(_sip, _sport, _ssrc, _mgid, _rid, _l2_xid):
            self.printc(f"Tofino interface: add_sender_to_replication_setup() called with"
                        + f" sip {_sip}, sport {_sport}, ssrc {_ssrc}, mgid {_mgid}, rid {_rid}, l2_xid {_l2_xid}")
            # Build the keys
            match_replication = Match([
                ExactKey("hdr.ipv4.src_addr", int(IPv4Address(_sip))),
                ExactKey("hdr.udp.src_port", _sport),
                ExactKey("ig_md.rtp_rtcp_ssrc", _ssrc)
            ])
            # Build the action
            action_setup_replication = Action([
                IntData("mgid", _mgid),
                IntData("packet_rid", _rid),
                IntData("l2_xid", _l2_xid)
            ], "setup_replication")
            # Add the entry
            self.bfa.add_entry("SwitchIngress.packet_replication",
                               match_replication, action_setup_replication)
        
        def add_multicast_node(_nid, _rid, _eport):
            self.printc(f"Tofino interface: add_multicast_node() called with"
                        + f" nid {_nid}, rid {_rid}, eport {_eport}")
            # Build the keys
            match_node = Match([
                ExactKey("$MULTICAST_NODE_ID", _nid)
            ])
            # Build the action
            action_node = Action([
                IntData("$MULTICAST_RID", _rid),
                IntArrayData("$DEV_PORT", [_eport])
            ])
            # Add the entry
            self.bfa.add_entry("$pre.node", match_node, action_node)
        
        def update_multicast_group(_mgid, _nids, _l1_xids, _l1_xids_validity, add=False):
            self.printc(f"Tofino interface: update_multicast_group() called with mgid"
                        + f" {_mgid}, nids {_nids}, l1_xids {_l1_xids}, validity: {_l1_xids_validity}, add {add}")
            # Build the keys
            match_mgid= Match([
                ExactKey("$MGID", _mgid)
            ])
            # Build the action
            action_mgid = Action([
                IntArrayData("$MULTICAST_NODE_ID", _nids),
                IntArrayData("$MULTICAST_NODE_L1_XID", _l1_xids),
                BoolArrayData("$MULTICAST_NODE_L1_XID_VALID", _l1_xids_validity)
            ])
            # Add/update the entry
            if add:
                self.bfa.add_entry("$pre.mgid", match_mgid, action_mgid)
            else:
                self.bfa.mod_entry("$pre.mgid", match_mgid, action_mgid)

        def add_l2_exclusion_port(_l2_xid, _eport):
            self.printc(f"Tofino interface: add_l2_exclusion_port() called with l2_xid {_l2_xid}, eport {_eport}")
            # Build the keys
            match_prune = Match([
                ExactKey("$MULTICAST_L2_XID", _l2_xid)
            ])
            # Build the action
            action_prune = Action([
                IntArrayData("$DEV_PORT", [_eport])
            ])
            # Add the entry
            self.bfa.add_entry("$pre.prune", match_prune, action_prune)
        
        def add_ipv4_route(_sip, _sport, _ssrc, _rid, _dip, _dport):
            self.printc(f"Tofino interface: add_ipv4_route() called with"
                        + f" sip {_sip}, sport {_sport}, ssrc {_ssrc},"
                        + f" rid {_rid}, dip {_dip}, dport {_dport}")
            # Build the keys
            match_route = Match([
                ExactKey("hdr.ipv4.src_addr", int(IPv4Address(_sip))),
                ExactKey("hdr.udp.src_port", _sport),
                ExactKey("eg_md.rtp_rtcp_ssrc", _ssrc),
                ExactKey("eg_intr_md.egress_rid", _rid)
            ])
            # Build the actions
            action_route = Action([
                    IntData("ip_dst_addr", int(IPv4Address(_dip))),
                    IntData("udp_dst_port", _dport)
                ], "set_destination_headers")
            # Add the entry
            self.bfa.add_entry("SwitchEgress.ipv4_route",
                               match_route, action_route)

        # 1. Update local state

        # 1.1. Assign sender to meeting
        if (sip, sport, ssrc) not in self.sender_meeting_map:
            self.sender_meeting_map[(sip, sport, ssrc)] = mid
            if ssrc_rtx != NO_RTX_SSRC:
                self.sender_meeting_map[(sip, sport, ssrc_rtx)] = mid

        # 1.2. Add participants to meeting
        if mid not in self.meeting_participants:
            self.meeting_participants[mid] = []
        if (sip, sport) not in self.meeting_participants[mid]:
            self.meeting_participants[mid].append((sip, sport))
        if (dip, dport) not in self.meeting_participants[mid]:
            self.meeting_participants[mid].append((dip, dport))

        # 1.3. Assign egress port to meeting and participant
        if (mid, sip, sport) not in self.meetingparticipant_eport_map:
            self.meetingparticipant_eport_map[(mid, sip, sport)] = EGRESS_PORTS[sip]
        if (mid, dip, dport) not in self.meetingparticipant_eport_map:
            self.meetingparticipant_eport_map[(mid, dip, dport)] = EGRESS_PORTS[dip]
        
        # 2. Update data plane-related state

        # 2.1. Assign meeting to multicast group
        if mid not in self.meeting_mgid_map:
            if len(self.meeting_mgid_map) == 64000:
                self.printc("Maximum number of meetings reached.")
                return
            mgid = None
            for i in range(1, 64001):
                if i not in self.meeting_mgid_map.values():
                    mgid = i
                    break
            self.meeting_mgid_map[mid] = mgid
            
        # 2.2. Assign participant to node ID
        for (ip, port) in [(sip, sport), (dip, dport)]:
            if (ip, port) not in self.participant_nodeid_map:
                if len(self.participant_nodeid_map) == (2**32) - 1:
                    self.printc("Maximum number of participants reached.")
                    return
                nid = None
                for i in range(1, 2**32):
                    if i not in self.participant_nodeid_map.values():
                        nid = i
                        break
                self.participant_nodeid_map[(ip, port)] = nid

        # 2.3. Assign meeting participant to replication ID
        for (ip, port) in [(sip, sport), (dip, dport)]:
            if (mid, ip, port) not in self.meetingparticipant_replicationid_map:
                if len(self.meetingparticipant_replicationid_map) == (2**16) - 1:
                    self.printc("Maximum number of replication IDs reached.")
                    return
                rid = None
                for i in range(1, 2**16):
                    if i not in self.meetingparticipant_replicationid_map.values():
                        rid = i
                        break
                self.meetingparticipant_replicationid_map[(mid, ip, port)] = rid

        # 2.4. Assign meeting participant to L1 exclusion ID
        for (ip, port) in [(sip, sport), (dip, dport)]:
            if (mid, ip, port) not in self.meetingparticipant_l1exclusionid_map:
                l1_xid = self.meetingparticipant_replicationid_map[(mid, ip, port)]
                self.meetingparticipant_l1exclusionid_map[(mid, ip, port)] = l1_xid

        # 2.5. Assign egress port to L2 exclusion ID        
        for ip in [sip, dip]:
            eport = EGRESS_PORTS[ip]
            if eport not in self.eport_l2exclusionid_map:
                self.eport_l2exclusionid_map[eport] = eport
        
        # 3. Process (add/update/delete) Tofino rules

        # 3.1. Add sender to replication setup
        mgid = self.meeting_mgid_map[mid]
        rid = self.meetingparticipant_replicationid_map[(mid, sip, sport)]
        eport = self.meetingparticipant_eport_map[(mid, sip, sport)]
        l2_xid = self.eport_l2exclusionid_map[eport]
        if (sip, sport, ssrc, mgid, rid, l2_xid) not in self.tofino_senders_replication:
            add_sender_to_replication_setup(sip, sport, ssrc, mgid, rid, l2_xid)
            self.tofino_senders_replication.add((sip, sport, ssrc, mgid, rid, l2_xid))
        if ssrc_rtx != NO_RTX_SSRC:
            if (sip, sport, ssrc_rtx, mgid, rid, l2_xid) not in self.tofino_senders_replication:
                add_sender_to_replication_setup(sip, sport, ssrc_rtx, mgid, rid, l2_xid)
                self.tofino_senders_replication.add((sip, sport, ssrc_rtx, mgid, rid, l2_xid))

        # 3.2. Add meeting participants to multicast nodes
        for (ip, port) in [(sip, sport), (dip, dport)]:
            nid = self.participant_nodeid_map[(ip, port)]
            rid = self.meetingparticipant_replicationid_map[(mid, ip, port)]
            eport = self.meetingparticipant_eport_map[(mid, ip, port)]
            if (nid, rid, eport) not in self.tofino_participants_nodes:
                add_multicast_node(nid, rid, eport)
                self.tofino_participants_nodes.add((nid, rid, eport))

        # 3.3. Add meeting participants to multicast group
        mgid = self.meeting_mgid_map[mid]
        nids = []
        l1_xids = []
        l1_xids_validity = []
        for (ip, port) in sorted(self.meeting_participants[mid]):
            nid = self.participant_nodeid_map[(ip, port)]
            l1_xid = self.meetingparticipant_l1exclusionid_map[(mid, ip, port)]
            nids.append(nid)
            l1_xids.append(l1_xid)
            l1_xids_validity.append(False)
        if mgid not in [m[0] for m in self.tofino_mgids_participants]:
            update_multicast_group(mgid, nids, l1_xids, l1_xids_validity, add=True)
            self.tofino_mgids_participants.append((mgid, nids, l1_xids, l1_xids_validity))
        elif (mgid, nids, l1_xids, l1_xids_validity) not in self.tofino_mgids_participants:
            update_multicast_group(mgid, nids, l1_xids, l1_xids_validity, add=False)
            self.tofino_mgids_participants.append((mgid, nids, l1_xids, l1_xids_validity))
        
        # 3.4. Add L2 exclusion ID to egress port
        eport = self.meetingparticipant_eport_map[(mid, sip, sport)]
        l2_xid = self.eport_l2exclusionid_map[eport]
        if (eport, l2_xid) not in self.tofino_eports_l2exclusionids:
            add_l2_exclusion_port(eport, l2_xid)
            self.tofino_eports_l2exclusionids.add((eport, l2_xid))
        
        # 3.5. Add IPv4 routes
        rid = self.meetingparticipant_replicationid_map[(mid, dip, dport)]
        if (sip, sport, ssrc, rid, dip, dport) not in self.tofino_ipv4_routes:
            add_ipv4_route(sip, sport, ssrc, rid, dip, dport)
            self.tofino_ipv4_routes.add((sip, sport, ssrc, rid, dip, dport))
        if ssrc_rtx != NO_RTX_SSRC:
            if (sip, sport, ssrc_rtx, rid, dip, dport) not in self.tofino_ipv4_routes:
                add_ipv4_route(sip, sport, ssrc_rtx, rid, dip, dport)
                self.tofino_ipv4_routes.add((sip, sport, ssrc_rtx, rid, dip, dport))

########################################

async def tofino_client_agent(hardware_mode=False, verbose=False):
    
    tofino_pre = PRE(hardware_mode, verbose)
    uri = "ws://localhost:8765"
    
    time_start = datetime.now()
    last_log_mins_lapsed = 0

    while True:
        try:
            async with websockets.connect(uri, ping_interval=None) as websocket:
                print(f"\nConnected to server: {uri}")
                while True:
                    # Receive JSON message from the server
                    json_message = await websocket.recv()

                    # Parse and print the received JSON message
                    api_message = json.loads(json_message)
                    print("Received:", api_message)

                    if api_message["api"] == "update_av1_svc_structure":
                        tofino_pre.update_av1_svc_structure(api_message["structure"])
                    elif api_message["api"] == "add_stream":
                        tofino_pre.add_stream(api_message["mid"],
                                              api_message["sip"], api_message["sport"],
                                              api_message["ssrc"], api_message["ssrc_rtx"],
                                              api_message["dip"], api_message["dport"])
                    elif api_message["api"] == "remove_stream":
                        tofino_pre.remove_stream(api_message["mid"],
                                                 api_message["sip"], api_message["sport"],
                                                 api_message["ssrc"], api_message["ssrc_rtx"],
                                                 api_message["dip"], api_message["dport"])
                    elif api_message["api"] == "set_quality":
                        tofino_pre.set_quality(api_message["mid"],
                                               api_message["sip"], api_message["sport"], api_message["ssrc"],
                                               api_message["dip"], api_message["dport"],
                                               api_message["quality"])
                    elif api_message["api"] == "add_receive_report_forwarding_rule":
                        tofino_pre.add_receive_report_forwarding_rule(api_message["mid"],
                                                                      api_message["sip"], api_message["sport"],
                                                                      api_message["dip"], api_message["dport"])
                    elif api_message["api"] == "remove_receive_report_forwarding_rule":
                        tofino_pre.remove_receive_report_forwarding_rule(api_message["mid"],
                                                                         api_message["sip"], api_message["sport"])
                    elif api_message["api"] == "add_nack_pli_forwarding_rule":
                        tofino_pre.add_nack_pli_forwarding_rule(api_message["mid"],
                                                                api_message["sip"], api_message["sport"],
                                                                api_message["dip"], api_message["dport"])
                    elif api_message["api"] == "remove_nack_pli_forwarding_rule":
                        tofino_pre.remove_nack_pli_forwarding_rule(api_message["mid"],
                                                                   api_message["sip"], api_message["sport"])
                    else:
                        print(f"Unknown API: {api_message['api']}")
                    
                    print()

        except (ConnectionRefusedError, websockets.ConnectionClosedError, OSError):
            mins_lapsed = int(timedelta.total_seconds(datetime.now() - time_start) // 60)
            if mins_lapsed > last_log_mins_lapsed and mins_lapsed % 5 == 0:
                print(f"Program active for {mins_lapsed} mins. Server not available. Retrying every 1 second...")
                last_log_mins_lapsed = mins_lapsed
            await asyncio.sleep(1)

########################################

if __name__ == "__main__":

    # Check if testing is enabled
    parser = argparse.ArgumentParser(description="Allows enabling hardware mode.")
    parser.add_argument('--hardware', action='store_true', help="Run on hardware switch.")
    parser.add_argument('--verbose', action='store_true', help="Display detailed messages.")
    args = parser.parse_args()
    
    # Run the WebSocket client
    asyncio.run(tofino_client_agent(args.hardware, args.verbose))

########################################
